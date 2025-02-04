#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Workers {

    namespace {

        int minWorkers = 0;
        int gasWorkers = 0;
        int boulderWorkers = 0;
        map<Position, double> distChecker;

        const BWEB::Station * const getTransferStation(UnitInfo& unit)
        {
            // Transfer counts depending on if we think it's safe or not
            auto desiredTransfer = 1;
            if (Spy::getEnemyOpener() == "8Rax" || Spy::getEnemyOpener() == "BBS")
                desiredTransfer = (total(Zerg_Creep_Colony) == 0 && total(Zerg_Sunken_Colony) == 0) ? 1 : 0;
            if (Spy::getEnemyOpener() == "9/9" || Spy::getEnemyOpener() == "Proxy9/9")
                desiredTransfer = (total(Zerg_Creep_Colony) == 0 && total(Zerg_Sunken_Colony) == 0) ? 1 : 0;
            if (Units::getImmThreat() > 0.0f)
                desiredTransfer = 0;
            if (Players::ZvZ())
                desiredTransfer = 0;

            // Keep damaged workers in the main
            if (Util::getTime() < Time(4, 00) && unit.getHealth() < unit.getType().maxHitPoints()) {
                if (unit.hasResource() && unit.getResource().lock()->getStation()->isNatural()) {
                    auto closestMain = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self, [&](auto &s) {
                        return s->isMain();
                    });
                    return closestMain;
                }
                return nullptr;
            }

            // Allow some workers to transfer early on so we can make early sunks if needed
            if (Util::getTime() < Time(4, 00) && desiredTransfer > 0 && unit.isHealthy() && vis(UnitTypes::Zerg_Drone) >= 9) {
                for (auto &station : Stations::getStations(PlayerState::Self)) {
                    int droneCount = 0;
                    auto mineable = true;
                    for (auto &mineral : Resources::getMyMinerals()) {
                        if (mineral->hasStation() && mineral->getStation() == station) {
                            if (mineral->getResourceState() != ResourceState::Mineable || mineral->isThreatened())
                                mineable = false;
                            droneCount += mineral->getGathererCount();
                        }
                    }
                    if (mineable && droneCount < desiredTransfer)
                        return station;
                }
            }
            return nullptr;
        }

        vector<const BWEB::Station *> getSafeStations(UnitInfo& unit)
        {
            vector<const BWEB::Station *> safeStations;            

            // Check if current station is safe
            auto stationGrdOkay = false;
            auto stationAirOkay = false;
            if (unit.hasResource()) {
                auto station = unit.getResource().lock()->getStation();
                stationGrdOkay = (Stations::getGroundDefenseCount(station) > 0 || Stations::getColonyCount(station) > 0);
                stationAirOkay = (Stations::getAirDefenseCount(station) > 0 || Stations::getColonyCount(station) > 0);

                // Current station is a fortress, we can stay
                if (stationGrdOkay && stationAirOkay) {
                    safeStations.push_back(station);
                    return safeStations;
                }

                // If this station has a defense to deal with whatever is near us, stay at this station
                auto currentStationSafe = true;
                for (auto &t : unit.getUnitsInReachOfThis()) {
                    if (auto targeter = t.lock()) {
                        if ((targeter->isFlying() && !stationAirOkay) || (!targeter->isFlying() && !stationGrdOkay))
                            currentStationSafe = false;
                    }
                }

                if (currentStationSafe) {
                    safeStations.push_back(unit.getResource().lock()->getStation());
                    return safeStations;
                }
            }

            // Find safe stations to mine resources from
            auto closestStation = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self);
            for (auto &station : Stations::getStations(PlayerState::Self)) {

                // If unit is close, it must be safe
                if (unit.getPosition().getDistance(station->getResourceCentroid()) < 320.0 || mapBWEM.GetArea(unit.getTilePosition()) == station->getBase()->GetArea())
                    safeStations.push_back(station);

                // If station has defenses or it's early, it's probably safe
                else if (Stations::getGroundDefenseCount(station) > 0 || Stations::getAirDefenseCount(station) > 0 || Util::getTime() < Time(6, 00))
                    safeStations.push_back(station);

                else {
                    auto &path = Stations::getPathBetween(closestStation, station);
                    auto threatPos = Util::findPointOnPath(path, [&](auto &t) {
                        return Grids::getGroundThreat(t, PlayerState::Enemy) > 0.0;
                    });
                    if (!threatPos)
                        safeStations.push_back(station);
                }
            }

            return safeStations;
        }

        bool isBuildingSafe(UnitInfo& unit)
        {
            auto buildCenter = Position(unit.getBuildPosition()) + Position(unit.getBuildType().tileWidth() * 16, unit.getBuildType().tileHeight() * 16);

            if (Util::getTime() < Time(4, 00))
                return true;

            for (auto &t : unit.getUnitsTargetingThis()) {
                if (auto targeter = t.lock()) {
                    if (targeter->isThreatening())
                        return false;
                }
            }

            // If around defenders
            auto aroundDefenders = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                if (u->getRole() != Role::Combat && u->getRole() != Role::Defender)
                    return false;

                return (unit.getPosition().getDistance(u->getPosition()) < u->getGroundReach() && u->getPosition().getDistance(buildCenter) < u->getGroundReach())
                    || (mapBWEM.GetArea(unit.getTilePosition()) == mapBWEM.GetArea(unit.getBuildPosition()) && u->getPosition().getDistance(buildCenter) < u->getGroundReach());
            });
            if (aroundDefenders)
                return true;

            // Generate a path that obeys refinery placement as well
            BWEB::Path newPath(unit.getPosition(), buildCenter, unit.getType());
            auto buildingWalkable = [&](const auto &t) {
                return newPath.unitWalkable(t) || (unit.getBuildType().isRefinery() && BWEB::Map::isUsed(t) == UnitTypes::Resource_Vespene_Geyser);
            };
            newPath.generateJPS([&](const TilePosition &t) { return buildingWalkable(t); });

            auto threatPosition = Util::findPointOnPath(newPath, [&](Position p) {
                return Grids::getGroundThreat(p, PlayerState::Enemy) > 0.0 && Broodwar->isVisible(TilePosition(p));
            });

            if (threatPosition && threatPosition.getDistance(unit.getPosition()) < 32.0 && Util::getTime() > Time(5, 00))
                return false;
            return true;
        }

        bool isResourceSafe(UnitInfo& unit)
        {
            // Determine if the resource we're at is safe or if we need to ditch this station entirely
            // 1. Safe, no threats around
            // 2. Any threat exists, but we have burrow and they dont have detection
            // 3. A minor threat exists, move between resources
            // 4. A major threat exists, ditch the station (TODO?)
            return true;
        }

        bool isResourceFlooded(UnitInfo& unit)
        {
            // Determine if we have over the work cap assigned
            if (unit.hasResource() && unit.getPosition().getDistance(unit.getResource().lock()->getPosition()) < 64.0 && unit.getResource().lock()->getGathererCount() >= unit.getResource().lock()->getWorkerCap() + 1)
                return true;

            // Determine if we have excess assigned resources with openings available at the current Station
            if (unit.unit()->isCarryingMinerals() || unit.unit()->isCarryingGas()) {
                if (unit.hasResource() && unit.getResource().lock()->getType().isMineralField()) {
                    for (auto &mineral : Resources::getMyMinerals()) {
                        if (mineral->getStation() == unit.getResource().lock()->getStation() && mineral->getGathererCount() < unit.getResource().lock()->getGathererCount() - 1)
                            return true;
                    }
                }
            }
            return false;
        }

        void updatePath(UnitInfo& unit)
        {
            // Create a path
            if (unit.getDestination().isValid() && (unit.getDestinationPath().getTarget() != TilePosition(unit.getDestination()) || !unit.getDestinationPath().isReachable()) && (!mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(unit.getDestination())) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(unit.getDestination()))))) {
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                auto resourceTile = unit.hasResource() ? unit.getResource().lock()->getTilePosition() : TilePositions::Invalid;
                auto resourceType = unit.hasResource() ? unit.getResource().lock()->getType() : UnitTypes::None;

                const auto resourceWalkable = [&](const TilePosition &tile) {
                    return (unit.hasResource() && !unit.getBuildPosition().isValid() && tile.x >= resourceTile.x && tile.x < resourceTile.x + resourceType.tileWidth() && tile.y >= resourceTile.y && tile.y < resourceTile.y + resourceType.tileHeight())
                        || newPath.unitWalkable(tile);
                };
                newPath.generateJPS(resourceWalkable);
                unit.setDestinationPath(newPath);
            }

            // Set destination to intermediate position along path
            unit.setNavigation(unit.getDestination());
            if (unit.getDestinationPath().getTarget() == TilePosition(unit.getDestination())) {
                auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= 96.0;
                });

                if (newDestination.isValid())
                    unit.setNavigation(newDestination);
            }
            //Visuals::drawLine(unit.getPosition(), unit.getNavigation(), Colors::Orange);
            //Visuals::drawPath(unit.getDestinationPath());
        }

        void updateDestination(UnitInfo& unit)
        {
            // If unit has a building type and we are ready to build
            if (unit.getBuildType() != UnitTypes::None && shouldMoveToBuild(unit, unit.getBuildPosition(), unit.getBuildType()) && isBuildingSafe(unit)) {
                auto center = Position(unit.getBuildPosition()) + Position(unit.getBuildType().tileWidth() * 16, unit.getBuildType().tileHeight() * 16);

                // Probes wants to try to place offcenter so the Probe doesn't have to reset collision on placement
                if (unit.getType() == Protoss_Probe && unit.hasResource()) {
                    center.x += unit.getResource().lock()->getPosition().x > center.x ? unit.getBuildType().dimensionRight() : -unit.getBuildType().dimensionLeft();
                    center.y += unit.getResource().lock()->getPosition().y > center.y ? unit.getBuildType().dimensionDown() : -unit.getBuildType().dimensionUp();
                }

                // Drone wants to be slightly offcenter to prevent a long animation before placing
                if (unit.getType() == Zerg_Drone) {
                    center.x -= 0;
                    center.y -= 7;
                }

                unit.setDestination(center);
            }

            // If unit has a transport
            else if (unit.hasTransport())
                unit.setDestination(unit.getTransport().lock()->getPosition());

            // If unit has a goal
            else if (unit.getGoal().isValid())
                unit.setDestination(unit.getGoal());

            // If unit has a resource
            else if (unit.hasResource() && unit.getResource().lock()->getResourceState() == ResourceState::Mineable && unit.getPosition().getDistance(unit.getResource().lock()->getPosition()) > 96.0)
                unit.setDestination(unit.getResource().lock()->getPosition());
        }

        void updateResource(UnitInfo& unit)
        {
            // Get some information of the workers current assignment
            const auto isGasunit =          unit.hasResource() && unit.getResource().lock()->getType().isRefinery();
            const auto isMineralunit =      unit.hasResource() && unit.getResource().lock()->getType().isMineralField();
            const auto threatened =         unit.hasResource() && unit.getResource().lock()->isThreatened() && (unit.getHealth() < unit.getType().maxHitPoints() || Spy::enemyPressure() || Spy::enemyRush());
            const auto excessAssigned =     isResourceFlooded(unit);
            const auto transferStation =    getTransferStation(unit);

            // Check if unit needs a re-assignment
            const auto needGas =            unit.unit()->isCarryingMinerals() && !Resources::isGasSaturated() && isMineralunit && gasWorkers < BuildOrder::gasWorkerLimit();
            const auto needMinerals =       unit.unit()->isCarryingGas() && !Resources::isMineralSaturated() && isGasunit && gasWorkers > BuildOrder::gasWorkerLimit();
            const auto needNewAssignment =  !unit.hasResource() || needGas || needMinerals || threatened || (excessAssigned && !threatened) || transferStation;
            auto distBest =                 threatened ? 0.0 : DBL_MAX;
            auto oldResource =              unit.hasResource() ? unit.getResource().lock() : nullptr;

            // Return if we dont need an assignment
            if (!needNewAssignment)
                return;

            // Get some information about the safe stations
            auto safeStations =             getSafeStations(unit);

            const auto resourceReady = [&](ResourceInfo& resource, int i) {
                if (!resource.unit()
                    || resource.getType() == UnitTypes::Resource_Vespene_Geyser
                    || resource.getGathererCount() >= i
                    || (resource.getResourceState() != ResourceState::Mineable && Stations::getMiningStationsCount() != 0))
                    return false;
                return true;
            };

            // Check if we need gas units
            if (needGas || !unit.hasResource() || threatened || excessAssigned) {
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = *r;
                    auto allowedGatherCount = threatened ? 50 : resource.getWorkerCap();

                    if (!resourceReady(resource, allowedGatherCount)
                        || (!transferStation && find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                        || (resource.isThreatened() && !threatened))
                        continue;

                    // Find closest unit that we will re-assign
                    if (!threatened) {
                        auto closestunit = Util::getClosestUnit(resource.getPosition(), PlayerState::Self, [&](auto &u) {
                            return u->getRole() == Role::Worker && !u->getBuildPosition().isValid() && (!u->hasResource() || u->getResource().lock()->getType().isMineralField());
                        });

                        // If this is the closest unit and it's doing a return trip
                        if (closestunit && (unit != *closestunit || !unit.unit()->isCarryingMinerals()))
                            continue;
                    }

                    auto dist = resource.getPosition().getDistance(unit.getPosition());
                    if ((dist < distBest && !threatened) || (dist > distBest && threatened)) {
                        unit.setResource(r.get());
                        distBest = dist;
                    }
                }
            }

            // Check if we need mineral units
            if (needMinerals || !unit.hasResource() || threatened || excessAssigned || transferStation) {
                for (int i = 1; i <= 2; i++) {
                    for (auto &r : Resources::getMyMinerals()) {
                        auto &resource = *r;
                        auto allowedGatherCount = threatened ? 50 : i;

                        if (!resourceReady(resource, allowedGatherCount)
                            || (!transferStation && find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                            || (transferStation && resource.getStation() != transferStation)
                            || (resource.isThreatened() && !threatened))
                            continue;

                        auto stationDist = unit.getPosition().getDistance(resource.getStation()->getBase()->Center());
                        auto resourceDist = Util::boxDistance(UnitTypes::Resource_Mineral_Field, resource.getPosition(), Broodwar->self()->getRace().getResourceDepot(), resource.getStation()->getBase()->Center());
                        auto dist = stationDist * resourceDist;

                        if ((dist < distBest && !threatened) || (dist > distBest && threatened)) {
                            unit.setResource(r.get());
                            distBest = dist;
                        }
                    }

                    // Don't check again if we assigned one
                    if (unit.hasResource() && unit.getResource().lock() != oldResource && !threatened)
                        break;
                }
            }

            // Assign resource
            if (unit.hasResource()) {

                // Remove current assignemt
                if (oldResource) {
                    oldResource->getType().isMineralField() ? minWorkers-- : gasWorkers--;
                    oldResource->removeTargetedBy(unit.weak_from_this());
                }

                // Add next assignment
                unit.getResource().lock()->getType().isMineralField() ? minWorkers++ : gasWorkers++;
                unit.getResource().lock()->addTargetedBy(unit.weak_from_this());
                Resources::recheckSaturation();
            }
        }

        void updateBuilding(UnitInfo& unit)
        {
            if (unit.getBuildPosition() == TilePositions::None
                || unit.getBuildType() == UnitTypes::None)
                return;

            if (!canAssignToBuild(unit)) {
                unit.setBuildingType(UnitTypes::None);
                unit.setBuildPosition(TilePositions::Invalid);
            }
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!Units::commandAllowed(unit))
                return;

            // Iterate commands, if one is executed then don't try to execute other commands
            static const auto commands ={ Command::misc, Command::attack, Command::burrow, Command::returnResource, Command::build, Command::clearNeutral, Command::click, Command::move, Command::gather };
            for (auto cmd : commands) {
                if (cmd(unit))
                    break;
            }
        }

        void updateunits()
        {
            // Iterate workers and make decisions
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Worker && unit.isAvailable()) {
                    updateResource(unit);
                    updateBuilding(unit);
                    updateDestination(unit);
                    updatePath(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame() {
        Visuals::startPerfTest();
        boulderWorkers = 0; // HACK: Need a better solution to limit boulder units
        updateunits();
        Visuals::endPerfTest("Workers");
    }

    void removeUnit(UnitInfo& unit) {
        unit.getResource().lock()->getType().isRefinery() ? gasWorkers-- : minWorkers--;
        unit.getResource().lock()->removeTargetedBy(unit.weak_from_this());
        unit.setResource(nullptr);
    }

    bool canAssignToBuild(UnitInfo& unit)
    {
        if (unit.isBurrowed()
            || unit.getRole() != Role::Worker
            || !unit.isHealthy()
            || unit.isStuck()
            || unit.wasStuckRecently()
            || (unit.hasResource() && !unit.getResource().lock()->getType().isMineralField() && Util::getTime() < Time(10, 00))
            || (unit.hasResource() && unit.getResource().lock()->isPocket()))
            return false;
        return true;
    }

    bool shouldMoveToBuild(UnitInfo& unit, TilePosition tile, UnitType type) {
        if (!mapBWEM.GetArea(unit.getTilePosition()))
            return true;
        auto center = Position(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);
        if (unit.getPosition().getDistance(center) < 64.0 && type.isResourceDepot())
            return true;

        auto mineralIncome = max(0.0, double(minWorkers - 1) * 0.045);
        auto gasIncome = max(0.0, double(gasWorkers - 1) * 0.07);
        auto speed = unit.getSpeed();
        auto dist = BWEB::Map::getGroundDistance(unit.getPosition(), center);
        auto time = (dist / speed);
        auto enoughGas = unit.getBuildType().gasPrice() > 0 ? Broodwar->self()->gas() + int(gasIncome * time) >= unit.getBuildType().gasPrice() * 0.8 : true;
        auto enoughMins = unit.getBuildType().mineralPrice() > 0 ? Broodwar->self()->minerals() + int(mineralIncome * time) >= unit.getBuildType().mineralPrice() * 0.8 : true;

        return (enoughGas && enoughMins);
    };

    int getMineralWorkers() { return minWorkers; }
    int getGasWorkers() { return gasWorkers; }
    int getBoulderWorkers() { return boulderWorkers; }
}