#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave
{
    UnitInfo::UnitInfo() {}

    namespace {

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

        double calcSimRadius(UnitInfo* unit)
        {
            if (unit->isFlying()) {
                if (Players::getTotalCount(PlayerState::Enemy, Terran_Goliath) > 0)
                    return 388.0;
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Carrier) > 0
                    || Players::getTotalCount(PlayerState::Enemy, Protoss_Arbiter) > 0
                    || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0
                    || Players::getTotalCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0)
                    return 352.0;
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0
                    || Players::getTotalCount(PlayerState::Enemy, Terran_Ghost) > 0
                    || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0
                    || Players::getTotalCount(PlayerState::Enemy, Terran_Missile_Turret) > 0)
                    return 320.0;
                return 288.0;
            }

            if (Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Battlecruiser) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Carrier) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Arbiter) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Guardian) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Defiler) > 0)
                return 540.0;
            if (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Goliath) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Bunker) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Marine) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Lurker) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Sunken_Colony) > 0)
                return 400.0;
            return 320.0;
        }
    }

    void UnitInfo::setLastPositions()
    {
        if (lastTile != getTilePosition())
            lastTilesVisited[getTilePosition()] = Broodwar->getFrameCount();

        lastPos = getPosition();
        lastTile = getTilePosition();
        lastWalk =  walkPosition;
    }

    void UnitInfo::verifyPaths()
    {
        if (lastTile != unit()->getTilePosition()) {
            BWEB::Path emptyPath;
            destinationPath = emptyPath;
            targetPath = emptyPath;
        }

        if (lastTile.isValid() && unit()->getTilePosition().isValid() && mapBWEM.GetArea(lastTile) != mapBWEM.GetArea(unit()->getTilePosition()))
            quickPath.clear();
    }

    void UnitInfo::update()
    {
        auto t = unit()->getType();
        auto p = unit()->getPlayer();

        if (unit()->exists()) {

            setLastPositions();
            verifyPaths();

            // Unit Stats
            type                        = t;
            player                      = p;
            health                      = unit()->getHitPoints() > 0 ? unit()->getHitPoints() : (health == 0 ? t.maxHitPoints() : health);
            shields                     = unit()->getShields() > 0 ? unit()->getShields() : (shields == 0 ? t.maxShields() : shields);
            energy                      = unit()->getEnergy();
            percentHealth               = t.maxHitPoints() > 0 ? double(health) / double(t.maxHitPoints()) : 0.0;
            percentShield               = t.maxShields() > 0 ? double(shields) / double(t.maxShields()) : 0.0;
            percentTotal                = t.maxHitPoints() + t.maxShields() > 0 ? double(health + shields) / double(t.maxHitPoints() + t.maxShields()) : 0.0;
            walkWidth                   = int(ceil(double(t.width()) / 8.0));
            walkHeight                  = int(ceil(double(t.height()) / 8.0));
            completed                   = unit()->isCompleted() && !unit()->isMorphing();

            // Points        
            position                    = unit()->getPosition();
            tilePosition                = t.isBuilding() ? unit()->getTilePosition() : TilePosition(unit()->getPosition());
            walkPosition                = calcWalkPosition(this);
            destination                 = Positions::Invalid;
            goal                        = Positions::Invalid;
            gType                       = GoalType::None;

            // McRave Stats
            groundRange                 = Math::groundRange(*this);
            groundDamage                = Math::groundDamage(*this);
            groundReach                 = Math::groundReach(*this);
            airRange                    = Math::airRange(*this);
            airReach                    = Math::airReach(*this);
            airDamage                   = Math::airDamage(*this);
            speed                       = Math::moveSpeed(*this);
            visibleGroundStrength       = Math::visibleGroundStrength(*this);
            maxGroundStrength           = Math::maxGroundStrength(*this);
            visibleAirStrength          = Math::visibleAirStrength(*this);
            maxAirStrength              = Math::maxAirStrength(*this);
            priority                    = Math::priority(*this);
            simRadius                   = calcSimRadius(this);

            // States
            lState                      = LocalState::None;
            gState                      = GlobalState::None;
            tState                      = TransportState::None;
            flying                      = unit()->isFlying() || getType().isFlyer() || unit()->getOrder() == Orders::LiftingOff || unit()->getOrder() == Orders::BuildingLiftOff;

            // Frames
            remainingTrainFrame         = max(0, remainingTrainFrame - 1);
            lastAttackFrame             = (t != Protoss_Reaver && (unit()->isStartingAttack() || unit()->isRepairing())) ? Broodwar->getFrameCount() : lastAttackFrame;
            minStopFrame                = Math::stopAnimationFrames(t);

            // BWAPI won't reveal isStartingAttack when hold position is executed if the unit can't use hold position
            if (getPlayer() != Broodwar->self() && getType().isWorker()) {
                if (unit()->getGroundWeaponCooldown() == 1 || unit()->getAirWeaponCooldown() == 1)
                    lastAttackFrame = Broodwar->getFrameCount();
            }

            if (unit()->exists())
                lastVisibleFrame = Broodwar->getFrameCount();

            checkHidden();
            checkStuck();
            checkProxy();

            if (!bwUnit->isCompleted()) {
                auto ratio = (double(health) - (0.1 * double(type.maxHitPoints()))) / (0.9 * double(type.maxHitPoints()));
                completeFrame = Broodwar->getFrameCount() + int(std::round((1.0 - ratio) * double(type.buildTime())));
                startedFrame = Broodwar->getFrameCount() - int(std::round((ratio) * double(type.buildTime())));
            }
            else if (startedFrame == -999 && completeFrame == -999) {
                startedFrame = Broodwar->getFrameCount();
                completeFrame = Broodwar->getFrameCount();
            }

            arriveFrame = Broodwar->getFrameCount() + int(BWEB::Map::getGroundDistance(position, BWEB::Map::getMainPosition()) / speed);
        }

        updateTarget();

        if (getPlayer()->isEnemy(Broodwar->self()))
            checkThreatening();

        // Check if this unit is close to a splash unit
        if (getPlayer() == Broodwar->self() && flying) {
            nearSplash = false;
            auto closestSplasher = Util::getClosestUnit(position, PlayerState::Enemy, [&](auto &u) {
                return u.getType() == Protoss_Corsair || u.getType() == Protoss_Archon || u.getType() == Terran_Valkyrie || u.getType() == Zerg_Devourer;
            });

            if (closestSplasher && Util::boxDistance(getType(), getPosition(), closestSplasher->getType(), closestSplasher->getPosition()) < closestSplasher->getAirRange())
                nearSplash = true;
        }

        // Check if this unit is targeted by a suicidal unit
        if (getPlayer() == Broodwar->self() && flying) {
            nearSuicide = false;
            auto closestSuicide = Util::getClosestUnit(position, PlayerState::Enemy, [&](auto &u) {
                return u.getType() == Zerg_Scourge;
            });

            if (closestSuicide && Util::boxDistance(getType(), getPosition(), closestSuicide->getType(), closestSuicide->unit()->getOrderTargetPosition()) < closestSuicide->getAirReach() && Util::boxDistance(getType(), getPosition(), closestSuicide->getType(), closestSuicide->unit()->getPosition()) < 48.0)
                nearSuicide = true;
        }
    }

    void UnitInfo::updateTarget()
    {
        // Update my target
        if (getPlayer() == Broodwar->self()) {
            target = weak_ptr<UnitInfo>();
            if (getType() == Terran_Vulture_Spider_Mine) {
                if (unit()->getOrderTarget())
                    target = Units::getUnitInfo(unit()->getOrderTarget());
            }
            else
                Targets::getTarget(*this);
        }
        else if (getPlayer()->isEnemy(Broodwar->self())) {
            if (unit()->getOrderTarget()) {
                auto &targetInfo = Units::getUnitInfo(unit()->getOrderTarget());
                if (targetInfo) {
                    target = targetInfo;
                    targetInfo->getTargetedBy().push_back(this->weak_from_this());
                }
            }
            else {
                auto closest = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                    return (u.isFlying() && getAirDamage() > 0.0) || (!u.isFlying() && getGroundDamage() > 0.0);
                });
                if (closest)
                    target = closest;
            }
        }
    }

    void UnitInfo::checkStuck() {

        // Check if a worker is being blocked from mining or returning cargo
        if (unit()->isCarryingGas() || unit()->isCarryingMinerals())
            resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
        else if (unit()->isGatheringGas() || unit()->isGatheringMinerals())
            resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
        else
            resourceHeldFrames = 0;

        // Check if clipped between terrain or buildings
        if (getType().isWorker() && getTilePosition().isValid() && BWEB::Map::isUsed(getTilePosition()) == None && BWEB::Map::isWalkable(getTilePosition(), getType())) {

            vector<TilePosition> directions{ {1,0}, {-1,0}, {0, 1}, {0,-1} };
            bool trapped = true;

            // Check if the unit is stuck by terrain and buildings
            for (auto &tile : directions) {
                auto current = getTilePosition() + tile;
                if (BWEB::Map::isUsed(current) == None || BWEB::Map::isWalkable(current, getType()))
                    trapped = false;
            }

            // Check if the unit intends to be here to build
            if (getBuildPosition().isValid()) {
                const auto center = Position(getBuildPosition()) + Position(getBuildType().tileWidth() * 16, getBuildType().tileHeight() * 16);
                if (center.getDistance(getPosition()) <= 160.0)
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
                lastTileMoveFrame = Broodwar->getFrameCount();
        }

        // Check if a unit hasn't moved in a while but is trying to
        if (getPlayer() != Broodwar->self() || getLastPosition() != getPosition() || !unit()->isMoving() || unit()->getLastCommand().getType() == UnitCommandTypes::Stop || getLastAttackFrame() == Broodwar->getFrameCount())
            lastTileMoveFrame = Broodwar->getFrameCount();
        else if (isStuck())
            lastStuckFrame = Broodwar->getFrameCount();
    }

    void UnitInfo::checkHidden()
    {
        // Burrowed check for non spider mine type units or units we can see using the order for burrowing
        burrowed = (getType() != Terran_Vulture_Spider_Mine && unit()->isBurrowed()) || unit()->getOrder() == Orders::Burrowing;

        // If this is a spider mine and doesn't have a target, then it is an inactive mine and unable to attack
        if (getType() == Terran_Vulture_Spider_Mine && (!unit()->exists() || (!hasTarget() && unit()->getSecondaryOrder() == Orders::Cloak))) {
            burrowed = true;
            groundReach = getGroundRange();
        }

        // A unit is considered hidden if it is burrowed or cloaked and not under detection
        hidden = (burrowed || bwUnit->isCloaked())
            && (player->isEnemy(BWAPI::Broodwar->self()) ? !Actions::overlapsDetection(bwUnit, position, PlayerState::Self) : !Actions::overlapsDetection(bwUnit, position, PlayerState::Enemy));

        if (hidden)
            circleYellow();
    }

    void UnitInfo::checkThreatening()
    {
        // Determine how close it is to strategic locations
        const auto choke = Terrain::isDefendNatural() ? BWEB::Map::getNaturalChoke() : BWEB::Map::getMainChoke();
        const auto area = Terrain::isDefendNatural() ? BWEB::Map::getNaturalArea() : BWEB::Map::getMainArea();
        const auto closestGeo = BWEB::Map::getClosestChokeTile(choke, getPosition());
        const auto closestStation = Stations::getClosestStation(PlayerState::Self, getPosition());
        const auto rangeCheck = max({ getAirRange() + 32.0, getGroundRange() + 32.0, 64.0 });
        const auto proximityCheck = max(rangeCheck, 200.0);

        // If the unit is close to stations, defenses or resources owned by us
        const auto atHome = Terrain::isInAllyTerritory(getTilePosition()) && closestStation && closestStation->getBase()->Center().getDistance(getPosition()) < 640.0;
        const auto atChoke = getPosition().getDistance(closestGeo) <= rangeCheck;

        const auto nearResources = closestStation && closestStation == Terrain::getMyMain() && closestStation->getResourceCentroid().getDistance(getPosition()) < proximityCheck && Terrain::isInAllyTerritory(closestStation->getBase()->GetArea());

        auto nearDefenders = false;
        if (Strategy::defendChoke()) {
            for (auto &pos : Combat::getDefendPositions()) {
                if (getPosition().getDistance(pos) < proximityCheck)
                    nearDefenders = true;
            }
            if (getTilePosition().isValid()) {
                if (Terrain::getDefendChoke() == BWEB::Map::getMainChoke() && mapBWEM.GetArea(getTilePosition()) == BWEB::Map::getMainArea())
                    nearDefenders = true;
                if (Terrain::getDefendChoke() == BWEB::Map::getNaturalChoke() && (mapBWEM.GetArea(getTilePosition()) == BWEB::Map::getNaturalArea() || mapBWEM.GetArea(getTilePosition()) == BWEB::Map::getNaturalArea()))
                    nearDefenders = true;
            }
        }

        // 
        auto allDefendersCanHelp = false;
        if (atHome) {
            auto closestDefense = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                return u.getRole() == Role::Defender;
            });

            if (closestDefense) {

                auto furthestDefense = Util::getFurthestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Defender && mapBWEM.GetArea(u.getTilePosition()) == mapBWEM.GetArea(closestDefense->getTilePosition());
                });

                if ((furthestDefense && furthestDefense->isWithinRange(*this) && closestDefense->isWithinRange(*this)) || !furthestDefense)
                    allDefendersCanHelp = true;
            }
        }

        auto nearBuildPosition = false;
        if (atHome && Util::getTime() < Time(5, 00)) {
            auto closestBuilder = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
                return u.getRole() == Role::Worker && u.getBuildPosition().isValid() && u.getBuildType().isValid();
            });

            if (closestBuilder) {
                auto center = Position(closestBuilder->getBuildPosition()) + Position(closestBuilder->getBuildType().tileWidth() * 16, closestBuilder->getBuildType().tileHeight() * 16);
                if (Util::boxDistance(getType(), getPosition(), closestBuilder->getBuildType(), center) < proximityCheck)
                    nearBuildPosition = true;
            }
        }

        auto fragileBuilding = Util::getClosestUnit(getPosition(), PlayerState::Self, [&](auto &u) {
            return !u.isHealthy() && u.getType().isBuilding();
        });
        auto nearFragileBuilding = fragileBuilding && Util::boxDistance(fragileBuilding->getType(), fragileBuilding->getPosition(), getType(), getPosition()) < proximityCheck;

        // If the unit attacked defenders, workers or buildings
        const auto attackedDefender = hasAttackedRecently() && hasTarget() && Terrain::isInAllyTerritory(getTarget().getTilePosition()) && (getTarget().getRole() == Role::Combat || getTarget().getRole() == Role::Defender);
        const auto attackedWorkers = hasAttackedRecently() && hasTarget() && Terrain::isInAllyTerritory(getTarget().getTilePosition()) && (getTarget().getRole() == Role::Worker || getTarget().getType() == Zerg_Overlord);
        const auto attackedBuildings = hasAttackedRecently() && hasTarget() && getTarget().getType().isBuilding();

        const auto constructing = unit()->exists() && (unit()->isConstructing() || unit()->getOrder() == Orders::ConstructingBuilding || unit()->getOrder() == Orders::PlaceBuilding);

        auto threateningThisFrame = false;

        // Building
        if (getType().isBuilding()) {
            threateningThisFrame = Buildings::overlapsPlan(*this, getPosition())
                || ((atChoke || atHome) && (airDamage > 0.0 || groundDamage > 0.0 || getType() == Protoss_Shield_Battery || getType().isRefinery()))
                || nearResources;
        }

        // Worker
        else if (getType().isWorker())
            threateningThisFrame = (atHome || atChoke) && (constructing || attackedWorkers || attackedDefender || attackedBuildings);

        // Unit
        else {

            if (getType().isFlyer())
                threateningThisFrame = nearResources || attackedWorkers || attackedDefender || nearFragileBuilding;
            else
                threateningThisFrame = nearResources || attackedWorkers || nearFragileBuilding || nearBuildPosition || allDefendersCanHelp || (nearDefenders && attackedDefender);
        }

        // Specific case: Marine near a proxy bunker
        if (getType() == Terran_Marine && Util::getTime() < Time(5, 00)) {
            auto closestThreateningBunker = Util::getClosestUnit(getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u.isThreatening() && u.getType() == Terran_Bunker;
            });
            if (closestThreateningBunker && closestThreateningBunker->getPosition().getDistance(getPosition()) < 160.0)
                threateningThisFrame = true;
        }

        // If determined threatening this frame
        if (threateningThisFrame) {
            lastThreateningFrame = Broodwar->getFrameCount();
            threatening = true;
        }

        threatening = Broodwar->getFrameCount() - lastThreateningFrame < 32;
    }

    void UnitInfo::checkProxy()
    {
        if (getType() == Terran_Barracks || getType() == Terran_Bunker || getType() == Protoss_Gateway || getType() == Protoss_Photon_Cannon || (getType() == Protoss_Pylon && Players::getVisibleCount(PlayerState::Enemy, Protoss_Forge) >= 1)) {
            auto closestMain = BWEB::Stations::getClosestMainStation(getTilePosition());
            auto closestNat = BWEB::Stations::getClosestNaturalStation(getTilePosition());
            auto isNotInMain = closestNat && closestNat->getBase()->GetArea() != mapBWEM.GetArea(getTilePosition()) && getPosition().getDistance(closestNat->getBase()->Center()) > 640.0;
            auto isNotInNat = closestMain && closestMain->getBase()->GetArea() != mapBWEM.GetArea(getTilePosition()) && getPosition().getDistance(closestMain->getBase()->Center()) > 640.0;

            auto closerToMe = closestMain && Stations::ownedBy(closestMain) == PlayerState::Self;
            auto proxyBuilding = (isNotInMain && isNotInNat) || closerToMe;

            if (proxyBuilding) {
                Broodwar->drawCircleMap(getPosition(), 3, Colors::Red, true);
                proxy = true;
            }
        }
    }

    void UnitInfo::createDummy(UnitType t) {
        type                    = t;
        player                  = Broodwar->self();
        groundRange             = Math::groundRange(*this);
        airRange                = Math::airRange(*this);
        groundDamage            = Math::groundDamage(*this);
        airDamage               = Math::airDamage(*this);
        speed                   = Math::moveSpeed(*this);
    }

    bool UnitInfo::command(UnitCommandType command, Position here)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames();

        if (cancelAttackRisk && !isLightAir())
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto newOrderPosition = unit()->getOrderTargetPosition().isValid() && unit()->getOrderTargetPosition().getDistance(here) > 32;
            auto newOrderType = unit()->getOrder() != Orders::Move;
            return newOrderPosition || newOrderType;
        };

        // Check if this is a new command
        const auto newCommand = [&]() {
            auto newCommandPosition = unit()->getLastCommand().getTargetPosition().getDistance(here) > 32;
            auto newCommandType = (unit()->getLastCommand().getType() != command || (Broodwar->getFrameCount() - unit()->getLastCommandFrame() - Broodwar->getLatencyFrames() > 24));
            return newCommandPosition || newCommandType;
        };

        // Check if we should overshoot for halting distance
        if (command == UnitCommandTypes::Move && (getType().isFlyer() || isHovering() || getType() == Protoss_High_Templar)) {
            auto distance = int(getPosition().getDistance(here));
            auto haltDistance = max({ distance, 32, getType().haltDistance() / 256 }) + 64.0;
            auto overShootHere = here;

            if (distance > 0) {
                overShootHere = getPosition() - ((getPosition() - here) * int(round(haltDistance / distance)));
                overShootHere = Util::clipLine(getPosition(), overShootHere);
            }
            if (getType().isFlyer() || (isHovering() && Util::isTightWalkable(*this, overShootHere)))
                here = overShootHere;
        }

        // Add action and grid movement
        if ((command == UnitCommandTypes::Move || command == UnitCommandTypes::Follow) && getPosition().getDistance(here) < 160.0) {
            Actions::addAction(unit(), here, getType(), PlayerState::Self);
            Grids::addMovement(here, *this);
        }

        // If this is a ground unit, wiggle the command a bit
        if (!isFlying())
            here = here + Position(rand() % 4 - 2, rand() % 4 - 2);

        // If this is a new order or new command than what we're requesting, we can issue it
        if (/*newOrder() ||*/ newCommand()) {
            if (command == UnitCommandTypes::Move || command == UnitCommandTypes::Follow)
                unit()->move(here);
        }
        return true;
    }

    bool UnitInfo::command(UnitCommandType command, UnitInfo& targetUnit)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames();

        if (cancelAttackRisk && !getType().isBuilding())
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto newOrderTarget = unit()->getOrderTarget() && unit()->getOrderTarget() != targetUnit.unit();
            return newOrderTarget;
        };

        // Check if this is a new command
        const auto newCommand = [&]() {
            auto newCommandTarget = (unit()->getLastCommand().getType() != command || unit()->getLastCommand().getTarget() != targetUnit.unit());
            return newCommandTarget;
        };

        // Add action
        Actions::addAction(unit(), targetUnit.getPosition(), getType(), PlayerState::Self);

        // If this is a new order or new command than what we're requesting, we can issue it
        if (newOrder() || newCommand() || command == UnitCommandTypes::Right_Click_Unit) {
            if (command == UnitCommandTypes::Attack_Unit)
                unit()->attack(targetUnit.unit());
            else if (command == UnitCommandTypes::Right_Click_Unit)
                unit()->rightClick(targetUnit.unit());
            return true;
        }
        return false;
    }

    bool UnitInfo::canStartAttack()
    {
        if (!hasTarget()
            || (getGroundDamage() == 0 && getAirDamage() == 0)
            || isSpellcaster()
            || (getType() == UnitTypes::Zerg_Lurker && !isBurrowed()))
            return false;

        // Special Case: Carriers
        if (getType() == UnitTypes::Protoss_Carrier) {
            auto leashRange = 320;
            if (getPosition().getDistance(getTarget().getPosition()) >= leashRange)
                return true;
            for (auto &interceptor : unit()->getInterceptors()) {
                if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->getShields() == interceptor->getType().maxShields() && interceptor->getHitPoints() == interceptor->getType().maxHitPoints() && interceptor->isCompleted())
                    return true;
            }
            return false;
        }

        auto weaponCooldown = getType() == Protoss_Reaver ? 60 : (getTarget().getType().isFlyer() ? getType().airWeapon().damageCooldown() : getType().groundWeapon().damageCooldown());
        auto cooldown = lastAttackFrame + (weaponCooldown / 2) - Broodwar->getFrameCount();
        auto range = (getTarget().getType().isFlyer() ? getAirRange() : getGroundRange());

        auto boxDistance = Util::boxDistance(getType(), getPosition(), getTarget().getType(), getTarget().getPosition());
        auto cooldownReady = getSpeed() > 0.0 ? max(0, cooldown) <= max(0.0, boxDistance - range) / (hasTransport() ? getTransport().getSpeed() : getSpeed()) : cooldown <= 0.0;
        return cooldownReady;
    }

    bool UnitInfo::canStartGather()
    {
        return false;
    }

    bool UnitInfo::canStartCast(TechType tech, Position here)
    {
        if (!hasTarget()
            || !getPlayer()->hasResearched(tech)
            || Actions::overlapsActions(unit(), here, tech, PlayerState::Self, int(Util::getCastRadius(tech))))
            return false;

        auto energyNeeded = tech.energyCost() - energy;
        auto framesToEnergize = 17.856 * energyNeeded;
        auto spellReady = energy >= tech.energyCost();
        auto spellWillBeReady = framesToEnergize <= getEngDist() / (hasTransport() ? getTransport().getSpeed() : getSpeed());

        if (!spellReady && !spellWillBeReady)
            return false;

        if (isSpellcaster() && getEngDist() >= 64.0)
            return true;

        auto ground = Grids::getEGroundCluster(here);
        auto air = Grids::getEAirCluster(here);

        if (ground + air >= Util::getCastLimit(tech) || (getType() == Protoss_High_Templar && getTarget().isHidden()))
            return true;
        return false;
    }

    bool UnitInfo::canAttackGround()
    {
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
        if (type.isBuilding())
            return percentHealth > 0.5;

        return (type.maxShields() > 0 && percentShield > LOW_SHIELD_PERCENT_LIMIT)
            || (type.isMechanical() && percentHealth > LOW_MECH_PERCENT_LIMIT)
            || (type.getRace() == BWAPI::Races::Zerg && percentHealth > LOW_BIO_PERCENT_LIMIT)
            || (type == BWAPI::UnitTypes::Zerg_Zergling && Players::ZvP() && health > 16)
            || (type == BWAPI::UnitTypes::Zerg_Zergling && Players::ZvT() && health > 16);
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

        auto range = getTarget().getType().isFlyer() ? getAirRange() : getGroundRange();
        auto cargoReady = getType() == BWAPI::UnitTypes::Protoss_High_Templar ? canStartCast(BWAPI::TechTypes::Psionic_Storm, getTarget().getPosition()) : canStartAttack();
        auto threat = Grids::getEGroundThreat(getWalkPosition()) > 0.0;

        return getLocalState() == LocalState::Retreat || getEngDist() > range + 32.0 || (!cargoReady && threat) || bulletCount >= 4 || Units::getSplashTargets().find(bwUnit) != Units::getSplashTargets().end();
    }

    bool UnitInfo::isWithinReach(UnitInfo& otherUnit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), otherUnit.getType(), otherUnit.getPosition());
        return otherUnit.getType().isFlyer() ? airReach >= boxDistance : groundReach >= boxDistance;
    }

    bool UnitInfo::isWithinRange(UnitInfo& otherUnit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), otherUnit.getType(), otherUnit.getPosition());
        auto range = otherUnit.getType().isFlyer() ? getAirRange() : getGroundRange();
        auto ff = (!isHovering() && !isFlying()) ? 32.0 : -8.0;
        if (isSuicidal())
            return boxDistance <= 0.0;
        return (range + ff) >= boxDistance;
    }

    bool UnitInfo::isWithinAngle(UnitInfo& otherUnit)
    {
        if (!isFlying())
            return true;

        auto desiredAngle = BWEB::Map::getAngle(make_pair(getPosition(), otherUnit.getPosition()));

        auto nextPosition = getPosition() + Position(int(round(unit()->getVelocityX())), int(round(unit()->getVelocityY())));
        auto currentAngle = (unit()->getAngle() * 180.0 / 3.14) - 180.0;

        return abs(desiredAngle - currentAngle) < 48.0 || abs(desiredAngle - currentAngle) >= (360.0 - 48.0) || abs(currentAngle - desiredAngle) >= (360.0 - 48.0);
    }

    bool UnitInfo::isWithinBuildRange()
    {
        if (!getBuildPosition().isValid())
            return false;

        const auto center = Position(getBuildPosition()) + Position(getBuildType().tileWidth() * 16, getBuildType().tileHeight() * 16);
        const auto close = getBuildType().isRefinery() ? getPosition().getDistance(center) <= 96.0 : getPosition().getDistance(center) <= 32.0;

        return close;
    }

    bool UnitInfo::isWithinGatherRange()
    {
        if (!hasResource())
            return false;

        // Great Barrier Reef was causing issues with ground distance to the resource
        const auto pathPoint = getResource().getStation()->getResourceCentroid();

        if (!pathPoint.isValid())
            return false;

        const auto close = BWEB::Map::getGroundDistance(pathPoint, getPosition()) <= 128.0 || getPosition().getDistance(pathPoint) < 64.0;
        const auto sameArea = mapBWEM.GetArea(getTilePosition()) == mapBWEM.GetArea(TilePosition(pathPoint));
        return close || sameArea;
    }

    bool UnitInfo::localEngage()
    {
        return ((!isFlying() && getTarget().isSiegeTank() && getTarget().getTargetedBy().size() >= 4 && getType() != Zerg_Lurker && ((isWithinRange(getTarget()) && getGroundRange() > 32.0) || (isWithinReach(getTarget()) && getGroundRange() <= 32.0)))
            || (getType() == Protoss_Reaver && !unit()->isLoaded() && isWithinRange(getTarget()))
            || (getTarget().getType() == Terran_Vulture_Spider_Mine && !getTarget().isBurrowed())
            || (hasTransport() && !unit()->isLoaded() && getType() == Protoss_High_Templar && canStartCast(TechTypes::Psionic_Storm, getTarget().getPosition()) && isWithinRange(getTarget()))
            || (hasTransport() && !unit()->isLoaded() && getType() == Protoss_Reaver && canStartAttack()) && isWithinRange(getTarget()));
    }

    bool UnitInfo::localRetreat()
    {
        auto targetedByHidden = false;
        for (auto &t : getTargetedBy()) {
            if (auto targeter = t.lock()) {
                if (targeter->isHidden())
                    targetedByHidden = true;
            }
        }
        if (targetedByHidden)
            return true;

        return (getType() == Protoss_Zealot && hasTarget() && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && getTarget().getType() == Terran_Vulture)                 // ...unit is a slow Zealot attacking a Vulture
            || (getType() == Protoss_Corsair && hasTarget() && getTarget().isSuicidal() && com(Protoss_Corsair) < 6)                                                                             // ...unit is a Corsair attacking Scourge with less than 6 completed Corsairs
            || (getType() == Terran_Medic && getEnergy() <= TechTypes::Healing.energyCost())                                                                                                     // ...unit is a Medic with no energy        
            || (getType() == Terran_SCV && Broodwar->getFrameCount() > 12000)                                                                                                                    // ...unit is an SCV outside of early game
            || (isLightAir() && hasTarget() && getType().maxShields() > 0 && getTarget().getType() == Zerg_Overlord && Grids::getEAirThreat(getEngagePosition()) * 5.0 > (double)getShields())   // ...unit is a low shield light air attacking a Overlord under threat greater than our shields
            //|| (getType().getRace() == Races::Zerg && getGroundRange() > 32.0 && !isHealthy() && (isLightAir() || Util::getTime() < Time(8, 0)))                                                 // ...unit is a Zerg unit and is not healthy in early game;
            || (getType() == Zerg_Zergling && Players::ZvZ() && getTargetedBy().size() >= 3 && !Terrain::isInAllyTerritory(getTilePosition()) && Util::getTime() < Time(3, 15))
            || (getType() == Zerg_Zergling && Players::ZvP() && !getTargetedBy().empty() && getHealth() < 10 && Util::getTime() < Time(3, 15))
            || (getType() == Zerg_Zergling && hasTarget() && !isHealthy() && getTarget().getType().isWorker())
            || (hasTarget() && getTarget().isSuicidal() && getTarget().unit()->getOrderTargetPosition().getDistance(getPosition()) < 64.0)
            || unit()->isIrradiated();
    }

    bool UnitInfo::globalEngage()
    {
        const auto oneShotThreshold = [&](UnitInfo &u) {
            if (u.unit()->isStimmed())
                return false;
            if (u.getType() == Terran_Marine || u.getType() == Protoss_Probe || u.getType() == Zerg_Drone)
                return Grids::getAAirCluster(getPosition()) >= 5.5f;
            if (u.getType() == Terran_SCV)
                return Grids::getAAirCluster(getPosition()) >= 7.5f;
            if (u.getType() == Terran_Missile_Turret)
                return Grids::getAAirCluster(getPosition()) >= 12.5f;
            return false;
        };

        auto lingVsVulture = hasTarget() && getTarget().getType() == Terran_Vulture && getType() == Zerg_Zergling;

        return (getTarget().isThreatening() && !lingVsVulture && !getTarget().isHidden() && (Util::getTime() < Time(6, 00) || getSimState() == SimState::Win || Players::ZvZ()))                                                                          // ...target is threatening                    
            || (!getType().isWorker() && (getGroundRange() > getTarget().getGroundRange() || getTarget().getType().isWorker()) && Terrain::isInAllyTerritory(getTarget().getTilePosition()) && !getTarget().isHidden())                 // ...unit can get free hits in our territory
            //|| (isWithinRange(getTarget()) && Terrain::isInAllyTerritory(getTarget().getTilePosition()) && Strategy::defendChoke())
            || (isSuicidal() && hasTarget() && (getTarget().isWithinRange(*this) || getTarget().isThreatening()))
            || (getType() == Zerg_Mutalisk && hasTarget() && oneShotThreshold(getTarget()) && isWithinReach(getTarget()))
            || (isLightAir() && hasTarget() && isWithinReach(getTarget()) && !getTarget().getType().isBuilding() && Broodwar->getFrameCount() - getTarget().getLastVisibleFrame() > 240 && Broodwar->getFrameCount() - getLastAttackFrame() > 240 && Grids::getAAirCluster(getPosition()) >= 6.0f)
            || ((isHidden() || getType() == Zerg_Lurker) && !Actions::overlapsDetection(unit(), getEngagePosition(), PlayerState::Enemy))
            || (!isFlying() && Actions::overlapsActions(unit(), getEngagePosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, 96))
            || (!isFlying() && Terrain::isInEnemyTerritory(getTilePosition()) && Util::getTime() > Time(8, 00))
            || (!isFlying() && Actions::overlapsActions(unit(), getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, 96));
    }

    bool UnitInfo::globalRetreat()
    {
        const auto targetReach = getType().isFlyer() ? getTarget().getAirReach() : getTarget().getGroundReach();

        return (Grids::getESplash(getWalkPosition()) > 0 && Units::getSplashTargets().find(unit()) == Units::getSplashTargets().end()) // ...unit is within splash radius of a Spider Mine or Scarab
            || (getTarget().isHidden() && getPosition().getDistance(getTarget().getPosition()) <= targetReach)                         // ...target is hidden and Unit is within target reach
            || (getGlobalState() == GlobalState::Retreat && !Terrain::isInAllyTerritory(getTilePosition()))                            // ...global state is retreating
            || (getType() == Zerg_Mutalisk && hasTarget() && !isWithinReach(getTarget()) && getEngagePosition().isValid() && Grids::getEAirThreat(getEngagePosition()) > 0.0 && getHealth() <= 20 && Util::getTime() < Time(8, 0))             // ...unit is a low HP Mutalisk attacking a target under air threat    
            || (getType() == Zerg_Hydralisk && BuildOrder::getCompositionPercentage(Zerg_Lurker) >= 1.00)
            || (isNearSuicide());
    }
}