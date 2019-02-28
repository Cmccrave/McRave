#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Transports {

    namespace {

        double defaultVisited(UnitInfo& unit, WalkPosition w) {
            return log(min(500.0, max(100.0, double(Broodwar->getFrameCount() - Grids::lastVisitedFrame(w)))));
        }

        void updateCargo(const shared_ptr<UnitInfo>& t)
        {
            auto &transport = *t;
            auto cargoSize = 0;
            for (auto &u : transport.getAssignedCargo())
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
                        unit.setTransport(t);
                        transport.getAssignedCargo().insert(u);
                        cargoSize += unit.getType().spaceRequired();
                    }

                    if (unit.getRole() == Role::Worker && readyToAssignWorker(u)) {
                        unit.setTransport(t);
                        transport.getAssignedCargo().insert(u);
                        cargoSize += unit.getType().spaceRequired();
                    }
                }
            }
        }

        void updateDecision(const shared_ptr<UnitInfo>& t)
        {
            auto &transport = *t;

            // TODO: Broke transports for islands, fix later
            transport.setDestination(BWEB::Map::getMainPosition());
            transport.setTransportState(TransportState::None);

            shared_ptr<UnitInfo> closestCargo = nullptr;
            auto distBest = DBL_MAX;
            for (auto &c : transport.getAssignedCargo()) {
                auto dist = c->getDistance(transport);
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
                if (Util::getHighestThreat(transport.getWalkPosition(), transport) < 1.0)
                    return true;

                if (cargo.getLocalState() == LocalState::Retreat || transport.unit()->isUnderAttack() || cargo.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT)
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

                if (transport.getPosition().getDistance(cargo.getPosition()) <= 160.0 || c == closestCargo) {
                    if (!cargo.hasTarget() || cargo.getLocalState() == LocalState::Retreat || (targetDist > 128.0 || (ht && cargo.unit()->getEnergy() < 75) || (reaver && attackCooldown && threat))) {
                        return true;
                    }
                }
                return false;
            };

            // Check if this worker is ready to mine
            const auto readyToMine = [&](const shared_ptr<UnitInfo>& w) {
                auto &worker = *w;
                if (Terrain::isIslandMap() && worker.hasResource() && worker.getResource().getTilePosition().isValid() && mapBWEM.GetArea(transport.getTilePosition()) == mapBWEM.GetArea(worker.getResource().getTilePosition()))
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
                double distance = transport.getPosition().getDistance(cargo.getPosition());
                Position direction = (transport.getPosition() - cargo.getPosition()) * 64 / (int)distance;
                return cargo.getPosition() - direction;
            };

            // Check if we should be loading/unloading any cargo	
            bool shouldMonitor = false;
            for (auto &c : transport.getAssignedCargo()) {
                auto &cargo = *c;

                // If the cargo is not loaded
                if (!cargo.unit()->isLoaded()) {

                    // If it's requesting a pickup, set load state to 1	
                    if (readyToPickup(c)) {
                        transport.setTransportState(TransportState::Loading);
                        transport.setDestination(pickupPosition(c));
                        cargo.unit()->rightClick(transport.unit());
                        return;
                    }
                    else {
                        transport.setDestination(cargo.getPosition());
                        shouldMonitor = true;
                    }
                }

                // Else if the cargo is loaded
                else if (cargo.unit()->isLoaded() && cargo.hasTarget() && cargo.getEngagePosition().isValid()) {
                    transport.setDestination(cargo.getEngagePosition());

                    if (readyToFight(c)) {
                        transport.setTransportState(TransportState::Engaging);

                        if (readyToUnload(c))
                            transport.unit()->unload(cargo.unit());
                    }
                    else
                        transport.setTransportState(TransportState::Retreating);
                }

                // Dont attack until we're ready
                else if (cargo.getGlobalState() == GlobalState::Retreat)
                    transport.setDestination(BWEB::Map::getNaturalPosition());
            }

            if (shouldMonitor)
                transport.setTransportState(TransportState::Monitoring);

            //for (auto &w : transport.getAssignedWorkers()) {
            //	bool miner = false;
            //	bool builder = false;

            //	if (!w || !w->unit())
            //		continue;

            //	auto &worker = *w;
            //	builder = worker.getBuildPosition().isValid();
            //	miner = !worker.getBuildPosition().isValid() && worker.hasResource();

            //	if (worker.unit()->exists() && !worker.unit()->isLoaded() && !worker.unit()->isCarryingMinerals() && !worker.unit()->isCarryingGas()) {
            //		if ((miner && readyToMine(worker)) || (builder && readyToBuild(worker))) {
            //			transport.removeWorker(&worker);
            //			worker.setTransport(nullptr);
            //			return;
            //		}
            //		else if (transport.unit()->getLoadedUnits().empty() || transport.getPosition().getDistance(worker.getPosition()) < 320.0) {
            //			transport.setLoading(true);
            //			transport.unit()->load(worker.unit());
            //			return;
            //		}
            //	}
            //	else {
            //		double airDist = DBL_MAX;
            //		double grdDist = DBL_MAX;

            //		if (worker.getBuildPosition().isValid())
            //			transport.setDestination((Position)worker.getBuildPosition());

            //		else if (worker.hasResource())
            //			transport.setDestination(worker.getResource().getPosition());

            //		if ((miner && readyToMine(worker)) || (builder && readyToBuild(worker)))
            //			transport.unit()->unload(worker.unit());
            //	}
            //}

            //if (transport.getDestination() == BWEB::Map::getMainPosition() && Terrain::isIslandMap()) {
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
            //		transport.setDestination(posBest);
            //	}
            //}
        }

        void updateMovement(const shared_ptr<UnitInfo>& t)
        {
            auto &transport = *t;

            // Check if the destination can be used for ground distance
            auto dropTarget = transport.getDestination();
            if (!Util::isWalkable(dropTarget) || BWEB::Map::getGroundDistance(transport.getPosition(), dropTarget) == DBL_MAX) {
                auto distBest = DBL_MAX;
                for (auto &cargo : transport.getAssignedCargo()) {
                    if (cargo->hasTarget()) {
                        auto cargoTarget = cargo->getTarget().getPosition();
                        auto dist = cargoTarget.getDistance(transport.getPosition());
                        if (Util::isWalkable(cargoTarget) && dist < distBest) {
                            dropTarget = cargoTarget;
                            distBest = dist;
                        }
                    }
                }
            }

            auto bestPos = Positions::Invalid;
            auto start = transport.getWalkPosition();
            auto best = 0.0;

            for (int x = start.x - 24; x < start.x + 24 + transport.getType().tileWidth() * 4; x++) {
                for (int y = start.y - 24; y < start.y + 24 + transport.getType().tileWidth() * 4; y++) {
                    WalkPosition w(x, y);
                    Position p = Position(w) + Position(4, 4);
                    TilePosition t(p);

                    if (!w.isValid()
                        //|| (transport.getTransportState() == TransportState::Engaging && !Util::isWalkable(start, w, UnitTypes::Protoss_Reaver && p.getDistance(transport.getDestination()) < 64.0)) disabled
                        || (transport.getTransportState() == TransportState::Engaging && Broodwar->getGroundHeight(TilePosition(w)) != Broodwar->getGroundHeight(TilePosition(dropTarget)) && p.getDistance(transport.getDestination()) < 64.0))
                        continue;

                    // If we just dropped units, we need to make sure not to leave them
                    if (transport.getTransportState() == TransportState::Monitoring) {
                        bool proximity = true;
                        for (auto &u : transport.getAssignedCargo()) {
                            if (!u->unit()->isLoaded() && u->getPosition().getDistance(p) > 64.0)
                                proximity = false;
                        }
                        if (!proximity)
                            continue;
                    }

                    double threat = (transport.getPercentShield() > LOW_SHIELD_PERCENT_LIMIT && transport.getTransportState() == TransportState::Engaging) ? 1.0 : Util::getHighestThreat(w, transport);
                    double distance = p.getDistance(transport.getDestination());
                    double visited = defaultVisited(transport, w);
                    double score = visited / (threat * distance);

                    if (score > best) {
                        best = score;
                        bestPos = p;
                    }
                }
            }

            if (bestPos.isValid())
                transport.command(UnitCommandTypes::Move, bestPos, true);            
            else
                Command::move(t);
        }

        void updateTransports()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() == Role::Transport) {
                    updateCargo(u);
                    updateDecision(u);
                    updateMovement(u);
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