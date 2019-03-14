#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Goals {

    namespace {

        void assignNumberToGoal(Position here, UnitType type, int count)
        {
            map<double, shared_ptr<UnitInfo>> unitByDist;
            map<UnitType, int> unitByType;

            // Store units by distance if they have a matching type
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;

                if (unit.getType() == type) {
                    double dist = unit.getType().isFlyer() ? unit.getPosition().getDistance(here) : BWEB::Map::getGroundDistance(unit.getPosition(), here);
                    unitByDist[dist] = u;
                }
            }

            // Iterate through closest units
            for (auto &u : unitByDist) {
                UnitInfo &unit = *u.second;
                if (count > 0 && !unit.getGoal().isValid()) {
                    unit.setGoal(here);
                    count--;
                }
            }
        }

        void assignPercentToGoal(Position here, UnitType type, double percent)
        {
            // Cap at 4
            int count = min(4, int(percent * double(Units::getMyVisible(type))));
            assignNumberToGoal(here, type, count);
        }

        void updateProtossGoals()
        {
            map<UnitType, int> unitTypes;
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);

            // Defend my expansions
            if (enemyStrength.groundToGround > 0) {
                for (auto &s : Stations::getMyStations()) {
                    auto station = *s.second;

                    if (station.getBWEMBase()->Location() != BWEB::Map::getNaturalTile() && station.getBWEMBase()->Location() != BWEB::Map::getMainTile() && station.getDefenseCount() == 0) {
                        assignPercentToGoal(station.getBWEMBase()->Center(), UnitTypes::Protoss_Dragoon, 0.15);
                    }
                }
            }

            // Attack enemy expansions with a small force
            // PvP / PvT
            if (Players::vP() || Players::vT()) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getBWEMBase()->Center();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist > distBest) {
                        distBest = dist;
                        posBest = pos;
                    }
                }
                if (Players::vP())
                    assignPercentToGoal(posBest, UnitTypes::Protoss_Dragoon, 0.15);
                else
                    assignPercentToGoal(posBest, UnitTypes::Protoss_Zealot, 0.15);
            }

            // Send a DT everywhere late game
            // PvE
            if (Stations::getMyStations().size() >= 4) {
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getBWEMBase()->Center();
                    assignNumberToGoal(pos, UnitTypes::Protoss_Dark_Templar, 1);
                }
            }

            // Secure our own future expansion position
            // PvE
            Position nextExpand(Buildings::getCurrentExpansion());
            if (nextExpand.isValid()) {
                UnitType building = Broodwar->self()->getRace().getResourceDepot();
                if (BuildOrder::buildCount(building) > Broodwar->self()->visibleUnitCount(building)) {
                    if (Players::vZ())
                        assignPercentToGoal(nextExpand, UnitTypes::Protoss_Zealot, 0.15);
                    else {
                        assignPercentToGoal(nextExpand, UnitTypes::Protoss_Dragoon, 0.25);
                    }
                }
            }

            // Escort shuttles
            // PvZ
            if (Players::vZ() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    UnitInfo &unit = *u;
                    if (unit.getRole() == Role::Transport)
                        assignPercentToGoal(unit.getPosition(), UnitTypes::Protoss_Corsair, 0.25);
                }
            }

            // Deny enemy expansions
            // PvT
            if (Stations::getMyStations().size() >= 3 && Terrain::getEnemyExpand().isValid()) {
                if (Players::vT() || Players::vP())
                    assignPercentToGoal((Position)Terrain::getEnemyExpand(), UnitTypes::Protoss_Dragoon, 0.15);
                else
                    assignNumberToGoal((Position)Terrain::getEnemyExpand(), UnitTypes::Protoss_Dark_Templar, 1);
            }
        }

        void updateTerranGoals()
        {

        }

        void updateZergGoals()
        {
            //// Send lurkers to expansions when turtling		
            //if (Broodwar->self()->getRace() == Races::Zerg && !Stations::getMyStations().empty()) {
            //    auto lurkerPerBase = Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Lurker) / Stations::getMyStations().size();

            //    for (auto &base : Stations::getMyStations()) {
            //        auto station = *base.second;
            //        assignPercentToGoal(station.getResourceCentroid(), UnitTypes::Zerg_Lurker, 0.25);
            //    }
            //}
        }
    }

    void onFrame()
    {
        updateProtossGoals();
        updateTerranGoals();
        updateZergGoals();
    }
}