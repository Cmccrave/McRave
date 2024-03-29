#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls {

    namespace {
        const BWEB::Wall * mainWall = nullptr;
        const BWEB::Wall * naturalWall = nullptr;
        vector<UnitType> buildings;
        vector<UnitType> defenses;
        bool tight;
        bool openWall;
        UnitType tightType = None;
        bool wantTerranWalls = false;

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
                tight = false;
                defenses ={ Terran_Missile_Turret };
                buildings ={ Terran_Barracks, Terran_Bunker };
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
                    if (Broodwar->self()->getRace() != Races::Zerg || buildings.empty())
                        break;

                    if (Broodwar->self()->getRace() == Races::Zerg) {
                        UnitType lastBuilding = *buildings.rbegin();
                        buildings.pop_back();
                        if (lastBuilding == Zerg_Hatchery)
                            buildings.push_back(Zerg_Evolution_Chamber);
                    }
                }
            };

            // Create an open wall at every natural
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

            naturalWall = BWEB::Walls::getWall(Terrain::getNaturalChoke());
            mainWall = BWEB::Walls::getWall(Terrain::getMainChoke());
        }

        // PvP
        int PvP_Defenses(const BWEB::Wall& wall)
        {
            return 0;
        }

        // PvZ
        int PvZ_Opener(const BWEB::Wall& wall)
        {
            if (Spy::getEnemyOpener() == "4Pool")
                return 2 + (Players::getSupply(PlayerState::Self, Races::Protoss) >= 24);
            return 0;
        }

        int PvZ_Transition(const BWEB::Wall& wall)
        {
            auto cannonCount = int(com(Protoss_Forge) > 0)
                + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 6)
                + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 12)
                + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 24)
                + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

            if (Spy::getEnemyTransition() == "2HatchHydra")
                return 5;
            else if (Spy::getEnemyTransition() == "3HatchHydra")
                return 4;
            else if (Spy::getEnemyTransition() == "2HatchMuta" && Util::getTime() > Time(4, 0))
                return 3;
            else if (Spy::getEnemyTransition() == "3HatchMuta" && Util::getTime() > Time(5, 0))
                return 3;
            return 0;
        }

        int PvZ_Defenses(const BWEB::Wall& wall)
        {
            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Zerg_Hydralisk) / 2
                + Players::getDeadCount(PlayerState::Enemy, Zerg_Zergling) / 4;

            auto minimum = 0;
            auto expected = max(PvZ_Opener(wall), PvZ_Transition(wall));
            auto reduction = max(0, unitsKilled / 8);

            if (expected > 0)
                minimum = 1;

            return max(minimum, expected - reduction);
        }

        // ZvP
        int ZvP_Opener(const BWEB::Wall& wall)
        {
            // If we are opening 12 hatch, we sometimes need a faster sunken
            auto greedyStart = BuildOrder::getCurrentOpener() == "12Hatch";

            // 1GateCore
            if (Spy::getEnemyBuild() == "1GateCore" || (Spy::getEnemyBuild() == "Unknown" && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) >= 1)) {
                return (Util::getTime() > Time(4, 30))
                    + (Util::getTime() > Time(5, 00));
            }

            // 2Gate
            if (Spy::getEnemyBuild() == "2Gate" && Util::getTime() < Time(5, 30) && !Spy::enemyProxy()) {
                if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                    return (Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 10))
                    + (Util::getTime() > Time(4, 40));
                if (Spy::getEnemyOpener() == "10/15")
                    return (Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 30))
                    + (Util::getTime() > Time(5, 00));
                if (Spy::getEnemyOpener() == "10/12" || Spy::getEnemyOpener() == "Unknown")
                    return (Util::getTime() > Time(3, 00))
                    + (Util::getTime() > Time(4, 00))
                    + (Util::getTime() > Time(4, 45));
                if (Spy::getEnemyOpener() == "9/9")
                    return (Util::getTime() > Time(2, 50))
                    + (greedyStart && Util::getTime() > Time(2, 50))
                    + (Util::getTime() > Time(4, 30));
            }

            // FFE
            if (Spy::getEnemyBuild() == "FFE") {
                if (BuildOrder::getCurrentTransition() == "6HatchHydra")
                    return 2 * (Util::getTime() > Time(6, 45));
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

        int ZvP_Transition(const BWEB::Wall& wall)
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
                        + (noExpand && Util::getTime() > Time(4, 20))
                        + (noExpand && Util::getTime() > Time(4, 40))
                        + (noExpand && Util::getTime() > Time(5, 00))
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

        int ZvP_Defenses(const BWEB::Wall& wall) {

            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Protoss_Zealot)
                + Players::getDeadCount(PlayerState::Enemy, Protoss_Dragoon);

            // Make at least one sunken once if below criteria fulfilled
            auto minimum = 0;
            if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2)
                minimum = 1;

            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected = max(ZvP_Opener(wall), ZvP_Transition(wall));
            auto reduction = max(0, unitsKilled / 4);

            // Kind of hacky solution to build less with 3h
            if (threeHatch && expected > 1)
                expected /= 2.0;
            if (expected > 0)
                minimum = 1;

            return max(minimum, expected - reduction);
        }

        // ZvT
        int ZvT_Opener(const BWEB::Wall& wall)
        {
            // 2Rax
            if (Spy::getEnemyBuild() == "2Rax") {
                if (Spy::enemyProxy())
                    return (Util::getTime() > Time(4, 30));
                if (!Spy::enemyFastExpand() && !Spy::enemyRush())
                    return (Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 30))
                    + (Util::getTime() > Time(5, 00));
                if (Spy::enemyRush() || Spy::getEnemyOpener() == "BBS")
                    return (Util::getTime() > Time(2, 50))
                    + (Util::getTime() > Time(4, 30));
            }

            // RaxCC
            if (Spy::getEnemyBuild() == "RaxCC") {
                if (Spy::getEnemyOpener() == "8Rax")
                    return (Util::getTime() > Time(4, 30));
                if (Spy::enemyProxy())
                    return (Util::getTime() > Time(4, 30));
                return (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 00));
            }

            // RaxFact
            if (Spy::getEnemyBuild() == "RaxFact") {
                if (Spy::getEnemyTransition() == "2Fact" || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 || Spy::enemyWalled())
                    return (Util::getTime() > Time(3, 30));
                if (Spy::getEnemyOpener() == "8Rax")
                    return (Util::getTime() > Time(4, 30));
            }

            // Enemy walled, which delays any push they have
            if (Spy::enemyWalled() && Util::getTime() < Time(4, 30))
                return (Util::getTime() > Time(4, 00));

            return (Util::getTime() > Time(3, 45));
        }

        int ZvT_Transition(const BWEB::Wall& wall)
        {
            // See if they expanded or got some tech at a reasonable point for 1 base play
            auto noExpand = !Spy::enemyFastExpand();
            auto noTech = Spy::getEnemyTransition() == "Unknown"
                && Players::getTotalCount(PlayerState::Enemy, Terran_Science_Vessel) == 0
                && Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) == 0
                && Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) == 0
                && Players::getTotalCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) == 0;
            auto noExpandOrTech = noExpand && noTech;
            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto firstHatchNeeded = !threeHatch || BuildOrder::getCurrentOpener() == "12Hatch";

            if (Spy::getEnemyTransition() == "3FactGoliath")
                return 3 * (Util::getTime() > Time(8, 00));

            if (Spy::getEnemyTransition() == "2PortWraith")
                return 1 * (Util::getTime() > Time(5, 30));

            if (Spy::getEnemyBuild() == "2Rax" || Spy::getEnemyBuild() == "RaxCC") {
                if (Spy::getEnemyTransition() == "Academy" || !Spy::enemyFastExpand())
                    return 2 * (Util::getTime() > Time(4, 30));
            }

            return 0;
        }

        int ZvT_Defenses(const BWEB::Wall& wall)
        {
            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Terran_Marine)
                + Players::getDeadCount(PlayerState::Enemy, Terran_Firebat)
                + Players::getDeadCount(PlayerState::Enemy, Terran_Medic);

            auto minimum = Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 ? 1 : 0;

            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected = max(ZvT_Opener(wall), ZvT_Transition(wall));
            auto reduction = max(0, unitsKilled / 8);

            // Kind of hacky solution to build less with 3h
            if (threeHatch && expected > 1)
                expected /= 2;
            if (expected > 0)
                minimum = 1;

            return max(minimum, expected - reduction);
        }

        // ZvZ
        int ZvZ_Defenses(const BWEB::Wall& wall) {
            return 0;
        }

        // ZvFFA
        int ZvFFA_Defenses(const BWEB::Wall& wall) {
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

    int needGroundDefenses(const BWEB::Wall& wall)
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
            if (Players::PvP())
                return PvP_Defenses(wall) - groundCount;
            if (Players::PvZ())
                return PvZ_Defenses(wall) - groundCount;
        }

        // Terran

        // Zerg
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Players::ZvP())
                return ZvP_Defenses(wall) - groundCount;
            if (Players::ZvT())
                return ZvT_Defenses(wall) - groundCount;
            if (Players::ZvZ())
                return ZvZ_Defenses(wall) - groundCount;
            if (Players::ZvFFA())
                return ZvFFA_Defenses(wall) - groundCount;
        }
        return 0;
    }

    int needAirDefenses(const BWEB::Wall& wall)
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

    const BWEB::Wall * const getMainWall() { return mainWall; }
    const BWEB::Wall * const getNaturalWall() { return naturalWall; }
}