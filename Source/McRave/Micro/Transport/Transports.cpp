#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Transports {

    namespace {

        constexpr tuple commands{ Command::escort, Command::retreat };

        void updatePath(UnitInfo& unit)
        {
            // Generate a flying path for dropping
            const auto transportDrop = [&](const TilePosition &t) {
                return Grids::getAirThreat(Position(t) + Position(16, 16), PlayerState::Enemy) * 250.0;
            };

            BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
            newPath.generateAS(transportDrop);
            unit.setDestinationPath(newPath);
            Visuals::drawPath(newPath);
        }

        void updateCargo(UnitInfo& unit)
        {
            auto cargoSize = 0;
            auto &cargoList = unit.getAssignedCargo();

            // Check if we are ready to assign this worker to a transport - TODO: Clean this up and make available for builds, enable islands again
            const auto readyToAssignWorker = [&](UnitInfo& cargo) {
                auto buildDist = cargo.getBuildType().isValid() ? BWEB::Map::getGroundDistance((Position)cargo.getBuildPosition(), (Position)cargo.getTilePosition()) : 0.0;
                auto resourceDist = cargo.hasResource() ? BWEB::Map::getGroundDistance(cargo.getPosition(), cargo.getResource().lock()->getPosition()) : 0.0;

                if ((cargo.getBuildPosition().isValid() && buildDist == DBL_MAX) || (cargo.getBuildType().isResourceDepot() && Terrain::isIslandMap()))
                    return true;
                if (cargo.hasResource() && resourceDist == DBL_MAX)
                    return true;
                return false;
            };

            // Check if we are ready to assign this unit to a transport
            const auto readyToAssignUnit = [&](UnitInfo& cargo) {
                auto targetDist = BWEB::Map::getGroundDistance(cargo.getPosition(), cargo.getEngagePosition());
                if (cargo.getType() == Terran_Ghost)
                    return cargo.getDestination().isValid() && !Terrain::inArea(mapBWEM.GetArea(TilePosition(cargo.getDestination())), cargo.getDestination());
                if (cargo.getType() == Protoss_Reaver || cargo.getType() == Protoss_High_Templar)
                    return cargo.getDestination().isValid();
                if (Terrain::isIslandMap() && !cargo.getType().isFlyer() && targetDist == DBL_MAX)
                    return true;
                return false;
            };

            // Check if we are ready to remove any units
            cargoList.erase(remove_if(cargoList.begin(), cargoList.end(), [&](auto &c) {
                auto &cargo = *(c.lock());
                if ((cargo.getRole() == Role::Combat && !readyToAssignUnit(cargo)) || (cargo.getRole() == Role::Worker && !readyToAssignWorker(cargo))) {
                    cargo.setTransport(nullptr);
                    return true;
                }
                return cargo.unit()->isStasised() || cargo.unit()->isLockedDown();
            }), cargoList.end());

            // Calculate cargo size allocated (but not necessarily loaded)
            for (auto &c : cargoList) {
                if (auto &cargo = c.lock())
                    cargoSize += cargo->getType().spaceRequired();
            }

            // Check if we are ready to add any units
            if (cargoSize < 8) {
                for (auto &c : Units::getUnits(PlayerState::Self)) {
                    auto &cargo = *c;

                    if (cargoSize + cargo.getType().spaceRequired() > 8
                        || cargo.hasTransport()
                        || !cargo.unit()->isCompleted())
                        continue;

                    if (cargo.getRole() == Role::Combat && readyToAssignUnit(cargo)) {
                        cargo.setTransport(&unit);
                        unit.getAssignedCargo().push_back(c);
                        cargoSize += unit.getType().spaceRequired();
                    }

                    if (cargo.getRole() == Role::Worker && readyToAssignWorker(cargo)) {
                        cargo.setTransport(&unit);
                        unit.getAssignedCargo().push_back(c);
                        cargoSize += unit.getType().spaceRequired();
                    }
                }
            }

            // Priority of units:
            // DT > Probe > Zealot > Dragoon > Reaver > HT
            // Lurker > Drone > Hydra > Zergling
            // Ghost > SCV > Medic > Marine == Firebat
            auto sortOrder ={ Protoss_Dark_Templar, Zerg_Lurker,
                Protoss_Probe, Zerg_Drone, Terran_SCV,
                Protoss_Zealot, Zerg_Hydralisk,
                Protoss_Dragoon,
                Protoss_Reaver,
                Protoss_High_Templar, Zerg_Zergling, Terran_Medic, Terran_Firebat, };
            sort(cargoList.begin(), cargoList.end(), [&](auto &l, auto &r) {
                const auto leftPos = find(sortOrder.begin(), sortOrder.end(), l.lock()->getType());
                const auto rightPos = find(sortOrder.begin(), sortOrder.end(), r.lock()->getType());
                return distance(sortOrder.begin(), leftPos) < distance(sortOrder.begin(), rightPos);
            });
        }

        void updateTransportState(UnitInfo& unit)
        {
            // Priority of states:
            // 1. Loading
            // 2. Retreating
            // 3. Reinforcing
            // 4. Engaging

            // Check if this cargo is ready to reinforce
            const auto readyToReinforce = [&](UnitInfo& cargo, UnitInfo& cargoTarget) {
                return !cargo.unit()->isLoaded();
            };

            // Check if this cargo is ready to be picked up
            const auto readyToLoad = [&](UnitInfo& cargo, UnitInfo& cargoTarget) {
                return (!cargo.unit()->isLoaded() && cargo.isRequestingPickup());
            };

            // Check if this cargo is ready to engage
            const auto readyToEngage = [&](UnitInfo& cargo, UnitInfo& cargoTarget) {
                const auto combatEngage = cargo.getLocalState() != LocalState::Retreat && unit.isHealthy() && cargo.isHealthy();
                const auto range = cargoTarget.getType().isFlyer() ? cargo.getAirRange() : cargo.getGroundRange();
                const auto dist = Util::boxDistance(cargo.getType(), cargo.getPosition(), cargoTarget.getType(), cargoTarget.getPosition());

                // Don't keep moving into range if we already are
                if (dist <= range)
                    return false;

                if (cargo.getType() == Protoss_High_Templar)
                    return combatEngage && cargo.canStartCast(TechTypes::Psionic_Storm, cargoTarget.getPosition());
                if (cargo.getType() == Protoss_Reaver)
                    return combatEngage && cargo.canStartAttack();
                if (cargo.getType() == Terran_Ghost)
                    return true;
                return false;
            };

            // Check if this cargo is ready to retreat
            const auto readyToRetreat = [&](UnitInfo& cargo, UnitInfo& cargoTarget) {
                const auto combatRetreat = cargo.getLocalState() == LocalState::Retreat || cargo.getGlobalState() == GlobalState::Retreat || !unit.isHealthy();
                const auto range =  cargoTarget.getType().isFlyer() ? cargo.getAirRange() : cargo.getGroundRange();
                const auto distRetreat = Util::boxDistance(cargo.getType(), cargo.getPosition(), cargoTarget.getType(), cargoTarget.getPosition()) <= 0.8 * range;
                if (cargo.getType() == Terran_Ghost)
                    return false;
                return distRetreat || combatRetreat;
            };

            const auto readyToDrop = [&](UnitInfo& cargo, UnitInfo& cargoTarget) {
                if (cargo.getType() == Terran_Ghost) {
                    auto cargoArea = mapBWEM.GetArea(TilePosition(cargo.getDestination()));
                    return cargoArea && Terrain::inArea(cargoArea, unit.getPosition());
                }
                if (cargo.getType() == Protoss_High_Templar || cargo.getType() == Protoss_Reaver)
                    return true;
                return false;
            };

            const auto setState = [&](TransportState state) {
                if (state == TransportState::Loading)
                    unit.setTransportState(state);
                if (state == TransportState::Retreating && unit.getTransportState() != TransportState::Loading)
                    unit.setTransportState(state);
                if (state == TransportState::Reinforcing && unit.getTransportState() != TransportState::Loading && unit.getTransportState() != TransportState::Retreating)
                    unit.setTransportState(state);
                if (unit.getTransportState() == TransportState::None)
                    unit.setTransportState(state);
            };

            for (auto &c : unit.getAssignedCargo()) {
                auto &cargo = *c.lock();
                if (!cargo.hasTarget())
                    continue;
                auto &cargoTarget = *cargo.getTarget().lock();

                if (readyToLoad(cargo, cargoTarget))
                    setState(TransportState::Loading);
                else if (readyToRetreat(cargo, cargoTarget))
                    setState(TransportState::Retreating);
                else if (readyToReinforce(cargo, cargoTarget))
                    setState(TransportState::Reinforcing);
                else if (readyToEngage(cargo, cargoTarget)) {
                    setState(TransportState::Engaging);
                    if (readyToDrop(cargo, cargoTarget))
                        unit.unit()->unload(cargo.unit());
                }
            }
        }

        void updateNavigation(UnitInfo& unit)
        {
            // If path is reachable, find a point n pixels away to set as new destination
            unit.setNavigation(unit.getDestination());
            if (unit.getDestinationPath().isReachable() && unit.getPosition().getDistance(unit.getDestination()) > 96.0) {
                auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= 96.0;
                });

                if (newDestination.isValid())
                    unit.setNavigation(newDestination);
            }
        }

        void updateDestination(UnitInfo& unit)
        {
            // Priority of destination:
            // 1. Loading: closest cargo needing loading immediately
            // 2. Retreating
            // 3. Reinforcing: closest cargo with loading state
            // 4. Engaging: closest engagement position
            unit.setDestination(Positions::Invalid);
            auto closestStation = Stations::getStations(PlayerState::Self).empty() ? Terrain::getMyMain() : Stations::getClosestRetreatStation(unit);
            UnitInfo * closestCargo = nullptr;
            auto distBest = DBL_MAX;
            for (auto &c : unit.getAssignedCargo()) {
                auto &cargo = c.lock();

                auto dist = cargo->getPosition().getDistance(unit.getPosition());
                if (dist < distBest) {
                    distBest = dist;
                    closestCargo = &*cargo;
                }
            }

            // If list isn't empty, our destination is the first valid one
            auto engageDestination = unit.getPosition();
            for (auto &c : unit.getAssignedCargo()) {
                auto &cargo = c.lock();
                if (cargo->getDestination().isValid()) {
                    engageDestination = cargo->getDestination();
                    break;
                }
            }
            Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Orange);

            // Set destination
            if (unit.getTransportState() == TransportState::Loading)
                unit.setDestination(closestCargo->getPosition());
            else if (unit.getTransportState() == TransportState::Retreating)
                unit.setDestination(closestStation->getBase()->Center());
            else if (unit.getTransportState() == TransportState::Reinforcing)
                unit.setDestination(closestCargo->getPosition());
            else if (unit.getTransportState() == TransportState::Engaging)
                unit.setDestination(engageDestination);
            Broodwar->drawLineMap(unit.getPosition() + Position(2, 2), unit.getDestination() + Position(2, 2), Colors::Yellow);

            // If we have no cargo, wait at nearest base
            if (unit.getAssignedCargo().empty()) {
                auto station = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self);
                if (station)
                    unit.setDestination(station->getBase()->Center());
            }
            Broodwar->drawLineMap(unit.getPosition() + Position(4, 4), unit.getDestination() + Position(4, 4), Colors::Green);

            // Resort to main, hopefully we still have it
            if (!unit.getDestination().isValid())
                unit.setDestination(Terrain::getMainPosition());
            Broodwar->drawLineMap(unit.getPosition() + Position(6, 6), unit.getDestination() + Position(6, 6), Colors::Blue);
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!unit.unit() || !unit.unit()->exists()                                                                                          // Prevent crashes
                || unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())    // If the unit is locked down, maelstrommed, stassised, or not completed
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Escort"),
                make_pair(1, "Retreat"),
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s (%d)", Text::White, commandNames[i].c_str(), unit.getTransportState());
        }

        void updateTransports()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() == Role::Transport) {
                    updateCargo(unit);
                    updateTransportState(unit);
                    updateDestination(unit);
                    updatePath(unit);
                    updateNavigation(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateTransports();
        Visuals::endPerfTest("Transports");
    }

    void removeUnit(UnitInfo& unit)
    {
        // If unit has a transport, remove it from the transports cargo, then set transport to nullptr
        if (unit.hasTransport()) {
            auto &cargoList = unit.getTransport().lock()->getAssignedCargo();
            for (auto &cargo = cargoList.begin(); cargo != cargoList.end(); cargo++) {
                if (cargo->lock() && cargo->lock() == unit.shared_from_this()) {
                    cargoList.erase(cargo);
                    break;
                }
            }
            unit.setTransport(nullptr);
        }

        // If unit is a transport, set the transport of all cargo to nullptr
        for (auto &c : unit.getAssignedCargo()) {
            if (auto &cargo = c.lock())
                cargo->setTransport(nullptr);
        }
    }
}


//// Check if the transport is too far
//const auto tooFar = [&](UnitInfo& cargo) {
//    if (cargo.getType() == Terran_Ghost) {
//        if (mapBWEM.GetArea(cargo.getTilePosition()) == Terrain::getEnemyMain()->getBase()->GetArea())
//            return true;
//    }
//    if (cargo.getType() == Protoss_High_Templar || cargo.getType() == Protoss_Reaver) {
//        const auto range = cargo.getTarget().lock()->getType().isFlyer() ? cargo.getAirRange() : cargo.getGroundRange();
//        const auto targetDist = cargo.getPosition().getDistance(cargo.getTarget().lock()->getPosition());
//        return targetDist >= range + 96.0;
//    }
//    return false;
//};

//// Check if this worker is ready to mine
//const auto readyToMine = [&](UnitInfo& worker) {
//    if (Terrain::isIslandMap() && worker.hasResource() && worker.getResource().lock()->getTilePosition().isValid() && mapBWEM.GetArea(unit.getTilePosition()) == mapBWEM.GetArea(worker.getResource().lock()->getTilePosition()))
//        return true;
//    return false;
//};

//// Check if this worker is ready to build
//const auto readyToBuild = [&](UnitInfo& worker) {
//    if (Terrain::isIslandMap() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getBuildPosition()))
//        return true;
//    return false;
//};  