#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls {

    namespace {
        const BWEB::Wall * mainWall = nullptr;
        const BWEB::Wall * naturalWall = nullptr;
        vector<vector<UnitType>> naturaltestingOrder;
        vector<vector<UnitType>> thirdTestingOrder;
        vector<UnitType> defenses;
        bool tight;
        bool openWall;
        UnitType tightType = None;
        bool wantTerranWalls = false;

        void generateWall(const BWEB::Station * const station, const BWEM::ChokePoint* choke)
        {
            auto area = station->getBase()->GetArea();

            // HACK: Zerg walls in the main are just necessary on pocket natural maps, we don't want extra buildings
            if (Broodwar->self()->getRace() == Races::Zerg && area == Terrain::getMainArea()) {
                vector<UnitType> hatch ={ Zerg_Hatchery };
                BWEB::Walls::createWall(hatch, area, choke, tightType, defenses, openWall, true);
                return;
            }

            auto &list = station->isNatural() ? naturaltestingOrder : thirdTestingOrder;

            for (auto &buildings : list) {
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
                    naturaltestingOrder ={ { Protoss_Gateway, Protoss_Forge, Protoss_Pylon } };
                }
                else {
                    tight = false;
                    naturaltestingOrder ={ { Protoss_Forge, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon } };
                }
            }

            // Terran wall parameters
            if (Broodwar->self()->getRace() == Races::Terran) {
                tight = false;
                defenses ={ Terran_Missile_Turret };
                thirdTestingOrder ={ { Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot },
                                     { Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot },
                                     { Terran_Supply_Depot, Terran_Supply_Depot }
                };

                naturaltestingOrder ={ { Terran_Barracks, Terran_Supply_Depot } };
            }

            // Zerg wall parameters
            if (Broodwar->self()->getRace() == Races::Zerg) {
                tight = false;
                defenses ={ Zerg_Sunken_Colony };

                if (Players::ZvT()) {
                    naturaltestingOrder ={ { Zerg_Evolution_Chamber, Zerg_Spire },
                                           { Zerg_Evolution_Chamber },
                                           { Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                           { Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                           {}
                    };
                }
                else {
                    naturaltestingOrder ={ { Zerg_Evolution_Chamber, Zerg_Hatchery },
                                           { Zerg_Hatchery, Zerg_Evolution_Chamber},
                                           { Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                           {}
                    };
                }
                thirdTestingOrder = naturaltestingOrder;
            }
        }

        void findWalls()
        {
            // Create an open wall at every natural
            openWall = true;

            // In FFA just make a wall at our natural (if we have one)
            if (Players::vFFA() && Terrain::getMyNatural() && Terrain::getNaturalChoke()) {
                generateWall(Terrain::getMyNatural(), Terrain::getNaturalChoke());
            }
            else {
                for (auto &station : BWEB::Stations::getStations()) {
                    if ((station.isMain() && !Terrain::isPocketNatural()) || (station.isNatural() && Terrain::isPocketNatural()))
                        continue;
                    generateWall(&station, station.getChokepoint());
                }
            }

            naturalWall = BWEB::Walls::getWall(Terrain::getNaturalChoke());
            mainWall = BWEB::Walls::getWall(Terrain::getMainChoke());
        }

        // PvP
        int PvP_Defenses(const BWEB::Wall& wall)
        {
            if (BuildOrder::getCurrentTransition() == "DT")
                return 3;
            return 0;
        }

        // PvZ
        int PvZ_Opener(const BWEB::Wall& wall)
        {
            if (Spy::getEnemyOpener() == "4Pool")
                return 2 + (Players::getSupply(PlayerState::Self, Races::Protoss) >= 24);
            if (Spy::getEnemyOpener() == "9Pool" || Spy::getEnemyOpener() == "OverPool")
                return 2;
            return total(Protoss_Gateway) > 0;
        }

        int PvZ_Transition(const BWEB::Wall& wall)
        {
            auto cannonCount = 0
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
            return cannonCount;
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
            // 1GateCore
            if (Spy::getEnemyBuild() == "1GateCore" || (Spy::getEnemyBuild() == "Unknown" && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) >= 1)) {
                return (Util::getTime() > Time(3, 45))
                    + (Util::getTime() > Time(4, 30));
            }

            // 2Gate
            if (Spy::getEnemyBuild() == "2Gate") {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                    return (Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 10))
                    + (Util::getTime() > Time(4, 30));
                if (Spy::getEnemyOpener() == "10/15")
                    return (Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 10))
                    + (Util::getTime() > Time(5, 00));
                if (Spy::getEnemyOpener() == "10/12" || Spy::getEnemyOpener() == "Unknown")
                    return 1
                    + (Util::getTime() > Time(4, 00))
                    + (Util::getTime() > Time(4, 45));
                if (Spy::getEnemyOpener() == "9/9")
                    return 1
                    + (Util::getTime() > Time(4, 30));
                if (Spy::getEnemyOpener() == "Proxy9/9")
                    return 0;
            }

            // FFE
            if (Spy::getEnemyBuild() == "FFE") {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2)
                    return 2;
                return Util::getTime() > Time(5, 00);
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
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(5, 20))
                        + (Util::getTime() > Time(5, 40));
                }

                // Corsair
                if (Spy::getEnemyTransition() == "Corsair") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 15))
                        + (Util::getTime() > Time(5, 15))
                        + (Util::getTime() > Time(7, 00));
                }

                // Speedlot
                if (Spy::getEnemyTransition() == "Speedlot") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 15))
                        + (Util::getTime() > Time(4, 45))
                        + (Util::getTime() > Time(5, 15));
                }

                // Zealot flood
                if (Spy::getEnemyTransition() == "ZealotRush") {
                    return (Util::getTime() > Time(3, 40))
                        + (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 20))
                        + (Util::getTime() > Time(4, 40))
                        + (Util::getTime() > Time(5, 00))
                        + (Util::getTime() > Time(5, 20))
                        + (Util::getTime() > Time(5, 40))
                        + (Util::getTime() > Time(6, 00))
                        - Spy::enemyProxy();
                }

                // Unknown + NoExpand + NoTech
                if (noExpandOrTech) {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 15))
                        + (Util::getTime() > Time(4, 45))
                        + (Util::getTime() > Time(5, 15));
                }

                // Unknown + Expand + NoTech
                if (noTech && Spy::enemyFastExpand() && Spy::getEnemyTransition() == "Unknown") {
                    return (Util::getTime() > Time(4, 00))
                        + (Util::getTime() > Time(4, 30))
                        + (Util::getTime() > Time(8, 00))
                        + (Util::getTime() > Time(8, 30))
                        - Spy::enemyProxy();
                }
            }

            // FFE transitions
            if (Spy::getEnemyBuild() == "FFE") {
                if (Spy::getEnemyTransition() == "5GateGoon")
                    return  (Util::getTime() > Time(6, 00))
                    + (Util::getTime() > Time(6, 30))
                    + (Util::getTime() > Time(8, 00));

                if (Spy::getEnemyTransition() == "CorsairGoon")
                    return (Util::getTime() > Time(7, 00));

                if (Spy::getEnemyTransition() == "Speedlot")
                    return (Util::getTime() > Time(6, 30))
                    + (Util::getTime() > Time(7, 00))
                    + (Util::getTime() > Time(8, 00));

                if (Spy::getEnemyTransition() == "Sairlot")
                    return 2 * (Util::getTime() > Time(6, 50));

                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) == 0)
                    return (Util::getTime() > Time(6, 00));

                return (Util::getTime() > Time(7, 15));
            }

            return 0;
        }

        int ZvP_Defenses(const BWEB::Wall& wall) 
        {
            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Protoss_Zealot)
                + Players::getDeadCount(PlayerState::Enemy, Protoss_Dragoon);
            auto buildingsKilled = Players::getDeadCount(PlayerState::Enemy, Protoss_Gateway);

            auto mutaBuild = BuildOrder::getCurrentTransition().find("Muta") != string::npos;
            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected = max(ZvP_Opener(wall), ZvP_Transition(wall));
            auto reduction = (unitsKilled / 8) + buildingsKilled;
            auto minimum = int(expected > 0);

            // 3h builds make roughly half as many
            if (threeHatch && expected > 1 && Spy::getEnemyBuild() != "FFE") {
                expected = int(floor(double(expected) / 2.0));
            }

            // Non natural walls are limited to 1 total
            if (!wall.getStation()->isNatural() && expected > 0 && Spy::getEnemyBuild() != "FFE") {
                expected = 1;
                minimum = 1;
            }

            // Make minimum sunkens if criteria fulfilled
            if (expected > 0)
                minimum = 1;
            if (Spy::getEnemyBuild() != "FFE") {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0
                    || Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Singularity_Charge, 1)
                    || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 4)
                    minimum = 2;
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0)
                    expected--;
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

                if (Spy::enemyRush() || Spy::getEnemyOpener() == "BBS")
                    return (Util::getTime() > Time(2, 50))
                    + (Util::getTime() > Time(4, 30));

                if (Spy::getEnemyOpener() == "11/13")
                    return (Util::getTime() > Time(3, 30));

                if (Spy::getEnemyOpener() == "11/18")
                    return (Util::getTime() > Time(4, 00));

                if (!Spy::enemyFastExpand())
                    return (Util::getTime() > Time(3, 15))
                    + (Util::getTime() > Time(4, 00))
                    + (Util::getTime() > Time(4, 45));
            }

            // RaxCC
            if (Spy::getEnemyBuild() == "RaxCC") {
                if (Spy::getEnemyOpener() == "8Rax")
                    return (Util::getTime() > Time(4, 00));
                return (Util::getTime() > Time(5, 00));
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
                return (Util::getTime() > Time(3, 45))
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
 
            if (Spy::getEnemyTransition() == "WorkerRush") {
                return 1
                    + (Util::getTime() > Time(4, 00));
            }

            if (Spy::getEnemyBuild() == "2Rax") {

                // Upwards of 4 sunkens vs marine flood
                if (Spy::getEnemyTransition() == "MarineRush")
                    return 2
                    + (Util::getTime() > Time(4, 30))
                    + (Util::getTime() > Time(5, 00));

                // Need 3 sunkens to defend against ~5:00 timing
                if (Spy::getEnemyTransition() == "Academy" || !Spy::enemyFastExpand())
                    return 3 * (Util::getTime() > Time(4, 30));
            }

            if (Spy::getEnemyBuild() == "RaxFact") {

                // Need 3 sunkens to defend against 7:30 timing
                if (Spy::getEnemyTransition() == "3FactGoliath")
                    return 1
                    + (Util::getTime() > Time(6, 00))
                    + (Util::getTime() > Time(6, 30));

                // Need 1 sunken to defend vulture threat
                if (Spy::getEnemyTransition() == "2PortWraith")
                    return (Util::getTime() > Time(5, 30));

                // Otherwise throw down 2 if they haven't expanded by 6:00
                if (!Spy::enemyFastExpand())
                    return 2 * (Util::getTime() > Time(6, 00));
            }

            return 0;
        }

        int ZvT_Defenses(const BWEB::Wall& wall)
        {
            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Terran_Marine)
                + Players::getDeadCount(PlayerState::Enemy, Terran_Firebat)
                + Players::getDeadCount(PlayerState::Enemy, Terran_Medic);
            auto buildingsKilled = Players::getDeadCount(PlayerState::Enemy, Terran_Barracks);

            auto minimum = Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 ? 1 : 0;

            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected = max(ZvT_Opener(wall), ZvT_Transition(wall));
            auto reduction = (unitsKilled / 8) + buildingsKilled;

            if (expected > 0)
                minimum = 1;

            return max(minimum, expected - reduction);
        }

        // ZvZ
        int ZvZ_Defenses(const BWEB::Wall& wall) {
            // 3 Hatch
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 || Spy::getEnemyTransition() == "3HatchSpeedling")
                return 1 + (Util::getTime() > Time(4, 15));
            if (Spy::getEnemyTransition() == "2HatchSpeedling" || (Stations::getStations(PlayerState::Enemy).size() <= 1 && Players::getTotalCount(PlayerState::Enemy, Zerg_Hatchery) >= 2))
                return 1 + (Util::getTime() > Time(4, 45));
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
        static vector<const BWEB::Station *> stationsTried;

        for (auto &station : BWEB::Stations::getStations()) {
            if (station.isMain() || station.isNatural() || find(stationsTried.begin(), stationsTried.end(), &station) != stationsTried.end())
                continue;

            auto defendPos = Stations::getDefendPosition(&station);
            if (defendPos.isValid()) {
                auto defendChoke = Util::getClosestChokepoint(defendPos);
                if (defendChoke && !BWEB::Walls::getWall(defendChoke)) {
                    generateWall(&station, defendChoke);
                    stationsTried.push_back(&station);
                }
            }
        }
    }

    int getColonyCount(const BWEB::Wall *  const wall)
    {
        auto colonies = 0;
        for (auto& tile : wall->getDefenses()) {
            if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                colonies++;
        }
        return colonies;
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

        // (ZvZ) If enemy adds defenses, we can start to cut defenses too
        if (Players::ZvZ() && Util::getTime() > Time(4, 00))
            groundCount += Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Creep_Colony)
            + Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony);

        // (ZvP) If they expanded, we can skip a sunk after a delay
        if (Players::ZvP() && Spy::enemyFastExpand() && Spy::getEnemyBuild() != "FFE" && Util::getTime() > Time(4, 30)) {
            static Time now = Util::getTime();
            if (Util::getTime() > now + Time(0, 45))
                groundCount++;
        }

        // (Zv) Can't build defensives early until closest hatch almost completes 
        if (Broodwar->self()->getRace() == Races::Zerg && Util::getTime() < Time(3, 30)) {
            auto nearestHatch = Util::getClosestUnit(Position(wall.getChokePoint()->Center()), PlayerState::Self, [&](auto &u) {
                return u->getType().isResourceDepot();
            });
            if (nearestHatch && nearestHatch->frameCompletesWhen() > Broodwar->getFrameCount() + 200)
                return 0;
        }

        // If they're only at home and not proxying units, don't make any defenses
        if (!Spy::enemyProxy() && (groundCount >= 1 || getColonyCount(&wall) >= 1)) {
            auto closestUnit = Util::getClosestUnit(Position(wall.getChokePoint()->Center()), PlayerState::Enemy, [&](auto &u) {
                return Units::inBoundUnit(*u);
            });
            if (!closestUnit)
                return 0;
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
            return 0;
        }
        return 0;
    }

    const BWEB::Wall * const getMainWall() { return mainWall; }
    const BWEB::Wall * const getNaturalWall() { return naturalWall; }
}