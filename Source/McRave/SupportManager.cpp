#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Support {

    namespace {

        constexpr tuple commands{ Command::misc, Command::special, Command::escort };

        void updateCounters()
        {
        }

        void updateDestination(UnitInfo& unit)
        {
            auto closestStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());

            // Send Overlords to a corner at least 10 tiles away when we know they will have air before we do
            if (unit.getType() == Zerg_Overlord && Terrain::getEnemyStartingPosition().isValid() && Strategy::getEnemyBuild() == "1GateCore" && Strategy::getEnemyTransition() == "Corsair" && Util::getTime() < Time(8, 00)) {
                vector<Position> directions ={ Position(0,0), Position(0, Broodwar->mapHeight() * 32), Position(Broodwar->mapWidth() * 32, 0), Position(Broodwar->mapWidth() * 32, Broodwar->mapHeight() * 32) };
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &p : directions) {
                    auto pos = Util::clipPosition(p);
                    auto distEnemy = pos.getDistance(Terrain::getEnemyStartingPosition());
                    auto distHome = pos.getDistance(BWEB::Map::getMainPosition());
                    Broodwar->drawLineMap(pos, BWEB::Map::getMainPosition(), Colors::Grey);
                    if (distHome > 640.0 && distEnemy > distBest) {
                        posBest = pos;
                        distBest = distEnemy;
                    }
                }
                unit.setDestination(posBest);
            }

            // Set goal as destination
            else if (unit.getGoal().isValid())
                unit.setDestination(unit.getGoal());

            // Send Overlords around edge of map if they are far from home
            else if (unit.getType() == Zerg_Overlord && Terrain::getEnemyStartingPosition().isValid() && closestStation && unit.getPosition().getDistance(closestStation->getBase()->Center()) > 320.0) {
                auto closestCorner = Terrain::getClosestMapCorner(BWEB::Map::getMainPosition());
                if (abs(unit.getPosition().x - closestCorner.x) > abs(unit.getPosition().y - closestCorner.y))
                    unit.setDestination(Position(unit.getPosition().x, closestCorner.y));
                else
                    unit.setDestination(Position(closestCorner.x, unit.getPosition().y));
            }

            // Send Overlords to safety if needed
            else if (unit.getType() == Zerg_Overlord && (Players::getStrength(PlayerState::Self).groundToAir == 0.0 || Players::getStrength(PlayerState::Self).airToAir == 0.0)) {

                auto closestSpore = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getType() == Zerg_Spore_Colony;
                });

                auto closestStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
                if (closestSpore)
                    unit.setDestination(closestSpore->getPosition());
                else if (closestStation)
                    unit.setDestination(closestStation->getBase()->Center());
            }

            // Find the highest combat cluster that doesn't overlap a current support action of this UnitType
            else {
                auto highestCluster = 0.0;
                for (auto &[cluster, position] : Combat::getCombatClusters()) {
                    const auto score = cluster / (position.getDistance(Terrain::getAttackPosition()) * position.getDistance(unit.getPosition()));
                    if (score > highestCluster && !Actions::overlapsActions(unit.unit(), position, unit.getType(), PlayerState::Self, 200)) {
                        highestCluster = score;
                        unit.setDestination(position);
                    }
                }

                // Move detectors between target and unit vs Terran
                if (unit.getType().isDetector() && Players::vT()) {
                    if (unit.hasTarget())
                        unit.setDestination((unit.getTarget().getPosition() + unit.getDestination()) / 2);
                    else {
                        auto closestEnemy = Util::getClosestUnit(unit.getDestination(), PlayerState::Enemy, [&](auto &u) {
                            return !u.getType().isWorker() && !u.getType().isBuilding();
                        });

                        if (closestEnemy)
                            unit.setDestination((closestEnemy->getPosition() + unit.getDestination()) / 2);
                    }
                }
            }

            Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Purple);

            if (!unit.getDestination().isValid())
                unit.setDestination(BWEB::Map::getMainPosition());
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!unit.unit() || !unit.unit()->exists()                                                                                          // Prevent crashes            
                || unit.unit()->isLoaded()
                || unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())    // If the unit is locked down, maelstrommed, stassised, or not completed
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Special"),
                make_pair(2, "Escort")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateUnits()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.getRole() == Role::Support && unit.unit()->isCompleted()) {
                    updateDestination(unit);
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