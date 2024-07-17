#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls {

    namespace {
        const BWEB::Wall * mainWall = nullptr;
        const BWEB::Wall * naturalWall = nullptr;
        vector<vector<UnitType>> testingOrder;
        vector<UnitType> defenses;
        bool tight;
        bool openWall;
        UnitType tightType = None;
        bool wantTerranWalls = false;

        void generateWall(const BWEM::Area* area, const BWEM::ChokePoint* choke)
        {
            for (auto &buildings : testingOrder) {
                string types = "";
                for (auto &building : buildings)
                    types += building.c_str() + string(", ");

                if (!BWEB::Walls::getWall(choke)) {
                    Util::debug(string("[Walls]: Generating wall: ") + types);
                    BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
                    if (!BWEB::Walls::getWall(choke)) {
                        Util::debug(string("[Walls]: Wall failed"));
                    }
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
                defenses ={ Protoss_Photon_Cannon };
                if (Players::vZ()) {
                    tight = false;
                    testingOrder ={ { Protoss_Gateway, Protoss_Forge, Protoss_Pylon } };
                }
                else {
                    tight = false;
                    testingOrder ={ { Protoss_Forge, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon } };
                }
            }

            // Terran wall parameters
            if (Broodwar->self()->getRace() == Races::Terran) {
                tight = false;
                defenses ={ Terran_Missile_Turret };
                testingOrder ={ { Terran_Barracks, Terran_Bunker } };
            }

            // Zerg wall parameters
            if (Broodwar->self()->getRace() == Races::Zerg) {
                tight = false;
                defenses ={ Zerg_Sunken_Colony };

                if (Players::ZvT()) {
                    testingOrder ={ { Zerg_Evolution_Chamber, Zerg_Spire },
                                    { Zerg_Evolution_Chamber },
                                    { Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                    { Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                    {}
                    };
                }
                else {
                    testingOrder ={ { Zerg_Evolution_Chamber, Zerg_Hatchery, Zerg_Evolution_Chamber },
                                    { Zerg_Evolution_Chamber, Zerg_Hatchery },
                                    { Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                    { Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                    {}
                    };
                }
            }
        }

        void findWalls()
        {
            initializeWallParameters();

            // Create an open wall at every natural
            openWall = true;

            // In FFA just make a wall at our natural (if we have one)
            if (Players::vFFA() && Terrain::getMyNatural() && Terrain::getNaturalChoke()) {
                generateWall(Terrain::getMyNatural()->getBase()->GetArea(), Terrain::getNaturalChoke());
            }
            else {
                for (auto &station : BWEB::Stations::getStations()) {
                    initializeWallParameters();
                    if (station.isMain())
                        continue;
                    generateWall(station.getBase()->GetArea(), station.getChokepoint());
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
            // If we are opening 12 hatch or a 2h build, we sometimes need a faster sunken
            auto earlySunk = BuildOrder::getCurrentOpener() == "12Hatch" || BuildOrder::getCurrentTransition().find("2Hatch") != string::npos || Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) >= 1;

            // 1GateCore
            if (Spy::getEnemyBuild() == "1GateCore" || (Spy::getEnemyBuild() == "Unknown" && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) >= 1)) {
                return (Util::getTime() > Time(3, 45))
                    + (Util::getTime() > Time(4, 30));
            }

            // 2Gate
            if (Spy::getEnemyBuild() == "2Gate") {
                if (Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                    return (earlySunk && Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 10))
                    + (Util::getTime() > Time(4, 30));
                if (Spy::getEnemyOpener() == "10/15")
                    return (earlySunk && Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 10))
                    + (Util::getTime() > Time(5, 00));
                if (Spy::getEnemyOpener() == "10/12" || Spy::getEnemyOpener() == "Unknown")
                    return (earlySunk && Util::getTime() > Time(3, 00))
                    + (Util::getTime() > Time(4, 00))
                    + (Util::getTime() > Time(4, 45));
                if (Spy::getEnemyOpener() == "9/9")
                    return (earlySunk && Util::getTime() > Time(2, 50))
                    + (Util::getTime() > Time(4, 30));
                if (Spy::getEnemyOpener() == "Proxy9/9")
                    return (Util::getTime() > Time(2, 50))
                    + (Util::getTime() > Time(4, 30));
            }

            // FFE
            if (Spy::getEnemyBuild() == "FFE") {
                return (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 00));
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
                if (Spy::getEnemyTransition() == "4Gate") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 30))
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(5, 15))
                        + (Util::getTime() > Time(5, 30))
                        + (Util::getTime() > Time(6, 00))
                        + (Util::getTime() > Time(7, 00))
                        + (Util::getTime() > Time(8, 00));
                }

                // DT
                if (Spy::getEnemyTransition() == "DT") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 15))
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(5, 20))
                        + (Util::getTime() > Time(5, 40));
                }

                // Corsair
                if (Spy::getEnemyTransition() == "Corsair") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 15))
                        + (Util::getTime() > Time(7, 00));
                }

                // Speedlot
                if (Spy::getEnemyTransition() == "Speedlot" || noExpandOrTech) {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 15))
                        + (Util::getTime() > Time(4, 45))
                        + (Util::getTime() > Time(5, 15));
                }

                // Zealot flood
                if (Spy::getEnemyTransition() == "ZealotRush") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 20))
                        + (Util::getTime() > Time(4, 40))
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(6, 00))
                        + (Util::getTime() > Time(6, 30))
                        - Spy::enemyProxy();
                }

                // Unknown + Expand + No Tech
                if (noTech && Spy::enemyFastExpand() && Spy::getEnemyTransition() == "Unknown") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 30))
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(8, 00))
                        + (Util::getTime() > Time(8, 30))
                        - Spy::enemyProxy();
                }
            }

            // FFE transitions
            if (Spy::getEnemyBuild() == "FFE") {
                if (Spy::getEnemyTransition() == "5GateGoon")
                    return (Util::getTime() > Time(6, 00))
                    + (Util::getTime() > Time(6, 30))
                    + (Util::getTime() > Time(7, 00))
                    + (Util::getTime() > Time(7, 30))
                    + (Util::getTime() > Time(8, 00));
            }

            return 0;
        }

        int ZvP_Defenses(const BWEB::Wall& wall) {

            // If they're only at home and not proxy, don't make any
            auto delayTime = Spy::getEnemyBuild() == "FFE" ? Time(6, 00) : Time(3, 45);
            if (!Spy::enemyProxy() && Util::getTime() < delayTime) {
                auto closestUnit = Util::getClosestUnit(Position(wall.getChokePoint()->Center()), PlayerState::Enemy, [&](auto &u) {
                    return !u->getType().isBuilding() && !u->getType().isWorker() && !Terrain::inTerritory(PlayerState::Enemy, u->getPosition());
                });
                if (!closestUnit)
                    return 0;
            }

            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Protoss_Zealot)
                + Players::getDeadCount(PlayerState::Enemy, Protoss_Dragoon);

            auto mutaBuild = BuildOrder::getCurrentTransition().find("Muta") != string::npos;
            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected = max(ZvP_Opener(wall), ZvP_Transition(wall));
            auto reduction = max(0, unitsKilled / 8);
            auto minimum = 0;

            // 3h builds make roughly half as many
            if (threeHatch && expected > 1) {
                expected /= 2;
                expected++;
            }

            // Non natural walls are limited to 1 total
            if (!wall.getStation()->isNatural() && expected > 0) {
                expected = 1;
                minimum = 1;
            }

            // Make minimum sunkens if criteria fulfilled
            if (expected > 0)
                minimum = 1;
            if (Spy::getEnemyBuild() != "FFE") {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0
                    || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                    minimum = 2;
            }

            return max(minimum, expected - reduction);
        }

        // ZvT
        int ZvT_Opener(const BWEB::Wall& wall)
        {
            // Proxy
            if (Spy::enemyProxy())
                return (Util::getTime() > Time(3, 30));

            // 2Rax
            if (Spy::getEnemyBuild() == "2Rax") {
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
                    return (Util::getTime() > Time(4, 00));
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
            if (Spy::enemyWalled() && Util::getTime() < Time(5, 00))
                return (Util::getTime() > Time(3, 30));

            // Enemy not expanding, probably something scary coming
            if (!Spy::enemyFastExpand() && !Spy::enemyRush())
                return (Util::getTime() > Time(3, 25))
                + (Util::getTime() > Time(4, 30))
                + (Util::getTime() > Time(5, 00));

            return (Util::getTime() > Time(3, 25));
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
            // 3 Hatch
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 || Spy::getEnemyTransition() == "3HatchSpeedling")
                return 1 + (Util::getTime() > Time(4, 15));
            if (Spy::getEnemyTransition() == "2HatchSpeedling")
                return 1;
            if (Spy::getEnemyOpener() == "12Hatch" || Spy::getEnemyOpener() == "10Hatch")
                return 1;
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
        for (auto &station : BWEB::Stations::getStations()) {
            if (station.isMain() || station.isNatural())
                continue;

            auto defendPos = Stations::getDefendPosition(&station);
            if (defendPos.isValid()) {
                auto defendChoke = Util::getClosestChokepoint(defendPos);
                if (defendChoke && !BWEB::Walls::getWall(defendChoke)) {
                    generateWall(station.getBase()->GetArea(), defendChoke);
                }
            }
        }
    }

    int needGroundDefenses(const BWEB::Wall& wall)
    {
        auto groundCount = wall.getGroundDefenseCount();
        if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()) || BuildOrder::isAllIn())
            return 0;

        // If any defense in the wall is severely damaged, we should build 1 extra
        for (auto &unit : Units::getUnits(PlayerState::Self)) {
            if (unit->isCompleted() && unit->getType().isBuilding() && !unit->isHealthy() && wall.getDefenses().find(unit->getTilePosition()) != wall.getDefenses().end()) {
                groundCount--;
                break;
            }
        }

        // If enemy adds defenses, we can start to cut defenses too
        if (Util::getTime() > Time(4, 00))
            groundCount += Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Creep_Colony)
            + Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony);

        // If they expanded, we can skip a sunk after a delay
        if (Players::ZvP() && Spy::enemyFastExpand() && Spy::getEnemyBuild() != "FFE" && Util::getTime() > Time(4, 30)) {
            static Time now = Util::getTime();
            if (Util::getTime() > now + Time(0, 45) && Util::getTime() < now + Time(1, 45))
                groundCount+=1;
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

        }
        return 0;
    }

    const BWEB::Wall * const getMainWall() { return mainWall; }
    const BWEB::Wall * const getNaturalWall() { return naturalWall; }
}