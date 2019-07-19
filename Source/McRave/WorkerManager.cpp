#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Workers {

    namespace {

        int minWorkers = 0;
        int gasWorkers = 0;
        int boulderWorkers = 0;

        bool closeToResource(UnitInfo& worker)
        {
            // Great Barrier Reef was causing issues with ground distance to the resource
            auto pathPoint = Combat::getClosestRetreatPosition(worker.getResource().getPosition());

            if (!pathPoint.isValid())
                return false;

            auto close = BWEB::Map::getGroundDistance(pathPoint, worker.getPosition()) <= 128.0 || worker.getPosition().getDistance(pathPoint) < 64.0;
            auto sameArea = mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(TilePosition(pathPoint));
            return close || sameArea;
        }

        bool misc(UnitInfo& worker)
        {
            // If worker is potentially stuck, try to find a manner pylon
            // TODO: Use workers target? Check if it's actually targeting pylon?
            if (worker.framesHoldingResource() >= 100 || worker.framesHoldingResource() <= -200) {
                auto &pylon = Util::getClosestUnit(worker.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getType() == UnitTypes::Protoss_Pylon;
                });
                if (pylon && pylon->unit() && pylon->unit()->exists() && pylon->getPosition().getDistance(worker.getPosition()) < 128.0) {
                    if (worker.unit()->getLastCommand().getTarget() != pylon->unit())
                        worker.unit()->attack(pylon->unit());
                    return true;
                }
            }
            return false;
        }

        bool transport(UnitInfo& worker)
        {
            // Workers that have a transport coming to pick them up should not do anything other than returning cargo
            if (worker.hasTransport() && !worker.unit()->isCarryingMinerals() && !worker.unit()->isCarryingGas()) {
                if (worker.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
                    worker.unit()->move(worker.getTransport().getPosition());
                return true;
            }
            return false;
        }

        bool build(UnitInfo& worker)
        {
            Position center = Position(worker.getBuildPosition()) + Position(worker.getBuildingType().tileWidth() * 16, worker.getBuildingType().tileHeight() * 16);
            Position topLeft = Position(worker.getBuildPosition());
            Position botRight = topLeft + Position(worker.getBuildingType().tileWidth() * 32, worker.getBuildingType().tileHeight() * 32);

            // Can't execute build if we have no building
            if (!worker.getBuildingType().isValid() || !worker.getBuildPosition().isValid())
                return false;

            // HACK - Attack any enemies inside the build area
            if (worker.hasTarget() && Util::rectangleIntersect(topLeft, botRight, worker.getTarget().getPosition())) {
                if (Command::attack(worker)) {}
                else if (Command::move(worker)) {}
                return true;
            }

            // Cancel any buildings we don't need or if the worker is stuck
            else if (worker.isStuck() || BuildOrder::buildCount(worker.getBuildingType()) <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings::isBuildable(worker.getBuildingType(), worker.getBuildPosition())) {
                worker.setBuildingType(UnitTypes::None);
                worker.setBuildPosition(TilePositions::Invalid);
                worker.unit()->stop();
                return true;
            }

            // Move to build
            else if (shouldMoveToBuild(worker, worker.getBuildPosition(), worker.getBuildingType())) {

                worker.setDestination(center);

                auto fullyVisible = Broodwar->isVisible(worker.getBuildPosition())
                    && Broodwar->isVisible(worker.getBuildPosition() + TilePosition(worker.getBuildingType().tileWidth(), 0))
                    && Broodwar->isVisible(worker.getBuildPosition() + TilePosition(worker.getBuildingType().tileWidth(), worker.getBuildingType().tileHeight()))
                    && Broodwar->isVisible(worker.getBuildPosition() + TilePosition(0, worker.getBuildingType().tileHeight()));

                if (fullyVisible && worker.getPosition().getDistance(center) < 128.0) {
                    if (Broodwar->self()->minerals() < worker.getBuildingType().mineralPrice() || Broodwar->self()->gas() < worker.getBuildingType().gasPrice())
                        worker.unit()->move(Position(worker.getBuildPosition()));
                    else if (worker.unit()->getOrder() != Orders::PlaceBuilding || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Build)
                        worker.unit()->build(worker.getBuildingType(), worker.getBuildPosition());
                    return true;
                }
                else {

                    if (worker.canCreatePath(worker.getDestination())) {
                        BWEB::Path newPath;
                        newPath.createUnitPath(worker.getPosition(), worker.getDestination());
                        worker.setPath(newPath);
                    }

                    auto newDestination = Util::findPointOnPath(worker.getPath(), [&](Position p) {
                        return p.getDistance(worker.getPosition()) >= 160.0;
                    });
                    if (newDestination.isValid())
                        worker.setDestination(newDestination);
                    else if (worker.hasResource() && worker.getPosition().getDistance(worker.getResource().getPosition()) < 160.0) {
                        worker.command(UnitCommandTypes::Move, Position(worker.getBuildPosition()));
                        return true;
                    }

                    if (Command::move(worker))
                        return true;
                }
            }
            return false;
        }

        bool clearNeutral(UnitInfo& worker)
        {
            auto resourceDepot = Broodwar->self()->getRace().getResourceDepot();
            if (vis(resourceDepot) < 2
                || (BuildOrder::buildCount(resourceDepot) == vis(resourceDepot) && BuildOrder::isOpener())
                || worker.unit()->isCarryingMinerals()
                || worker.unit()->isCarryingGas()
                || boulderWorkers != 0)
                return false;

            // Find boulders to clear
            for (auto &b : Resources::getMyBoulders()) {
                ResourceInfo &boulder = *b;
                if (!boulder.unit() || !boulder.unit()->exists())
                    continue;
                if ((worker.getPosition().getDistance(boulder.getPosition()) <= 320.0 && boulder.getGathererCount() == 0) || (worker.unit()->isGatheringMinerals() && worker.unit()->getOrderTarget() == boulder.unit())) {
                    if (worker.unit()->getOrderTarget() != boulder.unit())
                        worker.unit()->gather(boulder.unit());
                    boulderWorkers = 1;
                    return true;
                }
            }
            return false;
        }

        bool gather(UnitInfo& worker)
        {
            auto resourceExists = worker.hasResource() && worker.getResource().unit() && worker.getResource().unit()->exists();

            // Mine the closest mineral field
            const auto mineRandom =[&]() {
                auto closest = worker.unit()->getClosestUnit(Filter::IsMineralField);
                if (closest && closest->exists() && worker.unit()->getLastCommand().getTarget() != closest)
                    worker.unit()->gather(closest);
            };

            // Check if we need to re-issue a gather command
            const auto shouldIssueGather =[&]() {
                if (worker.hasResource() && worker.getResource().unit()->exists() && (worker.unit()->isGatheringMinerals() || worker.unit()->isGatheringGas() || worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather) && worker.unit()->getTarget() != worker.getResource().unit())
                    return true;
                if (!worker.hasResource() && (worker.unit()->isIdle() || worker.unit()->getLastCommand().getType() != UnitCommandTypes::Gather))
                    return true;
                return false;
            };

            // Can't gather if we are carrying a resource
            if (worker.unit()->isCarryingGas() || worker.unit()->isCarryingMinerals())
                return false;

            // If worker has a resource and it's mineable
            if (worker.hasResource() && worker.getResource().getResourceState() == ResourceState::Mineable) {

                // HACK: Use the closest retreat point, as we know it's always pathable
                auto pathPoint = Combat::getClosestRetreatPosition(worker.getResource().getPosition());
                worker.setDestination(pathPoint);

                // If it's close or same area, don't need a path, set to empty	
                if (closeToResource(worker) || Grids::getAGroundCluster(worker.getPosition()) > 0.0f) {
                    BWEB::Path emptyPath;
                    worker.setPath(emptyPath);
                }

                // If it's far, generate a path
                else if (worker.canCreatePath(worker.getDestination())) {
                    BWEB::Path newPath;
                    newPath.createUnitPath(worker.getPosition(), worker.getDestination());
                    worker.setPath(newPath);
                }

                // See if there's any threat on the path
                auto threatPosition = Util::findPointOnPath(worker.getPath(), [&](Position p) {
                    return worker.getType().isFlyer() ? Grids::getEGroundThreat(p) : Grids::getEAirThreat(p);
                });

                auto newDestination = Util::findPointOnPath(worker.getPath(), [&](Position p) {
                    return p.getDistance(worker.getPosition()) >= 160.0;
                });
                if (newDestination.isValid())
                    worker.setDestination(newDestination);

                // If no threat on path, mine it
                if (!threatPosition.isValid()) {
                    if ((closeToResource(worker) || Grids::getAGroundCluster(worker.getPosition()) > 0.0f)) {
                        if (shouldIssueGather())
                            worker.unit()->gather(worker.getResource().unit());
                    }
                    else if (!closeToResource(worker))
                        Command::move(worker);
                    return true;
                }
            }

            // Mine something else while waiting
            mineRandom();
            return false;
        }

        bool returnCargo(UnitInfo& worker)
        {
            // Can't return cargo if we aren't carrying a resource
            if (!worker.unit()->isCarryingGas() && !worker.unit()->isCarryingMinerals())
                return false;

            auto checkPath = (worker.hasResource() && worker.getPosition().getDistance(worker.getResource().getPosition()) > 320.0) || (!worker.hasResource() && !Terrain::isInAllyTerritory(worker.getTilePosition()));
            if (checkPath) {
                // TODO: Create a path to the closest station and check if it's safe
            }

            // TODO: Check if we have a building to place first?
            if (worker.unit()->getOrder() != Orders::ReturnMinerals && worker.unit()->getOrder() != Orders::ReturnGas && worker.unit()->getLastCommand().getType() != UnitCommandTypes::Return_Cargo)
                worker.unit()->returnCargo();
            return true;
        }

        void updateAssignment(UnitInfo& worker)
        {
            auto threatPosition = Util::findPointOnPath(worker.getPath(), [&](Position p) {
                return worker.getType().isFlyer() ? Grids::getEGroundThreat(p) : Grids::getEAirThreat(p);
            });

            // Check the status of the worker and the assigned resource
            const auto injured = worker.unit()->getHitPoints() + worker.unit()->getShields() < worker.getType().maxHitPoints() + worker.getType().maxShields();
            const auto threatened = (worker.hasResource() && !closeToResource(worker) && threatPosition.isValid()) || (worker.hasResource() && closeToResource(worker) && int(worker.getTargetedBy().size()) > 0);
            const auto excessAssigned = worker.hasResource() && !injured && !threatened && worker.getResource().getGathererCount() >= 3 + int(worker.getResource().getType().isRefinery());

            // Check if worker needs a re-assignment
            const auto isGasWorker = worker.hasResource() && worker.getResource().getType().isRefinery();
            const auto isMineralWorker = worker.hasResource() && worker.getResource().getType().isMineralField();
            const auto needGas = !Resources::isGasSaturated() && isMineralWorker && (gasWorkers < BuildOrder::gasWorkerLimit() || !BuildOrder::isOpener());
            const auto needMinerals = !Resources::isMinSaturated() && isGasWorker && gasWorkers > BuildOrder::gasWorkerLimit();
            const auto needNewAssignment = !worker.hasResource() || needGas || needMinerals || threatened || excessAssigned;

            auto closestStation = BWEB::Stations::getClosestStation(worker.getTilePosition());
            auto distBest = (injured || threatened) ? 0.0 : DBL_MAX;
            auto oldResource = worker.hasResource() ? worker.getResource().shared_from_this() : nullptr;
            vector<BWEB::Station *> safeStations;

            const auto resourceReady = [&](ResourceInfo& resource, int i) {
                if (!resource.unit()
                    || resource.getType() == UnitTypes::Resource_Vespene_Geyser
                    || (resource.unit()->exists() && !resource.unit()->isCompleted())
                    || resource.getGathererCount() >= i
                    || resource.getResourceState() == ResourceState::None)
                    return false;
                return true;
            };

            // Return if we dont need an assignment
            if (!needNewAssignment)
                return;

            // 1) Find safe stations to mine resources from
            if (closestStation) {
                for (auto &s : Stations::getMyStations()) {
                    auto station = s.second;
                    auto stationPath = Stations::pathStationToStation(closestStation, station);

                    // If worker is close, it must be safe
                    if (worker.getPosition().getDistance(station->getResourceCentroid()) < 320.0 || mapBWEM.GetArea(worker.getTilePosition()) == station->getBWEMBase()->GetArea())
                        safeStations.push_back(station);

                    // If worker is far, we need to check if it's safe
                    else if (stationPath) {
                        auto threatPosition = Util::findPointOnPath(*stationPath, [&](Position p) {
                            return worker.getType().isFlyer() ? Grids::getEGroundThreat(p) : Grids::getEAirThreat(p);
                        });

                        if (!threatPosition.isValid())
                            safeStations.push_back(station);
                    }
                }
            }

            // 2) Check if we need gas workers
            if (needGas || !worker.hasResource()) {
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = *r;

                    if (!resourceReady(resource, 3))
                        continue;
                    if (!resource.hasStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                        continue;

                    auto closestWorker = Util::getClosestUnit(resource.getPosition(), PlayerState::Self, [&](auto &u) {
                        return u.getRole() == Role::Worker && (!u.hasResource() || u.getResource().getType().isMineralField());
                    });
                    if (worker.shared_from_this() != closestWorker)
                        continue;

                    auto dist = resource.getPosition().getDistance(worker.getPosition());
                    if ((dist < distBest && !injured) || (dist > distBest && injured)) {
                        worker.setResource(r.get());
                        distBest = dist;
                    }
                }
            }

            // 3) Check if we need mineral workers
            if (needMinerals || !worker.hasResource()) {
                for (int i = 1; i <= 2; i++) {
                    for (auto &r : Resources::getMyMinerals()) {

                        auto &resource = *r;
                        if (!resourceReady(resource, i))
                            continue;
                        if (!resource.hasStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                            continue;

                        double dist = resource.getPosition().getDistance(worker.getPosition());
                        if ((dist < distBest && !injured && !threatened) || (dist > distBest && (injured || threatened))) {
                            worker.setResource(r.get());
                            distBest = dist;
                        }
                    }

                    // Don't check again if we assigned one
                    if (worker.hasResource() && worker.getResource().shared_from_this() != oldResource)
                        break;
                }

            }

            // 4) Assign resource
            if (worker.hasResource()) {

                // Remove current assignemt
                if (oldResource) {
                    oldResource->getType().isMineralField() ? minWorkers-- : gasWorkers--;
                    oldResource->removeTargetedBy(worker.weak_from_this());
                }

                // Add next assignment
                worker.getResource().getType().isMineralField() ? minWorkers++ : gasWorkers++;
                worker.getResource().addTargetedBy(worker.weak_from_this());

                BWEB::Path emptyPath;
                worker.setPath(emptyPath);

                // HACK: Update saturation checks
                Resources::onFrame();
            }
        }

        constexpr tuple commands{ misc, transport, returnCargo, clearNeutral, build, gather };
        void updateDecision(UnitInfo& worker)
        {
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Transport"),
                make_pair(2, "ReturnCargo"),
                make_pair(3, "ClearNeutral"),
                make_pair(4, "Build"),
                make_pair(5, "Gather")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = worker.getType().isBuilding() ? -16 : worker.getType().width() / 2;
            int i = Util::iterateCommands(commands, worker);
            Broodwar->drawTextMap(worker.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateWorkers()
        {
            // Sort units by distance to destination
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Worker) {
                    updateAssignment(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame() {
        Visuals::startPerfTest();
        boulderWorkers = 0; // HACK: Need a better solution to limit boulder workers
        updateWorkers();
        Visuals::endPerfTest("Workers");
    }

    void removeUnit(UnitInfo& worker) {
        worker.getResource().getType().isRefinery() ? gasWorkers-- : minWorkers--;
        worker.getResource().removeTargetedBy(worker.weak_from_this());
        worker.setResource(nullptr);
    }

    bool shouldMoveToBuild(UnitInfo& worker, TilePosition tile, UnitType type) {
        auto center = Position(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);
        auto mineralIncome = max(0.0, double(Workers::getMineralWorkers() - 1) * 0.045);
        auto gasIncome = max(0.0, double(Workers::getGasWorkers() - 1) * 0.07);
        auto speed = worker.getSpeed();
        auto dist = mapBWEM.GetArea(worker.getTilePosition()) ? BWEB::Map::getGroundDistance(worker.getPosition(), center) : worker.getPosition().getDistance(Position(worker.getBuildPosition()));
        auto time = (dist / speed) + 50.0;
        auto enoughGas = worker.getBuildingType().gasPrice() > 0 ? Broodwar->self()->gas() + int(gasIncome * time) >= worker.getBuildingType().gasPrice() : true;
        auto enoughMins = worker.getBuildingType().mineralPrice() > 0 ? Broodwar->self()->minerals() + int(mineralIncome * time) >= worker.getBuildingType().mineralPrice() : true;
        auto waitInstead = dist < 160.0 && (BuildOrder::isFastExpand() || BuildOrder::isProxy());

        return (enoughGas && enoughMins) || waitInstead;
    };

    int getMineralWorkers() { return minWorkers; }
    int getGasWorkers() { return gasWorkers; }
    int getBoulderWorkers() { return boulderWorkers; }
}