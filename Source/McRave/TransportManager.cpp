#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Transports {

    namespace {

        constexpr tuple commands{ Command::transport, Command::move, Command::retreat };

        double defaultVisited(UnitInfo& unit, WalkPosition w) {
            return log(min(500.0, max(100.0, double(Broodwar->getFrameCount() - Grids::lastVisitedFrame(w)))));
        }

        void updateCargo(UnitInfo& unit)
        {
            auto cargoSize = 0;
            for (auto &c : unit.getAssignedCargo()) {
                if (auto &cargo = c.lock())
                    cargoSize += cargo->getType().spaceRequired();
            }

            // Check if we are ready to assign this worker to a transport
            const auto readyToAssignWorker = [&](UnitInfo& cargo) {
                if (cargoSize + cargo.getType().spaceRequired() > 8 || cargo.hasTransport())
                    return false;

                return false;

                auto buildDist = cargo.getBuildingType().isValid() ? BWEB::Map::getGroundDistance((Position)cargo.getBuildPosition(), (Position)cargo.getTilePosition()) : 0.0;
                auto resourceDist = cargo.hasResource() ? BWEB::Map::getGroundDistance(cargo.getPosition(), cargo.getResource().getPosition()) : 0.0;

                if ((cargo.getBuildPosition().isValid() && buildDist == DBL_MAX) || (cargo.getBuildingType().isResourceDepot() && Terrain::isIslandMap()))
                    return true;
                if (cargo.hasResource() && resourceDist == DBL_MAX)
                    return true;
                return false;
            };

            // Check if we are ready to assign this unit to a transport
            const auto readyToAssignUnit = [&](UnitInfo& cargo) {
                if (cargoSize + cargo.getType().spaceRequired() > 8 || cargo.hasTransport())
                    return false;

                auto targetDist = BWEB::Map::getGroundDistance(cargo.getPosition(), cargo.getEngagePosition());

                if (cargo.getType() == UnitTypes::Protoss_Reaver || cargo.getType() == UnitTypes::Protoss_High_Templar)
                    return true;
                if (Terrain::isIslandMap() && !cargo.getType().isFlyer() && targetDist > 640.0)
                    return true;
                return false;
            };

            // Update cargo information
            if (cargoSize < 8) {
                for (auto &c : Units::getUnits(PlayerState::Self)) {
                    auto &cargo = *c;

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
        }

        void updateTransportState(UnitInfo& unit)
        {
            // TODO: Broke transports for islands, fix later
            unit.setDestination(BWEB::Map::getMainPosition());
            unit.setTransportState(TransportState::None);

            UnitInfo * closestCargo = nullptr;
            auto distBest = DBL_MAX;
            for (auto &c : unit.getAssignedCargo()) {
                if (c.expired())
                    continue;
                auto &cargo = c.lock();

                if (auto &cargo = c.lock()) {
                    auto dist = cargo->getDistance(unit);
                    if (dist < distBest) {
                        distBest = dist;
                        closestCargo = &*cargo;
                    }
                }
            }

            // Check if this unit is ready to fight
            const auto readyToFight = [&](UnitInfo& cargo) {
                auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
                auto ht = cargo.getType() == UnitTypes::Protoss_High_Templar;

                // TESTING THIS:
                if (Util::getHighestThreat(unit.getWalkPosition(), unit) < 1.0)
                    return true;

                if (cargo.getLocalState() == LocalState::Retreat || unit.unit()->isUnderAttack() || cargo.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT)
                    return false;
                if (cargo.getLocalState() == LocalState::Attack && ((reaver && cargo.canStartAttack()) || (ht && cargo.getEnergy() >= 75)))
                    return true;
                return false;
            };

            // Check if this unit is ready to unload
            const auto readyToUnload = [&](UnitInfo& cargo) {
                auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
                auto targetDist = cargo.getPosition().getDistance(cargo.getEngagePosition());
                if (targetDist <= 64.0 && cargo.canStartAttack())
                    return true;
                return false;
            };

            // Check if this unit is ready to be picked up
            const auto readyToPickup = [&](UnitInfo& cargo) {
                auto attackCooldown = (Broodwar->getFrameCount() - cargo.getLastAttackFrame() <= 60 - Broodwar->getLatencyFrames());
                auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
                auto ht = cargo.getType() == UnitTypes::Protoss_High_Templar;
                auto threat = Grids::getEGroundThreat(cargo.getWalkPosition()) > 0.0;
                auto targetDist = reaver && cargo.hasTarget() ? BWEB::Map::getGroundDistance(cargo.getPosition(), cargo.getTarget().getPosition()) - 256.0 : cargo.getPosition().getDistance(cargo.getEngagePosition());

                if (unit.getPosition().getDistance(cargo.getPosition()) <= 160.0 || &cargo == closestCargo) {
                    if (!cargo.hasTarget() || cargo.getLocalState() == LocalState::Retreat || targetDist > 128.0 || (ht && cargo.unit()->getEnergy() < 75) || (reaver && attackCooldown && threat)) {
                        return true;
                    }
                }
                return false;
            };

            // Check if this worker is ready to mine
            const auto readyToMine = [&](UnitInfo& worker) {
                if (Terrain::isIslandMap() && worker.hasResource() && worker.getResource().getTilePosition().isValid() && mapBWEM.GetArea(unit.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition()))
                    return true;
                return false;
            };

            // Check if this worker is ready to build
            const auto readyToBuild = [&](UnitInfo& worker) {
                if (Terrain::isIslandMap() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getBuildPosition()))
                    return true;
                return false;
            };

            const auto pickupPosition = [&](UnitInfo& cargo) {
                double distance = unit.getPosition().getDistance(cargo.getPosition());
                Position direction = (unit.getPosition() - cargo.getPosition()) * 64 / (int)distance;
                return cargo.getPosition() - direction;
            };

            // Check if we should be loading/unloading any cargo
            bool shouldMonitor = false;
            for (auto &c : unit.getAssignedCargo()) {
                if (!c.lock())
                    continue;
                auto &cargo = *(c.lock());

                // If the cargo is not loaded
                if (!cargo.unit()->isLoaded()) {

                    // If it's requesting a pickup, set load state to 1	
                    if (readyToPickup(cargo)) {
                        unit.setTransportState(TransportState::Loading);
                        unit.setDestination(pickupPosition(cargo));
                        return;
                    }
                    else {
                        unit.setDestination(cargo.getPosition());
                        shouldMonitor = true;
                    }
                }

                // Else if the cargo is loaded
                else if (cargo.unit()->isLoaded() && cargo.hasTarget() && cargo.getEngagePosition().isValid()) {
                    unit.setDestination(cargo.getEngagePosition());

                    if (readyToFight(cargo)) {
                        unit.setTransportState(TransportState::Engaging);

                        if (readyToUnload(cargo))
                            unit.unit()->unload(cargo.unit());
                    }
                    else
                        unit.setTransportState(TransportState::Retreating);
                }

                // Dont attack until we're ready
                else if (cargo.getGlobalState() == GlobalState::Retreat)
                    unit.setDestination(BWEB::Map::getNaturalPosition());
            }

            if (shouldMonitor)
                unit.setTransportState(TransportState::Monitoring);
        }

        void updateDestination(UnitInfo& unit)
        {
            // Disabled - this didn't do anything in AIST version
            // Check if the destination can be used for ground distance
            //if (!Util::isWalkable(unit.getDestination())) {
            //    auto distBest = DBL_MAX;
            //    for (auto &cargo : unit.getAssignedCargo()) {
            //        if (cargo->hasTarget()) {
            //            auto cargoTarget = cargo->getTarget().getPosition();
            //            auto dist = cargoTarget.getDistance(unit.getPosition());
            //            if (Util::isWalkable(cargoTarget) && dist < distBest) {
            //                unit.setDestination(cargoTarget);
            //                distBest = dist;
            //            }
            //        }
            //    }
            //}
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!unit.unit() || !unit.unit()->exists()																							// Prevent crashes
                || unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())	// If the unit is locked down, maelstrommed, stassised, or not completed
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Transport"),
                make_pair(1, "Move"),
                make_pair(2, "Retreat"),
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateTransports()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() == Role::Transport) {
                    updateCargo(unit);
                    updateTransportState(unit);
                    updateDestination(unit);
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
        if (unit.hasTransport()) {
            //for (auto &cargo : unit.getTransport().getAssignedCargo()) {
            //    if (cargo.lock())
            //        unit.getTransport().getAssignedCargo().erase(cargo);
            //}
        }

        for (auto &c : unit.getAssignedCargo()) {
            if (auto &cargo = c.lock())
                cargo->setTransport(nullptr);
        }
    }
}