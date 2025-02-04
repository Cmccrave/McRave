#include "Main/McRave.h"
#include "Main/Events.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave
{
    UnitInfo::UnitInfo() {}

    namespace {

        static map<Color, int> colorWidth ={ {Colors::Red, -3}, {Colors::Orange, -2}, {Colors::Yellow, -1}, {Colors::Green, 0}, {Colors::Blue, 1}, {Colors::Purple, 2} };

        WalkPosition calcWalkPosition(UnitInfo* unit)
        {
            auto walkWidth = unit->getType().isBuilding() ? unit->getType().tileWidth() * 4 : unit->getWalkWidth();
            auto walkHeight = unit->getType().isBuilding() ? unit->getType().tileHeight() * 4 : unit->getWalkHeight();

            if (!unit->getType().isBuilding())
                return WalkPosition(unit->getPosition()) - WalkPosition(walkWidth / 2, walkHeight / 2);
            else
                return WalkPosition(unit->getTilePosition());
            return WalkPositions::None;
        }

        bool allowCommand(UnitInfo* unit)
        {
            // Check if we need to wait a few frames before issuing a command due to stop frames
            const auto frameSinceAttack = Broodwar->getFrameCount() - unit->getLastAttackFrame();
            const auto cancelAttackRisk = frameSinceAttack <= unit->data.minStopFrame - Broodwar->getLatencyFrames();

            // Allows skipping the command but still printing the result to screen
            return (!cancelAttackRisk || unit->isLightAir() || unit->isHovering());
        }

        Position getOvershootPosition(UnitInfo* unit, Position here)
        {
            // Check if we should overshoot for halting distance
            if (!unit->getBuildPosition().isValid() && (unit->isFlying() || unit->isHovering() || unit->getType() == Protoss_High_Templar || unit->attemptingSurround())) {
                auto distance = unit->getPosition().getDistance(here);
                auto haltDistance = max({ distance, 32.0, double(unit->getType().haltDistance()) / 256.0 });
                auto overShootHere = here;

                if (haltDistance > 0) {
                    overShootHere = Util::shiftTowards(unit->getPosition(), here, haltDistance);
                    overShootHere = Util::clipLine(unit->getPosition(), overShootHere);
                }
                if (unit->isFlying() || (unit->isHovering() && Util::findWalkable(*unit, overShootHere)))
                    here = overShootHere;
            }
            return here;
        }
    }

    void UnitInfo::circle(Color color) {
        const auto width = type.width() + colorWidth[color];
        Visuals::drawCircle(position, width, color);
    }

    void UnitInfo::box(Color color) {
        Visuals::drawBox(
            Position(position.x - type.dimensionLeft(), position.y - type.dimensionUp()),
            Position(position.x + type.dimensionRight() + 1, position.y + type.dimensionDown() + 1),
            color);
    }

    void UnitInfo::updateHistory()
    {
        if (unit()->exists()) {
            positionHistory[Broodwar->getFrameCount()] = getPosition();
            orderHistory[Broodwar->getFrameCount()] = make_pair(unit()->getOrder(), unit()->getOrderTargetPosition());
            commandHistory[Broodwar->getFrameCount()] = unit()->getLastCommand().getType();

            if (positionHistory.size() > 30)
                positionHistory.erase(positionHistory.begin());
            if (orderHistory.size() > 10)
                orderHistory.erase(orderHistory.begin());
            if (commandHistory.size() > 10)
                commandHistory.erase(commandHistory.begin());
        }
    }

    void UnitInfo::update()
    {
        updateEvents();
        updateStatistics();
        updateHistory();
    }

    void UnitInfo::updateEvents()
    {
        // If it was last known as flying
        if (unit()->exists() && type.isBuilding() && type.getRace() == Races::Terran) {
            if (!isFlying() && (unit()->getOrder() == Orders::LiftingOff || unit()->getOrder() == Orders::BuildingLiftOff || unit()->isFlying()))
                Events::onUnitLift(*this);
            else if (!isFlying() && getTilePosition().isValid() && BWEB::Map::isUsed(getTilePosition()) == None && Broodwar->isVisible(TilePosition(getPosition())))
                Events::onUnitLand(*this);
        }
    }

    void UnitInfo::updateStatistics()
    {
        auto t = unit()->getType();
        auto p = unit()->getPlayer();

        if (!unit()->exists()) {
            threatening                 = false;
            framesVisible               = 0;
        }

        if (unit()->exists()) {

            lastPos                     = position;
            lastTile                    = tilePosition;
            lastWalk                    = walkPosition;

            // Unit Stats
            type                        = t;
            player                      = p;
            completed                   = unit()->isCompleted() && !unit()->isMorphing();
            currentSpeed                = sqrt(pow(unit()->getVelocityX(), 2) + pow(unit()->getVelocityY(), 2));
            invincible                  = unit()->isInvincible() || unit()->isStasised();

            data.health                 = unit()->getHitPoints() > 0 ? unit()->getHitPoints() : (data.health == 0 ? t.maxHitPoints() : data.health);
            data.armor                  = type.armor() + player->getUpgradeLevel(t.armorUpgrade());
            data.shields                = unit()->getShields() > 0 ? unit()->getShields() : (data.shields == 0 ? t.maxShields() : data.shields);
            data.shieldArmor            = player->getUpgradeLevel(UpgradeTypes::Protoss_Plasma_Shields);
            data.energy                 = unit()->getEnergy();
            data.percentHealth          = t.maxHitPoints() > 0 ? double(data.health) / double(t.maxHitPoints()) : 0.0;
            data.percentShield          = t.maxShields() > 0 ? double(data.shields) / double(t.maxShields()) : 0.0;
            data.percentTotal           = t.maxHitPoints() + t.maxShields() > 0 ? double(data.health + data.shields) / double(t.maxHitPoints() + t.maxShields()) : 0.0;
            data.walkWidth              = int(ceil(double(t.width()) / 8.0));
            data.walkHeight             = int(ceil(double(t.height()) / 8.0));

            // Points        
            position                    = unit()->getPosition();
            tilePosition                = t.isBuilding() ? unit()->getTilePosition() : TilePosition(unit()->getPosition());
            walkPosition                = calcWalkPosition(this);
            destination                 = Positions::Invalid;
            retreatPos                  = Positions::Invalid;
            marchPos                    = Positions::Invalid;
            surroundPosition            = Positions::Invalid;
            interceptPosition           = Positions::Invalid;
            formation                   = Positions::Invalid;
            navigation                  = Positions::Invalid;

            // Flags
            flying                      = unit()->isFlying() || getType().isFlyer() || unit()->getOrder() == Orders::LiftingOff || unit()->getOrder() == Orders::BuildingLiftOff;
            movedFlag                   = false;
            stunned                     = !unit()->isPowered() || unit()->isMaelstrommed() || unit()->isStasised() || unit()->isLockedDown() || unit()->isMorphing() || !unit()->isCompleted();
            cloaked                     = unit()->isCloaked();
            inDanger                    = Actions::isInDanger(*this, position);

            // McRave Stats
            data.groundRange            = Math::groundRange(*this);
            data.groundDamage           = Math::groundDamage(*this);
            data.groundReach            = Math::groundReach(*this);
            data.airRange               = Math::airRange(*this);
            data.airReach               = Math::airReach(*this);
            data.airDamage              = Math::airDamage(*this);
            data.speed                  = Math::moveSpeed(*this);
            data.visibleGroundStrength  = Math::visibleGroundStrength(*this);
            data.maxGroundStrength      = Math::maxGroundStrength(*this);
            data.visibleAirStrength     = Math::visibleAirStrength(*this);
            data.maxAirStrength         = Math::maxAirStrength(*this);
            data.priority               = Math::priority(*this);
            engageRadius                = Math::simRadius(*this) + 160.0;
            retreatRadius               = Math::simRadius(*this) + 64.0;

            // States
            lState                      = LocalState::None;
            gState                      = GlobalState::None;
            tState                      = TransportState::None;

            // Attack Frame
            if ((getType() == Protoss_Reaver && unit()->getGroundWeaponCooldown() >= 59)
                || unit()->isStartingAttack())
                lastAttackFrame         = Broodwar->getFrameCount();

            // Enemy attack frame (check weapon cooldown instead of animation
            if (player->isEnemy(Broodwar->self())) {
                if ((getType() != Protoss_Reaver && canAttackGround() && unit()->getGroundWeaponCooldown() >= type.groundWeapon().damageCooldown() - 1)
                    || (getType() != Protoss_Reaver && canAttackAir() && unit()->getAirWeaponCooldown() >= type.airWeapon().damageCooldown() - 1))
                    lastAttackFrame     = Broodwar->getFrameCount();
            }

            // Clear last path
            if (lastTile != tilePosition)
                destinationPath = BWEB::Path();

            // Frames
            remainingTrainFrame         = max(0, remainingTrainFrame - 1);
            lastRepairFrame             = (unit()->isRepairing() || unit()->isBeingHealed() || unit()->isConstructing()) ? Broodwar->getFrameCount() : lastRepairFrame;
            data.minStopFrame           = Math::stopAnimationFrames(t);
            lastStimFrame               = unit()->isStimmed() ? Broodwar->getFrameCount() : lastStimFrame;
            lastVisibleFrame            = Broodwar->getFrameCount();
            framesVisible++;

            checkHidden();
            checkStuck();
            checkProxy();
            checkCompletion();
            checkThreatening();
        }

        // Always update arrival frame even if we don't see it
        if (player->isEnemy(Broodwar->self())) {
            auto dist = isFlying() ? position.getDistance(Terrain::getMainPosition()) : BWEB::Map::getGroundDistance(position, Terrain::getMainPosition());
            arriveFrame = Broodwar->getFrameCount() + int(dist / data.speed);
        }

        // Create a list of units that are in reach of this unit
        unitsInReachOfThis.clear();
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;
            if (((this->isFlying() && unit.canAttackAir()) || (!this->isFlying() && unit.canAttackGround())) && unit.isWithinReach(*this)) {
                unitsInReachOfThis.push_back(unit.weak_from_this());
            }
        }

        // Create a list of units that are in range of this unit
        unitsInRangeOfThis.clear();
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;
            if (((this->isFlying() && unit.canAttackAir()) || (!this->isFlying() && unit.canAttackGround())) && unit.isWithinRange(*this)) {
                unitsInRangeOfThis.push_back(unit.weak_from_this());
            }
        }

        // Check if this unit is close to a splash unit
        if (getPlayer() == Broodwar->self()) {
            nearSplash = false;
            auto closestSplasher = Util::getClosestUnit(position, PlayerState::Enemy, [&](auto &u) {
                return u->isSplasher() && ((!this->isFlying() && u->canAttackGround()) || (this->isFlying() && u->canAttackAir()));
            });

            if (closestSplasher && closestSplasher->isWithinReach(*this))
                nearSplash = true;

            targetedBySplash = any_of(unitsTargetingThis.begin(), unitsTargetingThis.end(), [&](auto &t) {
                return !t.expired() && t.lock()->isSplasher();
            });
        }

        // Check if this unit is close to / targeted by a suicidal unit
        if (getPlayer() == Broodwar->self()) {
            nearSuicide = false;
            auto closestSuicide = Util::getClosestUnit(position, PlayerState::Enemy, [&](auto &u) {
                return u->isSuicidal() && ((!this->isFlying() && u->canAttackGround()) || (this->isFlying() && u->canAttackAir()));
            });

            if (closestSuicide && closestSuicide->unit()->getOrderTargetPosition()) {
                auto distToSelf = Util::boxDistance(getType(), getPosition(), closestSuicide->getType(), closestSuicide->getPosition());
                if (distToSelf < 80.0)
                    nearSuicide = true;
            }

            targetedBySuicide = any_of(unitsTargetingThis.begin(), unitsTargetingThis.end(), [&](auto &t) {
                return !t.expired() && t.lock()->isSuicidal();
            });
        }

        // Check if this unit is close to a hidden unit
        if (getPlayer() == Broodwar->self()) {
            nearHidden = false;
            auto closestHidden = Util::getClosestUnit(position, PlayerState::Enemy, [&](auto &u) {
                return u->isHidden() && u->unit()->exists() && ((!this->isFlying() && u->canAttackGround() && u->getType() != Terran_Wraith && u->getType() != Terran_Ghost) || (this->isFlying() && u->canAttackAir() && u->getType() != Terran_Ghost));
            });

            if (closestHidden && closestHidden->isWithinReach(*this))
                nearHidden = true;

            targetedByHidden = any_of(unitsTargetingThis.begin(), unitsTargetingThis.end(), [&](auto &t) {
                return !t.expired() && t.lock()->isHidden();
            });
        }

        target_.reset();
        unitsTargetingThis.clear();
    }

    // Strategic flags
    void UnitInfo::checkStuck()
    {
        // Buildings and stationary units can't get stuck
        if (data.speed <= 0.0) {
            lastMoveFrame = Broodwar->getFrameCount();
            return;
        }

        // Check if a worker is being blocked from mining or returning cargo
        if (unit()->isCarryingGas() || unit()->isCarryingMinerals())
            resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
        else if (unit()->isGatheringGas() || unit()->isGatheringMinerals())
            resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
        else
            resourceHeldFrames = 0;

        // Check if clipped between terrain or buildings
        bool trapped = false;
        if (getType().isWorker()) {
            trapped = true;
            vector<TilePosition> directions{ {1,0}, {-1,0}, {0, 1}, {0,-1} };

            // Check if the unit is stuck by terrain and buildings
            for (auto &tile : directions) {
                auto current = getTilePosition() + tile;
                if (BWEB::Map::isUsed(current) == None || BWEB::Map::isWalkable(current, getType()))
                    trapped = false;
            }

            // Check if the unit intends to be here to build
            if (getBuildPosition().isValid()) {
                const auto center = Position(getBuildPosition()) + Position(getBuildType().tileWidth() * 16, getBuildType().tileHeight() * 16);
                if (Util::boxDistance(getType(), getPosition(), getBuildType(), center) <= 0.0)
                    trapped = false;

                // Check if the unit is trapped between 3 mineral patches
                auto cnt = 0;
                for (auto &tile : directions) {
                    auto current = getTilePosition() + tile;
                    if (BWEB::Map::isUsed(current).isMineralField())
                        cnt++;
                }
                if (cnt < 3)
                    trapped = false;
            }

            if (!trapped)
                lastMoveFrame = Broodwar->getFrameCount();
        }

        // Check if the unit has attacked at least half as fast as it normally can
        const auto attacked = Broodwar->getFrameCount() - lastAttackFrame < 2 * max(type.groundWeapon().damageCooldown(), type.airWeapon().damageCooldown());
        if (attacked)
            lastMoveFrame = Broodwar->getFrameCount();

        // Check if a unit hasn't moved in a while but is trying to
        if (!bwUnit->isAttackFrame() && !trapped && (getPlayer() != Broodwar->self() || lastPos != getPosition() || !unit()->isMoving() || unit()->getLastCommand().getType() == UnitCommandTypes::Stop))
            lastMoveFrame = Broodwar->getFrameCount();
        else if (isStuck())
            lastStuckFrame = Broodwar->getFrameCount();
    }

    void UnitInfo::checkHidden()
    {
        // Burrowed check for non spider mine type units or units we can see using the order for burrowing
        burrowed = (getType() != Terran_Vulture_Spider_Mine && unit()->isBurrowed());

        // If this is a spider mine and doesn't have a target, then it is an inactive mine and unable to attack
        if (getType() == Terran_Vulture_Spider_Mine && (!unit()->exists() || (!hasTarget() && unit()->getSecondaryOrder() == Orders::Cloak))) {
            burrowed = true;
            data.groundReach = getGroundRange();
        }
        if (getType() == UnitTypes::Spell_Scanner_Sweep) {
            hidden = true;
            return;
        }

        // A unit is considered hidden if it is burrowed or cloaked and not under detection
        hidden = (burrowed || bwUnit->isCloaked())
            && (player->isEnemy(BWAPI::Broodwar->self()) ? !Actions::overlapsDetection(bwUnit, position, PlayerState::Self) : !Actions::overlapsDetection(bwUnit, position, PlayerState::Enemy));
    }

    void UnitInfo::checkThreatening()
    {
        if (!getPlayer()->isEnemy(Broodwar->self())
            || getType() == Zerg_Overlord
            || !hasTarget())
            return;

        auto &target = *target_.lock();

        // Determine how close it is to strategic locations
        const auto choke = Combat::isDefendNatural() ? Terrain::getNaturalChoke() : Terrain::getMainChoke();
        const auto area = Combat::isDefendNatural() ? Terrain::getNaturalArea() : Terrain::getMainArea();
        const auto closestGeo = BWEB::Map::getClosestChokeTile(choke, getPosition());
        const auto closestStation = Stations::getClosestStationAir(getPosition(), PlayerState::Self);
        const auto closestWall = BWEB::Walls::getClosestWall(getPosition());
        const auto rangeCheck = max({ getAirRange() + 32.0, getGroundRange() + 32.0, 64.0 });
        const auto proximityCheck = max(rangeCheck, 200.0);
        auto threateningThisFrame = false;

        // If the unit is close to stations, defenses or resources owned by us
        const auto atHome = Terrain::inTerritory(PlayerState::Self, getPosition()) && closestStation && closestStation->getBase()->Center().getDistance(getPosition()) < 640.0;
        const auto atChoke = getPosition().getDistance(closestGeo) <= rangeCheck;
        const auto nearMe = atHome || atChoke;

        // If the unit attacked defenders, workers or buildings
        const auto attackedDefender = hasAttackedRecently() && Terrain::inTerritory(PlayerState::Self, target.getPosition()) && target.getRole() == Role::Defender;
        const auto attackedWorkers = hasAttackedRecently() && Terrain::inTerritory(PlayerState::Self, target.getPosition()) && (target.getRole() == Role::Worker || target.getRole() == Role::Support);
        const auto attackedBuildings = hasAttackedRecently() && target.getType().isBuilding();

        // Check if enemy is generally in our territory
        auto nearTerritory = [&]() {
            if (isFlying())
                return false;

            if ((Terrain::inArea(Terrain::getMainArea(), position) && !Combat::isDefendNatural() && Combat::holdAtChoke())
                || (Terrain::inArea(Terrain::getMainArea(), position) && Combat::isDefendNatural() && !Terrain::isPocketNatural())
                || (Roles::getMyRoleCount(Role::Defender) == 0 && Terrain::inArea(Terrain::getNaturalArea(), position) && Combat::isDefendNatural()))
                return true;

            if (Players::ZvZ())
                return false;

            // If in a territory with a station/wall and there is no defenders, we will need to engage
            if (Util::getTime() > Time(5, 00) && atHome) {
                if (closestWall && Terrain::inArea(closestWall->getArea(), getPosition())) {
                    if ((closestWall->getGroundDefenseCount() == 0 && !isFlying()) || closestWall->getAirDefenseCount() == 0 && isFlying())
                        return true;
                }
                if (closestStation && Terrain::inArea(closestStation->getBase()->GetArea(), getPosition())) {
                    if ((Stations::getGroundDefenseCount(closestStation) == 0 && !isFlying()) || (Stations::getAirDefenseCount(closestStation) == 0 && isFlying()))
                        return true;
                }
            }

            return (Util::getTime() > Time(5, 00) && Terrain::inArea(Terrain::getMainArea(), position) && Stations::getGroundDefenseCount(Terrain::getMyMain()) == 0 && (!Walls::getMainWall() || Walls::getMainWall()->getGroundDefenseCount() == 0))
                || (Util::getTime() > Time(7, 00) && Terrain::inArea(Terrain::getNaturalArea(), position) && Stations::getGroundDefenseCount(Terrain::getMyNatural()) == 0 && (!Walls::getNaturalWall() || Walls::getNaturalWall()->getGroundDefenseCount() == 0))
                || (Util::getTime() > Time(9, 00) && atHome && !Terrain::inArea(Terrain::getMainArea(), position) && !Terrain::inArea(Terrain::getNaturalArea(), position));
        };

        // Check if our defenses can hit or be hit
        auto nearDefenders = [&]() {
            auto closestDefender = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                return u->getRole() == Role::Defender && ((u->canAttackGround() && !this->isFlying()) || (u->canAttackAir() && this->isFlying())) && (u->isCompleted() || Util::getTime() < Time(5, 00));
            });
            return (closestDefender && closestDefender->isWithinRange(*this))
                || (closestDefender && this->canAttackGround() && this->isWithinRange(*closestDefender));
        };

        // Checks if it can damage an already damaged building
        auto nearFragileBuilding = [&]() {
            if (!this->canAttackGround() || getType().groundWeapon().damageType() == DamageTypes::Concussive)
                return false;
            auto fragileBuilding = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                return !u->isHealthy() && u->getType().isBuilding() && u->isCompleted() && Terrain::inTerritory(PlayerState::Self, u->getPosition());
            });
            return fragileBuilding && Util::boxDistance(fragileBuilding->getType(), fragileBuilding->getPosition(), getType(), getPosition()) < proximityCheck;
        };

        // Check if any builders can be hit or blocked
        auto nearBuildPosition = [&]() {
            if (!this->canAttackGround())
                return false;
            if (atHome && !isFlying() && Util::getTime() < Time(5, 00)) {
                auto closestBuilder = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                    return u->getRole() == Role::Worker && u->getBuildPosition().isValid() && u->getBuildType().isValid();
                });
                if (closestBuilder) {
                    auto center = Position(closestBuilder->getBuildPosition()) + Position(closestBuilder->getBuildType().tileWidth() * 16, closestBuilder->getBuildType().tileHeight() * 16);
                    if (Util::boxDistance(getType(), getPosition(), closestBuilder->getBuildType(), center) < proximityCheck
                        || (attackedWorkers && Util::boxDistance(getType(), getPosition(), closestBuilder->getType(), closestBuilder->getPosition()) < rangeCheck))
                        return true;
                }
            }
            return false;
        };

        const auto constructing = unit()->exists() && (unit()->isConstructing() || unit()->getOrder() == Orders::ConstructingBuilding || unit()->getOrder() == Orders::PlaceBuilding);

        // Building
        if (getType().isBuilding()) {
            auto canDamage = (getType() != Terran_Bunker || unit()->isCompleted()) && (data.airDamage > 0.0 || data.groundDamage > 0.0);
            threateningThisFrame = Planning::overlapsPlan(*this, getPosition())
                || (nearMe && (canDamage || getType() == Protoss_Shield_Battery || getType().isRefinery()));
        }

        // Worker
        else if (getType().isWorker())
            threateningThisFrame = nearMe && (constructing || attackedWorkers || attackedDefender || attackedBuildings);

        // Unit
        else
            threateningThisFrame = attackedWorkers
            || nearTerritory()
            || nearFragileBuilding()
            || nearBuildPosition()
            || nearDefenders();

        // Specific case: Marine near a proxy bunker
        if (getType() == Terran_Marine && Util::getTime() < Time(5, 00)) {
            auto closestThreateningBunker = Util::getClosestUnit(getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->isThreatening() && u->getType() == Terran_Bunker;
            });
            if (closestThreateningBunker && closestThreateningBunker->getPosition().getDistance(getPosition()) < 160.0)
                threateningThisFrame = true;
        }

        // Determine if this unit is threatening
        if (threateningThisFrame)
            threateningFrames++;
        else
            threateningFrames = 0;
        
        if (threateningFrames > 4)
            lastThreateningFrame = Broodwar->getFrameCount();

        // Linger threatening
        //auto lingerFrames = min(4 + ((Util::getTime().minutes - 6) * 24), 120);
        threatening = Broodwar->getFrameCount() - lastThreateningFrame <= 4;

        // Apply to others
        if (threatening && threateningFrames > 4) {
            for (auto unit : Units::getUnits(PlayerState::Enemy)) {
                if (*unit != *this) {
                    if (unit->isFlying() == this->isFlying() && unit->getPosition().getDistance(position) < 320.0)
                        unit->lastThreateningFrame = Broodwar->getFrameCount();
                }
            }
        }
    }

    void UnitInfo::checkProxy()
    {
        // Reset flag, if we're safe, no longer flag for proxy
        proxy = false;
        if (Players::getSupply(PlayerState::Self, Races::None) >= 60 || Util::getTime() > Time(6, 00))
            return;

        // Check if an enemy building is a proxy
        if (player->isEnemy(Broodwar->self())) {
            auto proxyUnit = Spy::enemyProxy() && (getType() == Terran_Marine || getType() == Protoss_Zealot);
            if (getType().isWorker()
                || getType() == Terran_Barracks || getType() == Terran_Factory || getType() == Terran_Engineering_Bay || getType() == Terran_Bunker
                || getType() == Protoss_Gateway || getType() == Protoss_Photon_Cannon || getType() == Protoss_Pylon || getType() == Protoss_Forge
                || proxyUnit) {
                const auto closestMain = BWEB::Stations::getClosestMainStation(getTilePosition());
                const auto closestNat = BWEB::Stations::getClosestNaturalStation(getTilePosition());
                const auto closerToMyMain = closestMain && closestMain == Terrain::getMyMain();
                const auto closerToMyNat = closestNat && closestNat == Terrain::getMyNatural();
                const auto farFromHome = position.getDistance(closestMain->getBase()->Center()) > 960.0 && position.getDistance(closestNat->getBase()->Center()) > 960.0;

                // When to consider a worker a proxy (potential proxy shenanigans)
                const auto proxyWorker = (getType() == Protoss_Probe && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) == 0)
                    || (getType() == Terran_SCV && (Players::getTotalCount(PlayerState::Enemy, Terran_Marine) == 0 || bwUnit->isConstructing()))
                    || (getType() == Zerg_Drone && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) == 0);

                const auto timedOrKnown = Util::getTime() < Time(3, 00) || Spy::getEnemyBuild() == "CannonRush" || Spy::getEnemyOpener() == "8Rax";

                // Workers are proxy if they're close enough to our bases and considered suspicious
                if (getType().isWorker()) {
                    if (timedOrKnown && proxyWorker && (position.getDistance(closestMain->getBase()->Center()) < 960.0 || position.getDistance(closestNat->getBase()->Center()) < 960.0))
                        proxy = closerToMyMain || closerToMyNat;
                }
                else
                    proxy = closerToMyMain || closerToMyNat || farFromHome;
            }
        }
    }

    void UnitInfo::checkCompletion()
    {
        int extra = 0;
        if (type.getRace() == BWAPI::Races::Terran)
            extra = 2;
        else if (type.getRace() == BWAPI::Races::Protoss)
            extra = 72;
        else
            extra = 9;

        // Reset frames when a Zerg building is morphing to another stage
        if ((type == Zerg_Lair || type == Zerg_Hive || type == Zerg_Greater_Spire || type == Zerg_Sunken_Colony || type == Zerg_Spore_Colony) && !completed) {
            completeFrame = -999;
            startedFrame = -999;
        }

        // Calculate completion based on build time
        else if (!completed) {
            auto ratio = (double(data.health) - (0.1 * double(type.maxHitPoints()))) / (0.9 * double(type.maxHitPoints()));
            completeFrame = Broodwar->getFrameCount() + int(std::round((1.0 - ratio) * double(type.buildTime()))) + extra;
            startedFrame = Broodwar->getFrameCount() - int(std::round((ratio) * double(type.buildTime())));
        }

        // Set completion based on seeing it already completed and this is the first time visible
        else if (startedFrame == -999 && completeFrame == -999) {
            completeFrame = Broodwar->getFrameCount();
            startedFrame = Broodwar->getFrameCount();
        }
    }

    // Execute a command
    void UnitInfo::setCommand(UnitCommandType cmd, Position here)
    {
        // Check if this is very similar to the last command
        if (allowCommand(this)) {
            commandPosition = here;
            commandType = cmd;

            // Try adding randomness
            auto dist = 1 + int(getPosition().getDistance(here) / 32.0);
            here.x += dist - rand() % dist;
            here.y += dist - rand() % dist;

            if (cmd == UnitCommandTypes::Move) {
                here = getOvershootPosition(this, here);
                unit()->move(here);
            }
            if (cmd == UnitCommandTypes::Right_Click_Position)
                unit()->move(here);
            if (cmd == UnitCommandTypes::Stop)
                unit()->stop();
            if (cmd == UnitCommandTypes::Burrow)
                unit()->burrow();
            if (cmd == UnitCommandTypes::Unburrow)
                unit()->unburrow();
        }
    }

    void UnitInfo::setCommand(UnitCommandType cmd, UnitInfo& target)
    {
        // Check if this is identical to last command
        if (commandType == cmd && unit()->getLastCommand().getTarget() == target.unit())
            return;

        if (allowCommand(this)) {
            commandPosition = target.getPosition();
            commandType = cmd;

            if (cmd == UnitCommandTypes::Attack_Unit)
                unit()->attack(target.unit());
            else if (cmd == UnitCommandTypes::Right_Click_Unit)
                unit()->rightClick(target.unit());
        }
    }

    void UnitInfo::setCommand(UnitCommandType cmd)
    {
        // Check if this is identical to last command
        if (commandType == cmd)
            return;

        if (allowCommand(this)) {
            commandPosition = position;
            commandType = cmd;

            if (cmd == UnitCommandTypes::Hold_Position)
                unit()->holdPosition();
        }
    }

    void UnitInfo::setCommand(TechType tech, Position here)
    {
        if (allowCommand(this)) {
            commandPosition = here;
            unit()->useTech(tech, here);
        }
    }

    void UnitInfo::setCommand(TechType tech, UnitInfo& target)
    {
        if (allowCommand(this)) {
            commandPosition = target.getPosition();
            unit()->useTech(tech, target.unit());
        }
    }

    void UnitInfo::setCommand(TechType tech)
    {
        if (allowCommand(this)) {
            commandPosition = getPosition();
            unit()->useTech(tech);
        }
    }

    // Check for ability to execute a command
    bool UnitInfo::canStartAttack()
    {
        if (!hasTarget()
            || (!targetsFriendly() && getGroundDamage() == 0 && getAirDamage() == 0)
            || (getType() == UnitTypes::Zerg_Lurker && !isBurrowed()))
            return false;
        auto& target = *getTarget().lock();

        if (isSuicidal() || getType().topSpeed() <= 0.0)
            return true;

        // Special Case: Medics
        if (getType() == Terran_Medic)
            return target.getPercentTotal() < 1.0 && getEnergy() > 0;

        // Special Case: Carriers
        if (getType() == UnitTypes::Protoss_Carrier) {
            auto leashRange = 320;
            if (getPosition().getDistance(target.getPosition()) >= leashRange)
                return true;
            for (auto &interceptor : unit()->getInterceptors()) {
                if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->getShields() == interceptor->getType().maxShields() && interceptor->getHitPoints() == interceptor->getType().maxHitPoints() && interceptor->isCompleted())
                    return true;
            }
            return false;
        }

        // Special Case: Reavers - Shuttles reset the cooldown of their attacks to 30 frames not 60 frames
        if (getType() == Protoss_Reaver && hasTransport() && unit()->isLoaded()) {
            auto dist = Util::boxDistance(getType(), getPosition(), target.getType(), target.getPosition());
            return (dist <= getGroundRange());
        }

        // Last attack frame - confirmed
        auto weaponCooldown = (getType() == Protoss_Reaver) ? 60 : (target.getType().isFlyer() ? getType().airWeapon().damageCooldown() : getType().groundWeapon().damageCooldown());
        const auto framesSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cooldown = weaponCooldown - framesSinceAttack;


        auto angle = BWEB::Map::getAngle(getPosition(), target.getPosition());
        auto facingAngle = 6.18 - unit()->getAngle(); // Reverse direction to counter clockwise, as it should be
        auto angleDiff = (M_PI - fabs(fmod(fabs(angle - facingAngle), 2 * M_PI) - M_PI));

        // Time to turn and face - confirmed
        auto turnFrames = 0.0;
        if (getType().turnRadius() > 0.0) {
            auto turnSpeed = (getType().turnRadius() * 0.0174533); // Degs/framme -> Rads/frame, muta is 40deg per frame, so a turn around takes 5 frames ceil(180/40)
            turnFrames = angleDiff / turnSpeed;
        }

        // Time to arrive in range - confirmed
        auto arrivalFrames = 0.0;
        if (getSpeed() > 0.0) {
            auto boxDistance = double(Util::boxDistance(getType(), getPosition(), target.getType(), target.getPosition()));
            auto range = (target.getType().isFlyer() ? getAirRange() : getGroundRange());
            auto speed = hasTransport() ? getTransport().lock()->getSpeed() : getSpeed();
            arrivalFrames = max(0.0, (boxDistance - range) / speed);
        }

        // Time to deccel/accel - WIP it's kinda fucked idk?
        auto celcelFrames = 0.0;
        if (isFlying() || isHovering()) {
            auto velocityVector = getPosition() + Position(32.0 * unit()->getVelocityX(), 32.0 * unit()->getVelocityY()); // You think I care it's a Position?
            auto velocityDirection = BWEB::Map::getAngle(getPosition(), velocityVector);
            auto directionDiff = (M_PI - fabs(fmod(fabs(angle - velocityDirection), 2 * M_PI) - M_PI));
            auto allowableAngle = 0.279253; // Mutas (TODO)
            directionDiff = max(0.0, directionDiff - allowableAngle);
            auto directionPercent = clamp(directionDiff / angleDiff, 0.0, 1.0);

            auto velocityPercent = clamp((getType().topSpeed() - currentSpeed) / getType().topSpeed(), 0.0, 1.0);
            auto accelPercent = clamp(velocityPercent + directionPercent, 0.0, 2.0);
            celcelFrames =  accelPercent / (getType().acceleration() / 256.0);
        }

        // Always important
        auto lagFrames = Broodwar->getLatencyFrames();

        auto anticipatedCooldown = int(turnFrames + arrivalFrames + celcelFrames + lagFrames);
        auto cooldownReady = cooldown <= anticipatedCooldown; // min(anticipatedCooldown, weaponCooldown / 2);
        return cooldownReady;
    }

    bool UnitInfo::canStartGather()
    {
        return false;
    }

    bool UnitInfo::canStartCast(TechType tech, Position here)
    {
        // Not researched or overlaps our own cast
        if (!getPlayer()->hasResearched(tech)
            || Actions::overlapsActions(unit(), here, tech, PlayerState::Self, Util::getCastRadius(tech)))
            return false;

        // Neutral affecting techs check neutral player too
        if ((tech == TechTypes::Dark_Swarm || tech == TechTypes::Disruption_Web || tech == TechTypes::EMP_Shockwave || tech == TechTypes::Psionic_Storm)
            && Actions::overlapsActions(unit(), here, tech, PlayerState::Neutral, Util::getCastRadius(tech)))
            return false;

        auto energyNeeded = tech.energyCost() - data.energy;
        auto framesToEnergize = 17.856 * energyNeeded;
        auto spellReady = data.energy >= tech.energyCost();
        auto spellWillBeReady = framesToEnergize <= getEngDist() / (hasTransport() ? getTransport().lock()->getSpeed() : getSpeed());

        if (!spellReady && !spellWillBeReady)
            return false;

        if (isSpellcaster() && getEngDist() >= 64.0)
            return true;

        auto ground = Grids::getGroundDensity(here, PlayerState::Enemy);
        auto air = Grids::getAirDensity(here, PlayerState::Enemy);

        if (ground + air >= Util::getCastLimit(tech) || (getType() == Protoss_High_Templar && hasTarget() && getTarget().lock()->isHidden()))
            return true;
        return false;
    }

    bool UnitInfo::canAttackGround()
    {
        // Can attack ground if weapon is capable or has an ability that can target ground
        return getGroundDamage() > 0.0
            || getType() == Protoss_High_Templar
            || getType() == Protoss_Dark_Archon
            || getType() == Protoss_Carrier
            || getType() == Terran_Medic
            || getType() == Terran_Science_Vessel
            || getType() == Zerg_Defiler
            || getType() == Zerg_Queen;
    }

    bool UnitInfo::canAttackAir()
    {
        // Can attack air if weapon is capable or has an ability that can target air
        return getAirDamage() > 0.0
            || getType() == Protoss_High_Templar
            || getType() == Protoss_Dark_Archon
            || getType() == Protoss_Carrier
            || getType() == Terran_Science_Vessel
            || getType() == Zerg_Defiler
            || getType() == Zerg_Queen;
    }

    bool UnitInfo::isHealthy()
    {
        if (type.isBuilding() || type.isWorker())
            return data.percentHealth > 0.5;

        return (type.maxShields() > 0 && data.percentShield > 0.5)
            || (type.isMechanical() && data.percentHealth > 0.25)
            || (type.getRace() == BWAPI::Races::Zerg && data.percentHealth > 0.25)
            || (type == BWAPI::UnitTypes::Zerg_Zergling && Players::ZvP() && data.health > 16)
            || (type == BWAPI::UnitTypes::Zerg_Zergling && Players::ZvT() && data.health > 16);
    }

    bool UnitInfo::isRequestingPickup()
    {
        if (!hasTarget()
            || !hasTransport()
            || unit()->isLoaded())
            return false;

        // Check if we Unit being attacked by multiple bullets
        auto bulletCount = 0;
        for (auto &bullet : BWAPI::Broodwar->getBullets()) {
            if (bullet && bullet->exists() && bullet->getPlayer() != BWAPI::Broodwar->self() && bullet->getTarget() == bwUnit)
                bulletCount++;
        }

        // If this is a ghost, it shouldn't need picking up unless near nuke location
        if (getType() == Terran_Ghost)
            return !Terrain::inArea(mapBWEM.GetArea(TilePosition(getDestination())), getPosition());

        auto &unitTarget = getTarget().lock();
        auto range = unitTarget->isFlying() ? getAirRange() : getGroundRange();
        auto cargoPickup = getType() == BWAPI::UnitTypes::Protoss_High_Templar ? (!canStartCast(BWAPI::TechTypes::Psionic_Storm, unitTarget->getPosition()) || Grids::getGroundThreat(getPosition(), PlayerState::Enemy) <= 0.1f) : !canStartAttack();

        return getLocalState() == LocalState::Retreat || getEngDist() > range + 32.0 || cargoPickup || bulletCount >= 4 || isTargetedBySuicide();
    }

    bool UnitInfo::isWithinReach(UnitInfo& otherUnit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), otherUnit.getType(), otherUnit.getPosition());
        return otherUnit.getType().isFlyer() ? data.airReach >= boxDistance : data.groundReach >= boxDistance;
    }

    bool UnitInfo::isWithinRange(UnitInfo& otherUnit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), otherUnit.getType(), otherUnit.getPosition());
        auto range = max(32.0, otherUnit.getType().isFlyer() ? getAirRange() : getGroundRange());
        auto latencyDist = (Broodwar->getLatencyFrames() * getSpeed()) - (Broodwar->getLatencyFrames() * otherUnit.getSpeed());
        auto ff = (!isHovering() && !isFlying()) ? 0.0 : -8.0;
        return range + latencyDist + ff >= boxDistance;
    }

    bool UnitInfo::isWithinAngle(UnitInfo& otherUnit)
    {
        if (!isFlying())
            return true;

        auto angle = BWEB::Map::getAngle(make_pair(getPosition(), otherUnit.getPosition()));
        auto facingAngle = 6.18 - unit()->getAngle(); // Reverse direction to counter clockwise, as it should be
        return (M_PI - fabs(fmod(fabs(angle - facingAngle), 2 * M_PI) - M_PI)) < 0.279253;
    }

    bool UnitInfo::isWithinBuildRange()
    {
        if (!getBuildPosition().isValid())
            return false;

        const auto center = Position(getBuildPosition()) + Position(getBuildType().tileWidth() * 16, getBuildType().tileHeight() * 16);
        const auto close = Util::boxDistance(getType(), getPosition(), getBuildType(), center) <= 32;

        return close;
    }

    bool UnitInfo::isWithinGatherRange()
    {
        if (!hasResource())
            return false;
        auto resource = getResource().lock();
        auto sameArea = mapBWEM.GetArea(resource->getTilePosition()) == mapBWEM.GetArea(getTilePosition());
        auto distResource = getPosition().getDistance(resource->getPosition());
        auto distStation = getPosition().getDistance(resource->getStation()->getBase()->Center());
        return (sameArea && distResource < 256.0) || distResource < 128.0 || distStation < 128.0;
    }

    bool UnitInfo::canOneShot(UnitInfo& target) {

        // Create an offset that increases over time to prevent low muta counts engaging large numbers
        auto minutesPastPressure = clamp(Util::getTime().minutes - 7, 0, 2);
        auto offset = Grids::getAirThreat(target.getPosition(), PlayerState::Enemy) > 0.0f ? minutesPastPressure : 0.0f;

        // Check if this unit could load into a bunker
        if (Util::getTime() < Time(10, 00) && (target.getType() == Terran_Marine || target.getType() == Terran_SCV || target.getType() == Terran_Firebat)) {
            if (Players::getVisibleCount(PlayerState::Enemy, Terran_Bunker) > 0) {
                auto closestBunker = Util::getClosestUnit(target.getPosition(), PlayerState::Enemy, [&](auto &b) {
                    return b->getType() == Terran_Bunker;
                });
                if (closestBunker && closestBunker->getPosition().getDistance(target.getPosition()) < 200.0) {
                    return false;
                }
            }
        }

        // One shotting workers
        if (target.getType().isWorker()) {
            if (target.getType() == Protoss_Probe || target.getType() == Zerg_Drone)
                return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 5.0f + offset;
            if (target.getType() == Terran_SCV)
                return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 7.0f + offset;
        }

        // One shot threshold for individual units
        if (target.getType() == Terran_Marine)
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 5.0f + offset;
        if (target.getType() == Terran_Firebat)
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 7.0f + offset;
        if (target.getType() == Terran_Medic)
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 8.0f + offset;
        if (target.getType() == Terran_Vulture)
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 9.0f + offset;
        if (target.getType() == Protoss_Dragoon)
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 24.0f + offset;
        if (target.getType() == Terran_Goliath) {
            if (target.isWithinRange(*this))
                return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 16.0f + offset;
            else
                return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 18.0f + offset;
        }

        // Check if it's a low life unit - doesn't account for armor 
        if (target.unit()->exists() && (target.getHealth() + target.getShields()) < Grids::getAirDensity(getPosition(), PlayerState::Self) * 7.0f)
            return true;
        return false;
    }

    bool UnitInfo::canTwoShot(UnitInfo& target) {
        if (target.getType() == Terran_Goliath || target.getType() == Protoss_Scout)
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 8.0f;
        if (target.getType() == Protoss_Zealot || target.isSiegeTank())
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 10.0f;
        if (target.getType() == Protoss_Dragoon || target.getType() == Protoss_Photon_Cannon || target.getType() == Protoss_Corsair || target.getType() == Terran_Missile_Turret)
            return Grids::getAirDensity(getPosition(), PlayerState::Self) >= 12.0f;
        return false;
    }

    bool UnitInfo::attemptingRunby()
    {
        auto runbyDesired = false;
        static auto pounce = false;

        if (Players::ZvP() && getType() == Zerg_Zergling) {
            auto enemyOneBase = Spy::getEnemyBuild() == "2Gate" || Spy::getEnemyBuild() == "1GateCore";
            auto backstabTiming = (Spy::getEnemyBuild() == "2Gate") ? Time(5, 00) : Time(4, 00);

            if (enemyOneBase && Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) == 0 && !Terrain::isPocketNatural() && timeCompletesWhen() < Time(4, 00) && vis(Zerg_Sunken_Colony) > 0 && total(Zerg_Zergling) >= 12 && Util::getTime() > backstabTiming)
                return true;
            if (enemyOneBase && Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) == 0 && !Terrain::isPocketNatural() && timeCompletesWhen() < Time(5, 00) && vis(Zerg_Sunken_Colony) >= 4)
                return true;
            if (Spy::getEnemyOpener() == "Proxy9/9" && timeCompletesWhen() < Time(4, 00) && Util::getTime() < Time(5, 00))
                return true;

            // FFE runby are just automatically triggered, don't wait
            if (Spy::getEnemyBuild() == "FFE" && Spy::getEnemyOpener() == "Nexus" && timeCompletesWhen() < Time(5, 00) && Util::getTime() > Time(4, 45))
                runbyDesired = true;
        }

        // Already started a runby
        if (runbyDesired && pounce)
            return true;

        // Look to see if a runby would work
        if (runbyDesired) {
            const auto closest = Util::getClosestUnit(Terrain::getMyNatural()->getBase()->Center(), PlayerState::Enemy, [&](auto &u) {
                return !u->getType().isWorker() || u->isThreatening();
            });
            if (!closest || (Terrain::getEnemyNatural() && closest->getPosition().getDistance(Terrain::getEnemyNatural()->getBase()->Center()) >= 640.0)) {
                pounce = true;
            }
        }
        return runbyDesired;
    }

    bool UnitInfo::attemptingSurround()
    {
        if (!hasTarget() || !surroundPosition.isValid() || position.getDistance(surroundPosition) < 24.0)
            return false;


        auto &target = *getTarget().lock();
        if (target.isThreatening())
            return false;
        if (!target.getType().isWorker() && Util::getTime() < Time(4, 00) && Broodwar->getGameType() != GameTypes::Use_Map_Settings)
            return false;

        if (target.getType().isWorker() && Terrain::inTerritory(PlayerState::Self, target.getPosition()))
            return true;
        return position.getDistance(surroundPosition) > 24.0;

        //if (target.getType().isWorker() && Terrain::inTerritory(PlayerState::Self, target.getPosition()))
        //    return true;

        //if (attemptingRunby() || (Util::getTime() < Time(4, 00) && Broodwar->getGameType() != GameTypes::Use_Map_Settings))
        //    return false;
        //return false;
    }

    bool UnitInfo::attemptingHarass()
    {
        if (!isLightAir() || !Combat::getHarassPosition().isValid())
            return false;

        // ZvZ
        if (Players::ZvZ()) {
            if ((vis(Zerg_Zergling) + 4 * com(Zerg_Sunken_Colony) < Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling))
                || (Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0)
                || BuildOrder::getCurrentTransition() == Spy::getEnemyTransition()
                || (vis(Zerg_Mutalisk) <= Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk))
                || (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0 && com(Zerg_Mutalisk) < 3))
                return false;
        }

        return true;
    }

    bool UnitInfo::attemptingRegroup()
    {
        if (!isLightAir())
            return false;
        return hasCommander() && getPosition().getDistance(getCommander().lock()->getPosition()) > 128.0;
    }
}