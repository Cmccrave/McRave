#include "McRave.h"

using namespace std;
using namespace BWAPI;

namespace McRave
{
    UnitInfo::UnitInfo() {}

    void UnitInfo::setLastPositions()
    {
        lastPos = position;
        lastTile = tilePosition;
        lastWalk =  walkPosition;
    }

    void UnitInfo::verifyPaths()
    {
        if (lastTile != this->unit()->getTilePosition()) {
            BWEB::Path emptyPath;
            this->setPath(emptyPath);
        }
    }

    void UnitInfo::update()
    {
        auto t = thisUnit->getType();
        auto p = thisUnit->getPlayer();

        if (thisUnit->exists()) {

            setLastPositions();
            verifyPaths();

            // Points		
            position				= thisUnit->getPosition();
            destination				= Positions::Invalid;
            goal                    = Positions::Invalid;
            tilePosition			= Math::getTilePosition(thisUnit);
            walkPosition			= Math::getWalkPosition(thisUnit);

            // Stats
            unitType				= t;
            player					= p;
            health					= thisUnit->getHitPoints() > 0 ? thisUnit->getHitPoints() : health;
            shields					= thisUnit->getShields() > 0 ? thisUnit->getShields() : shields;
            energy					= thisUnit->getEnergy();
            percentHealth			= t.maxHitPoints() > 0 ? double(health) / double(t.maxHitPoints()) : 0.0;
            percentShield			= t.maxShields() > 0 ? double(shields) / double(t.maxShields()) : 0.0;
            percentTotal			= t.maxHitPoints() + t.maxShields() > 0 ? double(health + shields) / double(t.maxHitPoints() + t.maxShields()) : 0.0;
            groundRange				= Math::groundRange(*this);
            groundDamage			= Math::groundDamage(*this);
            groundReach				= Math::groundReach(*this);
            airRange				= Math::airRange(*this);
            airReach				= Math::airReach(*this);
            airDamage				= Math::airDamage(*this);
            speed 					= Math::moveSpeed(*this);
            minStopFrame			= Math::stopAnimationFrames(t);
            visibleGroundStrength	= Math::visibleGroundStrength(*this);
            maxGroundStrength		= Math::maxGroundStrength(*this);
            visibleAirStrength		= Math::visibleAirStrength(*this);
            maxAirStrength			= Math::maxAirStrength(*this);
            priority				= Math::priority(*this);

            burrowed				= (unitType != UnitTypes::Terran_Vulture_Spider_Mine && thisUnit->isBurrowed()) || thisUnit->getOrder() == Orders::Burrowing;
            flying					= thisUnit->isFlying() || thisUnit->getType().isFlyer() || thisUnit->getOrder() == Orders::LiftingOff || thisUnit->getOrder() == Orders::BuildingLiftOff;

            // States
            lState					= LocalState::None;
            gState					= GlobalState::None;
            tState					= TransportState::None;

            // Frames
            remainingTrainFrame =   max(0, remainingTrainFrame - 1);
            lastAttackFrame			= (t != UnitTypes::Protoss_Reaver && (thisUnit->isStartingAttack() || thisUnit->isRepairing())) ? Broodwar->getFrameCount() : lastAttackFrame;

            // BWAPI won't reveal isStartingAttack when hold position is executed if the unit can't use hold position, XIMP uses this on workers
            if (player != Broodwar->self() && unitType.isWorker()) {
                if (thisUnit->getGroundWeaponCooldown() == 1 || thisUnit->getAirWeaponCooldown() == 1)
                    lastAttackFrame = Broodwar->getFrameCount();
            }

            target = weak_ptr<UnitInfo>();
            updateTarget();
            updateStuckCheck();
        }

        // If this is a spider mine and doesn't have a target, it's still considered burrowed, reduce its ground reach
        if (unitType == UnitTypes::Terran_Vulture_Spider_Mine && (!thisUnit->exists() || (!target.lock() && thisUnit->getSecondaryOrder() == Orders::Cloak))) {
            burrowed = true;
            groundReach = groundRange;
        }

        if (player->isEnemy(Broodwar->self()))
            updateThreatening();
    }

    void UnitInfo::updateTarget()
    {
        // Update my target
        if (player && player == Broodwar->self()) {
            if (unitType == UnitTypes::Terran_Vulture_Spider_Mine) {
                if (thisUnit->getOrderTarget())
                    target = Units::getUnitInfo(thisUnit->getOrderTarget());
            }
            else
                Targets::getTarget(*this);
        }

        // Assume enemy target
        else if (player && player->isEnemy(Broodwar->self())) {
            if (thisUnit->getOrderTarget()) {
                auto &targetInfo = Units::getUnitInfo(thisUnit->getOrderTarget());
                if (targetInfo) {
                    target = targetInfo;
                    targetInfo->getTargetedBy().push_back(make_shared<UnitInfo>(*this));
                }
            }
            else
                target = weak_ptr<UnitInfo>();
        }
    }

    void UnitInfo::updateStuckCheck() {

        // Check if a worker is being blocked from mining or returning cargo
        if (thisUnit->isCarryingGas() || thisUnit->isCarryingMinerals())
            resourceHeldFrames = max(resourceHeldFrames, 0) + 1;
        else if (thisUnit->isGatheringGas() || thisUnit->isGatheringMinerals())
            resourceHeldFrames = min(resourceHeldFrames, 0) - 1;
        else
            resourceHeldFrames = 0;

        // Check if a unit hasn't moved in a while but is trying to
        if (player != Broodwar->self() || lastTile != tilePosition || !thisUnit->isMoving() || thisUnit->getLastCommand().getType() == UnitCommandTypes::Stop || lastAttackFrame == Broodwar->getFrameCount())
            lastTileMoveFrame = Broodwar->getFrameCount();

        // Check if clipped between terrain or buildings
        if (this->getTilePosition().isValid() && BWEB::Map::isUsed(this->getTilePosition()) == UnitTypes::None && Util::isWalkable(this->getTilePosition())) {
            vector<TilePosition> directions{ {1,0}, {-1,0}, {0, 1}, {0,-1} };
            bool trapped = true;
            for (auto &tile : directions) {
                auto current = this->getTilePosition() + tile;
                if (BWEB::Map::isUsed(current) == UnitTypes::None && Util::isWalkable(current))
                    trapped = false;
            }

            if (trapped)
                lastTileMoveFrame = 0;
        }

        // HACK: Workers are never stuck if they're within range of building
        if (this->getBuildPosition().isValid() && this->getTilePosition().getDistance(this->getBuildPosition()) < 2)
            lastTileMoveFrame= Broodwar->getFrameCount();
    }

    void UnitInfo::createDummy(UnitType t) {
        unitType				= t;
        player					= Broodwar->self();
        groundRange				= Math::groundRange(*this);
        airRange				= Math::airRange(*this);
        groundDamage			= Math::groundDamage(*this);
        airDamage				= Math::airDamage(*this);
        speed 					= Math::moveSpeed(*this);
    }

    bool UnitInfo::command(UnitCommandType command, Position here)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames();

        if (cancelAttackRisk)
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto newOrderPosition = thisUnit->getOrderTargetPosition().getDistance(here) > 96;
            auto newOrderType = thisUnit->getOrder() != Orders::Move;
            return (newOrderPosition || newOrderType);
        };

        // Check if this is a new command
        const auto newCommand = [&]() {
            auto newCommandPosition = (thisUnit->getLastCommand().getType() != command || thisUnit->getLastCommand().getTargetPosition().getDistance(here) > 32);
            return newCommandPosition;
        };

        // Check if we should overshoot for halting distance
        if (unitType.isFlyer()) {
            auto distance = position.getApproxDistance(here);
            auto haltDistance = max({ distance, 32, unitType.haltDistance() / 256 });

            if (distance > 0) {
                here = position - ((position - here) * haltDistance / distance);
                here = Util::clipLine(position, here);
            }
        }

        // Add action and grid movement
        Command::addAction(thisUnit, here, unitType, PlayerState::Self);
        Grids::addMovement(WalkPosition(here), unitType);

        // If this is a new order or new command than what we're requesting, we can issue it
        if (newOrder() ||newCommand()) {
            if (command == UnitCommandTypes::Move)
                thisUnit->move(here);
            return true;
        }
        return false;
    }

    bool UnitInfo::command(UnitCommandType command, UnitInfo& targetUnit)
    {
        // Check if we need to wait a few frames before issuing a command due to stop frames
        auto frameSinceAttack = Broodwar->getFrameCount() - lastAttackFrame;
        auto cancelAttackRisk = frameSinceAttack <= minStopFrame - Broodwar->getLatencyFrames();

        if (cancelAttackRisk)
            return false;

        // Check if this is a new order
        const auto newOrder = [&]() {
            auto newOrderTarget = thisUnit->getOrderTarget() && thisUnit->getOrderTarget() != targetUnit.unit();
            return newOrderTarget;
        };

        // Check if this is a new command
        const auto newCommand = [&]() {
            auto newCommandTarget = (thisUnit->getLastCommand().getType() != command || thisUnit->getLastCommand().getTarget() != targetUnit.unit());
            return newCommandTarget;
        };

        // Add action
        Command::addAction(thisUnit, targetUnit.getPosition(), unitType, PlayerState::Self);

        // If this is a new order or new command than what we're requesting, we can issue it
        if (newOrder() || newCommand() || command == UnitCommandTypes::Right_Click_Unit) {
            if (command == UnitCommandTypes::Attack_Unit)
                thisUnit->attack(targetUnit.unit());
            else if (command == UnitCommandTypes::Right_Click_Unit) {
                thisUnit->rightClick(targetUnit.unit());
                circleBlue();
            }
            return true;
        }
        return false;
    }

    void UnitInfo::updateThreatening()
    {
        if ((burrowed || (thisUnit && thisUnit->exists() && thisUnit->isCloaked())) && !Command::overlapsDetection(thisUnit, position, PlayerState::Self) || Stations::getMyStations().size() > 2) {
            threatening = false;
            return;
        }

        auto node1 = Position(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end1));
        auto node2 = Position(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end2));

        // Define "close" - TODO: define better
        auto atHome = Terrain::isInAllyTerritory(tilePosition);
        auto atDefense = Terrain::isDefendNatural() ? 
            Terrain::getDefendPosition().getDistance(this->getPosition()) <= groundRange 
            || node1.getDistance(this->getPosition()) <= groundRange
            || node2.getDistance(this->getPosition()) <= groundRange

            : Terrain::getDefendPosition().getDistance(this->getPosition()) < 64.0;

        auto close = (!Strategy::defendChoke() && this->getPosition().getDistance(Terrain::getDefendPosition()) < groundReach / 2) || (Strategy::defendChoke() && (atHome || atDefense));
        auto manner = position.getDistance(Terrain::getMineralHoldPosition()) < 256.0;
        auto exists = thisUnit && thisUnit->exists();
        auto attacked = exists && hasAttackedRecently() && target.lock() && (target.lock()->getType().isBuilding() || target.lock()->getType().isWorker()) && (close || atHome);
        auto constructingClose = exists && (position.getDistance(Terrain::getDefendPosition()) < 320.0 || close) && (thisUnit->isConstructing() || thisUnit->getOrder() == Orders::ConstructingBuilding || thisUnit->getOrder() == Orders::PlaceBuilding);
        auto inRangePieces = Terrain::inRangeOfWallPieces(*this);
        auto inRangeDefenses = Terrain::inRangeOfWallDefenses(*this);

        const auto threateningCheck = [&] {
            // Building: blocking any buildings, is close or at home and can attack or is a battery, is a manner building
            if (unitType.isBuilding()) {
                if (Buildings::overlapsQueue(*this, tilePosition))
                    return true;
                if ((close || atHome) && (airDamage > 0.0 || groundDamage > 0.0 || unitType == UnitTypes::Protoss_Shield_Battery || unitType.isRefinery()))
                    return true;
                if (manner)
                    return true;
            }
            // Worker: constructing close, has attacked and close
            else if (unitType.isWorker()) {
                if (constructingClose)
                    return true;
                if (close || attacked)
                    return true;
            }
            // Unit: close, close to shield battery, in territory while defending choke
            else {
                if (Terrain::isDefendNatural()) {
                    if (BuildOrder::isWallNat() && inRangeDefenses)
                        return true;
                    if (!BuildOrder::isWallNat() && (close || attacked))
                        return true;
                }
                else {
                    if (close || attacked)
                        return true;
                }
                if (attacked && inRangePieces)
                    return true;
                if (atHome && Strategy::defendChoke())
                    return true;
                if (com(UnitTypes::Protoss_Shield_Battery) > 0) {
                    auto &battery = Util::getClosestUnit(position, PlayerState::Self, [&](auto &u) {
                        return u.getType() == UnitTypes::Protoss_Shield_Battery && u.unit()->isCompleted();
                    });
                    if (battery && position.getDistance(battery->getPosition()) <= 128.0 && Terrain::isInAllyTerritory(tilePosition))
                        return true;
                }
            }
            return false;
        };

        if (threateningCheck()) {
            lastThreateningFrame = Broodwar->getFrameCount();
            threatening = true;
        }
        threatening = Broodwar->getFrameCount() - lastThreateningFrame < 50;
    }

    bool UnitInfo::canStartAttack()
    {
        if (!this->hasTarget()
            || (this->getGroundDamage() == 0 && this->getAirDamage() == 0)
            || this->isSpellcaster())
            return false;

        // Units that don't hover or fly have animation times to start and continue attacks
        auto cooldown = (this->getTarget().getType().isFlyer() ? this->unit()->getAirWeaponCooldown() : this->unit()->getGroundWeaponCooldown()) - Broodwar->getLatencyFrames();

        if (this->getType() == UnitTypes::Protoss_Reaver)
            cooldown = this->getLastAttackFrame() - Broodwar->getFrameCount() + 60;

        auto cooldownReady = cooldown <= this->getEngDist() / (this->hasTransport() ? this->getTransport().getSpeed() : this->getSpeed());
        return cooldownReady;
    }

    bool UnitInfo::canStartCast(TechType tech)
    {
        if (!target.lock()
            || Command::overlapsActions(thisUnit, target.lock()->getPosition(), tech, PlayerState::Self, Util::getCastRadius(tech)))
            return false;

        auto energyNeeded = tech.energyCost() - energy;
        auto framesToEnergize = 17.856 * energyNeeded;
        auto spellReady = energy >= tech.energyCost();
        auto spellWillBeReady = framesToEnergize < engageDist / (transport.lock() ? transport.lock()->getSpeed() : speed);

        if (!spellReady && !spellWillBeReady)
            return false;

        if (unitType == UnitTypes::Protoss_High_Templar && engageDist >= 64.0)
            return true;

        if (auto currentTarget = target.lock()) {
            auto ground = Grids::getEGroundCluster(currentTarget->getPosition());
            auto air = Grids::getEAirCluster(currentTarget->getPosition());

            if (ground + air >= Util::getCastLimit(tech) || (unitType == UnitTypes::Protoss_High_Templar && currentTarget->isHidden()))
                return true;
        }
        return false;
    }

    bool UnitInfo::canAttackGround()
    {
        return groundDamage > 0.0
            || unitType == UnitTypes::Protoss_High_Templar
            || unitType == UnitTypes::Protoss_Dark_Archon
            || unitType == UnitTypes::Protoss_Carrier
            || unitType == UnitTypes::Terran_Medic
            || unitType == UnitTypes::Terran_Science_Vessel
            || unitType == UnitTypes::Zerg_Defiler
            || unitType == UnitTypes::Zerg_Queen;
    }

    bool UnitInfo::canAttackAir()
    {
        return airDamage > 0.0
            || unitType == UnitTypes::Protoss_High_Templar
            || unitType == UnitTypes::Protoss_Dark_Archon
            || unitType == UnitTypes::Protoss_Carrier
            || unitType == UnitTypes::Terran_Science_Vessel
            || unitType == UnitTypes::Zerg_Defiler
            || unitType == UnitTypes::Zerg_Queen;
    }

    bool UnitInfo::isWithinReach(UnitInfo& unit)
    {
        auto sizes = (max(unit.getType().width(), unit.getType().height()) + max(unitType.width(), unitType.height())) / 2;
        auto dist = position.getDistance(unit.getPosition()) - sizes - 32.0;
        return unit.getType().isFlyer() ? airReach >= dist : groundReach >= dist;
    }

    bool UnitInfo::isWithinRange(UnitInfo& unit)
    {
        auto sizes = (max(unit.getType().width(), unit.getType().height()) + max(unitType.width(), unitType.height())) / 2;
        auto dist = position.getDistance(unit.getPosition()) - sizes - 32.0;
        return unit.getType().isFlyer() ? airRange >= dist : groundRange >= dist;
    }
}