#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Transports {

    namespace {

        constexpr tuple commands{ Command::transport, Command::move, Command::retreat };

        double defaultVisited(UnitInfo& unit, WalkPosition w) {
            return log(min(500.0, max(100.0, double(Broodwar->getFrameCount() - Grids::lastVisitedFrame(w)))));
        }

        void updateCargo(const shared_ptr<UnitInfo>& u)
        {
            auto &unit = *u;
            auto cargoSize = 0;
            for (auto &u : unit.getAssignedCargo())
                cargoSize += u->getType().spaceRequired();

            // Check if we are ready to assign this worker to a transport
            const auto readyToAssignWorker = [&](const shared_ptr<UnitInfo>& u) {
                auto &unit = *u;
                if (cargoSize + unit.getType().spaceRequired() > 8 || unit.hasTransport())
                    return false;

                return false;

                auto buildDist = unit.getBuildingType().isValid() ? BWEB::Map::getGroundDistance((Position)unit.getBuildPosition(), (Position)unit.getTilePosition()) : 0.0;
                auto resourceDist = unit.hasResource() ? BWEB::Map::getGroundDistance(unit.getPosition(), unit.getResource().getPosition()) : 0.0;

                if ((unit.getBuildPosition().isValid() && buildDist == DBL_MAX) || (unit.getBuildingType().isResourceDepot() && Terrain::isIslandMap()))
                    return true;
                if (unit.hasResource() && resourceDist == DBL_MAX)
                    return true;
                return false;
            };

            // Check if we are ready to assign this unit to a transport
            const auto readyToAssignUnit = [&](const shared_ptr<UnitInfo>& u) {
                auto &unit = *u;
                if (cargoSize + unit.getType().spaceRequired() > 8 || unit.hasTransport())
                    return false;

                auto targetDist = BWEB::Map::getGroundDistance(unit.getPosition(), unit.getEngagePosition());

                if (unit.getType() == UnitTypes::Protoss_Reaver || unit.getType() == UnitTypes::Protoss_High_Templar)
                    return true;
                if (Terrain::isIslandMap() && !unit.getType().isFlyer() && targetDist > 640.0)
                    return true;
                return false;
            };

            // Update cargo information
            if (cargoSize < 8) {
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    auto &unit = *u;

                    if (unit.getRole() == Role::Combat && readyToAssignUnit(u)) {
                        unit.setTransport(u);
                        unit.getAssignedCargo().insert(u);
                        cargoSize += unit.getType().spaceRequired();
                    }

                    if (unit.getRole() == Role::Worker && readyToAssignWorker(u)) {
                        unit.setTransport(u);
                        unit.getAssignedCargo().insert(u);
                        cargoSize += unit.getType().spaceRequired();
                    }
                }
            }
        }

        void updateTransportState(const shared_ptr<UnitInfo>& u)
        {
            auto &unit = *u;

            // TODO: Broke transports for islands, fix later
            unit.setDestination(BWEB::Map::getMainPosition());
            unit.setTransportState(TransportState::None);

            shared_ptr<UnitInfo> closestCargo = nullptr;
            auto distBest = DBL_MAX;
            for (auto &c : unit.getAssignedCargo()) {
                auto dist = c->getDistance(unit);
                if (dist < distBest) {
                    distBest = dist;
                    closestCargo = c;
                }
            }

            // Check if this unit is ready to fight
            const auto readyToFight = [&](const shared_ptr<UnitInfo>& c) {
                auto &cargo = *c;

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
            const auto readyToUnload = [&](const shared_ptr<UnitInfo>& c) {
                auto &cargo = *c;
                auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
                auto targetDist = cargo.getPosition().getDistance(cargo.getEngagePosition());
                if (targetDist <= 64.0 && cargo.canStartAttack())
                    return true;
                return false;
            };

            // Check if this unit is ready to be picked up
            const auto readyToPickup = [&](const shared_ptr<UnitInfo>& c) {
                auto &cargo = *c;
                auto attackCooldown = (Broodwar->getFrameCount() - cargo.getLastAttackFrame() <= 60 - Broodwar->getLatencyFrames());
                auto reaver = cargo.getType() == UnitTypes::Protoss_Reaver;
                auto ht = cargo.getType() == UnitTypes::Protoss_High_Templar;
                auto threat = Grids::getEGroundThreat(cargo.getWalkPosition()) > 0.0;
                auto targetDist = reaver && cargo.hasTarget() ? BWEB::Map::getGroundDistance(cargo.getPosition(), cargo.getTarget().getPosition()) - 256.0 : cargo.getPosition().getDistance(cargo.getEngagePosition());

                if (unit.getPosition().getDistance(cargo.getPosition()) <= 160.0 || c == closestCargo) {
                    if (!cargo.hasTarget() || cargo.getLocalState() == LocalState::Retreat || (targetDist > 128.0 || (ht && cargo.unit()->getEnergy() < 75) || (reaver && attackCooldown && threat))) {
                        return true;
                    }
                }
                return false;
            };

            // Check if this worker is ready to mine
            const auto readyToMine = [&](const shared_ptr<UnitInfo>& w) {
                auto &worker = *w;
                if (Terrain::isIslandMap() && worker.hasResource() && worker.getResource().getTilePosition().isValid() && mapBWEM.GetArea(unit.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition()))
                    return true;
                return false;
            };

            // Check if this worker is ready to build
            const auto readyToBuild = [&](const shared_ptr<UnitInfo>& w) {
                auto &worker = *w;
                if (Terrain::isIslandMap() && worker.getBuildPosition().isValid() && mapBWEM.GetArea(worker.getTilePosition()) == mapBWEM.GetArea(worker.getBuildPosition()))
                    return true;
                return false;
            };

            const auto pickupPosition = [&](const shared_ptr<UnitInfo>& c) {
                auto &cargo = *c;
                double distance = unit.getPosition().getDistance(cargo.getPosition());
                Position direction = (unit.getPosition() - cargo.getPosition()) * 64 / (int)distance;
                return cargo.getPosition() - direction;
            };

            // Check if we should be loading/unloading any cargo	
            bool shouldMonitor = false;
            for (auto &c : unit.getAssignedCargo()) {
                auto &cargo = *c;

                // If the cargo is not loaded
                if (!cargo.unit()->isLoaded()) {

                    // If it's requesting a pickup, set load state to 1	
                    if (readyToPickup(c)) {
                        unit.setTransportState(TransportState::Loading);
                        unit.setDestination(pickupPosition(c));
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

                    if (readyToFight(c)) {
                        unit.setTransportState(TransportState::Engaging);

                        if (readyToUnload(c))
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

            //for (auto &w : unit.getAssignedWorkers()) {
            //	bool miner = false;
            //	bool builder = false;

            //	if (!w || !w->unit())
            //		continue;

            //	auto &worker = *w;
            //	builder = worker.getBuildPosition().isValid();
            //	miner = !worker.getBuildPosition().isValid() && worker.hasResource();

            //	if (worker.unit()->exists() && !worker.unit()->isLoaded() && !worker.unit()->isCarryingMinerals() && !worker.unit()->isCarryingGas()) {
            //		if ((miner && readyToMine(worker)) || (builder && readyToBuild(worker))) {
            //			unit.removeWorker(&worker);
            //			worker.setTransport(nullptr);
            //			return;
            //		}
            //		else if (unit.unit()->getLoadedUnits().empty() || unit.getPosition().getDistance(worker.getPosition()) < 320.0) {
            //			unit.setLoading(true);
            //			unit.unit()->load(worker.unit());
            //			return;
            //		}
            //	}
            //	else {
            //		double airDist = DBL_MAX;
            //		double grdDist = DBL_MAX;

            //		if (worker.getBuildPosition().isValid())
            //			unit.setDestination((Position)worker.getBuildPosition());

            //		else if (worker.hasResource())
            //			unit.setDestination(worker.getResource().getPosition());

            //		if ((miner && readyToMine(worker)) || (builder && readyToBuild(worker)))
            //			unit.unit()->unload(worker.unit());
            //	}
            //}

            //if (unit.getDestination() == BWEB::Map::getMainPosition() && Terrain::isIslandMap()) {
            //	double distBest = DBL_MAX;
            //	Position posBest = Positions::None;
            //	for (auto &tile : mapBWEM.StartingLocations()) {
            //		Position center = Position(tile) + Position(64, 48);
            //		double dist = center.getDistance(BWEB::Map::getMainPosition());

            //		if (!Broodwar->isExplored(tile) && dist < distBest) {
            //			distBest = dist;
            //			posBest = center;
            //		}
            //	}
            //	if (posBest.isValid()) {
            //		unit.setDestination(posBest);
            //	}
            //}
        }

        void updateDestination(const shared_ptr<UnitInfo>& u)
        {
            auto &unit = *u;

            // Check if the destination can be used for ground distance
            if (!Util::isWalkable(unit.getDestination()) || BWEB::Map::getGroundDistance(unit.getPosition(), unit.getDestination()) == DBL_MAX) {
                auto distBest = DBL_MAX;
                for (auto &cargo : unit.getAssignedCargo()) {
                    if (cargo->hasTarget()) {
                        auto cargoTarget = cargo->getTarget().getPosition();
                        auto dist = cargoTarget.getDistance(unit.getPosition());
                        if (Util::isWalkable(cargoTarget) && dist < distBest) {
                            unit.setDestination(cargoTarget);
                            distBest = dist;
                        }
                    }
                }
            }
        }

        void updateDecision(const shared_ptr<UnitInfo>& u)
        {
            auto &unit = *u;
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
            int i = Util::iterateCommands(commands, u);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateTransports()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() == Role::Transport) {
                    updateCargo(u);
                    updateTransportState(u);
                    updateDestination(u);
                    updateDecision(u);
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

    void removeUnit(const shared_ptr<UnitInfo>& unit)
    {
        if (unit->hasTransport())
            unit->getTransport().getAssignedCargo().erase(unit);

        for (auto &cargo : unit->getAssignedCargo())
            cargo->setTransport(nullptr);
    }
}