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

                /*if (Broodwar->self()->getRace() == Races::Zerg) {
                    if (abs(p1.x - p2.x) * 8 >= 320 || abs(p1.y - p2.y) * 8 >= 256 || p1.getDistance(p2) * 8 >= 352)
                        buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
                    else if (abs(p1.x - p2.x) * 8 >= 288 || abs(p1.y - p2.y) * 8 >= 224 || p1.getDistance(p2) * 8 >= 288)
                        buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber };
                    else
                        buildings ={ Zerg_Hatchery };
                }*/
                if (Players::PvT() || Players::PvP()) {
                    if (abs(p1.x - p2.x) * 8 >= 352 || abs(p1.y - p2.y) * 8 >= 352 || p1.getDistance(p2) * 8 >= 386 || Broodwar->mapFileName().find("Destination") != string::npos)
                        buildings ={ Protoss_Pylon, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon };
                    else if (abs(p1.x - p2.x) * 8 >= 288 || abs(p1.y - p2.y) * 8 >= 288 || p1.getDistance(p2) * 8 >= 288)
                        buildings ={ Protoss_Pylon, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon };
                    else
                        buildings ={ Protoss_Pylon, Protoss_Pylon, Protoss_Pylon };
                }

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
            defenses.insert(defenses.end(), 20, Zerg_Creep_Colony);
            //buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
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
        auto closestMain = BWEB::Stations::getClosestMainStation(TilePosition(wall.getCentroid()));
        auto resourceCount = 0.0;
        auto workerCount = 0;

        for (auto &mineral : Resources::getMyMinerals()) {
            if (mineral->getStation() == closestNatural || mineral->getStation() == closestMain) {
                workerCount += int(mineral->targetedByWhat().size());
                resourceCount++;
            }
        }
        for (auto &gas : Resources::getMyGas()) {
            if (gas->getStation() == closestNatural || gas->getStation() == closestMain) {
                workerCount += int(gas->targetedByWhat().size()) * 2;
                resourceCount++;
            }
        }

        auto saturationRatio = clamp(double(workerCount) / double(resourceCount), 0.1, 1.0);

        const auto calculateDefensesNeeded = [&](int defensesDesired) {
            if (Strategy::enemyRush() || Strategy::enemyPressure())
                return defensesDesired - groundCount;
            return int(round(saturationRatio * double(defensesDesired))) - groundCount;
        };

        // Protoss
        if (Broodwar->self()->getRace() == Races::Protoss) {
            auto prepareExpansionDefenses = Util::getTime() < Time(10, 0) && vis(Protoss_Nexus) >= 2 && com(Protoss_Forge) > 0;

            if (Players::vP() && prepareExpansionDefenses && BuildOrder::isWallNat()) {
                auto cannonCount = 2 + int(1 + Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) + Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                return cannonCount - groundCount;
            }

            if (Players::vZ() && BuildOrder::isWallNat()) {
                auto cannonCount = int(com(Protoss_Forge) > 0)
                    + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 6)
                    + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 12)
                    + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 24)
                    + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

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
            if (BuildOrder::isOpener() && BuildOrder::buildCount(Zerg_Spire) > 0 && vis(Zerg_Spire) == 0)
                return 0;

            if (Players::vP()) {

                // 1GateCore
                if (Strategy::getEnemyBuild() == "1GateCore") {
                    if (Strategy::getEnemyTransition() == "Corsair" && Util::getTime() < Time(5, 30))
                        return (Util::getTime() > Time(3, 00)) + (Util::getTime() > Time(5, 00)) - groundCount;
                    else if (Strategy::getEnemyTransition() == "4Gate" && Util::getTime() < Time(8, 00))
                        return (Util::getTime() > Time(3, 00)) + (2 * (Util::getTime() > Time(4, 45))) + (2 * (Util::getTime() > Time(5, 45))) + (2 * (Util::getTime() > Time(6, 15))) - groundCount;
                    else if (Util::getTime() < Time(5, 30))
                        return (Util::getTime() > Time(3, 00)) + (Util::getTime() > Time(5, 00)) + (2 * (Util::getTime() > Time(5, 15))) - groundCount;
                }

                // 2Gate
                if (Strategy::getEnemyBuild() == "2Gate") {
                    if (Strategy::enemyProxy())
                        return 0;

                    else if (Strategy::enemyRush() && Util::getTime() < Time(5, 15))
                        return (2 * (Util::getTime() > Time(3, 00))) + (2 * (Util::getTime() > Time(4, 15))) + (Util::getTime() > Time(4, 45)) - groundCount;

                    else if (Strategy::getEnemyTransition() == "4Gate" && Util::getTime() < Time(7, 00))
                        return (Util::getTime() > Time(3, 00)) + (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 15)) + (Util::getTime() > Time(5, 30)) + (Util::getTime() > Time(5, 45)) - groundCount;

                    else if (Util::getTime() < Time(5, 15))
                        return (Util::getTime() > Time(3, 00)) + (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(3, 45)) + (Util::getTime() > Time(4, 45)) - groundCount;
                }

                // FFE
                if (Strategy::getEnemyBuild() == "FFE") {
                    if (Strategy::getEnemyTransition() == "Carriers")
                        return 0;
                    if (Strategy::getEnemyTransition() == "5GateGoon" && Util::getTime() < Time(10, 30))
                        return calculateDefensesNeeded((2 * (Util::getTime() > Time(5, 00))) + (2 * (Util::getTime() > Time(6, 30))) + (2 * (Util::getTime() > Time(8, 00))) + (2 * (Util::getTime() > Time(9, 30))));
                    if (Strategy::getEnemyTransition() == "NeoBisu" && Util::getTime() < Time(6, 30))
                        return calculateDefensesNeeded((2 * (Util::getTime() > Time(6, 00))));
                    if (Strategy::getEnemyTransition() == "Speedlot" && Util::getTime() < Time(7, 00))
                        return calculateDefensesNeeded((2 * (Util::getTime() > Time(5, 00))) + (2 * (Util::getTime() > Time(5, 30))) + (2 * (Util::getTime() > Time(6, 00))));
                    if (Strategy::getEnemyTransition() == "Unknown" && Util::getTime() < Time(5, 15))
                        return calculateDefensesNeeded((2 * (Util::getTime() > Time(5, 00))));
                }

                // Outside of openers, base it off how large the ground army of enemy is
                const auto divisor = 2.5 + max(0, ((Util::getTime().minutes - 10) / 4));
                const auto count = int((Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) + Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon)) / divisor);
                return calculateDefensesNeeded(count);
            }

            if (Players::vZ() && BuildOrder::getCurrentTransition().find("Muta") != string::npos && (BuildOrder::takeNatural() || int(Stations::getMyStations().size()) >= 2)) {
                if (Strategy::getEnemyTransition() == "2HatchSpeedling")
                    return (Util::getTime() > Time(3, 45)) + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(5, 30)) - groundCount;
                else if (Util::getTime() < Time(6, 00) && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 40)
                    return 6 - groundCount;
                else if (Util::getTime() < Time(6, 00) && (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 || Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 26))
                    return 4 - groundCount;
                else if (Strategy::enemyPressure())
                    return (Util::getTime() > Time(4, 10)) + (vis(Zerg_Sunken_Colony) > 0) + (vis(Zerg_Drone) >= 8 && com(Zerg_Sunken_Colony) >= 2) - groundCount;
                else if (Strategy::enemyRush() && total(Zerg_Zergling) >= 6)
                    return 1 + (vis(Zerg_Sunken_Colony) > 0) + (vis(Zerg_Drone) >= 8 && com(Zerg_Sunken_Colony) >= 2) - groundCount;
            }

            if (Players::vT()) {
                if (Strategy::getEnemyTransition() == "2Fact" || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 || Strategy::getEnemyBuild() == "RaxFact" || Strategy::enemyWalled())
                    return 1 - groundCount;
                if (Strategy::getEnemyBuild() == "2Rax" && !Strategy::enemyFastExpand() && !Strategy::enemyRush() && Util::getTime() < Time(5, 00))
                    return calculateDefensesNeeded(2 * (Util::getTime() > Time(3, 15))) + (2 * (Util::getTime() > Time(4, 30)));
                if (Strategy::enemyProxy())
                    return 0;
                if (Strategy::enemyRush())
                    return calculateDefensesNeeded(2 + (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 30)));
                if (!Strategy::enemyFastExpand() && Strategy::getEnemyBuild() != "RaxCC")
                    return calculateDefensesNeeded((Util::getTime() > Time(3, 45)) + (Util::getTime() > Time(4, 30)));
                if (Util::getTime() > Time(10, 00))
                    return min(1, (Util::getTime().minutes / 4)) - groundCount;
            }
        }
        return 0;
    }

    int needAirDefenses(BWEB::Wall& wall)
    {
        auto airCount = wall.getAirDefenseCount();
        const auto enemyAir = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0
            || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0
            || (Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0 && Util::getTime() > Time(4, 45));

        if (!Terrain::isInAllyTerritory(wall.getArea()))
            return 0;

        // P
        if (Broodwar->self()->getRace() == Races::Protoss) {

        }

        // Z
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (!BuildOrder::isTechUnit(Zerg_Mutalisk) && vis(Zerg_Lair) == 0 && Util::getTime() > Time(5, 00)) {
                if (enemyAir && !Strategy::enemyFastExpand() && Util::getTime() > Time(4, 0))
                    return 1 - airCount;
                if (enemyAir && Util::getTime() > Time(10, 0))
                    return 1 - airCount;
            }
            if (Players::ZvZ() && total(Zerg_Zergling) > Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) && com(Zerg_Spire) == 0 && Util::getTime() > Time(4, 30) && Strategy::getEnemyTransition() == "Unknown" && BuildOrder::getCurrentTransition() == "2HatchMuta")
                return 1 - airCount;
            if (Util::getTime() > Time(4, 15) && Players::ZvZ() && Strategy::getEnemyTransition() == "1HatchMuta" && BuildOrder::getCurrentTransition() == "2HatchMuta")
                return 1 - airCount;
        }
        return 0;
    }

    BWEB::Wall* getMainWall() { return mainWall; }
    BWEB::Wall* getNaturalWall() { return naturalWall; }
}