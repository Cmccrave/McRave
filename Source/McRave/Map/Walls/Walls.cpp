#include "Walls.h"

#include "BWEB.h"
#include "Builds/All/BuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Micro/Scout/Scouts.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls {

    namespace {
        BWEB::Wall *mainWall    = nullptr;
        BWEB::Wall *naturalWall = nullptr;
        vector<vector<UnitType>> naturaltestingOrder;
        vector<vector<UnitType>> thirdTestingOrder;
        vector<UnitType> defenses;
        bool tight;
        bool openWall;
        UnitType tightType   = None;
        bool wantTerranWalls = false;

        void generateWall(const BWEB::Station *const station, const BWEM::ChokePoint *choke)
        {
            auto area = station->getBase()->GetArea();

            // HACK: Zerg walls in the main are just necessary on pocket natural maps, we don't want extra buildings
            if (Broodwar->self()->getRace() == Races::Zerg && area == Terrain::getMainArea()) {
                vector<UnitType> hatch = {Zerg_Hatchery};
                BWEB::Walls::createWall(hatch, area, choke, tightType, defenses, openWall, true);
                return;
            }

            auto &list = station->isNatural() ? naturaltestingOrder : thirdTestingOrder;

            for (auto &buildings : list) {
                if (!BWEB::Walls::getWall(choke)) {
                    BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
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
                defenses = {Protoss_Photon_Cannon};
                if (Players::vZ()) {
                    tight               = false;
                    naturaltestingOrder = {{Protoss_Gateway, Protoss_Forge, Protoss_Pylon}};
                }
                else {
                    tight               = false;
                    naturaltestingOrder = {{Protoss_Forge, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon}};
                }
            }

            // Terran wall parameters
            if (Broodwar->self()->getRace() == Races::Terran) {
                tight             = false;
                defenses          = {Terran_Missile_Turret};
                thirdTestingOrder = {{Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot},
                                     {Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot},
                                     {Terran_Supply_Depot, Terran_Supply_Depot}};

                naturaltestingOrder = {{Terran_Barracks, Terran_Supply_Depot}};
            }

            // Zerg wall parameters
            if (Broodwar->self()->getRace() == Races::Zerg) {
                tight    = false;
                defenses = {Zerg_Sunken_Colony};

                if (Players::ZvT()) {
                    naturaltestingOrder = {{Zerg_Evolution_Chamber, Zerg_Spire},
                                           {Zerg_Evolution_Chamber},
                                           {Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                           {Zerg_Evolution_Chamber, Zerg_Evolution_Chamber},
                                           {}};
                }
                else {
                    naturaltestingOrder = {{Zerg_Evolution_Chamber, Zerg_Hatchery}, {Zerg_Hatchery, Zerg_Evolution_Chamber}, {Zerg_Evolution_Chamber, Zerg_Evolution_Chamber}, {}};
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
            mainWall    = BWEB::Walls::getWall(Terrain::getMainChoke());
        }

        // PvP
        int PvP_Defenses(BWEB::Wall &wall)
        {
            if (BuildOrder::getCurrentTransition() == P_DT)
                return 3;
            return 0;
        }

        // PvZ
        int PvZ_Opener(BWEB::Wall &wall)
        {
            if (Spy::getEnemyOpener() == Z_4Pool)
                return 2 + (Players::getSupply(PlayerState::Self, Races::Protoss) >= 24);
            if (Spy::getEnemyOpener() == Z_9Pool || Spy::getEnemyOpener() == Z_Overpool)
                return 2;
            return total(Protoss_Gateway) > 0;
        }

        int PvZ_Transition(BWEB::Wall &wall)
        {
            auto cannonCount = 1 + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 6) + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 12) +
                               (Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) >= 24) + (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

            if (Spy::getEnemyTransition() == Z_2HatchHydra)
                return 5;
            else if (Spy::getEnemyTransition() == Z_3HatchHydra)
                return 4;
            else if (Spy::getEnemyTransition() == Z_2HatchMuta && Util::getTime() > Time(4, 0))
                return 3;
            else if (Spy::getEnemyTransition() == Z_3HatchMuta && Util::getTime() > Time(5, 0))
                return 3;
            return cannonCount;
        }

        int PvZ_Defenses(BWEB::Wall &wall)
        {
            // Determine how much we have traded
            auto unitsKilled = Players::getDeadCount(PlayerState::Enemy, Zerg_Hydralisk) / 2 + Players::getDeadCount(PlayerState::Enemy, Zerg_Zergling) / 4;

            auto minimum   = 0;
            auto expected  = max(PvZ_Opener(wall), PvZ_Transition(wall));
            auto reduction = max(0, unitsKilled / 8);

            if (expected > 0)
                minimum = 1;

            return max(minimum, expected - reduction);
        }

        // ZvP
        int ZvP_Opener(BWEB::Wall &wall)
        {
            // 1GateCore
            if (Spy::getEnemyBuild() == P_1GateCore || (Spy::getEnemyBuild() == "Unknown" && Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) >= 1)) {
                return (Util::getTime() > Time(3, 30)) + (Util::getTime() > Time(4, 30));
            }

            // 2Gate
            if (Spy::getEnemyBuild() == P_2Gate) {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                    return (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(4, 10)) + (Util::getTime() > Time(4, 30));
                if (Spy::getEnemyOpener() == P_10_15)
                    return (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(4, 10)) + (Util::getTime() > Time(5, 00));
                if (Spy::getEnemyOpener() == P_10_12 || Spy::getEnemyOpener() == "Unknown")
                    return 1 + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 45));
                if (Spy::getEnemyOpener() == P_9_9)
                    return 1 + (BuildOrder::getCurrentOpener() == Z_12Hatch) + (Util::getTime() > Time(4, 30));
                if (Spy::getEnemyOpener() == P_Proxy_9_9)
                    return 2;
                if (Spy::getEnemyOpener() == P_Horror_9_9)
                    return 1;
            }

            // FFE
            if (Spy::getEnemyBuild() == P_FFE) {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 2 || Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 6)
                    return 2;
                return Util::getTime() > Time(5, 00);
            }

            // CannonRush
            if (Spy::getEnemyBuild() == P_CannonRush) {
                return 1;
            }

            // Always make one that is a safety measure vs unknown builds
            return Util::getTime() > Time(3, 45);
        }

        int ZvP_Transition(BWEB::Wall &wall)
        {
            // 1 base transitions
            if (Spy::getEnemyBuild() == P_2Gate || Spy::getEnemyBuild() == P_1GateCore) {

                // 4Gate
                if (Spy::getEnemyTransition() == P_4Gate) {
                    return (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 00)) + (Util::getTime() > Time(5, 15)) +
                           (Util::getTime() > Time(5, 30)) + (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(7, 00)) + (Util::getTime() > Time(8, 00));
                }

                // DT
                if (Spy::getEnemyTransition() == P_DT) {
                    return (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(5, 30));
                }

                // Corsair
                if (Spy::getEnemyTransition() == P_Corsair || Spy::getEnemyTransition() == P_CorsairGoon) {
                    return (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 15)) + (Util::getTime() > Time(5, 15)) + (Util::getTime() > Time(7, 00));
                }

                // Speedlot
                if (Spy::getEnemyTransition() == P_Speedlot) {
                    return (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 15)) + (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(5, 15));
                }

                // Zealot flood
                if (Spy::getEnemyTransition() == P_Rush) {
                    return (Util::getTime() > Time(3, 40)) + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 20)) + (Util::getTime() > Time(4, 40)) + (Util::getTime() > Time(5, 00)) +
                           (Util::getTime() > Time(5, 20)) + (Util::getTime() > Time(5, 40)) + (Util::getTime() > Time(6, 00)) - Spy::enemyProxy();
                }

                if (Util::getTime() > Time(4, 00) && Util::getTime() < Time(6, 30))
                    return (Players::getVisibleCount(PlayerState::Enemy, Protoss_Zealot) / 2) + (Players::getVisibleCount(PlayerState::Enemy, Protoss_Dragoon) / 2);
            }

            // FFE transitions
            if (Spy::getEnemyBuild() == P_FFE) {
                if (Spy::getEnemyTransition() == P_5GateGoon)
                    return (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 30)) + (Util::getTime() > Time(8, 00));

                if (Spy::getEnemyTransition() == P_CorsairGoon)
                    return (Util::getTime() > Time(7, 00));

                if (Spy::getEnemyTransition() == P_Speedlot)
                    return (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 20)) + (Util::getTime() > Time(6, 40));

                if (Spy::getEnemyTransition() == P_Sairlot)
                    return 2 * (Util::getTime() > Time(6, 50));

                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) == 0)
                    return (Util::getTime() > Time(6, 00));

                return (Util::getTime() > Time(7, 15));
            }

            return 0;
        }

        int ZvP_Defenses(BWEB::Wall &wall)
        {
            if (BuildOrder::getCurrentTransition() == Z_3HatchHydra)
                return 0;

            // Determine how much we have traded
            auto unitsKilled     = Players::getDeadCount(PlayerState::Enemy, Protoss_Zealot) + Players::getDeadCount(PlayerState::Enemy, Protoss_Dragoon);
            auto buildingsKilled = Players::getDeadCount(PlayerState::Enemy, Protoss_Gateway);

            auto mutaBuild  = BuildOrder::getCurrentTransition().find("Muta") != string::npos;
            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected   = max(ZvP_Opener(wall), ZvP_Transition(wall));
            auto reduction  = (unitsKilled / 8) + buildingsKilled + (Spy::enemyFastExpand() && Spy::getEnemyBuild() != P_FFE) * 2;
            auto minimum    = int(expected > 0);

            // 3h builds make roughly half as many
            if (threeHatch && expected > 1 && Spy::getEnemyBuild() != P_FFE && Spy::getEnemyBuild() != P_CannonRush && comHatchCount() >= 3) {
                if (com(Zerg_Zergling) >= 24)
                    expected = int(ceil(double(expected) / 1.5));
                else
                    expected = int(floor(double(expected) / 2.0));
            }

            // Non natural walls are limited to 1 total
            if (!wall.getStation()->isNatural() && expected > 0 && Spy::getEnemyBuild() != P_FFE) {
                expected = 1;
                minimum  = 1;
            }

            // Make minimum sunkens if criteria fulfilled
            if (expected > 0)
                minimum = 1;
            if (Spy::getEnemyBuild() != P_FFE && Spy::getEnemyBuild() != P_CannonRush) {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 || Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Singularity_Charge, 1) ||
                    Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 4)
                    minimum = 2;
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0)
                    expected--;
            }

            LOG_SLOW("Expected defenses: ", expected);

            // Against turtle builds we don't need any for a while
            if (Spy::enemyTurtle() && Util::getTime() < Time(5, 00))
                return 0;

            return max(minimum, expected - reduction);
        }

        // ZvT
        int ZvT_Opener(BWEB::Wall &wall)
        {
            // 8Rax / Proxy
            if (Spy::enemyProxy() || Spy::getEnemyOpener() == T_8Rax || Spy::getEnemyOpener() == T_Proxy_8Rax)
                return 1 + (Util::getTime() > Time(4, 30));

            // 2Rax
            if (Spy::getEnemyBuild() == T_2Rax) {

                if (Spy::enemyRush() || Spy::getEnemyOpener() == T_BBS)
                    return (Util::getTime() > Time(2, 50)) + (Util::getTime() > Time(4, 30));

                if (Spy::getEnemyOpener() == T_11_13)
                    return (Util::getTime() > Time(3, 30));

                if (Spy::getEnemyOpener() == T_11_18)
                    return (Util::getTime() > Time(4, 00));

                if (!Spy::enemyFastExpand())
                    return 1 + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(4, 45));
            }

            // RaxCC
            if (Spy::getEnemyBuild() == T_RaxCC) {
                return (Util::getTime() > Time(5, 00));
            }

            // RaxFact
            if (Spy::getEnemyBuild() == T_RaxFact || Spy::getEnemyBuild() == "Unknown") {
                return 1;
            }

            // 
            if (Scouts::enemyDeniedScout() || Spy::enemyWalled())
                return 1;

            // Fall through unknown opener
            if (!Spy::enemyFastExpand() && !Spy::enemyRush())
                return (Util::getTime() > Time(3, 15)) + (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(5, 00));

            return (Util::getTime() > Time(3, 15));
        }

        int ZvT_Transition(BWEB::Wall &wall)
        {
            if (Spy::getEnemyTransition() == U_WorkerRush) {
                return 0;
            }

            if (Spy::getEnemyBuild() == T_2Rax) {

                // Upwards of 5 sunkens vs marine flood
                if (Spy::getEnemyTransition() == T_Rush)
                    return 2 + (Util::getTime() > Time(4, 20)) + (Util::getTime() > Time(4, 40)) + (Util::getTime() > Time(5, 00)) + (Util::getTime() > Time(5, 45)) + (Util::getTime() > Time(6, 30));

                // Need 5 sunkens to defend against ~5:00 timing, then add slowly
                if (Spy::getEnemyTransition() == T_Academy)
                    return 5 * (Util::getTime() > Time(4, 30)) + (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 45));
            }

            if (Spy::getEnemyBuild() == T_RaxFact) {

                // Need 3 sunkens to defend against 7:30 timing
                if (Spy::getEnemyTransition() == T_3FactGoliath)
                    return 1 + (Util::getTime() > Time(6, 00)) + (Util::getTime() > Time(6, 30));

                // Need 1 sunken to defend vulture threat
                if (Spy::getEnemyTransition() == T_2PortWraith)
                    return (Util::getTime() > Time(5, 30));

                // Otherwise throw down 2 if they haven't expanded by 6:00
                if (!Spy::enemyFastExpand())
                    return 2 * (Util::getTime() > Time(6, 00));
            }

            if (Spy::getEnemyBuild() == T_RaxCC) {
                if (Spy::getEnemyTransition() == T_1FactTanks || !BuildOrder::isFocusUnit(Zerg_Lurker))
                    return (Util::getTime() > Time(7, 45)) + (Util::getTime() > Time(8, 15)) + (Util::getTime() > Time(8, 45)) + (Util::getTime() > Time(9, 15)) + (Util::getTime() > Time(9, 45)) +
                           (Util::getTime() > Time(10, 15));
            }

            if (!Spy::enemyFastExpand())
                return 2 * (Util::getTime() > Time(4, 30));

            return 0;
        }

        int ZvT_Defenses(BWEB::Wall &wall)
        {
            // Determine how much we have traded
            auto bioKilled       = Players::getDeadCount(PlayerState::Enemy, Terran_Marine, Terran_Firebat, Terran_Medic);
            auto mechKilled      = Players::getDeadCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode, Terran_Siege_Tank_Tank_Mode);
            auto buildingsKilled = Players::getDeadCount(PlayerState::Enemy, Terran_Barracks);

            auto minimum = Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 ? 1 : 0;

            auto threeHatch = BuildOrder::getCurrentTransition().find("2Hatch") == string::npos;
            auto expected   = max(ZvT_Opener(wall), ZvT_Transition(wall));
            auto reduction  = (bioKilled / 16) + (mechKilled / 2) + buildingsKilled + (Spy::enemyFastExpand() && Spy::getEnemyBuild() != T_RaxCC);

            if (expected > 0)
                minimum = 1;

            return max(minimum, expected - reduction);
        }

        // ZvZ
        int ZvZ_Defenses(BWEB::Wall &wall)
        {
            if (Spy::getEnemyTransition() == Z_1HatchMuta || Spy::getEnemyTransition() == Z_2HatchMuta)
                return 0;

            // 3 Hatch
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 || Spy::getEnemyTransition() == Z_3HatchSpeedling)
                return 1 + (Util::getTime() > Time(4, 15));
            if (Spy::getEnemyTransition() == Z_2HatchSpeedling || (Stations::getStations(PlayerState::Enemy).size() <= 1 && Players::getTotalCount(PlayerState::Enemy, Zerg_Hatchery) >= 2))
                return 1 + (Util::getTime() > Time(4, 45)) + (Util::getTime() > Time(5, 15));
            if (Spy::getEnemyOpener() == Z_12Hatch || Spy::getEnemyOpener() == Z_10Hatch)
                return 1;

            // 1Hatch Hydra/Lurker
            if (Spy::getEnemyTransition() == Z_1HatchLurker) {
                return 2 * (Util::getTime() > Time(4, 30));
            }

            return 0;
        }

        // ZvFFA
        int ZvFFA_Defenses(BWEB::Wall &wall) { return 1 + (Util::getTime() > Time(5, 20)) + (Util::getTime() > Time(5, 40)); }
    } // namespace

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

    bool isDefenseFilled(BWEB::Wall *const wall)
    {
        for (auto &defense : wall->getDefenses(1)) {
            if (BWEB::Map::isUsed(defense) == None)
                return false;
        }
        for (auto &defense : wall->getDefenses(2)) {
            if (BWEB::Map::isUsed(defense) == None)
                return false;
        }
        return true;
    }

    bool isComplete(BWEB::Wall *const wall)
    {
        for (auto large : wall->getLargeTiles()) {
            if (BWEB::Map::isUsed(large) == UnitTypes::None)
                return false;
        }
        for (auto medium : wall->getMediumTiles()) {
            if (BWEB::Map::isUsed(medium) == UnitTypes::None)
                return false;
        }
        for (auto small : wall->getSmallTiles()) {
            if (BWEB::Map::isUsed(small) == UnitTypes::None)
                return false;
        }
        return true;
    }

    int getColonyCount(BWEB::Wall *const wall)
    {
        auto colonies = 0;
        for (auto &tile : wall->getDefenses()) {
            if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                colonies++;
        }
        return colonies;
    }

    int needGroundDefenses(BWEB::Wall &wall)
    {
        auto groundCount = wall.getGroundDefenseCount();
        if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()) || BuildOrder::isAllIn() || (!Combat::isDefendNatural() && wall.getStation()->isNatural()))
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
            groundCount += Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Creep_Colony) +
                           Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony);

        // (ZvP) If they expanded, we can skip a sunk after a delay
        if (Players::ZvP() && Spy::enemyFastExpand() && Spy::getEnemyBuild() != P_FFE && Util::getTime() > Time(4, 30)) {
            static Time now = Util::getTime();
            if (Util::getTime() > now + Time(0, 45))
                groundCount++;
        }

        // (Zv) Can't build defensives early until closest hatch almost completes
        if (Broodwar->self()->getRace() == Races::Zerg && Util::getTime() < Time(3, 30)) {
            auto nearestHatch = Util::getClosestUnit(Position(wall.getChokePoint()->Center()), PlayerState::Self, [&](auto &u) { return u->getType().isResourceDepot(); });
            if (nearestHatch && nearestHatch->frameCompletesWhen() > Broodwar->getFrameCount() + 200)
                return 0;
        }

        // If they're only at home and not proxying units, don't make any defenses for a bit
        auto minimumColonyNeeded = Util::getTime() > Time(6, 00) ? 2 : 1;
        if (!Players::vFFA() && !Spy::enemyProxy() && (groundCount >= minimumColonyNeeded || getColonyCount(&wall) >= minimumColonyNeeded)) {
            auto closestUnit = Util::getClosestUnit(Position(wall.getChokePoint()->Center()), PlayerState::Enemy, [&](auto &u) { return Units::inBoundUnit(*u) && !u->getType().isWorker(); });
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
            if (Stations::ownedBy(wall.getStation()) != PlayerState::Self)
                return 0;

            if (isDefenseFilled(&wall))
                wall.requestAddedLayer();

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

    int needAirDefenses(BWEB::Wall &wall)
    {
        auto airCount       = wall.getAirDefenseCount();
        const auto enemyAir = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0 ||
                              Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0 ||
                              Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0 || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 ||
                              (Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0 && Util::getTime() > Time(4, 45));

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

    BWEB::Wall *const getMainWall() { return mainWall; }
    BWEB::Wall *const getNaturalWall() { return naturalWall; }
} // namespace McRave::Walls