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
            else if (unit.hasResource() && unit.getResource().getResourceState() == ResourceState::Mineable && unit.getPosition().getDistance(unit.getResource().getPosition()) > 96.0)
                unit.setDestination(unit.getResource().getPosition());

            if (unit.getDestination().isValid() && (unit.getDestinationPath().getTarget() != TilePosition(unit.getDestination()) || !unit.getDestinationPath().isReachable())) {
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                const auto resourceWalkable = [&](const TilePosition &tile) {
                    return (unit.hasResource() && unit.getBuildType() == UnitTypes::None && BWEB::Map::isUsed(tile) == unit.getResource().getType() && tile.getDistance(TilePosition(unit.getDestination())) <= 8) || newPath.unitWalkable(tile) || (unit.getBuildType().isRefinery() && BWEB::Map::isUsed(tile) == UnitTypes::Resource_Vespene_Geyser);
                };
                newPath.generateJPS(resourceWalkable);
                unit.setDestinationPath(newPath);
            }

            if (unit.getDestinationPath().getTarget() == TilePosition(unit.getDestination())) {
                auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= 64.0;
                });

                if (newDestination.isValid())
                    unit.setDestination(newDestination);
            }
        }

        void updateResource(UnitInfo& unit)
        {
            // If unit has a resource currently, we need to check if it's safe
            auto resourceSafe = true;
            if (unit.hasResource()) {
                BWEB::Path newPath(unit.getPosition(), unit.getResource().getStation()->getResourceCentroid(), unit.getType());
                const auto walkable = [&](const TilePosition &t) {
                    return newPath.terrainWalkable(t);
                };
                newPath.generateJPS(walkable);

                auto threatPosition = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return unit.getType().isFlyer() ? Grids::getEAirThreat(p) > 0.0f : Grids::getEGroundThreat(p) > 0.0f;
                });

                if (threatPosition)
                    resourceSafe = false;
                else
                    unit.setDestinationPath(newPath);
            }

            // Check the status of the unit and the assigned resource
            const auto time = (Strategy::enemyRush() || Broodwar->self()->getRace() == Races::Zerg) ? Time(3, 30) : Time(8, 00);
            const auto threatenedEarly = unit.hasResource() && Util::getTime() < time && Util::getTime() > Time(2, 10) && !unit.getTargetedBy().empty();
            const auto threatenedAll = unit.hasResource() && Util::getTime() > time && Grids::getEGroundThreat(unit.getResource().getPosition()) > 0.0f && unit.hasTarget() && unit.getTarget().isThreatening() && !unit.getTarget().isFlying() && !unit.getTarget().getType().isWorker() && unit.getTarget().hasAttackedRecently();
            const auto injured = unit.unit()->getHitPoints() + unit.unit()->getShields() < unit.getType().maxHitPoints() + unit.getType().maxShields();
            const auto threatened = (threatenedEarly || threatenedAll);
            const auto excessAssigned = unit.hasResource() && !threatened && ((unit.getPosition().getDistance(unit.getResource().getPosition()) < 64.0 && unit.getResource().getGathererCount() >= 3 + int(unit.getResource().getType().isRefinery())) || unit.getResource().getResourceState() != ResourceState::Mineable);

            // Check if unit needs a re-assignment
            const auto isGasunit = unit.hasResource() && unit.getResource().getType().isRefinery();
            const auto isMineralunit = unit.hasResource() && unit.getResource().getType().isMineralField();
            const auto needGas = !Resources::isGasSaturated() && isMineralunit && gasWorkers < BuildOrder::gasWorkerLimit();
            const auto needMinerals = (!Resources::isMinSaturated() || BuildOrder::isOpener()) && isGasunit && gasWorkers > BuildOrder::gasWorkerLimit();
            const auto needNewAssignment = !unit.hasResource() || needGas || needMinerals || threatened || (excessAssigned && !threatened);

            auto distBest = threatened ? 0.0 : DBL_MAX;
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
                if (unit.getPosition().getDistance(station->getResourceCentroid()) < 320.0 || mapBWEM.GetArea(unit.getTilePosition()) == station->getBase()->GetArea())
                    safeStations.push_back(station);

                // If we own a main attached to this natural
                if (station->isNatural() && !Players::ZvZ() && ((unit.hasResource() && unit.getResource().getStation() == station) || (mapBWEM.GetArea(unit.getTilePosition()) == station->getBase()->GetArea()))) {
                    auto closestMain = BWEB::Stations::getClosestMainStation(station->getBase()->Location());
                    if (closestMain && Stations::ownedBy(closestMain) == PlayerState::Self)
                        safeStations.push_back(closestMain);
                }

                // Or we own a natural attached to this main
                else if (station->isMain() && !Players::ZvZ() && ((unit.hasResource() && unit.getResource().getStation() == station) || (mapBWEM.GetArea(unit.getTilePosition()) == station->getBase()->GetArea()))) {
                    auto closestNatural = BWEB::Stations::getClosestNaturalStation(station->getBase()->Location());
                    if (closestNatural && Stations::ownedBy(closestNatural) == PlayerState::Self)
                        safeStations.push_back(closestNatural);
                }

                // Otherwise create a path to it to check if it's safe
                else {
                    BWEB::Path newPath(unit.getPosition(), station->getResourceCentroid(), unit.getType());
                    newPath.generateJPS([&](const TilePosition &t) { return newPath.terrainWalkable(t); });
                    unit.setDestinationPath(newPath);

                    // If unit is far, we need to check if it's safe
                    auto threatPosition = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                        return unit.getType().isFlyer() ? Grids::getEAirThreat(p) > 0.0f : Grids::getEGroundThreat(p) > 0.0f;
                    });

                    // If JPS path is safe
                    if (!threatPosition.isValid())
                        safeStations.push_back(station);
                }
            }

            // Check if we need gas units
            if (needGas || !unit.hasResource() || excessAssigned) {
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = *r;

                    if (!resourceReady(resource, 3)
                        || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                        continue;

                    auto closestunit = Util::getClosestUnit(resource.getPosition(), PlayerState::Self, [&](auto &u) {
                        return u.getRole() == Role::Worker && (!u.hasResource() || u.getResource().getType().isMineralField());
                    });
                    if (closestunit && unit.shared_from_this() != closestunit)
                        continue;

                    auto dist = resource.getPosition().getDistance(unit.getPosition());
                    if ((dist < distBest && !threatened) || (dist > distBest && threatened)) {
                        unit.setResource(r.get());
                        distBest = dist;
                    }
                }
            }

            // Check if we need mineral units
            if (needMinerals || !unit.hasResource() || threatened || excessAssigned) {
                Broodwar->drawCircleMap(unit.getPosition(), 2, Colors::Green, true);
                for (int i = 1; i <= 2; i++) {
                    for (auto &r : Resources::getMyMinerals()) {
                        auto &resource = *r;
                        auto allowedGatherCount = threatened ? 50 : i;

                        if (!resourceReady(resource, allowedGatherCount)
                            || find(safeStations.begin(), safeStations.end(), resource.getStation()) == safeStations.end())
                            continue;

                        auto topLeftMineral = Position(resource.getPosition().x - resource.getType().dimensionLeft(), resource.getPosition().y - resource.getType().dimensionUp());
                        auto botRightMineral = Position(resource.getPosition().x + resource.getType().dimensionRight(), resource.getPosition().y + resource.getType().dimensionDown());
                        auto stationPos = resource.getStation()->getBase()->Center();
                        auto stationType = Broodwar->self()->getRace().getResourceDepot();
                        auto topLeftStation = Position(stationPos.x - stationType.dimensionLeft(), stationPos.y - stationType.dimensionUp());
                        auto botRightStation = Position(stationPos.x + stationType.dimensionRight(), stationPos.y + stationType.dimensionDown());

                        auto stationDist = unit.getPosition().getDistance(resource.getStation()->getBase()->Center());
                        auto resourceDist = Util::boxDistance(UnitTypes::Resource_Mineral_Field, resource.getPosition(), Broodwar->self()->getRace().getResourceDepot(), resource.getStation()->getBase()->Center());
                        auto dist = log(stationDist) * (unit.hasTarget() ? resource.getPosition().getDistance(unit.getTarget().getPosition()) : resourceDist);


                        if ((dist < distBest && !threatened) || (dist > distBest && threatened)) {
                            unit.setResource(r.get());
                            distBest = dist;
                        }
                    }

                    // Don't check again if we assigned one
                    if (unit.hasResource() && unit.getResource().shared_from_this() != oldResource && !threatened)
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
                unit.getResource().getType().isMineralField() ? minWorkers++ : gasWorkers++;
                unit.getResource().addTargetedBy(unit.weak_from_this());

                Resources::recheckSaturation();
            }
        }

        void updateBuilding(UnitInfo& unit)
        {
            if (unit.getBuildPosition() == TilePositions::None
                || unit.getBuildType() == UnitTypes::None)
                return;

            auto buildCenter = Position(unit.getBuildPosition()) + Position(unit.getBuildType().tileWidth() * 16, unit.getBuildType().tileHeight() * 16);

            // Generate a path that obeys refinery placement as well
            BWEB::Path newPath(unit.getPosition(), buildCenter, unit.getType());
            auto buildingWalkable = [&](const auto &t) {
                return newPath.unitWalkable(t) || (unit.getBuildType().isRefinery() && BWEB::Map::isUsed(t) == UnitTypes::Resource_Vespene_Geyser);
            };
            newPath.generateJPS([&](const TilePosition &t) { return buildingWalkable(t); });

            auto threatPosition = Util::findPointOnPath(newPath, [&](Position p) {
                return Grids::getEGroundThreat(p) > 0.0 && Broodwar->isVisible(TilePosition(p));
            });

            auto aroundDefenders = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return (unit.isWithinBuildRange() && u.getPosition().getDistance(buildCenter) < u.getGroundReach())
                    || ((u.getGoal() == unit.getPosition() || (unit.hasTarget() && u.getGoal() == unit.getTarget().getPosition())) && (u.getPosition().getDistance(unit.getPosition()) < 256.0 || (unit.hasTarget() && u.getPosition().getDistance(unit.getTarget().getPosition()) < 256.0)));
            });

            if ((!aroundDefenders && threatPosition && threatPosition.getDistance(unit.getPosition()) < 200.0 && Util::getTime() > Time(5, 00)) || unit.isBurrowed()) {
                unit.setBuildingType(UnitTypes::None);
                unit.setBuildPosition(TilePositions::Invalid);
            }
        }

        constexpr tuple commands{ Command::misc, Command::attack, Command::click, Command::burrow, Command::returnResource, Command::build, Command::clearNeutral, Command::move, Command::gather };
        void updateDecision(UnitInfo& unit)
        {
            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Attack"),
                make_pair(2, "Click"),
                make_pair(3, "Burrow"),
                make_pair(4, "Return"),
                make_pair(5, "Build"),
                make_pair(6, "Clear"),
                make_pair(7, "Move"),
                make_pair(8, "Gather")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateunits()
        {
            // Sort units by distance to destination
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getRole() == Role::Worker) {
                    updateResource(unit);
                    updateBuilding(unit);
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