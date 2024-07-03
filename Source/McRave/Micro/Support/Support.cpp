#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Support {

    namespace {
        set<Position> assignedOverlords;
        set<UnitType> types ={ Zerg_Hydralisk, Zerg_Mutalisk, Protoss_Dragoon, Terran_Marine, Terran_Siege_Tank_Siege_Mode, Terran_Siege_Tank_Tank_Mode };

        void updateCounters()
        {
            assignedOverlords.clear();
        }

        void assignInBox(Point<int> h, UnitInfo& unit)
        {
            auto here = Position(h);

            // Space out for splash
            auto distBest = DBL_MAX;
            for (auto x = -2; x < 2; x++) {
                for (auto y = -2; y < 2; y++) {
                    auto position = here + Position(64 * x, 64 * y);
                    auto dist = position.getDistance(here);

                    if (dist < distBest && assignedOverlords.find(position) == assignedOverlords.end()) {
                        unit.setDestination(position);
                        distBest = dist;
                    }
                }
            }
        }

        void getSafeHome(UnitInfo& unit)
        {
            auto closestStation = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self);
            auto closestSpore = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return u->getType() == Zerg_Spore_Colony;
            });

            // Overlords safe at a spore
            if (closestSpore) {
                assignInBox(closestSpore->getPosition(), unit);
            }

            // No spore, look for hydras, go to natural where we expect them to be
            else if (total(Zerg_Hydralisk) > 0 && Players::ZvP() && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0) {
                assignInBox(Terrain::getMyNatural()->getResourceCentroid(), unit);
            }

            // Attempting to build a spore
            else if (Stations::needAirDefenses(Terrain::getMyMain()) > 0) {
                assignInBox(Terrain::getMyMain()->getResourceCentroid(), unit);
            }
            else if (Stations::needAirDefenses(Terrain::getMyNatural()) > 0) {
                assignInBox(Terrain::getMyNatural()->getResourceCentroid(), unit);
            }

            // Just go to closest station
            else if (closestStation) {
                assignInBox(closestStation->getResourceCentroid(), unit);
            }

            if (!unit.getDestination().isValid() && closestStation) {
                unit.setDestination(closestStation->getBase()->Center());
            }
            assignedOverlords.insert(unit.getDestination());
        }

        void getArmyPlacement(UnitInfo& unit)
        {
            // For each cluster, assign an overlord to the commander
            auto distBest = DBL_MAX;
            for (auto &cluster : Combat::Clusters::getClusters()) {
                auto commander = cluster.commander.lock();
                if (commander && types.find(commander->getType()) != types.end() && assignedOverlords.find(commander->getPosition()) == assignedOverlords.end()) {
                    auto dist = commander->getPosition().getDistance(unit.getPosition());
                    if (dist < distBest) {
                        unit.setDestination(commander->getPosition());
                        distBest = dist;
                    }
                }
            }

            // Find the closest unit without a detector assigned - this doesn't work great so far
            if (!unit.getDestination().isValid()) {
                distBest = DBL_MAX;
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    auto assignedDist = 320.0;
                    if (types.find(u->getType()) != types.end()) {
                        for (auto &position : assignedOverlords)
                            assignedDist = min(assignedDist, position.getDistance(u->getPosition()));

                        auto dist = u->getPosition().getDistance(unit.getPosition()) / assignedDist;
                        if (dist < distBest) {
                            unit.setDestination(u->getPosition());
                            distBest = dist;
                        }
                    }
                }
            }

            // Assign placement
            assignedOverlords.insert(unit.getDestination());

            // Adjust detectors between target and current destination
            if (unit.getType().isDetector() && unit.hasTarget()) {
                unit.setDestination((unit.getTarget().lock()->getPosition() + unit.getDestination()) / 2);
            }

            // Adjust detectors not in use between closest self station
            auto closestStation = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self);
            if (unit.getType() == Zerg_Overlord && closestStation) {
                unit.setDestination(Util::shiftTowards(unit.getDestination(), closestStation->getBase()->Center(), 96.0));
            }
        }

        void updateDestination(UnitInfo& unit)
        {
            auto closestStation = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self);

            auto enemyAir = Spy::getEnemyTransition() == "Corsair"
                || Spy::getEnemyTransition() == "2PortWraith"
                || Players::getStrength(PlayerState::Enemy).airToAir > 0.0;

            auto followArmyPossible = unit.isHealthy() && (unit.getType() != Zerg_Overlord || any_of(types.begin(), types.end(), [&](auto &t) { return com(t) >= 6; }) && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace));

            if (Util::getTime() < Time(7, 00) && Stations::getStations(PlayerState::Self).size() >= 2 && !Players::ZvZ())
                closestStation = Terrain::getMyNatural();

            // Set goal as destination
            if (unit.getGoal().isValid() && unit.getUnitsTargetingThis().empty() && unit.getUnitsInReachOfThis().empty()) {
                unit.setDestination(unit.getGoal());
            }

            // Send support units to army
            else if (followArmyPossible) {
                getArmyPlacement(unit);
            }

            // Send Overlords to a safe home
            else if (!followArmyPossible) {
                getSafeHome(unit);
            }

            if (!unit.getDestination().isValid())
                unit.setDestination(Terrain::getMainPosition());
        }

        void updatePath(UnitInfo& unit)
        {
            // Check if we need a new path
            if (!unit.getDestination().isValid() || (!unit.getDestinationPath().getTiles().empty() && unit.getDestinationPath().getTarget() == TilePosition(unit.getDestination())))
                return;

            if (unit.getType() == Zerg_Overlord && (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0 || unit.getGoal().isValid())) {
                const auto xMax = Broodwar->mapWidth() - 4;
                const auto yMax = Broodwar->mapHeight() - 4;
                const auto centerDist = min(unit.getDestination().getDistance(mapBWEM.Center()), unit.getPosition().getDistance(mapBWEM.Center()));
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                newPath.generateJPS([&](const TilePosition &t) { return !Terrain::inTerritory(PlayerState::Enemy, Position(t)) && t.isValid() && (t.x < 4 || t.y < 4 || t.x > xMax || t.y > yMax || Position(t).getDistance(mapBWEM.Center()) >= centerDist); });
                unit.setDestinationPath(newPath);
            }
            Visuals::drawPath(unit.getDestinationPath());
        }

        void updateNavigation(UnitInfo& unit)
        {
            // If path is reachable, find a point n pixels away to set as new destination
            unit.setNavigation(unit.getDestination());
            auto dist = unit.isFlying() ? 96.0 : 160.0;
            if (unit.getDestinationPath().isReachable() && unit.getPosition().getDistance(unit.getDestination()) > 96.0) {
                auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= dist;
                });

                if (newDestination.isValid())
                    unit.setNavigation(newDestination);
            }
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!Units::commandAllowed(unit))
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            static auto commands ={ Command::misc, Command::special, Command::escort };
            for (auto cmd : commands) {
                if (cmd(unit))
                    break;
            }
        }

        void updateUnits()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.getRole() == Role::Support && unit.isAvailable()) {
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
        updateCounters();
        updateUnits();
    }
}