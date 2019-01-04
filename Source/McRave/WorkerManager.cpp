#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Workers {

    namespace {

        int minWorkers = 0;
        int gasWorkers = 0; 

        bool closeToResource(UnitInfo& worker)
        {
            auto close = BWEB::Map::getGroundDistance(worker.getResource().getPosition(), worker.getPosition()) <= 320.0;
            auto sameArea = mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition());
            return close || sameArea;
        }

        bool misc(UnitInfo& worker)
        {
            // If worker is potentially stuck, try to find a manner pylon
            // TODO: Use workers target? Check if it's actually targeting pylon?
            if (worker.framesHoldingResource() >= 100 || worker.framesHoldingResource() <= -200) {
                auto pylon = Util::getClosestUnit(worker.getPosition(), Broodwar->enemy(), UnitTypes::Protoss_Pylon);
                if (pylon && pylon->unit() && pylon->unit()->exists()) {
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

            const auto shouldMoveToBuild = [&]() {
                auto mineralIncome = (minWorkers - 1) * 0.045;
                auto gasIncome = (gasWorkers - 1) * 0.07;
                auto speed = worker.getType().topSpeed();
                auto dist = mapBWEM.GetArea(worker.getTilePosition()) ? BWEB::Map::getGroundDistance(worker.getPosition(), center) : worker.getPosition().getDistance(Position(worker.getBuildPosition()));
                auto time = (dist / speed) + 50.0;
                auto enoughGas = worker.getBuildingType().gasPrice() > 0 ? Broodwar->self()->gas() + int(gasIncome * time) >= worker.getBuildingType().gasPrice() : true;
                auto enoughMins = worker.getBuildingType().mineralPrice() > 0 ? Broodwar->self()->minerals() + int(mineralIncome * time) >= worker.getBuildingType().mineralPrice() : true;

                return enoughGas && enoughMins;
            };

            // 1) Attack any enemies inside the build area
            if (worker.hasTarget() && worker.getTarget().getPosition().getDistance(worker.getPosition()) < 160 && (Util::rectangleIntersect(topLeft, botRight, worker.getTarget().getPosition()) || worker.getTarget().getPosition().getDistance(center) < 256.0)) {
                Command::attack(worker);
                return true;
            }

            // 2) Cancel any buildings we don't need
            else if ((BuildOrder::buildCount(worker.getBuildingType()) <= Broodwar->self()->visibleUnitCount(worker.getBuildingType()) || !Buildings::isBuildable(worker.getBuildingType(), worker.getBuildPosition()))) {
                worker.setBuildingType(UnitTypes::None);
                worker.setBuildPosition(TilePositions::Invalid);
                worker.unit()->stop();
                return true;
            }

            // 3) Move to build if we have the resources
            else if (shouldMoveToBuild()) {
                worker.setDestination(center);

                if (worker.getPosition().getDistance(center) > 128.0) {
                    if (worker.getBuildingType().isResourceDepot())
                        Command::move(worker);
                    else if (worker.unit()->getOrderTargetPosition() != center)
                        Command::move(worker); // worker.unit()->move(center);
                }
                else if (worker.unit()->getOrder() != Orders::PlaceBuilding || worker.unit()->isIdle())
                    worker.unit()->build(worker.getBuildingType(), worker.getBuildPosition());
                return true;
            }
            return false;
        }

        bool clearPath(UnitInfo& worker)
        {
            auto resourceDepot = Broodwar->self()->getRace().getResourceDepot();
            if (Units::getMyTypeCount(resourceDepot) < 2 || (BuildOrder::buildCount(resourceDepot) == Units::getMyTypeCount(resourceDepot) && BuildOrder::isOpener()))
                return false;

            // Find boulders to clear
            for (auto &b : Resources::getMyBoulders()) {
                ResourceInfo &boulder = b.second;
                if (!boulder.unit() || !boulder.unit()->exists())
                    continue;
                if (worker.getPosition().getDistance(boulder.getPosition()) >= 480.0)
                    continue;

                if (!worker.unit()->isGatheringMinerals()) {
                    if (worker.unit()->getOrderTargetPosition() != b.second.getPosition())
                        worker.unit()->gather(b.first);
                }
                return true;
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
                auto resourceCentroid = worker.getResource().getStation() ? worker.getResource().getStation()->ResourceCentroid() : Positions::Invalid;
                worker.setDestination(resourceCentroid);

                // 1) If it's close or same area, don't need a path		
                if (closeToResource(worker)) {
                    BWEB::PathFinding::Path emptyPath;
                    worker.setPath(emptyPath);
                }

                // 2) If it's far, generate a path
                else if (worker.getLastTile() != worker.getTilePosition() && resourceCentroid.isValid()) {
                    BWEB::PathFinding::Path newPath;
                    newPath.createUnitPath(worker.getPosition(), resourceCentroid);
                    worker.setPath(newPath);
                }

                Visuals::displayPath(worker.getPath().getTiles());

                // 3) If no threat on path, mine it
                if (!Util::accurateThreatOnPath(worker, worker.getPath())) {
                    if (shouldIssueGather())
                        worker.unit()->gather(worker.getResource().unit());
                    else if (!resourceExists)
                        Command::move(worker);
                    return true;
                }

                // 4) If we are under a threat, try to get away from it
                else if (Grids::getEGroundThreat(worker.getWalkPosition()) > 0.0) {
                    Command::kite(worker);
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
            ResourceInfo* bestResource = nullptr;
            auto injured = (worker.unit()->getHitPoints() + worker.unit()->getShields() < worker.getType().maxHitPoints() + worker.getType().maxShields());
            auto threatened = (worker.hasResource() && Util::accurateThreatOnPath(worker, worker.getPath()));
            auto distBest = (injured || threatened) ? 0.0 : DBL_MAX;
            auto needNewAssignment = false;
            vector<const BWEB::Stations::Station *> safeStations;
            auto closest = BWEB::Stations::getClosestStation(worker.getTilePosition());

            const auto resourceReady = [&](ResourceInfo& resource, int i) {
                if (!resource.unit()
                    || resource.getType() == UnitTypes::Resource_Vespene_Geyser
                    || (resource.unit()->exists() && !resource.unit()->isCompleted())
                    || (resource.getGathererCount() >= i + int(injured || threatened))
                    || resource.getResourceState() == ResourceState::None
                    || ((!Resources::isMinSaturated() || !Resources::isGasSaturated()) && Grids::getEGroundThreat(WalkPosition(resource.getPosition())) > 0.0))
                    return false;
                return true;
            };

            const auto needGas = [&]() {
                if (!Resources::isGasSaturated() && ((gasWorkers < BuildOrder::gasWorkerLimit() && BuildOrder::isOpener()) || !BuildOrder::isOpener() || Resources::isMinSaturated()))
                    return true;
                return false;
            };

            // Worker has no resource, we need gas, we need minerals, workers resource is threatened
            if (!worker.hasResource()
                || needGas()
                || (worker.hasResource() && !worker.getResource().getType().isMineralField() && gasWorkers > BuildOrder::gasWorkerLimit())
                || (worker.hasResource() && !closeToResource(worker) && Util::accurateThreatOnPath(worker, worker.getPath()) && Grids::getEGroundThreat(worker.getWalkPosition()) == 0.0)
                || (worker.hasResource() && closeToResource(worker) && Grids::getEGroundThreat(worker.getWalkPosition()) > 0.0))
                needNewAssignment = true;

            // HACK: Just return if we dont need an assignment, should make this better
            if (!needNewAssignment)
                return;

            // 1) If threatened, find safe stations to move to on the Station network or generate a new path
            if (threatened) {

                // Find a path on the station network
                if (worker.getPosition().getDistance(closest->ResourceCentroid()) < 320.0) {
                    for (auto &s : Stations::getMyStations()) {
                        auto station = s.second;
                        auto closePath = Stations::pathStationToStation(closest, station);
                        BWEB::PathFinding::Path path;
                        if (closest  && closePath)
                            path = *closePath;

                        // Store station if it's safe
                        if (!Util::accurateThreatOnPath(worker, path) || worker.getPosition().getDistance(station->ResourceCentroid()) < 128.0)
                            safeStations.push_back(station);
                    }
                }
            }

            // 2) Check if we need gas workers
            if (needGas()) {
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = r.second;
                    if (!resourceReady(resource, 3))
                        continue;
                    if (threatened && !safeStations.empty() && (!resource.getStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end()))
                        continue;

                    auto dist = resource.getPosition().getDistance(worker.getPosition());
                    if ((dist < distBest && !injured) || (dist > distBest && injured)) {
                        bestResource = &resource;
                        distBest = dist;
                    }
                }
            }

            // 3) Check if we need mineral workers
            else {
                for (int i = 1; i <= 2; i++) {
                    for (auto &r : Resources::getMyMinerals()) {
                        auto &resource = r.second;
                        if (!resourceReady(resource, i))
                            continue;
                        if (threatened && !safeStations.empty() && (!resource.getStation() || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end()))
                            continue;

                        double dist = resource.getPosition().getDistance(worker.getPosition());
                        if ((dist < distBest && !injured && !threatened) || (dist > distBest && (injured || threatened))) {
                            bestResource = &resource;
                            distBest = dist;
                        }
                    }
                    if (bestResource)
                        break;
                }

            }

            // 4) Assign resource
            if (bestResource) {

                // Remove current assignment
                if (worker.hasResource()) {
                    worker.getResource().setGathererCount(worker.getResource().getGathererCount() - 1);
                    worker.getResource().getType().isMineralField() ? minWorkers-- : gasWorkers--;
                }

                // Add next assignment
                bestResource->setGathererCount(bestResource->getGathererCount() + 1);
                bestResource->getType().isMineralField() ? minWorkers++ : gasWorkers++;

                BWEB::PathFinding::Path emptyPath;
                worker.setResource(bestResource);
                worker.setPath(emptyPath);
            }
        }

        constexpr tuple commands{ misc, transport, returnCargo, clearPath, build, gather };
        void updateDecision(UnitInfo& worker)
        {
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Transport"),
                make_pair(2, "ReturnCargo"),
                make_pair(3, "ClearPath"),
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
            for (auto &w : Units::getMyUnits()) {
                auto &worker = w.second;
                if (worker.getRole() == Role::Working) {
                    updateAssignment(worker);
                    updateDecision(worker);
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateWorkers();
        Visuals::endPerfTest("Workers");
    }

    void removeUnit(Unit unit)
    {
    }
}