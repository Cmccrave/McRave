#include "Support.h"

#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"
#include "Micro/All/Commands.h"
#include "Micro/Combat/Combat.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Support {

    namespace {
        set<Position> assignedOverlords;
        set<UnitType> types;

        void reset()
        {
            assignedOverlords.clear();
            types.clear();

            auto validTypes = {Zerg_Hydralisk, Protoss_Dragoon, Terran_Marine, Terran_Siege_Tank_Siege_Mode, Terran_Siege_Tank_Tank_Mode};
            for (auto &t : validTypes) {
                if (!Combat::State::isStaticRetreat(t))
                    types.insert(t);
            }

            if (Players::ZvT() && Spy::enemyInvis() && vis(Zerg_Hydralisk) == 0)
                types.insert(Zerg_Mutalisk);
            if (Players::ZvP() && Players::getVisibleCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) == 0 &&
                com(Zerg_Hydralisk) == 0)
                types.insert(Zerg_Zergling);
        }

        void assignInBox(Point<int> h, UnitInfo &unit)
        {
            auto here = Position(h);

            // Space out for splash
            auto distBest = DBL_MAX;
            for (auto x = -2; x < 2; x++) {
                for (auto y = -2; y < 2; y++) {
                    auto position = here + Position(64 * x, 64 * y);
                    auto dist     = position.getDistance(here);

                    if (dist < distBest && assignedOverlords.find(position) == assignedOverlords.end()) {
                        unit.setDestination(position);
                        distBest = dist;
                    }
                }
            }
        }

        void getSafeHome(UnitInfo &unit)
        {
            auto closestStation = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self);
            auto closestSpore   = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) { return u->getType() == Zerg_Spore_Colony; });

            // If this overlord was excessively far from home (maybe it was scouting), send it far away
            if (Util::getTime() > Time(4, 00) && closestStation && unit.getPosition().getDistance(closestStation->getBase()->Center()) >= 960.0 && Terrain::getEnemyNatural() &&
                Terrain::getMyNatural()) {
                auto furthest = 0.0;
                for (auto &station : BWEB::Stations::getStations()) {
                    auto dist = station.getBase()->Center().getDistance(Terrain::getEnemyNatural()->getBase()->Center()) +
                                station.getBase()->Center().getDistance(Terrain::getMyNatural()->getBase()->Center());
                    if (dist > furthest) {
                        unit.setDestination(station.getBase()->Center());
                        furthest = dist;
                    }
                }
            }

            // Assign in a safe box formation around a station or spore
            else if (closestStation && !closestStation->isMain()) {
                if (closestSpore && closestSpore->getPosition().getDistance(closestStation->getBase()->Center()) < 160.0)
                    assignInBox(closestSpore->getPosition(), unit);
                else
                    assignInBox(closestStation->getBase()->Center(), unit);
            }

            // Overlords safe at a spore
            else if (closestSpore) {
                assignInBox(closestSpore->getPosition(), unit);
            }

            // No spore, look for hydras, go to natural where we expect them to be
            else if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0) {
                assignInBox(Terrain::getMyNatural()->getBase()->Center(), unit);
            }

            // Attempting to build a spore
            else if (Stations::needAirDefenses(Terrain::getMyNatural()) > 0) {
                assignInBox(Terrain::getMyNatural()->getBase()->Center(), unit);
            }
            else if (Stations::needAirDefenses(Terrain::getMyMain()) > 0) {
                assignInBox(Terrain::getMyMain()->getBase()->Center(), unit);
            }

            // Just go to closest station
            else if (closestStation) {
                assignInBox(closestStation->getBase()->Center(), unit);
            }

            if (!unit.getDestination().isValid() && closestStation) {
                unit.setDestination(closestStation->getBase()->Center());
            }
            assignedOverlords.insert(unit.getDestination());
        }

        void getArmyPlacement(UnitInfo &unit)
        {
            // For each cluster, assign an overlord to the commander
            auto distBest = DBL_MAX;
            auto cnt      = 0;

            for (auto &cluster : Combat::Clusters::getClusters()) {
                auto commander = cluster.commander.lock();

                if (commander && types.find(commander->getType()) != types.end() && assignedOverlords.find(commander->getPosition()) == assignedOverlords.end()) {
                    auto dist = commander->getPosition().getDistance(unit.getPosition());
                    if (dist < distBest) {
                        auto position = commander->getPosition();
                        if (commander->getSimState() == SimState::Win && commander->hasTarget()) {
                            auto &commanderTarget = commander->getTarget().lock();
                            if ((commanderTarget->isCloaked() || commanderTarget->isBurrowed()) && commander->isWithinReach(*commanderTarget))
                                position = commanderTarget->getPosition();
                        }
                        unit.setDestination(position);
                        distBest = dist;
                    }
                }
            }

            // Assign placement
            assignedOverlords.insert(unit.getDestination());
        }

        void updateDestination(UnitInfo &unit)
        {
            auto enemyAir = Spy::getEnemyTransition() == P_Corsair || Spy::getEnemyTransition() == T_2PortWraith || Players::getStrength(PlayerState::Enemy).airToAir > 0.0;

            auto followArmyPossible = unit.isHealthy() && unit.getType() != Zerg_Queen && (unit.getType() != Zerg_Overlord || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace)) &&
                                      any_of(types.begin(), types.end(), [&](auto &t) { return com(t) >= 6; });

            // Set goal as destination
            if (unit.getGoal().isValid() && unit.getUnitsTargetingThis().empty() && unit.getUnitsInReachOfThis().empty()) {
                unit.setDestination(unit.getGoal());
            }

            // Send support units to army
            else if (followArmyPossible) {
                getArmyPlacement(unit);
            }

            // Send Overlords to a safe home
            if (!unit.getDestination().isValid()) {
                getSafeHome(unit);
            }
        }

        void updatePath(UnitInfo &unit)
        {
            // Check if we need a new path
            if (!unit.getDestination().isValid() || (!unit.getMarchPath().getTiles().empty() && unit.getMarchPath().getTarget() == TilePosition(unit.getDestination())))
                return;

            if (unit.getType() == Zerg_Overlord && (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0 || unit.getGoal().isValid())) {
                const auto xMax       = Broodwar->mapWidth() - 4;
                const auto yMax       = Broodwar->mapHeight() - 4;
                const auto centerDist = min(Terrain::getMainPosition().getDistance(mapBWEM.Center()), unit.getPosition().getDistance(mapBWEM.Center()));
                BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
                newPath.generateJPS([&](const TilePosition &t) {
                    return !Terrain::inTerritory(PlayerState::Enemy, Position(t)) && t.isValid() &&
                           (t.x < 4 || t.y < 4 || t.x > xMax || t.y > yMax || Position(t).getDistance(mapBWEM.Center()) >= centerDist);
                });
                unit.setMarchPath(newPath);
            }
            Visuals::drawPath(unit.getMarchPath());
        }

        void updateNavigation(UnitInfo &unit)
        {
            // If path is reachable, find a point n pixels away to set as new destination
            unit.setNavigation(unit.getDestination());
            auto dist = unit.isFlying() ? 96.0 : 160.0;
            if (unit.getMarchPath().isReachable() && unit.getPosition().getDistance(unit.getDestination()) > 96.0) {
                auto newDestination = Util::findPointOnPath(unit.getMarchPath(), [&](Position p) { return p.getDistance(unit.getPosition()) >= dist; });

                if (newDestination.isValid())
                    unit.setNavigation(newDestination);
            }
        }

        void updateDecision(UnitInfo &unit)
        {
            if (!Units::commandAllowed(unit))
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            static auto commands = {Command::misc, Command::special, Command::escort};
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
    } // namespace

    void onFrame()
    {
        reset();
        updateUnits();
    }
} // namespace McRave::Support