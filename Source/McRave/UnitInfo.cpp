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
            if (unit->isFlying())
                return 256.0;

            if (Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Carrier) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Arbiter) > 0)
                return 480.0;
            if (Players::getTotalCount(PlayerState::Enemy, Terran_Marine) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Bunker) > 0
                || Players::getTotalCount(PlayerState::Enemy, Terran_Missile_Turret) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Lurker) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Sunken_Colony) > 0
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0)
                return 320.0;
            return 256.0;
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

            // Points        
            position                    = unit()->getPosition();
            tilePosition                = t.isBuilding() ? unit()->getTilePosition() : TilePosition(unit()->getPosition());
            walkPosition                = calcWalkPosition(this);
            destination                 = Positions::Invalid;
            goal                        = Positions::Invalid;

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

            updateTarget();
            updateHidden();
            updateStuckCheck();
        }

        if (getPlayer()->isEnemy(Broodwar->self()))
            updateThreatening();

        // Check if this unit is close to a splash unit
        if (getPlayer() == Broodwar->self() && flying) {
            nearSplash = false;
            auto closestSplasher = Util::getClosestUnit(position, PlayerState::Enemy, [&](auto &u) {
                return u.getType() == Protoss_Corsair || u.getType() == Terran_Valkyrie || u.getType() == Zerg_Devourer;
            });

            if (closestSplasher && closestSplasher->getPosition().getDistance(position) < 320.0)
                nearSplash = true;
        }

        // Check if this unit is targeted by a suicidal unit
        if (getPlayer() == Broodwar->self() && flying) {
            targetedBySuicide = false;
            for (auto &target : getTargetedBy()) {
                if (!target.expired() && target.lock()->isSuicidal()) {
                    targetedBySuicide = true;
                    nearSplash = true;
                }
            }
        }
    }

    void UnitInfo::updateTarget()
    {
        // Reset target
        target = weak_ptr<UnitInfo>();

        // Update my target
        if (getPlayer() == Broodwar->self()) {
            if (getType() == Terran_Vulture_Spider_Mine) {
                if (unit()->getOrderTarget())
                    target = Units::getUnitInfo(unit()->getOrderTarget());
            }
            else
                Targets::getTarget(*this);
        }

        // Assume enemy target
        else if (getPlayer()->isEnemy(Broodwar->self())) {
            if (unit()->getOrderTarget()) {
                auto &targetInfo = Units::getUnitInfo(unit()->getOrderTarget());
                if (targetInfo) {
                    target = targetInfo;
                    targetInfo->getTargetedBy().push_back(this->shared_from_this());
                }
            }
            else
                target = weak_ptr<UnitInfo>();
        }
    }

    void UnitInfo::updateStuckCheck() {

        // Check if a worker is being blocked from mining or returning cargo
        if (unit()->isCarryingGas() || unit()->isCarryingMinerals())
            resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
        else if (unit()->isGatheringGas() || unit()->isGatheringMinerals())
            resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
        else
            resourceHeldFrames = 0;

        // Check if clipped between terrain or buildings
        if (getTilePosition().isValid() && BWEB::Map::isUsed(getTilePosition()) == None && BWEB::Map::isWalkable(getTilePosition())) {

            vector<TilePosition> directions{ {1,0}, {-1,0}, {0, 1}, {0,-1} };
            bool trapped = true;

            // Check if the unit is stuck by terrain and buildings
            for (auto &tile : directions) {
                auto current = getTilePosition() + tile;
                if (BWEB::Map::isUsed(current) == None || BWEB::Map::isWalkable(current))
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
        if (getPlayer() != Broodwar->self() || getLastTile() != getTilePosition() || !unit()->isMoving() || unit()->getLastCommand().getType() == UnitCommandTypes::Stop || getLastAttackFrame() == Broodwar->getFrameCount())
            lastTileMoveFrame = Broodwar->getFrameCount();
    }

    void UnitInfo::updateHidden()
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
    }

    void UnitInfo::updateThreatening()
    {
        // Determine how close it is to strategic locations
        auto maxRange = max(getGroundRange(), getAirRange());
        auto choke = Terrain::isDefendNatural() ? BWEB::Map::getNaturalChoke() : BWEB::Map::getMainChoke();
        auto closestGeo = BWEB::Map::getClosestChokeTile(choke, getPosition());
        auto closestStation = Stations::getClosestStation(PlayerState::Self, getPosition());

        // If the unit is close to stations, defenses or resources owned by us
        auto atHome = (Stations::getMyStations().size() <= 2 || !closestStation) ? Terrain::isInAllyTerritory(getTilePosition()) : (closestStation && getPosition().getDistance(closestStation->getBWEMBase()->Center()) < getGroundRange());
        auto atChoke = getPosition().getDistance(closestGeo) <= maxRange;
        auto inRangeOfResources = closestStation && closestStation->getResourceCentroid().getDistance(getPosition()) < max({ getAirRange(), getGroundRange(), 200.0 });

        // If the unit attacked defenders, citizens or is building something
        auto attackedDefender = hasAttackedRecently() && hasTarget() && Terrain::isInAllyTerritory(getTarget().getTilePosition()) && getTarget().getRole() == Role::Combat;
        auto attackedWorkers = hasAttackedRecently() && hasTarget() && Terrain::isInAllyTerritory(getTarget().getTilePosition()) && getTarget().getRole() == Role::Worker;
        auto attackedBuildings = hasAttackedRecently() && hasTarget() && getTarget().getType().isBuilding();
        auto constructing = unit()->exists() && (unit()->isConstructing() || unit()->getOrder() == Orders::ConstructingBuilding || unit()->getOrder() == Orders::PlaceBuilding);
        auto preventRunby = getType() == Terran_Vulture && Broodwar->self()->getRace() == Races::Zerg && BuildOrder::isPlayPassive();

        bool threateningThisFrame = false;

        // Building
        if (getType().isBuilding()) {
            threateningThisFrame = Buildings::overlapsQueue(*this, getPosition())
                || ((atChoke || atHome) && (airDamage > 0.0 || groundDamage > 0.0 || getType() == Protoss_Shield_Battery || getType().isRefinery()))
                || inRangeOfResources;
        }

        // Worker
        else if (getType().isWorker())
            threateningThisFrame = (atHome || atChoke) && (constructing || attackedWorkers || attackedDefender || attackedBuildings);

        // Unit
        else {
            auto attackedFragileBuilding = attackedBuildings && !getTarget().isHealthy();

            if (getType().isFlyer())
                threateningThisFrame = inRangeOfResources || attackedDefender || attackedWorkers;
            else if (Strategy::defendChoke())
                threateningThisFrame = inRangeOfResources || (((atChoke || atHome) && (attackedWorkers || attackedFragileBuilding || attackedDefender)) || (atHome && preventRunby));
            else
                threateningThisFrame = inRangeOfResources || ((atChoke || atHome) && (attackedWorkers || attackedFragileBuilding));
        }

        // If determined threatening this frame
        if (threateningThisFrame) {
            lastThreateningFrame = Broodwar->getFrameCount();
            threatening = true;
        }

        threatening = Broodwar->getFrameCount() - lastThreateningFrame < 64;
    }

    void UnitInfo::createDummy(UnitType t) {
        type                    = t;
        player                    = Broodwar->self();
        groundRange                = Math::groundRange(*this);
        airRange                = Math::airRange(*this);
        groundDamage            = Math::groundDamage(*this);
        airDamage                = Math::airDamage(*this);
        speed                     = Math::moveSpeed(*this);
    }

    bool UnitInfo::move(UnitCommandType command, Position here)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames() - 1;

        if (cancelAttackRisk)
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
        if (getType().isFlyer()) {
            auto distance = int(getPosition().getDistance(here));
            auto haltDistance = max({ distance, 32, getType().haltDistance() / 256 }) + 32.0;

            if (distance > 0) {
                here = getPosition() - ((getPosition() - here) * haltDistance / distance);
                here = Util::clipLine(getPosition(), here);
            }
        }

        // Add action and grid movement
        if (command == UnitCommandTypes::Move && getPosition().getDistance(here) < 128.0) {
            Actions::addAction(unit(), here, getType(), PlayerState::Self);
            Grids::addMovement(here, getType());
        }

        // If this is a new order or new command than what we're requesting, we can issue it
        if (newOrder() || newCommand()) {
            if (command == UnitCommandTypes::Move)
                unit()->move(here);
            else if (command == UnitCommandTypes::Stop)
                unit()->stop();
        }
        return true;
    }

    bool UnitInfo::click(UnitCommandType command, UnitInfo& targetUnit)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames() - 1;

        if (cancelAttackRisk)
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
            || isSpellcaster())
            return false;

        auto weaponCooldown = getType() == Protoss_Reaver ? 60 : (getTarget().getType().isFlyer() ? getType().airWeapon().damageCooldown() : getType().groundWeapon().damageCooldown());
        auto cooldown = lastAttackFrame + weaponCooldown - Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - (getType().turnRadius() / 2);
        auto range = (getTarget().getType().isFlyer() ? getAirRange() : getGroundRange());

        auto boxDistance = Util::boxDistance(getType(), getPosition(), getTarget().getType(), getTarget().getPosition());
        auto cooldownReady = cooldown <= (boxDistance - range) / (hasTransport() ? getTransport().getSpeed() : getSpeed());
        return cooldownReady;
    }

    bool UnitInfo::canStartGather()
    {
        const auto hasMineableResource = hasResource() && getResource().getResourceState() == ResourceState::Mineable;

        if (hasMineableResource && (isWithinGatherRange() || Grids::getAGroundCluster(getPosition()) > 0.0f) && getResource().unit()->exists() && unit()->getTarget() != getResource().unit())
            return true;
        if (!hasMineableResource && (unit()->isIdle() || unit()->getLastCommand().getType() != UnitCommandTypes::Gather))
            return true;
        if (Buildings::overlapsQueue(*this, getPosition()))
            return true;
        return false;
    }

    bool UnitInfo::canStartCast(TechType tech)
    {
        if (!hasTarget()
            || !getPlayer()->hasResearched(tech)
            || Actions::overlapsActions(unit(), getTarget().getPosition(), tech, PlayerState::Self, int(Util::getCastRadius(tech))))
            return false;

        auto energyNeeded = tech.energyCost() - energy;
        auto framesToEnergize = 17.856 * energyNeeded;
        auto spellReady = energy >= tech.energyCost();
        auto spellWillBeReady = framesToEnergize < getEngDist() / (hasTransport() ? getTransport().getSpeed() : getSpeed());

        if (!spellReady && !spellWillBeReady)
            return false;

        if (isSpellcaster() && getEngDist() >= 64.0)
            return true;

        auto ground = Grids::getEGroundCluster(getTarget().getPosition());
        auto air = Grids::getEAirCluster(getTarget().getPosition());

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
            || getPosition().getDistance(getTransport().getPosition()) > 128.0)
            return false;

        // Check if we Unit being attacked by multiple bullets
        auto bulletCount = 0;
        for (auto &bullet : BWAPI::Broodwar->getBullets()) {
            if (bullet && bullet->exists() && bullet->getPlayer() != BWAPI::Broodwar->self() && bullet->getTarget() == bwUnit)
                bulletCount++;
        }

        auto range = getTarget().getType().isFlyer() ? getAirRange() : getGroundRange();
        auto cargoReady = getType() == BWAPI::UnitTypes::Protoss_High_Templar ? canStartCast(BWAPI::TechTypes::Psionic_Storm) : canStartAttack();
        auto threat = Grids::getEGroundThreat(getWalkPosition()) > 0.0;

        return getLocalState() == LocalState::Retreat || getEngDist() > range + 32.0 || !cargoReady || bulletCount >= 4 || Units::getSplashTargets().find(bwUnit) != Units::getSplashTargets().end();
    }

    bool UnitInfo::isWithinReach(UnitInfo& unit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), unit.getType(), unit.getPosition());
        return unit.getType().isFlyer() ? airReach >= boxDistance : groundReach >= boxDistance;
    }

    bool UnitInfo::isWithinRange(UnitInfo& unit)
    {
        auto boxDistance = Util::boxDistance(getType(), getPosition(), unit.getType(), unit.getPosition());
        return unit.getType().isFlyer() ? max(getAirRange(), 64.0) >= boxDistance : max(getGroundRange(), 64.0) >= boxDistance;
    }

    bool UnitInfo::isWithinBuildRange()
    {
        if (!getBuildPosition().isValid())
            return false;

        const auto center = Position(getBuildPosition()) + Position(getBuildType().tileWidth() * 16, getBuildType().tileHeight() * 16);
        const auto close = getPosition().getDistance(center) <= 160.0;
        const auto sameArea = mapBWEM.GetArea(getTilePosition()) == mapBWEM.GetArea(TilePosition(center));

        return close || sameArea;
    }

    bool UnitInfo::isWithinGatherRange()
    {
        if (!hasResource())
            return false;

        // Great Barrier Reef was causing issues with ground distance to the resource
        const auto pathPoint = Combat::getClosestRetreatPosition(getResource().getPosition());

        if (!pathPoint.isValid())
            return false;

        const auto close = BWEB::Map::getGroundDistance(pathPoint, getPosition()) <= 128.0 || getPosition().getDistance(pathPoint) < 64.0;
        const auto sameArea = mapBWEM.GetArea(getTilePosition()) == mapBWEM.GetArea(TilePosition(pathPoint));
        return close || sameArea;
    }

    bool UnitInfo::localEngage()
    {
        return ((!getType().isFlyer() && getTarget().isSiegeTank() && getTarget().getTargetedBy().size() >= 4 && ((isWithinRange(getTarget()) && getGroundRange() > 32.0) || (isWithinReach(getTarget()) && getGroundRange() <= 32.0)))
            || (isHidden() && !Actions::overlapsDetection(unit(), getEngagePosition(), PlayerState::Enemy))
            || (getType() == Protoss_Reaver && !unit()->isLoaded() && isWithinRange(getTarget()))
            || (getTarget().getType() == Terran_Vulture_Spider_Mine && !getTarget().isBurrowed())
            || (hasTransport() && !unit()->isLoaded() && getType() == Protoss_High_Templar && canStartCast(TechTypes::Psionic_Storm) && isWithinRange(getTarget()))
            || (hasTransport() && !unit()->isLoaded() && getType() == Protoss_Reaver && canStartAttack()) && isWithinRange(getTarget())
            || (getGroundRange() < 64 && !isFlying() && Actions::overlapsActions(unit(), getEngagePosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, 32)));
    }

    bool UnitInfo::localRetreat()
    {
        return (getType() == Protoss_Zealot && hasTarget() && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && getTarget().getType() == Terran_Vulture)                 // ...unit is a slow Zealot attacking a Vulture
            || (getType() == Protoss_Corsair && hasTarget() && getTarget().getType() == Zerg_Scourge && com(Protoss_Corsair) < 6)                                                                // ...unit is a Corsair attacking Scourge with less than 6 completed Corsairs
            || (getType() == Terran_Medic && getEnergy() <= TechTypes::Healing.energyCost())                                                                                                     // ...unit is a Medic with no energy
            || (getType() == Zerg_Mutalisk && getEngagePosition().isValid() && Grids::getEAirThreat(getEngagePosition()) > 0.0 && getHealth() <= 50 && Util::getTime() < Time(8, 0))             // ...unit is a low HP Mutalisk attacking a target under air threat
            || (getType().maxShields() > 0 && getPercentShield() < LOW_SHIELD_PERCENT_LIMIT && !BuildOrder::isRush() && Broodwar->getFrameCount() < 10000)                                       // ...unit is a low shield unit in the early stages of the game
            || (getType() == Terran_SCV && Broodwar->getFrameCount() > 12000)                                                                                                                    // ...unit is an SCV outside of early game
            || (isLightAir() && hasTarget() && getType().maxShields() > 0 && getTarget().getType() == Zerg_Overlord && Grids::getEAirThreat(getEngagePosition()) * 5.0 > (double)getShields())   // ...unit is a low shield light air attacking a Overlord under threat greater than our shields
            || (getType().getRace() == Races::Zerg && !isHealthy() && Util::getTime() < Time(8, 0))                                                                                              // ...unit is a Zerg unit and is not healthy in early game;
            || (isTargetedBySuicide())
            || (hasTarget() && getTarget().isSuicidal() && getTarget().unit()->getOrderTargetPosition().getDistance(getPosition()) < 64.0);
    }

    bool UnitInfo::globalEngage()
    {
        return (getTarget().isThreatening() && !getTarget().isHidden())                                                                                                                           // ...target is threatening                    
            || (!getType().isWorker() && getGroundRange() > getTarget().getGroundRange() && Terrain::isInAllyTerritory(getTarget().getTilePosition()) && !getTarget().isHidden())                 // ...unit can get free hits in our territory
            || unit()->isIrradiated();
    }

    bool UnitInfo::globalRetreat()
    {
        const auto targetReach = getType().isFlyer() ? getTarget().getAirReach() : getTarget().getGroundReach();

        return Grids::getESplash(getWalkPosition()) > 0                                                                                // ...unit is within splash radius of a Spider Mine or Scarab
            || (getTarget().isHidden() && getPosition().getDistance(getTarget().getPosition()) <= targetReach)                         // ...target is hidden and Unit is within target reach
            || getGlobalState() == GlobalState::Retreat;                                                                               // ...global state is retreating
    }
}