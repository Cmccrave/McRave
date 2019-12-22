#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave {
    enum class GoalType {
        None, Contain, Explore // ...possible future improvements here for better goal positioning
    };
}

namespace McRave::Goals {

    namespace {

        map<Position, int> goalFrameUpdate;

        void assignNumberToGoal(Position here, UnitType type, int count, GoalType gType = GoalType::None)
        {
            //// Only updates goals every 10 seconds to prevent indecisiveness
            //if (goalFrameUpdate[here] < Broodwar->getFrameCount() + 240)
            //    return;
            goalFrameUpdate[here] = Broodwar->getFrameCount();

            map<double, shared_ptr<UnitInfo>> unitByDist;
            map<UnitType, int> unitByType;

            for (int current = 0; current < count; current++) {
                shared_ptr<UnitInfo> closest;
                if (type.isFlyer())
                    closest = Util::getClosestUnit(here, PlayerState::Self, [&](auto &u) {
                    return u.getType() == type && !u.getGoal().isValid();
                });                

                if (closest) {
                    if (gType == GoalType::Contain)
                        here = Util::getConcavePosition(*closest, mapBWEM.GetArea(TilePosition(here)));
                    // else if (state == GoalState::Explore)
                        // find unexplored tiles in the area?
                    closest->setGoal(here);
                }
            }

            //// Store units by distance if they have a matching type
            //for (auto &u : Units::getUnits(PlayerState::Self)) {
            //    UnitInfo &unit = *u;

            //    if (unit.getType() == type) {
            //        double dist = unit.getType().isFlyer() ? unit.getPosition().getDistance(here) : BWEB::Map::getGroundDistance(unit.getPosition(), here);
            //        unitByDist[dist] = u;
            //    }
            //}

            //// Iterate through closest units
            //for (auto &u : unitByDist) {
            //    UnitInfo &unit = *u.second;
            //    if (count > 0 && !unit.getGoal().isValid()) {
            //        unit.setGoal(here);
            //        count--;
            //    }
            //}
        }

        void assignPercentToGoal(Position here, UnitType type, double percent, GoalType gType = GoalType::None)
        {
            // Clamp between 1 and 4
            int count = clamp(int(percent * double(vis(type))), 1, 4);
            assignNumberToGoal(here, type, count, gType);
        }

        void updateProtossGoals()
        {
            map<UnitType, int> unitTypes;
            auto enemyStrength = Players::getStrength(PlayerState::Enemy);
            auto myRace = Broodwar->self()->getRace();

            // Defend my expansions
            if (enemyStrength.groundToGround > 0) {
                for (auto &s : Stations::getMyStations()) {
                    auto station = *s.second;

                    if (station.getBWEMBase()->Location() != BWEB::Map::getNaturalTile() && station.getBWEMBase()->Location() != BWEB::Map::getMainTile() && station.getGroundDefenseCount() == 0) {
                        assignNumberToGoal(station.getBWEMBase()->Center(), Protoss_Dragoon, 2, GoalType::Contain);
                    }
                }
            }

            // Attack enemy expansions with a small force
            // PvP / PvT
            if (Players::vP() || Players::vT()) {
                auto distBest = DBL_MAX;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getBWEMBase()->Center();
                    auto dist = BWEB::Map::getGroundDistance(pos, Terrain::getEnemyStartingPosition());
                    if (dist < distBest) {
                        distBest = dist;
                        posBest = pos;
                    }
                }
                if (Players::vP())
                    assignPercentToGoal(posBest, Protoss_Dragoon, 0.15);
                else
                    assignPercentToGoal(posBest, Protoss_Zealot, 0.15);
            }

            // Send a DT / Zealot + Goon squad to enemys furthest station
            // PvE
            if (Stations::getMyStations().size() >= 2) {
                auto distBest = 0.0;
                auto posBest = Positions::Invalid;
                for (auto &s : Stations::getEnemyStations()) {
                    auto station = *s.second;
                    auto pos = station.getBWEMBase()->Center();
                    auto dist = pos.getDistance(Terrain::getEnemyStartingPosition());

                    if (dist > distBest) {
                        posBest = pos;
                        distBest = dist;
                    }
                }

                if (Actions::overlapsDetection(nullptr, posBest, PlayerState::Enemy) || vis(Protoss_Dark_Templar) == 0) {
                    if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) > 0)
                        assignNumberToGoal(posBest, Protoss_Zealot, 4);
                    else
                        assignNumberToGoal(posBest, Protoss_Dragoon, 4);
                }
                else
                    assignNumberToGoal(posBest, Protoss_Dark_Templar, vis(Protoss_Dark_Templar));
            }

            // Secure our own future expansion position
            // PvE
            Position nextExpand(Buildings::getCurrentExpansion());
            if (nextExpand.isValid()) {

                auto building = Broodwar->self()->getRace().getResourceDepot();

                if (vis(building) >= 2 && BuildOrder::buildCount(building) > vis(building)) {
                    if (Players::vZ())
                        assignPercentToGoal(nextExpand, Protoss_Zealot, 0.15);
                    else {
                        assignPercentToGoal(nextExpand, Protoss_Dragoon, 0.25);
                    }

                    assignNumberToGoal(nextExpand, Protoss_Observer, 1);
                }
            }

            // Escort shuttles
            // PvZ
            if (Players::vZ() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    UnitInfo &unit = *u;
                    if (unit.getRole() == Role::Transport)
                        assignPercentToGoal(unit.getPosition(), Protoss_Corsair, 0.25);
                }
            }

            // Deny enemy expansions
            // PvT
            if (Stations::getMyStations().size() >= 3 && Terrain::getEnemyExpand().isValid() && Terrain::getEnemyExpand() != Buildings::getCurrentExpansion() && BWEB::Map::isUsed(Terrain::getEnemyExpand()) == None) {
                if (Players::vT() || Players::vP())
                    assignPercentToGoal((Position)Terrain::getEnemyExpand(), Protoss_Dragoon, 0.15);
                else
                    assignNumberToGoal((Position)Terrain::getEnemyExpand(), Protoss_Dark_Templar, 1);
            }

            // Defend our natural versus 2Fact
            // PvT
            if (Players::vT() && Strategy::enemyPressure() && !BuildOrder::isPlayPassive() && Broodwar->getFrameCount() < 15000) {
                assignNumberToGoal(BWEB::Map::getNaturalPosition(), Protoss_Dragoon, 2);
            }
        }

        void updateTerranGoals()
        {

        }

        void updateZergGoals()
        {
            Position nextExpand(Buildings::getCurrentExpansion());
            if (nextExpand.isValid()) {
                auto building = Broodwar->self()->getRace().getResourceDepot();
                if (vis(building) >= 2 && BuildOrder::buildCount(building) > vis(building))
                    assignNumberToGoal(nextExpand, Zerg_Overlord, 1);
            }
        }
    }

    void onFrame()
    {
        updateProtossGoals();
        updateTerranGoals();
        updateZergGoals();
    }
}