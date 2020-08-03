#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls {

    namespace {
        BWEB::Wall* mainWall = nullptr;
        BWEB::Wall* naturalWall = nullptr;
        vector<UnitType> buildings;
        vector<UnitType> defenses;
        bool tight;
        bool openWall;
        UnitType tightType = None;
    }

    void findNaturalWall()
    {
        naturalWall = BWEB::Walls::getWall(BWEB::Map::getNaturalChoke());
    }

    void findMainWall()
    {
        mainWall = BWEB::Walls::getWall(BWEB::Map::getMainChoke());
    }

    void findWalls()
    {
        // Create a Zerg/Protoss wall at every natural
        if (Broodwar->self()->getRace() != Races::Terran) {
            openWall = true;
            for (auto &station : BWEB::Stations::getStations()) {
                if (!station.isNatural())
                    continue;

                auto p1 = station.getChokepoint()->Pos(station.getChokepoint()->end1);
                auto p2 = station.getChokepoint()->Pos(station.getChokepoint()->end2);

                if (abs(p1.x - p2.x) * 8 >= 320 || abs(p1.y - p2.y) * 8 >= 256 || p1.getDistance(p2) * 8 >= 352 || Broodwar->mapFileName().find("Destination") != string::npos)
                    buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
                else if (abs(p1.x - p2.x) * 8 >= 288 || abs(p1.y - p2.y) * 8 >= 224 || p1.getDistance(p2) * 8 >= 288)
                    buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber };
                else
                    buildings ={ Zerg_Hatchery };

                BWEB::Walls::createWall(buildings, station.getBase()->GetArea(), station.getChokepoint(), tightType, defenses, openWall, tight);
            }
        }

        // Create a Terran wall at every main
        else {
            for (auto &station : BWEB::Stations::getStations()) {
                if (!station.isMain())
                    continue;
                BWEB::Walls::createWall(buildings, station.getBase()->GetArea(), station.getChokepoint(), tightType, defenses, openWall, tight);
            }
        }
    }

    void initializeWallParameters()
    {
        // Figure out what we need to be tight against
        if (Players::TvP())
            tightType = Protoss_Zealot;
        else if (Players::TvZ())
            tightType = Zerg_Zergling;
        else
            tightType = None;

        // Protoss wall parameters
        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Players::vZ()) {
                tightType = None;
                tight = false;
                buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
                defenses.insert(defenses.end(), 10, Protoss_Photon_Cannon);
            }
            else {
                int count = 2;
                tight = false;
                buildings.insert(buildings.end(), count, Protoss_Pylon);
                defenses.insert(defenses.end(), 10, Protoss_Photon_Cannon);
            }
        }

        // Terran wall parameters
        if (Broodwar->self()->getRace() == Races::Terran) {
            tight = true;

            if (Broodwar->mapFileName().find("Heartbreak") != string::npos || Broodwar->mapFileName().find("Empire") != string::npos)
                buildings ={ Terran_Barracks, Terran_Supply_Depot };
            else
                buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };
        }

        // Zerg wall parameters
        if (Broodwar->self()->getRace() == Races::Zerg) {
            tight = false;
            defenses.insert(defenses.end(), 10, Zerg_Creep_Colony);

            auto p1 = BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end1);
            auto p2 = BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end2);
            buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
        }
    }

    void onStart()
    {
        initializeWallParameters();
        findWalls();
        findMainWall();
        findNaturalWall();
    }

    int needGroundDefenses(BWEB::Wall& wall)
    {
        auto groundCount = wall.getGroundDefenseCount();
        if (!Terrain::isInAllyTerritory(wall.getArea()))
            return 0;

        auto closestNatural = BWEB::Stations::getClosestNaturalStation(TilePosition(wall.getCentroid()));
        auto mineralCount = 0;
        auto workerCount = 0;
        for (auto &mineral : Resources::getMyMinerals()) {
            if (mineral->getStation() == closestNatural) {
                workerCount += int(mineral->targetedByWhat().size());
                mineralCount++;
            }
        }
        auto saturationRatio = clamp(double(workerCount) * 2 / double(mineralCount), 0.5, 1.0);

        const auto calculateDefensesNeeded = [&](int defensesDesired) {
            if (Strategy::enemyRush() || Strategy::enemyPressure())
                return defensesDesired - groundCount;
            return int(round(saturationRatio * double(defensesDesired))) - groundCount;
        };

        // Protoss
        if (Broodwar->self()->getRace() == Races::Protoss) {
            auto prepareExpansionDefenses = Util::getTime() < Time(10, 0) && vis(Protoss_Nexus) >= 2 && com(Protoss_Forge) > 0;

            if (Players::vP() && prepareExpansionDefenses && BuildOrder::isWallNat()) {
                auto cannonCount = 2 + int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                return cannonCount - groundCount;
            }

            if (Players::vZ() && BuildOrder::isWallNat()) {
                auto cannonCount = int(com(Protoss_Forge) > 0)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 6)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 12)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 24)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

                // TODO: If scout died, go to 2 cannons, if next scout dies, go 3 cannons   
                if (Strategy::getEnemyTransition() == "2HatchHydra")
                    return 5 - groundCount;
                else if (Strategy::getEnemyTransition() == "3HatchHydra")
                    return 4 - groundCount;
                else if (Strategy::getEnemyTransition() == "2HatchMuta" && Util::getTime() > Time(4, 0))
                    return 3 - groundCount;
                else if (Strategy::getEnemyTransition() == "3HatchMuta" && Util::getTime() > Time(5, 0))
                    return 3 - groundCount;
                else if (Strategy::getEnemyOpener() == "4Pool")
                    return 2 + (Players::getSupply(PlayerState::Self) >= 24) - groundCount;
                return cannonCount - groundCount;
            }
        }

        // Zerg
        if (Broodwar->self()->getRace() == Races::Zerg) {

            if (Players::vP()) {

                // 1GateCore
                if (Strategy::getEnemyBuild() == "1GateCore" && Util::getTime() < Time(7, 00))
                    return calculateDefensesNeeded((2 * (Util::getTime() > Time(5, 30))) + (2 * (Util::getTime() > Time(7, 00))));

                // 2Gate
                if (Strategy::getEnemyBuild() == "2Gate" && Util::getTime() < Time(5, 00)) {
                    if (Strategy::enemyProxy())
                        return 0;
                    if (Strategy::enemyRush())
                        return calculateDefensesNeeded(2 + (Util::getTime() > Time(4, 45)));
                    return calculateDefensesNeeded((Util::getTime() > Time(3, 30)) + (Util::getTime() > Time(4, 30)));
                }

                // FFE
                if (Strategy::getEnemyBuild() == "FFE") {
                    if (Strategy::getEnemyTransition() == "5GateGoon" && Util::getTime() < Time(10, 30))
                        return calculateDefensesNeeded((2 * (Util::getTime() > Time(5, 00))) + (2 * (Util::getTime() > Time(6, 30))) + (2 * (Util::getTime() > Time(8, 00))) + (2 * (Util::getTime() > Time(9, 30))));
                    if (Strategy::getEnemyTransition() == "NeoBisu" && Util::getTime() < Time(6, 00))
                        return calculateDefensesNeeded((2 * (Util::getTime() > Time(6, 00))));
                    if (Strategy::getEnemyTransition() == "Unknown" && Util::getTime() < Time(6, 00))
                        return calculateDefensesNeeded(Util::getTime() > Time(5, 00));
                }

                // Outside of openers, base it off how large the ground army of enemy is
                const auto divisor = 2.0 + (0.35 * vis(Zerg_Sunken_Colony));
                const auto count = int((Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon)) / divisor) + (2 * Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar));
                return max(int(Strategy::getEnemyBuild() == "2Gate") * 2, count) - groundCount;
            }

            if (Players::vT()) {
                if (Strategy::getEnemyTransition() == "2Fact" || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0)
                    return calculateDefensesNeeded(2 * (Util::getTime() > Time(3, 45)));
                if (Strategy::getEnemyTransition() == "1FactTanks")
                    return calculateDefensesNeeded(6 * (Util::getTime() > Time(10, 00)));
                if (Strategy::getEnemyTransition() == "5FactGoliath")
                    return 0;
                if (Strategy::getEnemyBuild() == "RaxFact")
                    return calculateDefensesNeeded(1 * (Util::getTime() > Time(3, 45))) + (1 * (Util::getTime() > Time(4, 15))) + (1 * (Util::getTime() > Time(4, 45)));
                if (Strategy::getEnemyBuild() == "2Rax" && !Strategy::enemyRush() && (Util::getTime() < Time(4, 30)))
                    return calculateDefensesNeeded(2 * (Util::getTime() > Time(2, 00))) + (2 * (Util::getTime() > Time(4, 30)));
                if (Strategy::enemyProxy())
                    return 0;
                if (Strategy::enemyRush())
                    return calculateDefensesNeeded(2 + (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 30)));
                if (!Strategy::enemyFastExpand())
                    return calculateDefensesNeeded(3 * (Util::getTime() > Time(4, 00)));
            }

            if (Players::vZ()) {
                if (vis(Zerg_Drone) >= 12)
                    return 2 - groundCount;
            }
        }
        return 0;
    }

    int needAirDefenses(BWEB::Wall& wall)
    {
        auto airCount = wall.getAirDefenseCount();
        if (!Terrain::isInAllyTerritory(wall.getArea()))
            return 0;

        // P
        if (Broodwar->self()->getRace() == Races::Protoss) {

        }

        // Z
        if ((Players::ZvP() || Players::ZvT()) && !BuildOrder::isTechUnit(Zerg_Mutalisk) && vis(Zerg_Lair) == 0) {
            if ((Strategy::enemyFastExpand() && Util::getTime() > Time(8, 0)) || (!Strategy::enemyFastExpand() && Util::getTime() > Time(7, 0)))
                return Strategy::enemyAir() - airCount;
        }
        return 0;
    }

    BWEB::Wall* getMainWall() { return mainWall; }
    BWEB::Wall* getNaturalWall() { return naturalWall; }
}