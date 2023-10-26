#include "Main/McRave.h"

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
        constexpr bool wantTerranWalls = false;

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
                    tight = false;
                    buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
                    defenses.insert(defenses.end(), 10, Protoss_Photon_Cannon);
                }
                else {
                    tight = false;
                    buildings ={ Protoss_Forge, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon };
                    defenses.insert(defenses.end(), 10, Protoss_Photon_Cannon);
                }
            }

            // Terran wall parameters
            if (Broodwar->self()->getRace() == Races::Terran) {
                tight = true;
                buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };
            }

            // Zerg wall parameters
            if (Broodwar->self()->getRace() == Races::Zerg) {
                tight = false;
                defenses ={ Zerg_Sunken_Colony };
                buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber };
            }
        }

        void findWalls()
        {
            initializeWallParameters();

            // Create a wall and reduce the building count on each iteration
            const auto genWall = [&](auto buildings, auto area, auto choke) {
                while (!BWEB::Walls::getWall(choke)) {
                    BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
                    if (Players::PvZ() || buildings.empty())
                        break;

                    UnitType lastBuilding = *buildings.rbegin();
                    buildings.pop_back();
                    if (lastBuilding == Zerg_Hatchery)
                        buildings.push_back(Zerg_Evolution_Chamber);
                }
            };

            // Create a Zerg/Protoss wall at every natural
            if (Broodwar->self()->getRace() != Races::Terran) {
                openWall = true;

                // In FFA just make a wall at our natural (if we have one)
                if (Players::vFFA() && Terrain::getMyNatural() && Terrain::getNaturalChoke()) {
                    genWall(buildings, Terrain::getMyNatural()->getBase()->GetArea(), Terrain::getNaturalChoke());
                }
                else {
                    for (auto &station : BWEB::Stations::getStations()) {
                        initializeWallParameters();
                        if (!station.isNatural())
                            continue;
                        genWall(buildings, station.getBase()->GetArea(), station.getChokepoint());
                    }
                }
            }

            // Create a Terran wall at every main
            else if (wantTerranWalls) {
                for (auto &station : BWEB::Stations::getStations()) {
                    if (!station.isMain())
                        continue;
                    BWEB::Walls::createWall(buildings, station.getBase()->GetArea(), station.getChokepoint(), tightType, defenses, openWall, tight);
                }
            }

            naturalWall = BWEB::Walls::getWall(Terrain::getNaturalChoke());
            mainWall = BWEB::Walls::getWall(Terrain::getMainChoke());
        }

        int ZvPOpener(BWEB::Wall& wall)
        {
            // If we are opening 12 hatch, we sometimes need a faster sunken
            auto firstHatchNeeded = BuildOrder::getCurrentOpener() == "12Hatch";

            // 1GateCore
            if (Spy::getEnemyBuild() == "1GateCore" || (Spy::getEnemyBuild() == "Unknown" && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) >= 1)) {
                return (firstHatchNeeded && Util::getTime() > Time(3, 45))
                    + (Util::getTime() > Time(4, 30))
                    + (Util::getTime() > Time(5, 00));
            }

            // 2Gate
            if (Spy::getEnemyBuild() == "2Gate" && Util::getTime() < Time(5, 30) && !Spy::enemyProxy()) {
                if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                    return (Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 10))
                    + (Util::getTime() > Time(4, 40));
                if (Spy::getEnemyOpener() == "10/15")
                    return (firstHatchNeeded && Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 30))
                    + (Util::getTime() > Time(5, 00));
                if (Spy::getEnemyOpener() == "10/12" || Spy::getEnemyOpener() == "Unknown")
                    return (firstHatchNeeded && Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 00))
                    + (Util::getTime() > Time(4, 45));
                if (Spy::getEnemyOpener() == "9/9")
                    return (firstHatchNeeded && Util::getTime() > Time(3, 00))
                    + (Util::getTime() > Time(4, 30));
            }

            // FFE
            if (Spy::getEnemyBuild() == "FFE") {
                if (BuildOrder::getCurrentTransition() == "6HatchHydra")
                    return 2 * (Util::getTime() > Time(5, 45));
                if (Spy::getEnemyTransition() == "Carriers")
                    return 0;
                if (Spy::getEnemyTransition() == "NeoBisu" && Util::getTime() < Time(6, 30))
                    return ((2 * (Util::getTime() > Time(6, 00))));
                if (Spy::getEnemyTransition() == "Speedlot" && Util::getTime() < Time(7, 00))
                    return (2 * (Util::getTime() > Time(6, 00)))
                    + (2 * (Util::getTime() > Time(6, 30)))
                    + (2 * (Util::getTime() > Time(7, 00)));
                if (Spy::getEnemyTransition() == "Unknown" && Util::getTime() < Time(5, 15))
                    return (Util::getTime() > Time(6, 00));
                if (Util::getTime() < Time(8, 00))
                    return (Util::getTime() > Time(5, 30))
                    + (Util::getTime() > Time(6, 15))
                    + (Util::getTime() > Time(6, 45));
                if (Spy::getEnemyTransition() == "5GateGoon" && Util::getTime() < Time(10, 00))
                    return (Util::getTime() > Time(5, 40))
                    + (Util::getTime() > Time(6, 00))
                    + (Util::getTime() > Time(6, 20))
                    + (Util::getTime() > Time(6, 40))
                    + (Util::getTime() > Time(7, 00))
                    + (Util::getTime() > Time(8, 00))
                    + (Util::getTime() > Time(9, 00));
                if (Spy::getEnemyTransition() == "CorsairGoon" && Util::getTime() < Time(10, 00))
                    return (Util::getTime() > Time(5, 30))
                    + (Util::getTime() > Time(6, 15))
                    + (Util::getTime() > Time(6, 45))
                    + 2 * (Util::getTime() > Time(7, 15));
                return 0;
            }

            // Always make one that is a safety measure vs unknown builds
            return Util::getTime() > Time(3, 45);
        }

        int ZvPTransition(BWEB::Wall& wall)
        {
            // See if they expanded or got some tech at a reasonable point for 1 base play
            auto noExpand = !Spy::enemyFastExpand();
            auto noTech = (Spy::getEnemyTransition() == "Unknown" || Spy::getEnemyTransition() == "ZealotRush")
                && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) == 0
                && Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) == 0
                && Players::getTotalCount(PlayerState::Enemy, Protoss_High_Templar) == 0
                && Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) == 0
                && Players::getTotalCount(PlayerState::Enemy, Protoss_Archon) == 0;
            auto noExpandOrTech = noExpand && noTech;

            // 1 base transitions
            if (Spy::getEnemyBuild() == "2Gate" || Spy::getEnemyBuild() == "1GateCore") {

                // 4Gate
                if (Spy::getEnemyTransition() == "4Gate" && Util::getTime() < Time(9, 00)) {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 10))
                        + (noExpand && Util::getTime() > Time(4, 40))
                        + (noExpand && Util::getTime() > Time(5, 10))
                        + (noExpand && Util::getTime() > Time(5, 40))
                        + (noExpand && Util::getTime() > Time(6, 10))
                        + (noExpand && Util::getTime() > Time(6, 40))
                        + (noExpand && Util::getTime() > Time(7, 10))
                        + (noExpand && Util::getTime() > Time(7, 40));
                }

                // DT
                if (Spy::getEnemyTransition() == "DT") {
                    return (Util::getTime() > Time(4, 00))
                        + (noExpand && Util::getTime() > Time(4, 15))
                        + (noExpand && Util::getTime() > Time(5, 00))
                        + (noExpand && Util::getTime() > Time(5, 20))
                        + (noExpand && Util::getTime() > Time(5, 40));
                }

                // Corsair
                if (Spy::getEnemyTransition() == "Corsair") {
                    return (Util::getTime() > Time(4, 00))
                        + (noExpand && Util::getTime() > Time(4, 30))
                        + (noExpand && Util::getTime() > Time(7, 00));
                }

                // Speedlot
                if (Spy::getEnemyTransition() == "Speedlot") {
                    return (Util::getTime() > Time(4, 00))
                        + (noExpand && Util::getTime() > Time(4, 00))
                        + (noExpand && Util::getTime() > Time(4, 30))
                        + (noExpand && Util::getTime() > Time(5, 00))
                        + (noExpand && Util::getTime() > Time(5, 30));
                }

                // Zealot flood
                if (Spy::getEnemyTransition() == "ZealotRush") {
                    return (Util::getTime() > Time(4, 00))
                        + (noExpand && Util::getTime() > Time(4, 00))
                        + (noExpand && Util::getTime() > Time(4, 30))
                        + (noExpand && Util::getTime() > Time(5, 00))
                        + (noExpand && Util::getTime() > Time(5, 30))
                        + (noExpand && Util::getTime() > Time(6, 00))
                        + (noExpand && Util::getTime() > Time(6, 30));
                }

                // Unknown + Expand + No Tech
                if (noTech && Spy::enemyFastExpand() && Spy::getEnemyTransition() == "Unknown") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 30))
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(8, 00))
                        + (Util::getTime() > Time(8, 30));
                }

                // Unknown + No Expand + No Tech
                if (noExpandOrTech) {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 30))
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(5, 20))
                        + (Util::getTime() > Time(5, 40))
                        + (Util::getTime() > Time(6, 00));
                }
            }
            return 0;
        }

        int ZvTOpener(BWEB::Wall& wall)
        {
            // If we are opening 12 hatch into a 2h tech, we sometimes need a faster sunken
            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto greedyStart = !threeHatch || BuildOrder::getCurrentOpener() == "12Hatch" || BuildOrder::getCurrentOpener() == "12Pool";

            // 2Rax
            if (Spy::getEnemyBuild() == "2Rax") {
                if (Spy::enemyProxy())
                    return 0;
                if (!Spy::enemyFastExpand() && !Spy::enemyRush() && Util::getTime() < Time(5, 00))
                    return greedyStart
                    + (!threeHatch && Util::getTime() > Time(3, 15))
                    + (!threeHatch && Util::getTime() > Time(4, 30));
                if (Spy::enemyRush())
                    return (greedyStart && Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(3, 15))
                    + (!threeHatch && Util::getTime() > Time(4, 30));
            }

            // RaxCC
            if (Spy::getEnemyBuild() == "RaxCC") {
                if (Spy::getEnemyOpener() == "8Rax")
                    return 1;
                if (Spy::enemyProxy())
                    return 0;
                return (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(4, 45));
            }

            // RaxFact
            if (Spy::getEnemyBuild() == "RaxFact") {
                if (Spy::getEnemyTransition() == "2Fact" || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 || Spy::enemyWalled())
                    return (Util::getTime() > Time(3, 30));
            }

            // Enemy walled, which delays any push they have
            if (Spy::enemyWalled() && Util::getTime() < Time(4, 30))
                return (Util::getTime() > Time(4, 00));

            return (Util::getTime() > Time(3, 45));
        }

        int ZvTTransition(BWEB::Wall& wall)
        {
            // See if they expanded or got some tech at a reasonable point for 1 base play
            auto noExpand = !Spy::enemyFastExpand();
            auto noTech = Spy::getEnemyTransition() == "Unknown"
                && Players::getTotalCount(PlayerState::Enemy, Terran_Science_Vessel) == 0
                && Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) == 0
                && Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) == 0
                && Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) == 0;
            auto noExpandOrTech = noExpand && noTech;
            auto threeHatch = BuildOrder::getCurrentTransition().find("3Hatch") != string::npos;
            auto firstHatchNeeded = !threeHatch || BuildOrder::getCurrentOpener() == "12Hatch";

            if (Spy::getEnemyTransition() == "5FacGoliath")
                return 5 * (Util::getTime() > Time(11, 00));

            if (Spy::getEnemyTransition() == "3FacGoliath")
                return 3 * (Util::getTime() > Time(8, 00));

            if (Spy::getEnemyTransition() == "2PortWraith")
                return 2 * (Util::getTime() > Time(5, 30));

            if (Spy::getEnemyTransition() == "Academy")
                return 2 * (Util::getTime() > Time(4, 30));

            return 0;
        }

        int groundDefensesNeededZvP(BWEB::Wall& wall) {

            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Protoss_Zealot)
                + Players::getDeadCount(PlayerState::Enemy, Protoss_Dragoon);

            // Make at least one sunken once if below criteria fulfilled
            auto minimum = 0;
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2
                || Players::getTotalCount(PlayerState::Self, Zerg_Drone) >= 30)
                minimum = 1;

            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected = max(ZvPOpener(wall), ZvPTransition(wall)) - max(0, unitsKilled / 4);

            // Kind of hacky solution to build less with 3h
            if (threeHatch && expected > 1)
                expected /= 2;

            return max(minimum, expected);
        }

        int groundDefensesNeededZvT(BWEB::Wall& wall)
        {
            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Terran_Marine)
                + Players::getDeadCount(PlayerState::Enemy, Terran_Firebat)
                + Players::getDeadCount(PlayerState::Enemy, Terran_Medic);

            auto minimum = Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 ? 1 : 0;
            auto expected = max(ZvTOpener(wall), ZvTTransition(wall)) - max(0, unitsKilled / 8);

            return max(minimum, expected);
        }

        int groundDefensesNeededZvZ(BWEB::Wall& wall) {
            return 0;
        }

        int groundDefensesNeededZvFFA(BWEB::Wall& wall) {
            return 1
                + (Util::getTime() > Time(5, 20))
                + (Util::getTime() > Time(5, 40));
        }
    }

    void onStart()
    {
        initializeWallParameters();
        findWalls();
    }

    void onFrame()
    {

    }

    int needGroundDefenses(BWEB::Wall& wall)
    {
        auto groundCount = wall.getGroundDefenseCount();
        if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()))
            return 0;

        // If any defense in the wall is severely damaged, we should build 1 extra
        for (auto &unit : Units::getUnits(PlayerState::Self)) {
            if (unit->isCompleted() && unit->getType().isBuilding() && !unit->isHealthy() && wall.getDefenses().find(unit->getTilePosition()) != wall.getDefenses().end()) {
                groundCount--;
                break;
            }
        }

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
                if (Spy::getEnemyTransition() == "2HatchHydra")
                    return 5 - groundCount;
                else if (Spy::getEnemyTransition() == "3HatchHydra")
                    return 4 - groundCount;
                else if (Spy::getEnemyTransition() == "2HatchMuta" && Util::getTime() > Time(4, 0))
                    return 3 - groundCount;
                else if (Spy::getEnemyTransition() == "3HatchMuta" && Util::getTime() > Time(5, 0))
                    return 3 - groundCount;
                else if (Spy::getEnemyOpener() == "4Pool")
                    return 2 + (Players::getSupply(PlayerState::Self, Races::Protoss) >= 24) - groundCount;
                return cannonCount - groundCount;
            }
        }

        // Terran

        // Zerg
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Players::ZvP())
                return groundDefensesNeededZvP(wall) - groundCount;
            if (Players::ZvT())
                return groundDefensesNeededZvT(wall) - groundCount;
            if (Players::ZvZ())
                return groundDefensesNeededZvZ(wall) - groundCount;
            if (Players::ZvFFA())
                return groundDefensesNeededZvFFA(wall) - groundCount;
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

        if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()))
            return 0;

        // Protoss
        if (Broodwar->self()->getRace() == Races::Protoss) {
            return 0;
        }

        // Terran

        // Zerg
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Players::ZvZ() && Util::getTime() > Time(4, 30) && total(Zerg_Zergling) > Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) && com(Zerg_Spire) == 0 && Spy::getEnemyTransition() == "Unknown" && BuildOrder::getCurrentTransition() == "2HatchMuta")
                return 1 - airCount;
            if (Players::ZvZ() && Util::getTime() > Time(4, 15) && Spy::getEnemyTransition() == "1HatchMuta" && BuildOrder::getCurrentTransition() == "2HatchMuta")
                return 1 - airCount;
        }
        return 0;
    }

    BWEB::Wall* getMainWall() { return mainWall; }
    BWEB::Wall* getNaturalWall() { return naturalWall; }
}