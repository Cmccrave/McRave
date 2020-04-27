#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Workers {

    namespace {

        int minWorkers = 0;
        int gasWorkers = 0;
        int boulderWorkers = 0;
        map<Position, double> distChecker;

        void updateDestination(UnitInfo& unit)
        {
            // If unit has a building type and we are ready to build
            if (unit.getBuildType() != UnitTypes::None && shouldMoveToBuild(unit, unit.getBuildPosition(), unit.getBuildType())) {
                const auto center = Position(unit.getBuildPosition()) + Position(unit.getBuildType().tileWidth() * 16, unit.getBuildType().tileHeight() * 16);
                unit.setDestination(center);
            }

            // If unit has a transport
            else if (unit.hasTransport())
                unit.setDestination(unit.getTransport().getPosition());

            // If unit has a resource
            else if (unit.hasResource() && unit.getResource().getResourceState() == ResourceState::Mineable)
                unit.setDestination(unit.getResource().getPosition());

            const auto resourceWalkable = [&](TilePosition tile) {
                return (unit.hasResource() && BWEB::Map::isUsed(tile) == unit.getResource().getType() && tile.getDistance(TilePosition(unit.getDestination())) <= 5) || BWEB::Pathfinding::unitWalkable(tile);
            };

            if (unit.getDestination().isValid() && unit.getDestinationPath().getTarget() != TilePosition(unit.getDestination())) {
                BWEB::Path newPath;
                newPath.jpsPath(unit.getPosition(), unit.getDestination(), resourceWalkable);
                unit.setDestinationPath(newPath);
            }

            if (unit.getDestinationPath().getTarget() == TilePosition(unit.getDestination())) {
                auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= 160.0;
                });

                if (newDestination.isValid())
                    unit.setDestination(newDestination);
            }

            if (unit.unit()->isSelected())
                Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Cyan);
        }

        void updateAssignment(UnitInfo& unit)
        {
            // If unit has a resource currently, we need to check if it's safe
            auto resourceSafe = true;
            if (unit.hasResource()) {
                BWEB::Path newPath;
                newPath.jpsPath(unit.getPosition(), unit.getResource().getStation()->getResourceCentroid(), BWEB::Pathfinding::terrainWalkable);
                unit.setDestinationPath(newPath);

                auto threatPosition = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return unit.getType().isFlyer() ? Grids::getEAirThreat(p) > 0.0f : Grids::getEGroundThreat(p) > 0.0f;
                });

                if (threatPosition)
                    resourceSafe = false;
            }

            // Check the status of the unit and the assigned resource
            const auto injured = unit.unit()->getHitPoints() + unit.unit()->getShields() < unit.getType().maxHitPoints() + unit.getType().maxShields();
            const auto threatened = (unit.hasResource() && unit.isWithinGatherRange() && int(unit.getTargetedBy().size()) > 0) || (!unit.isWithinGatherRange() && !resourceSafe);
            const auto excessAssigned = unit.hasResource() && !injured && !threatened && unit.getResource().getGathererCount() >= 3 + int(unit.getResource().getType().isRefinery());

            // Check if unit needs a re-assignment
            const auto isGasunit = unit.hasResource() && unit.getResource().getType().isRefinery();
            const auto isMineralunit = unit.hasResource() && unit.getResource().getType().isMineralField();
            const auto needGas = !Resources::isGasSaturated() && isMineralunit && gasWorkers < BuildOrder::gasWorkerLimit();
            const auto needMinerals = !Resources::isMinSaturated() && isGasunit && gasWorkers > BuildOrder::gasWorkerLimit();
            const auto needNewAssignment = !unit.hasResource() || needGas || needMinerals || threatened || excessAssigned;

            auto distBest = DBL_MAX;
            auto &oldResource = unit.hasResource() ? unit.getResource().shared_from_this() : nullptr;
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

            // Find safe stations to mine resources from
            for (auto &[_, station] : Stations::getMyStations()) {

                // If unit is close, it must be safe
                if (unit.getPosition().getDistance(station->getResourceCentroid()) < 320.0 || mapBWEM.GetArea(unit.getTilePosition()) == station->getBWEMBase()->GetArea()) {
                    safeStations.push_back(station);
                    continue;
                }

                // Otherwise create a path to it
                BWEB::Path newPath;
                newPath.jpsPath(unit.getPosition(), station->getResourceCentroid(), BWEB::Pathfinding::terrainWalkable);
                unit.setDestinationPath(newPath);

                // If unit is far, we need to check if it's safe
                auto threatPosition = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return unit.getType().isFlyer() ? Grids::getEAirThreat(p) > 0.0f : Grids::getEGroundThreat(p) > 0.0f;
                });

                if (!threatPosition.isValid())
                    safeStations.push_back(station);
            }

            // Check if we need gas units
            if (needGas || !unit.hasResource()) {
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = *r;

                    if (!resourceReady(resource, 3)
                        || !resource.hasStation()
                        || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                        continue;

                    auto closestunit = Util::getClosestUnit(resource.getPosition(), PlayerState::Self, [&](auto &u) {
                        return u.getRole() == Role::Worker && (!u.hasResource() || u.getResource().getType().isMineralField());
                    });
                    if (unit.shared_from_this() != closestunit)
                        continue;

                    auto dist = resource.getPosition().getDistance(unit.getPosition());
                    if ((dist < distBest && !injured) || (dist > distBest && injured)) {
                        unit.setResource(r.get());
                        distBest = dist;
                    }
                }
            }

            // Check if we need mineral units
            if (needMinerals || !unit.hasResource()) {

                for (int i = 1; i <= 2; i++) {

                    // Check closest station first if we can mine there
                    auto closestStation = BWEB::Stations::getClosestStation(unit.getTilePosition());
                    if (closestStation) {
                        for (auto &r : Resources::getMyMinerals()) {
                            auto &resource = *r;
                            if (!resourceReady(resource, i)
                                || !resource.hasStation()
                                || resource.getStation() != closestStation)
                                continue;


                        }
                    }

                    // Check other stations
                    for (auto &r : Resources::getMyMinerals()) {
                        auto &resource = *r;

                        if (!resourceReady(resource, i)
                            || !resource.hasStation()
                            || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                            continue;

                        auto topLeftMineral = Position(resource.getPosition().x - resource.getType().dimensionLeft(), resource.getPosition().y - resource.getType().dimensionUp());
                        auto botRightMineral = Position(resource.getPosition().x + resource.getType().dimensionRight(), resource.getPosition().y + resource.getType().dimensionDown());
                        auto stationPos = resource.getStation()->getBWEMBase()->Center();
                        auto stationType = Broodwar->self()->getRace().getResourceDepot();
                        auto topLeftStation = Position(stationPos.x - stationType.dimensionLeft(), stationPos.y - stationType.dimensionUp());
                        auto botRightStation = Position(stationPos.x + stationType.dimensionRight(), stationPos.y + stationType.dimensionDown());

                        double dist = Util::pxDistanceBB(topLeftMineral.x, topLeftMineral.y, botRightMineral.x, botRightMineral.y, topLeftStation.x, topLeftStation.y, botRightStation.x, botRightStation.y);
                        if ((dist < distBest && !injured && !threatened) || (dist > distBest && (injured || threatened))) {
                            unit.setResource(r.get());
                            distBest = dist;
                        }
                    }

                    // Don't check again if we assigned one
                    if (unit.hasResource() && unit.getResource().shared_from_this() != oldResource)
                        break;
                }
            }

            // Remove resource if needed new assignment and didn't get one due to threat
            if (unit.hasResource() && threatened && unit.getResource().shared_from_this() == oldResource) {

                // Remove current assignemt
                if (oldResource) {
                    oldResource->getType().isMineralField() ? minWorkers-- : gasWorkers--;
                    oldResource->removeTargetedBy(unit.weak_from_this());
                }

                unit.setResource(nullptr);

                // HACK: Update saturation checks
                Resources::onFrame();
            }

            // Assign resource
            if (unit.hasResource()) {

                // Remove current assignemt
                if (oldResource) {
                    oldResource->getType().isMineralField() ? minWorkers-- : gasWorkers--;
                    oldResource->removeTargetedBy(unit.weak_from_this());
                }

                // Add next assignment
                unit.getResource().getType().isMineralField() ? minWorkers++ : gasWorkers++;
                unit.getResource().addTargetedBy(unit.weak_from_this());

                // HACK: Update saturation checks
                Resources::onFrame();
            }
        }

        constexpr tuple commands{ Command::misc, Command::click, Command::burrow, Command::returnResource, Command::clearNeutral, Command::build, Command::gather, Command::move };
        void updateDecision(UnitInfo& unit)
        {
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Click"),
                make_pair(2, "Burrow"),
                make_pair(3, "Return"),
                make_pair(4, "Clear"),
                make_pair(5, "Build"),
                make_pair(6, "Gather"),
                make_pair(7, "Move")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            //Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateunits()
        {
            // Sort units by distance to destination
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Worker) {
                    updateAssignment(unit);
                    updateDestination(unit);
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
        unit.getResource().getType().isRefinery() ? gasWorkers-- : minWorkers--;
        unit.getResource().removeTargetedBy(unit.weak_from_this());
        unit.setResource(nullptr);
    }

    bool shouldMoveToBuild(UnitInfo& unit, TilePosition tile, UnitType type) {
        if (!mapBWEM.GetArea(unit.getTilePosition()))
            return true;

        auto center = Position(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);
        auto mineralIncome = max(0.0, double(minWorkers - 1) * 0.045);
        auto gasIncome = max(0.0, double(gasWorkers - 1) * 0.07);
        auto speed = unit.getSpeed();
        auto dist = BWEB::Map::getGroundDistance(unit.getPosition(), center);
        auto time = (dist / speed) + 96.0;
        auto enoughGas = unit.getBuildType().gasPrice() > 0 ? Broodwar->self()->gas() + int(gasIncome * time) >= unit.getBuildType().gasPrice() : true;
        auto enoughMins = unit.getBuildType().mineralPrice() > 0 ? Broodwar->self()->minerals() + int(mineralIncome * time) >= unit.getBuildType().mineralPrice() : true;

        return (enoughGas && enoughMins);
    };

    int getMineralWorkers() { return minWorkers; }
    int getGasWorkers() { return gasWorkers; }
    int getBoulderWorkers() { return boulderWorkers; }
}