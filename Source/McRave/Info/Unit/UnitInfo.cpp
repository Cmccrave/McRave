#include "UnitInfo.h"

#include "Builds/All/BuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Resource/Resources.h"
#include "Info/Roles.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Main/Events.h"
#include "Main/Util.h"
#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Map/Walls/Walls.h"
#include "Micro/Combat/Combat.h"
#include "Strategy/Actions/Actions.h"
#include "Strategy/Spy/Spy.h"
#include "UnitMath.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave {
    UnitInfo::UnitInfo() {}

    namespace {

        static map<Color, int> colorWidth = {{Colors::Red, -3}, {Colors::Orange, -2}, {Colors::Yellow, -1}, {Colors::Green, 0}, {Colors::Blue, 1}, {Colors::Purple, 2}};

        Position getOvershootPosition(UnitInfo *unit, Position here)
        {
            if (unit->getType() == Zerg_Overlord)
                return here;

            // Check if we should overshoot for halting distance
            if (!unit->getBuildPosition().isValid() && (unit->isFlying() || unit->isHovering() || unit->getType() == Protoss_High_Templar || unit->attemptingSurround())) {
                auto distance      = unit->getPosition().getDistance(here);
                auto haltDistance  = max({distance, 32.0, double(unit->getType().haltDistance()) / 256.0});
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
    } // namespace

    void UnitInfo::circle(Color color)
    {
        const auto width = type.width() + colorWidth[color];
        Visuals::drawCircle(position, width, color);
    }

    void UnitInfo::box(Color color)
    {
        Visuals::drawBox(Position(position.x - type.dimensionLeft(), position.y - type.dimensionUp()), Position(position.x + type.dimensionRight() + 1, position.y + type.dimensionDown() + 1), color);
    }

    void UnitInfo::updateHistory()
    {
        if (unit()->exists()) {
            positionHistory[Broodwar->getFrameCount()] = getPosition();
            orderHistory[Broodwar->getFrameCount()]    = make_pair(unit()->getOrder(), unit()->getOrderTargetPosition());
            commandHistory[Broodwar->getFrameCount()]  = unit()->getLastCommand().getType();

            if (positionHistory.size() > 30)
                positionHistory.erase(positionHistory.begin());
            if (orderHistory.size() > 10)
                orderHistory.erase(orderHistory.begin());
            if (commandHistory.size() > 10)
                commandHistory.erase(commandHistory.begin());

            lastPos       = position;
            lastFormation = formation;
            lastWalk      = walkPosition;
            lastTile      = tilePosition;
            lastRole      = role;
            lastType      = type;

            lastGState = gState;
            lastLState = lState;
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
            threatening   = false;
            framesVisible = 0;
        }

        if (unit()->exists()) {

            lastPos  = position;
            lastTile = tilePosition;
            lastWalk = walkPosition;

            // Unit Stats
            type         = t;
            player       = p;
            completed    = unit()->isCompleted() && !unit()->isMorphing();
            currentSpeed = sqrt(pow(unit()->getVelocityX(), 2) + pow(unit()->getVelocityY(), 2));
            invincible   = unit()->isInvincible() || unit()->isStasised();

            updateUnitData(*this);

            // Points
            position          = unit()->getPosition();
            tilePosition      = t.isBuilding() ? unit()->getTilePosition() : TilePosition(position);
            walkPosition      = t.isBuilding() ? WalkPosition(tilePosition) : WalkPosition(position);
            destination       = Positions::Invalid;
            retreatPos        = Positions::Invalid;
            marchPos          = Positions::Invalid;
            surroundPosition  = Positions::Invalid;
            trapPosition      = Positions::Invalid;
            interceptPosition = Positions::Invalid;
            formation         = Positions::Invalid;
            navigation        = Positions::Invalid;

            double angle = unit()->getAngle();

            double dx = cos(angle);
            double dy = sin(angle);

            facingPosition = position + Position(int(dx * 32.0), int(dy * 32.0));

            // Flags
            flying    = unit()->isFlying() || getType().isFlyer() || unit()->getOrder() == Orders::LiftingOff || unit()->getOrder() == Orders::BuildingLiftOff;
            movedFlag = false;
            stunned   = !unit()->isPowered() || unit()->isMaelstrommed() || unit()->isStasised() || unit()->isLockedDown() || unit()->isMorphing() || !unit()->isCompleted();
            cloaked   = unit()->isCloaked();
            inDanger  = Actions::isInDanger(*this, position);

            // McRave Stats
            engageRadius  = Math::calcSimRadius(*this) + 160.0;
            retreatRadius = Math::calcSimRadius(*this) + 64.0;

            // States
            lState = LocalState::None;
            gState = GlobalState::None;
            tState = TransportState::None;

            // Attack Frame
            if ((getType() == Protoss_Reaver && unit()->getGroundWeaponCooldown() >= 59) || unit()->isStartingAttack())
                lastAttackFrame = Broodwar->getFrameCount();

            // Enemy attack frame (check weapon cooldown instead of animation)
            if (player->isEnemy(Broodwar->self())) {
                if ((getType() != Protoss_Reaver && canAttackGround() && unit()->getGroundWeaponCooldown() >= type.groundWeapon().damageCooldown() - 1) ||
                    (getType() != Protoss_Reaver && canAttackAir() && unit()->getAirWeaponCooldown() >= type.airWeapon().damageCooldown() - 1))
                    lastAttackFrame = Broodwar->getFrameCount();
            }

            // Clear last path
            if (lastTile != tilePosition) {
                marchPath   = BWEB::Path();
                retreatPath = BWEB::Path();
            }

            // Frames
            remainingTrainFrame = max(0, remainingTrainFrame - 1);
            lastRepairFrame     = (unit()->isRepairing() || unit()->isBeingHealed() || unit()->isConstructing()) ? Broodwar->getFrameCount() : lastRepairFrame;
            minStopFrame        = Math::stopAnimationFrames(t);
            lastStimFrame       = unit()->isStimmed() ? Broodwar->getFrameCount() : lastStimFrame;
            lastVisibleFrame    = Broodwar->getFrameCount();
            framesVisible++;

            checkHidden();
            checkStuck();
            checkProxy();
            checkCompletion();
            checkThreatening();
        }

        // Always update arrival frame even if we don't see it
        if (player->isEnemy(Broodwar->self())) {
            auto dist   = isFlying() ? position.getDistance(Terrain::getMainPosition()) : BWEB::Map::getGroundDistance(position, Terrain::getMainPosition());
            arriveFrame = Broodwar->getFrameCount() + int(dist / speed);
        }

        // Create a list of units that are in reach of this unit
        unitsInReachOfThis.clear();
        typesReachingThis.clear();
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;
            if (((this->isFlying() && unit.canAttackAir()) || (!this->isFlying() && unit.canAttackGround())) && unit.isWithinReach(*this)) {
                unitsInReachOfThis.push_back(unit.weak_from_this());
                typesReachingThis.insert(unit.getType());
            }
        }

        // Create a list of units that are in range of this unit
        unitsInRangeOfThis.clear();
        typesRangingThis.clear();
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            auto &unit = *u;
            if (((this->isFlying() && unit.canAttackAir()) || (!this->isFlying() && unit.canAttackGround())) && unit.isWithinRange(*this)) {
                unitsInRangeOfThis.push_back(unit.weak_from_this());
                typesRangingThis.insert(unit.getType());
            }
        }

        // Check if this unit is close to a splash unit
        if (getPlayer() == Broodwar->self()) {
            nearSplash           = false;
            auto closestSplasher = Util::getClosestUnit(position, PlayerState::Enemy,
                                                        [&](auto &u) { return u->isSplasher() && ((!this->isFlying() && u->canAttackGround()) || (this->isFlying() && u->canAttackAir())); });

            if (closestSplasher && closestSplasher->isWithinReach(*this))
                nearSplash = true;

            targetedBySplash = any_of(unitsTargetingThis.begin(), unitsTargetingThis.end(), [&](auto &t) { return !t.expired() && t.lock()->isSplasher() && t.lock()->isWithinRange(*this); });
        }

        // Check if this unit is close to / targeted by a suicidal unit
        if (getPlayer() == Broodwar->self()) {
            nearSuicide         = false;
            auto closestSuicide = Util::getClosestUnit(position, PlayerState::Enemy,
                                                       [&](auto &u) { return u->isSuicidal() && ((!this->isFlying() && u->canAttackGround()) || (this->isFlying() && u->canAttackAir())); });

            if (closestSuicide && closestSuicide->unit()->getOrderTargetPosition()) {
                auto distToSelf = Util::boxDistance(getType(), getPosition(), closestSuicide->getType(), closestSuicide->getPosition());
                if (distToSelf < 80.0)
                    nearSuicide = true;
            }

            targetedBySuicide = any_of(unitsTargetingThis.begin(), unitsTargetingThis.end(), [&](auto &t) {
                return !t.expired() && t.lock()->isSuicidal() && t.lock()->isWithinReach(*this);
                ;
            });
        }

        // Check if this unit is close to a hidden unit
        if (getPlayer() == Broodwar->self()) {
            nearHidden         = false;
            auto closestHidden = Util::getClosestUnit(position, PlayerState::Enemy, [&](auto &u) {
                return u->isHidden() && u->unit()->exists() &&
                       ((!this->isFlying() && u->canAttackGround() && u->getType() != Terran_Wraith && u->getType() != Terran_Ghost) ||
                        (this->isFlying() && u->canAttackAir() && u->getType() != Terran_Ghost));
            });

            if (closestHidden && closestHidden->isWithinReach(*this))
                nearHidden = true;
            if (closestHidden && closestHidden->getSpeed() == 0.0 && closestHidden->isWithinEngage(*this))
                nearHidden = true;

            targetedByHidden = any_of(unitsTargetingThis.begin(), unitsTargetingThis.end(), [&](auto &t) { return !t.expired() && t.lock()->isHidden(); });
        }

        target_.reset();
        unitsTargetingThis.clear();
        typesTargetingThis.clear();
    }

    // Strategic flags
    void UnitInfo::checkStuck()
    {
        // Buildings and stationary units can't get stuck
        if (speed <= 0.0 || unit()->getLastCommand().getType() == UnitCommandTypes::Stop) {
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

        // Check if the unit has attacked at least half as fast as it normally can
        const auto attacked = Broodwar->getFrameCount() - lastAttackFrame < 2 * max(type.groundWeapon().damageCooldown(), type.airWeapon().damageCooldown());
        if (attacked)
            lastMoveFrame = Broodwar->getFrameCount();

        // Check if a worker hasn't moved off their tile
        if (getType().isWorker()) {
            if (lastTile != getTilePosition() || unit()->isGatheringMinerals() || unit()->isGatheringGas() || unit()->isConstructing() || isWithinBuildRange())
                lastMoveFrame = Broodwar->getFrameCount();
        }
        // Check if a unit hasn't moved in a while but is trying to
        else {
            if (!bwUnit->isAttackFrame() && (getPlayer() != Broodwar->self() || lastTile != getTilePosition() || !unit()->isMoving()))
                lastMoveFrame = Broodwar->getFrameCount();
        }

        if (isStuck())
            lastStuckFrame = Broodwar->getFrameCount();
    }

    void UnitInfo::checkHidden()
    {
        // Burrowed check for non spider mine type units or units we can see using the order for burrowing
        burrowed = (getType() != Terran_Vulture_Spider_Mine && unit()->isBurrowed());

        // If this is a spider mine and doesn't have a target, then it is an inactive mine and unable to attack
        if (getType() == Terran_Vulture_Spider_Mine && (!unit()->exists() || (!hasTarget() && unit()->getSecondaryOrder() == Orders::Cloak))) {
            burrowed    = true;
            groundReach = getGroundRange();
        }
        if (getType() == UnitTypes::Spell_Scanner_Sweep) {
            hidden = true;
            return;
        }

        // A unit is considered hidden if it is burrowed or cloaked and not under detection
        hidden = (burrowed || bwUnit->isCloaked()) &&
                 (player->isEnemy(BWAPI::Broodwar->self()) ? !Actions::overlapsDetection(bwUnit, position, PlayerState::Self) : !Actions::overlapsDetection(bwUnit, position, PlayerState::Enemy));
    }

    void UnitInfo::checkThreatening()
    {
        if (!getPlayer()->isEnemy(Broodwar->self()) || getType() == Zerg_Overlord || !hasTarget())
            return;

        auto &target = *target_.lock();

        // Determine how close it is to strategic locations
        const auto choke          = (Combat::isDefendNatural() && Combat::isHoldNatural()) ? Terrain::getNaturalChoke() : Terrain::getMainChoke();
        const auto closestGeo     = BWEB::Map::getClosestChokeTile(choke, getPosition());
        const auto closestStation = Stations::getClosestStationAir(getPosition(), PlayerState::Self);
        const auto closestWall    = BWEB::Walls::getClosestWall(getPosition());
        const auto rangeCheck     = max({getAirRange() + 32.0, getGroundRange() + 32.0, 64.0});
        const auto proximityCheck = max(rangeCheck, 200.0);
        auto threateningThisFrame = false;

        // If the unit is close to stations, defenses or resources owned by us
        const auto inTerritory = Terrain::inTerritory(PlayerState::Self, getPosition());
        const auto atHome      = Terrain::isAtHome(getPosition());
        const auto atChoke     = getPosition().getDistance(closestGeo) <= rangeCheck;
        const auto nearMe      = atHome || atChoke;

        // If the unit attacked defenders, workers or buildings
        const auto attackedDefender = hasAttackedRecently() && Terrain::inTerritory(PlayerState::Self, target.getPosition()) && target.getRole() == Role::Defender;
        const auto attackedWorkers  = hasAttackedRecently() && Terrain::inTerritory(PlayerState::Self, target.getPosition()) && (target.getRole() == Role::Worker || target.getRole() == Role::Support);
        const auto attackedBuildings = hasAttackedRecently() && target.getType().isBuilding();

        auto nearResources = [&]() {
            auto closestMineral = Resources::getClosestMineral(position, [&](auto &r) { return r->getResourceState() == ResourceState::Mineable; });
            if (closestMineral && closestMineral->getPosition().getDistance(position) < max(200.0, getGroundRange()))
                return true;
            return false;
        };

        // Check if enemy is generally in our territory
        auto nearTerritory = [&]() {
            if (isFlying() && getType().spaceProvided() == 0)
                return atHome;

            if ((Terrain::inArea(Terrain::getMainArea(), position) && !Combat::isDefendNatural() && Combat::holdAtChoke()) ||
                (Terrain::inArea(Terrain::getMainArea(), position) && Combat::isDefendNatural() && !Terrain::isPocketNatural()))
                return true;

            // If in a territory with a station/wall and there is no defenders, we will need to engage
            if (atHome) {
                if (closestWall && Terrain::inArea(closestWall->getArea(), getPosition())) {
                    if ((closestWall->getGroundDefenseCount() == 0 && !isFlying()) || closestWall->getAirDefenseCount() == 0 && isFlying())
                        return true;
                }
                if (closestStation && Terrain::inArea(closestStation->getBase()->GetArea(), getPosition())) {
                    if ((Stations::getGroundDefenseCount(closestStation) == 0 && !isFlying()) || (Stations::getAirDefenseCount(closestStation) == 0 && isFlying()))
                        return true;
                }
            }

            // Dangerous harassing units are always a threat in our area
            if (getType() == Protoss_Reaver || getType() == Protoss_High_Templar || getType() == Protoss_Dark_Templar || getType() == Terran_Vulture)
                return inTerritory;

            return (Util::getTime() > Time(5, 00) && Terrain::inArea(Terrain::getMainArea(), position) && Stations::getGroundDefenseCount(Terrain::getMyMain()) == 0 &&
                    (!Walls::getMainWall() || Walls::getMainWall()->getGroundDefenseCount() == 0)) ||
                   (Util::getTime() > Time(7, 00) && Terrain::inArea(Terrain::getNaturalArea(), position) && Stations::getGroundDefenseCount(Terrain::getMyNatural()) == 0 &&
                    (!Walls::getNaturalWall() || Walls::getNaturalWall()->getGroundDefenseCount() == 0)) ||
                   (Util::getTime() > Time(9, 00) && atHome && !Terrain::inArea(Terrain::getMainArea(), position) && !Terrain::inArea(Terrain::getNaturalArea(), position));
        };

        // Check if our defenses can hit or be hit
        auto nearDefenders = [&]() {
            auto closestDefender = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                return u->getRole() == Role::Defender && ((u->canAttackGround() && !this->isFlying()) || (u->canAttackAir() && this->isFlying())) && (u->isCompleted() || hasAttackedRecently());
            });
            return (closestDefender && closestDefender->isWithinRange(*this)) || (closestDefender && this->canAttackGround() && this->isWithinRange(*closestDefender));
        };

        // Checks if it can damage an already damaged building
        auto nearFragileBuilding = [&]() {
            if (!this->canAttackGround() || getType().groundWeapon().damageType() == DamageTypes::Concussive)
                return false;
            auto fragileBuilding = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                return !u->isHealthy() && u->getType().isBuilding() && u->isCompleted() && Terrain::inTerritory(PlayerState::Self, u->getPosition());
            });
            return fragileBuilding && hasTarget() && !getTarget().lock()->isHealthy() &&
                   Util::boxDistance(fragileBuilding->getType(), fragileBuilding->getPosition(), getType(), getPosition()) < proximityCheck;
        };

        // Check if any builders can be hit or blocked
        auto nearBuildPosition = [&]() {
            if (!this->canAttackGround())
                return false;
            if (atHome && !isFlying() && Util::getTime() < Time(5, 00)) {
                auto closestBuilder = Util::getClosestUnit(getPosition(), PlayerState::Self,
                                                           [&](auto &u) { return u->getRole() == Role::Worker && u->getBuildPosition().isValid() && u->getBuildType().isValid(); });
                if (closestBuilder) {
                    auto center = Position(closestBuilder->getBuildPosition()) + Position(closestBuilder->getBuildType().tileWidth() * 16, closestBuilder->getBuildType().tileHeight() * 16);
                    if (Util::boxDistance(getType(), getPosition(), closestBuilder->getBuildType(), center) < proximityCheck ||
                        (attackedWorkers && Util::boxDistance(getType(), getPosition(), closestBuilder->getType(), closestBuilder->getPosition()) < rangeCheck))
                        return true;
                }
            }
            return false;
        };

        const auto constructing = unit()->exists() && (unit()->isConstructing() || unit()->getOrder() == Orders::ConstructingBuilding || unit()->getOrder() == Orders::PlaceBuilding);

        // Building
        if (getType().isBuilding()) {
            auto canDamage       = (getType() != Terran_Bunker || unit()->isCompleted()) && (airDamage > 0.0 || groundDamage > 0.0);
            threateningThisFrame = Planning::overlapsPlan(*this, getPosition()) || (nearMe && (canDamage || getType() == Protoss_Shield_Battery || getType().isRefinery()));
        }

        // Worker
        else if (getType().isWorker())
            threateningThisFrame = atHome && (constructing || hasAttackedRecently());

        // Unit
        else
            threateningThisFrame = attackedWorkers || nearResources() || nearTerritory() || nearFragileBuilding() || nearBuildPosition() || nearDefenders();

        // Specific case: Marine near a proxy bunker
        if (getType() == Terran_Marine && Util::getTime() < Time(5, 00)) {
            auto closestThreateningBunker = Util::getClosestUnit(getPosition(), PlayerState::Enemy, [&](auto &u) { return u->isThreatening() && u->getType() == Terran_Bunker; });
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
        // auto lingerFrames = min(4 + ((Util::getTime().minutes - 6) * 24), 120);
        threatening = Broodwar->getFrameCount() - lastThreateningFrame <= 4;

        // Apply to others
        if (threatening && threateningFrames > 4) {
            for (auto unit : Units::getUnits(PlayerState::Enemy)) {
                if (*unit != *this) {
                    if (unit->isFlying() == this->isFlying() && unit->getType().isWorker() == this->getType().isWorker() && unit->getPosition().getDistance(position) < 320.0)
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
            if (getType().isWorker() || getType() == Terran_Barracks || getType() == Terran_Factory || getType() == Terran_Engineering_Bay || getType() == Terran_Bunker ||
                getType() == Protoss_Gateway || getType() == Protoss_Photon_Cannon || getType() == Protoss_Pylon || getType() == Protoss_Forge || proxyUnit) {
                const auto closestMain = BWEB::Stations::getClosestMainStation(getTilePosition());
                const auto closestNat  = BWEB::Stations::getClosestNaturalStation(getTilePosition());
                if (closestMain && closestNat) {
                    const auto closerToMyMain = closestMain && closestMain == Terrain::getMyMain();
                    const auto closerToMyNat  = closestNat && closestNat == Terrain::getMyNatural();
                    const auto farFromHome    = position.getDistance(closestMain->getBase()->Center()) > 960.0 && position.getDistance(closestNat->getBase()->Center()) > 960.0;

                    const auto timedOrKnown = Util::getTime() < Time(4, 00) || Spy::getEnemyBuild() == P_CannonRush || Spy::getEnemyOpener() == T_Proxy_8Rax ||
                                              Spy::getEnemyTransition() == U_WorkerRush;

                    // Workers are proxy if they're close enough to our bases and considered suspicious
                    if (getType().isWorker()) {
                        if (timedOrKnown && (position.getDistance(closestMain->getBase()->Center()) < 960.0 || position.getDistance(closestNat->getBase()->Center()) < 960.0))
                            proxy = closerToMyMain || closerToMyNat;
                    }
                    else
                        proxy = closerToMyMain || closerToMyNat || farFromHome;
                }
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
        if (player == Broodwar->self() && (type == Zerg_Lair || type == Zerg_Hive || type == Zerg_Greater_Spire || type == Zerg_Sunken_Colony || type == Zerg_Spore_Colony) && !completed) {
            completeFrame = -999;
            startedFrame  = -999;
        }

        // Calculate completion based on build time
        else if (!completed && (completeFrame <= 0 || startedFrame <= 0)) {
            auto ratio    = (double(health) - (0.1 * double(type.maxHitPoints()))) / (0.9 * double(type.maxHitPoints()));
            ratio         = std::clamp(ratio, 0.0, 1.0);
            completeFrame = Broodwar->getFrameCount() + int(std::round((1.0 - ratio) * double(type.buildTime()))) + extra;
            startedFrame  = Broodwar->getFrameCount() - int(std::round((ratio) * double(type.buildTime())));
        }

        // Set completion based on seeing it already completed and this is the first time visible
        else if (completed && startedFrame == -999 && completeFrame == -999) {
            completeFrame = Broodwar->getFrameCount();
            startedFrame  = Broodwar->getFrameCount();
        }

        // Make a bad assumption that we saw it change types and assume completion time
        else if (getLastType() != getType()) {
            completeFrame = Broodwar->getFrameCount() + (!completed * type.buildTime());
            startedFrame  = Broodwar->getFrameCount();
        }
    }

    // Execute a command
    void UnitInfo::setCommand(UnitCommandType cmd, Position here)
    {
        // Check if this is identical to last command
        if (commandType == cmd && unit()->getLastCommand().getTargetPosition() == here && unit()->getLastCommandFrame() + 4 >= Broodwar->getFrameCount())
            return;

        if (isCommandable()) {
            commandPosition = here;
            commandType     = cmd;

            // Try adding overshoot to non air units if they aren't pixel perfect, this seems to work great
            if (!isFlying() && getPosition().getDistance(here) >= 2.0) {
                auto dist = 2 + int(getPosition().getDistance(here) / 8.0);
                here      = Util::shiftTowards(getPosition(), here, getPosition().getDistance(here) + dist);
            }

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

    void UnitInfo::setCommand(UnitCommandType cmd, UnitInfo &target)
    {
        // Check if this is identical to last command
        if (commandType == cmd && unit()->getLastCommand().getTarget() == target.unit() && unit()->getLastCommandFrame() + 4 >= Broodwar->getFrameCount())
            return;

        if (isCommandable()) {
            commandPosition = target.getPosition();
            commandType     = cmd;

            if (cmd == UnitCommandTypes::Attack_Unit)
                unit()->attack(target.unit());
            else if (cmd == UnitCommandTypes::Right_Click_Unit)
                unit()->rightClick(target.unit());
        }
    }

    void UnitInfo::setCommand(UnitCommandType cmd)
    {
        // Check if this is identical to last command
        if (commandType == cmd && unit()->getLastCommandFrame() + 4 >= Broodwar->getFrameCount())
            return;

        if (isCommandable()) {
            commandPosition = position;
            commandType     = cmd;

            if (cmd == UnitCommandTypes::Hold_Position)
                unit()->holdPosition();
            if (cmd == UnitCommandTypes::Stop)
                unit()->stop();
        }
    }

    void UnitInfo::setCommand(TechType tech, Position here)
    {
        if (commandType == UnitCommandTypes::Use_Tech_Position && commandPosition == here)
            return;

        if (isCommandable()) {
            commandPosition = here;
            commandType     = UnitCommandTypes::Use_Tech_Position;
            unit()->useTech(tech, here);
        }
    }

    void UnitInfo::setCommand(TechType tech, UnitInfo &target)
    {
        if (commandType == UnitCommandTypes::Use_Tech_Unit && unit()->getLastCommand().getTarget() == target.unit())
            return;

        if (isCommandable()) {
            commandPosition = target.getPosition();
            unit()->useTech(tech, target.unit());
        }
    }

    void UnitInfo::setCommand(TechType tech)
    {
        if (commandType == UnitCommandTypes::Use_Tech)
            return;

        if (isCommandable()) {
            commandPosition = getPosition();
            unit()->useTech(tech);
        }
    }

    // Check for ability to execute a command
    bool UnitInfo::canStartAttack()
    {
        if (!hasTarget() || (!targetsFriendly() && getGroundDamage() == 0 && getAirDamage() == 0) || (getType() == UnitTypes::Zerg_Lurker && !isBurrowed()))
            return false;
        auto &target = *getTarget().lock();

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
                if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->getShields() == interceptor->getType().maxShields() &&
                    interceptor->getHitPoints() == interceptor->getType().maxHitPoints() && interceptor->isCompleted())
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
        auto weaponCooldown          = (getType() == Protoss_Reaver) ? 60 : (target.getType().isFlyer() ? getType().airWeapon().damageCooldown() : getType().groundWeapon().damageCooldown());
        const auto framesSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        const auto cooldown = (getType() == Protoss_Reaver) ? weaponCooldown - framesSinceAttack : (target.getType().isFlyer() ? unit()->getAirWeaponCooldown() : unit()->getGroundWeaponCooldown());

        auto angle       = BWEB::Map::getAngle(getPosition(), target.getPosition());
        auto facingAngle = M_PI_T2 - unit()->getAngle(); // Reverse direction to counter clockwise, as it should be
        auto angleDiff   = (M_PI - fabs(fmod(fabs(angle - facingAngle), M_PI_T2) - M_PI));

        // Time to turn and face - confirmed
        auto turnFrames = 0;
        if (getType().turnRadius() > 0.0) {
            auto turnSpeed = double(getType().turnRadius()) * 0.0174533 * 2.0; // Degs/framme -> Rads/frame, muta is 40deg per frame, so a turn around takes 5 frames ceil(180/40)
            turnFrames     = int(ceil(angleDiff / turnSpeed));
        }

        // Time to arrive in range - confirmed
        auto arrivalFrames = 0;
        if (getSpeed() > 0.0) {
            auto boxDistance = double(Util::boxDistance(getType(), getPosition(), target.getType(), target.getPosition()));
            auto range       = (target.getType().isFlyer() ? getAirRange() : getGroundRange());
            auto speed       = hasTransport() ? getTransport().lock()->getSpeed() : getSpeed();
            arrivalFrames    = int(ceil(max(0.0, (boxDistance - range) / speed)));
        }

        // Time to deccel/accel - reasonably confirmed
        auto celcelFrames = 0;
        if (isFlying() || isHovering()) {
            auto velocityVector    = getPosition() + Position(int(32.0 * unit()->getVelocityX()), int(32.0 * unit()->getVelocityY())); // You think I care it's a Position?
            auto velocityDirection = BWEB::Map::getAngle(getPosition(), velocityVector);
            auto directionDiff     = (M_PI - fabs(fmod(fabs(angle - velocityDirection), 2 * M_PI) - M_PI));
            auto allowableAngle    = 0.279253; // Mutas (TODO)
            directionDiff          = max(0.0, directionDiff - allowableAngle);
            auto directionPercent  = clamp(directionDiff / angleDiff, 0.0, 1.0);

            auto velocityPercent = clamp((getType().topSpeed() - currentSpeed) / getType().topSpeed(), 0.0, 1.0);
            auto accelPercent    = clamp(velocityPercent + directionPercent, 0.0, 2.0);
            celcelFrames         = int(ceil(accelPercent / (getType().acceleration() / 256.0)) * 2.0);
        }

        // Always important
        auto lagFrames           = Broodwar->getRemainingLatencyFrames();
        auto randomFrames        = 2;
        auto anticipatedCooldown = turnFrames + arrivalFrames + celcelFrames + lagFrames - randomFrames;
        auto cooldownReady       = cooldown <= anticipatedCooldown;
        return cooldownReady;
    }

    bool UnitInfo::canStartGather() { return false; }

    bool UnitInfo::canStartCast(TechType tech, Position here)
    {
        // Not researched or overlaps our own cast
        if (!getPlayer()->hasResearched(tech) || Actions::overlapsActions(unit(), here, tech, PlayerState::Self, Util::getCastRadius(tech)))
            return false;

        // Neutral affecting techs check neutral player too
        if ((tech == TechTypes::Dark_Swarm || tech == TechTypes::Disruption_Web || tech == TechTypes::EMP_Shockwave || tech == TechTypes::Psionic_Storm) &&
            Actions::overlapsActions(unit(), here, tech, PlayerState::Neutral, Util::getCastRadius(tech)))
            return false;

        auto energyNeeded     = tech.energyCost() - energy;
        auto framesToEnergize = 17.856 * energyNeeded;
        auto spellReady       = energy >= tech.energyCost();
        auto spellWillBeReady = framesToEnergize <= getEngDist() / (hasTransport() ? getTransport().lock()->getSpeed() : getSpeed());

        if (!spellReady && !spellWillBeReady)
            return false;

        if (isSpellcaster() && getEngDist() >= 64.0)
            return true;

        auto ground = Grids::getGroundDensity(here, PlayerState::Enemy);
        auto air    = Grids::getAirDensity(here, PlayerState::Enemy);

        if (ground + air >= Util::getCastLimit(tech) || (getType() == Protoss_High_Templar && hasTarget() && getTarget().lock()->isHidden()))
            return true;
        return false;
    }

    bool UnitInfo::canStartCast(TechType tech, UnitInfo &otherUnit)
    {
        // Not researched or overlaps our own cast
        if (!getPlayer()->hasResearched(tech) || Actions::overlapsActions(unit(), otherUnit.unit(), tech))
            return false;

        auto energyNeeded     = tech.energyCost() - energy;
        auto framesToEnergize = 17.856 * energyNeeded;
        auto spellReady       = energy >= tech.energyCost();
        auto spellWillBeReady = framesToEnergize <= getEngDist() / (hasTransport() ? getTransport().lock()->getSpeed() : getSpeed());

        if (!spellReady && !spellWillBeReady)
            return false;
        return true;
    }

    bool UnitInfo::canAttackGround()
    {
        // Can attack ground if weapon is capable or has an ability that can target ground
        return getGroundDamage() > 0.0 || getType() == Protoss_High_Templar || getType() == Protoss_Dark_Archon || getType() == Protoss_Carrier || getType() == Terran_Medic ||
               getType() == Terran_Science_Vessel || getType() == Zerg_Defiler || getType() == Zerg_Queen;
    }

    bool UnitInfo::canAttackAir()
    {
        // Can attack air if weapon is capable or has an ability that can target air
        return getAirDamage() > 0.0 || getType() == Protoss_High_Templar || getType() == Protoss_Dark_Archon || getType() == Protoss_Carrier || getType() == Terran_Science_Vessel ||
               getType() == Zerg_Defiler || getType() == Zerg_Queen;
    }

    bool UnitInfo::canAttack(UnitInfo &otherUnit) { return (canAttackAir() && otherUnit.isFlying()) || (canAttackGround() && !otherUnit.isFlying()); }

    // Get damage per frame for this unit against a target
    double UnitInfo::getDpsAgainst(UnitInfo &otherUnit)
    {
        auto dps    = otherUnit.isFlying() ? Math::calcAirDPS(*this) : Math::calcGroundDPS(*this);
        auto weapon = otherUnit.isFlying() ? type.airWeapon() : type.groundWeapon();
        auto size   = otherUnit.getType().size();

        if (weapon == DamageTypes::Explosive) {
            if (size == UnitSizeTypes::Small)
                return 0.5 * dps;
            if (size == UnitSizeTypes::Medium)
                return 0.75 * dps;
        }
        if (weapon == DamageTypes::Concussive) {
            if (size == UnitSizeTypes::Medium)
                return 0.5 * dps;
            if (size == UnitSizeTypes::Large)
                return 0.25 * dps;
        }
        return 1.0 * dps;
    }

    bool UnitInfo::isHealthy()
    {
        if (type.isBuilding() || type.isWorker() || type == Zerg_Overlord)
            return percentHealth > 0.5;

        return (type.maxShields() > 0 && percentShield > 0.5) || (type.isMechanical() && percentHealth > 0.25) || (type.getRace() == BWAPI::Races::Zerg && percentHealth > 0.25) ||
               (type == BWAPI::UnitTypes::Zerg_Zergling && Players::ZvP() && health > 16) || (type == BWAPI::UnitTypes::Zerg_Zergling && Players::ZvT() && health > 16);
    }

    bool UnitInfo::isRequestingPickup()
    {
        if (!hasTarget() || !hasTransport() || unit()->isLoaded())
            return false;

        // Check if we are being attacked by multiple bullets
        auto bulletCount = 0;
        for (auto &bullet : BWAPI::Broodwar->getBullets()) {
            if (bullet && bullet->exists() && bullet->getPlayer() != BWAPI::Broodwar->self() && bullet->getTarget() == bwUnit)
                bulletCount++;
        }

        // If this is a ghost, it shouldn't need picking up unless near nuke location
        if (getType() == Terran_Ghost)
            return !Terrain::inArea(mapBWEM.GetArea(TilePosition(getDestination())), getPosition());

        auto &unitTarget = getTarget().lock();
        auto range       = unitTarget->isFlying() ? getAirRange() : getGroundRange();
        auto cargoPickup = getType() == BWAPI::UnitTypes::Protoss_High_Templar
                               ? (!canStartCast(BWAPI::TechTypes::Psionic_Storm, unitTarget->getPosition()) || Grids::getGroundThreat(getPosition(), PlayerState::Enemy) <= 0.1f)
                               : !canStartAttack();

        return getLocalState() == LocalState::Retreat || getEngDist() > range + 32.0 || cargoPickup || bulletCount >= 4 || isTargetedBySuicide();
    }

    bool UnitInfo::isWithinEngage(UnitInfo &otherUnit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), otherUnit.getType(), otherUnit.getPosition());
        return otherUnit.getType().isFlyer() ? engageRadius >= boxDistance : engageRadius >= boxDistance;
    }

    bool UnitInfo::isWithinReach(UnitInfo &otherUnit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), otherUnit.getType(), otherUnit.getPosition());
        return otherUnit.getType().isFlyer() ? airReach >= boxDistance : groundReach >= boxDistance;
    }

    bool UnitInfo::isWithinRange(UnitInfo &otherUnit)
    {
        // Special case: hydras have a long initial animation with a short repeated animation. Get them much further in range before attacking
        if (getType() == Zerg_Hydralisk) {
            if (!hasAttackedRecently() && !otherUnit.isMelee() && getSpeed() >= otherUnit.getSpeed() && !otherUnit.isSplasher())
                return Util::boxDistance(getType(), getPosition(), otherUnit.getType(), otherUnit.getPosition()) <= 96.0;
        }

        auto boxDistance = Util::boxDistance(*this, otherUnit);
        auto range       = max(32.0, otherUnit.getType().isFlyer() ? getAirRange() : getGroundRange());
        auto latencyDist = (Broodwar->getLatencyFrames() * getSpeed()) - (Broodwar->getLatencyFrames() * otherUnit.getSpeed());
        return range + latencyDist >= boxDistance;
    }

    bool UnitInfo::isWithinAngle(UnitInfo &otherUnit)
    {
        if (!isFlying())
            return true;

        auto angle       = BWEB::Map::getAngle(make_pair(getPosition(), otherUnit.getPosition()));
        auto facingAngle = 6.18 - unit()->getAngle(); // Reverse direction to counter clockwise, as it should be
        return (M_PI - fabs(fmod(fabs(angle - facingAngle), 2 * M_PI) - M_PI)) < 0.279253;
    }

    bool UnitInfo::isWithinBuildRange()
    {
        if (!getBuildPosition().isValid())
            return false;

        const auto center = Position(getBuildPosition()) + Position(getBuildType().tileWidth() * 16, getBuildType().tileHeight() * 16);
        const auto dist   = Util::boxDistance(getType(), getPosition(), getBuildType(), center);

        // https://github.com/bwapi/bwapi/issues/914
        if (getBuildType() == Zerg_Nydus_Canal) {
            return dist <= 96.0;
        }
        return dist <= 32.0;
    }

    bool UnitInfo::isWithinGatherRange()
    {
        if (!hasResource())
            return false;
        auto resource     = getResource().lock();
        auto sameArea     = mapBWEM.GetArea(resource->getTilePosition()) == mapBWEM.GetArea(getTilePosition());
        auto distResource = getPosition().getDistance(resource->getPosition());
        auto distStation  = getPosition().getDistance(resource->getStation()->getBase()->Center());
        return (sameArea && distResource < 256.0) || distResource < 128.0 || distStation < 128.0;
    }

    bool UnitInfo::isCommandable()
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        const auto frameSinceAttack = Broodwar->getFrameCount() - getLastAttackFrame();
        const auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getRemainingLatencyFrames();

        // Allows skipping the command but still printing the result to screen
        return !cancelAttackRisk;
    }

    bool UnitInfo::attemptingRunby() { return Terrain::getEnemyStartingPosition().isValid() && gType == GoalType::Runby; }

    bool UnitInfo::attemptingSurround()
    {
        if (!hasTarget() || !surroundPosition.isValid() || position.getDistance(surroundPosition) < 16.0 || lState != LocalState::Attack)
            return false;

        auto &target = *getTarget().lock();
        if (target.isThreatening())
            return false;
        if (!target.getType().isWorker() && Util::getTime() < Time(4, 00))
            return false;
        if (target.getSpeed() >= speed)
            return false;
        return position.getDistance(surroundPosition) > 8.0;
    }

    bool UnitInfo::attemptingTrap()
    {
        if (!hasTarget() || !trapPosition.isValid() || position.getDistance(trapPosition) < 16.0 || lState != LocalState::Attack)
            return false;

        auto &target = *getTarget().lock();

        if (target.getType().isWorker() && Terrain::inTerritory(PlayerState::Self, target.getPosition()))
            return true;
        if (target.getSpeed() >= speed && Terrain::inTerritory(PlayerState::Self, target.getPosition()))
            return true;
        return false;
    }

    bool UnitInfo::attemptingHarass()
    {
        if (!isLightAir() || !Combat::getHarassPosition().isValid())
            return false;

        // ZvZ
        if (Players::ZvZ()) {
            if ((vis(Zerg_Zergling) + 4 * com(Zerg_Sunken_Colony) < Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling)) ||
                (Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0) ||
                BuildOrder::getCurrentTransition() == Spy::getEnemyTransition() || (vis(Zerg_Mutalisk) <= Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk)) ||
                (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0 && com(Zerg_Mutalisk) < 3))
                return false;
        }

        return true;
    }

    bool UnitInfo::attemptingRegroup()
    {
        if (!isLightAir() || saveUnit)
            return false;
        if (hasCommander(); auto cmder = commander.lock()) {
            if (cmder->hasTarget()) {
                if (position.getDistance(cmder->getTarget().lock()->getPosition()) < 200.0)
                    return false;
            }

            if (cmder->getLocalState() == LocalState::Attack)
                return position.getDistance(cmder->getPosition()) > 160.0;
            return position.getDistance(cmder->getPosition()) > 96.0;
        }
        return false;
    }

    bool UnitInfo::attemptingAvoidance()
    {
        if (!isLightAir())
            return false;

        if (isRangedByType(Protoss_Corsair) || isRangedByType(Zerg_Devourer) || isReachedByType(Protoss_Archon) || isTargetedByType(Terran_Valkyrie))
            circle(Colors::Red);

        return (isRangedByType(Protoss_Corsair) || isRangedByType(Zerg_Devourer) || isReachedByType(Protoss_Archon) || isTargetedByType(Terran_Valkyrie));
    }

    bool UnitInfo::isTargetedByType(UnitType type) { return Util::contains(typesTargetingThis, type); }
    bool UnitInfo::isReachedByType(UnitType type) { return Util::contains(typesReachingThis, type); }
    bool UnitInfo::isRangedByType(UnitType type) { return Util::contains(typesRangingThis, type); }

    void UnitInfo::setResource(ResourceInfo *unit) { unit ? resource = unit->weak_from_this() : resource.reset(); }
} // namespace McRave