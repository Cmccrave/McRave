#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Strategy {

    namespace {

        bool enemyFE = false;
        bool invis = false;
        bool rush = false;
        bool holdChoke = false;
        bool blockedScout = false;

        bool proxyCheck = false;
        bool gasSteal = false;
        bool air = false;
        bool enemyScout = false;
        bool pressure = false;
        Position scoutPosition = Positions::Invalid;
        string enemyBuild = "Unknown";
        string enemyOpener = "Unknown";
        string enemyTransition = "Unknown";

        int poolFrame = INT_MAX;
        int enemyGas = 0;
        int inboundScoutFrame = 0;
        Time enemyBuildTime;
        Time enemyOpenerTime;
        Time enemyTransitionTime;
        pair<bool, Time> proxyTiming;

        bool goonRange = false;
        bool vultureSpeed = false;

        Time rushArrivalTime;

        struct Timings {
            vector<Time> countCompletedWhen;
            vector<Time> countArrivesWhen;
            Time firstCompletedWhen;
            Time firstArrivesWhen;
        };
        map<UnitType, Timings> timingsList;

        bool finishedSooner(UnitType t1, UnitType t2)
        {
            if (timingsList.find(t1) == timingsList.end())
                return false;
            if (timingsList.find(t2) == timingsList.end())
                return true;

            auto timings1 = timingsList[t1];
            auto timings2 = timingsList[t2];
            return (timings1.firstCompletedWhen < timings2.firstCompletedWhen);
        }

        bool completesBy(int count, UnitType type, Time beforeThis)
        {
            int current = 0;
            for (auto &time : timingsList[type].countCompletedWhen) {
                if (time < beforeThis)
                    current++;
            }
            return current >= count;
        }

        bool arrivesBy(int count, UnitType type, Time beforeThis)
        {
            int current = 0;
            for (auto &time : timingsList[type].countArrivesWhen) {
                if (time < beforeThis)
                    current++;
            }
            return current >= count;
        }

        void enemyStrategicInfo(PlayerInfo& player)
        {
            timingsList.clear();
            auto workerNearUs = 0;
            for (auto &u : player.getUnits()) {
                UnitInfo &unit =*u;

                // Monitor the soonest the enemy will scout us
                if (unit.getType().isWorker()) {
                    if (inboundScoutFrame == 0)
                        inboundScoutFrame = unit.frameArrivesWhen();
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()) || (inboundScoutFrame > 0 && inboundScoutFrame - Broodwar->getFrameCount() < 64))
                        enemyScout = true;
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()))
                        workerNearUs++;
                }

                // Monitor gas intake or gas steal
                if (unit.getType().isRefinery() && unit.unit()->exists()) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()))
                        gasSteal = true;
                    else
                        enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
                }

                // Estimate the finishing frame
                if (!unit.getType().isResourceDepot() || !Terrain::isStartingBase(unit.getTilePosition())) {
                    timingsList[unit.getType()].countCompletedWhen.push_back(unit.timeCompletesWhen());
                    timingsList[unit.getType()].countArrivesWhen.push_back(unit.timeArrivesWhen());

                    if (timingsList[unit.getType()].firstArrivesWhen == Time(999, 0) || unit.timeArrivesWhen() < timingsList[unit.getType()].firstArrivesWhen)
                        timingsList[unit.getType()].firstArrivesWhen = unit.timeArrivesWhen();

                    if (timingsList[unit.getType()].firstCompletedWhen == Time(999, 0) || unit.timeCompletesWhen() < timingsList[unit.getType()].firstCompletedWhen)
                        timingsList[unit.getType()].firstCompletedWhen = unit.timeCompletesWhen();
                }

                if (workerNearUs > 3 && Util::getTime() < Time(2, 30))
                    enemyTransition = "WorkerRush";

                // Monitor for a fast expand
                if (unit.getType().isResourceDepot()) {
                    for (auto &base : Terrain::getAllBases()) {
                        if (!base->Starting() && base->Center().getDistance(unit.getPosition()) < 64.0)
                            enemyFE = true;
                    }
                }

                // Proxy detection
                if (Util::getTime() < Time(5, 00)) {
                    if ((unit.getType() == Protoss_Gateway && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2) || unit.getType() == Terran_Barracks) {
                        auto closestMain = BWEB::Stations::getClosestMainStation(unit.getTilePosition());
                        if (closestMain && closestMain->getBase()->GetArea() != mapBWEM.GetArea(unit.getTilePosition()))
                            proxyTiming.first = true;

                        if (Terrain::isInAllyTerritory(unit.getTilePosition())
                            || unit.getPosition().getDistance(mapBWEM.Center()) < 960.0
                            || (BWEB::Map::getNaturalChoke() && unit.getPosition().getDistance((Position)BWEB::Map::getNaturalChoke()->Center()) < 480.0)) {
                            proxyTiming.first = true;
                        }
                    }
                    if (unit.getType() == Protoss_Pylon && Players::getCurrentCount(PlayerState::Enemy, Protoss_Forge) >= 1) {
                        auto closestMain = BWEB::Stations::getClosestMainStation(unit.getTilePosition());
                        auto closestNat = BWEB::Stations::getClosestNaturalStation(unit.getTilePosition());
                        if (closestNat && closestNat->getBase()->GetArea() != mapBWEM.GetArea(unit.getTilePosition()) && closestMain->getBase()->GetArea() != mapBWEM.GetArea(unit.getTilePosition()))
                            proxyTiming.first = true;
                    }

                    if (unit.getType() == Protoss_Pylon) {
                        auto closestMain = BWEB::Stations::getClosestMainStation(unit.getTilePosition());
                        if (closestMain && Stations::ownedBy(closestMain) == PlayerState::Self && closestMain->getBase()->Center().getDistance(unit.getPosition()) < 960.0)
                            proxyTiming.first = true;
                    }
                }
            }
        }

        void enemyZergBuilds(PlayerInfo& player)
        {
            if (Util::getTime() - enemyBuildTime > Time(0, 05))
                return;
            enemyBuild = "Unknown";

            auto enemyHatchCount = Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hive);

            for (auto &u : player.getUnits()) {
                UnitInfo &unit =*u;

                // If this is our first time seeing a Zergling, see how soon until it arrives
                if (unit.getType() == UnitTypes::Zerg_Zergling) {
                    if (unit.timeArrivesWhen() < rushArrivalTime)
                        rushArrivalTime = unit.timeArrivesWhen();
                }
            }

            // Zerg builds
            if ((enemyFE || enemyHatchCount > 1) && completesBy(1, Zerg_Hatchery, Time(3, 15)))
                enemyBuild = "HatchPool";
            else if ((enemyFE || enemyHatchCount > 1) && completesBy(1, Zerg_Hatchery, Time(4, 00)))
                enemyBuild = "PoolHatch";
            else if ((!enemyFE && enemyHatchCount == 1) && Util::getTime() > Time(4, 00))
                enemyBuild = "PoolLair";
        }

        void enemyZergOpeners(PlayerInfo& player)
        {
            if (Util::getTime() - enemyOpenerTime > Time(0, 05))
                return;
            enemyOpener = "Unknown";

            auto enemyHatchCount = Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hive);

            // Pool timing
            if (enemyOpener == "Unknown") {
                if (rushArrivalTime < Time(2, 30) || completesBy(1, Zerg_Zergling, Time(2, 00)) || completesBy(1, Zerg_Spawning_Pool, Time(1, 30)))
                    enemyOpener = "4Pool";
                else if (rushArrivalTime < Time(3, 20) || completesBy(1, Zerg_Zergling, Time(2, 30)) || completesBy(1, Zerg_Spawning_Pool, Time(2, 00)))
                    enemyOpener = "9Pool";
                else if (enemyFE && (rushArrivalTime < Time(3, 50) || completesBy(1, Zerg_Zergling, Time(3, 00)) || completesBy(1, Zerg_Spawning_Pool, Time(2, 00))))
                    enemyOpener = "12Pool";
            }

            // Hatch timing
        }

        void enemyZergTransitions(PlayerInfo& player)
        {
            if (Util::getTime() - enemyTransitionTime > Time(0, 05))
                return;
            enemyTransition = "Unknown";

            auto enemyHatchCount = Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hive);

            // Ling rush detection
            if (enemyOpener == "4Pool" || (enemyOpener == "9Pool" && Util::getTime() < Time(3, 20) && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 14 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 1))
                enemyTransition = "LingRush";

            // Zergling all-in transitions
            if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 0) {
                if (Players::ZvZ() && !enemyFE && completesBy(2, Zerg_Hatchery, Time(4, 00)) && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 8)
                    enemyTransition = "2HatchLing";
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 3 && enemyGas < 148 && enemyGas >= 50 && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 12)
                    enemyTransition = "3HatchLing";
            }

            // Hydralisk/Lurker build detection
            if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0) {
                if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 3)
                    enemyTransition = "3HatchHydra";
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 2)
                    enemyTransition = "2HatchHydra";
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 1)
                    enemyTransition = "1HatchHydra";
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 2)
                    enemyTransition = "2HatchLurker";
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 1 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 0)
                    enemyTransition = "1HatchLurker";
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) >= 4)
                    enemyTransition = "5Hatch";
            }

            // Mutalisk transition detection
            if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0) {
                if ((Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) > 0) && enemyHatchCount == 3)
                    enemyTransition = "3HatchMuta";
                else if (Util::getTime() < Time(3, 30) && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) > 0 && (enemyHatchCount == 2 || enemyFE))
                    enemyTransition = "2HatchMuta";
                else if (enemyHatchCount = 1 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) > 0 && Players::ZvZ())
                    enemyTransition = "1HatchMuta";
            }
        }

        void enemyTerranBuilds(PlayerInfo& player)
        {
            if (Util::getTime() - enemyBuildTime > Time(0, 05))
                return;
            enemyBuild = "Unknown";

            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // Marine timing
                if (unit.getType() == Terran_Marine && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 2) {
                    if (unit.timeArrivesWhen() < rushArrivalTime)
                        rushArrivalTime = unit.timeArrivesWhen();
                }

                // Factory Research
                if (unit.getType() == Terran_Machine_Shop) {
                    if (unit.unit()->exists() && unit.unit()->isUpgrading())
                        vultureSpeed = true;
                }

                // FE Detection
                if (Broodwar->getFrameCount() < 10000 && unit.getType() == Terran_Bunker) {
                    if (unit.getPosition().getDistance(Position(Terrain::getEnemyNatural()->getChokepoint()->Center())) < 320.0 || unit.getPosition().getDistance(Position(Terrain::getEnemyNatural()->getBase()->Center())) < 320.0)
                        enemyFE = true;
                }
            }

            // 2Rax
            if ((rushArrivalTime < Time(3, 10) && Util::getTime() < Time(3, 25) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 3)
                || (Util::getTime() < Time(2, 55) && Players::getTotalCount(PlayerState::Enemy, Terran_Barracks) >= 2)
                || (Util::getTime() < Time(4, 00) && Players::getTotalCount(PlayerState::Enemy, Terran_Barracks) >= 2 && Players::getTotalCount(PlayerState::Enemy, Terran_Refinery) == 0)
                || (Util::getTime() < Time(2, 55) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 3))
                enemyBuild = "2Rax";

            // RaxCC
            if ((Util::getTime() < Time(4, 00) && enemyFE)
                || rushArrivalTime < Time(2, 45)
                || completesBy(1, Terran_Barracks, Time(1, 45))
                || (Scouts::gotFullScout() && Util::getTime() > Time(2, 45) && Players::getTotalCount(PlayerState::Enemy, Terran_Barracks) == 1 && Players::getTotalCount(PlayerState::Enemy, Terran_Refinery) == 0))
                enemyBuild = "RaxCC";

            // RaxFact
            if (Util::getTime() < Time(3, 00) && Players::getTotalCount(PlayerState::Enemy, Terran_Refinery) > 0)
                enemyBuild = "RaxFact";
        }

        void enemyTerranOpeners(PlayerInfo& player)
        {
            if (Util::getTime() - enemyOpenerTime > Time(0, 05))
                return;
            enemyOpener = "Unknown";

            // Bio builds
            if (enemyBuild == "2Rax") {
                if (enemyFE)
                    enemyOpener = "Expand";
                else if (proxyTiming.first)
                    enemyOpener = "Proxy";
                else if (Util::getTime() > Time(4, 00))
                    enemyOpener = "Main";
            }

            // Mech builds
            if (enemyBuild == "RaxFact") {
                // 1-5Fact???
            }

            // Expand builds
            if (enemyBuild == "RaxCC") {
                if (rushArrivalTime < Time(2, 45)
                    || completesBy(1, Terran_Barracks, Time(1, 45)))
                    enemyOpener = "8Rax";
                else if (enemyFE)
                    enemyOpener = "1RaxFE";
            }
        }

        void enemyTerranTransitions(PlayerInfo& player)
        {
            if (Util::getTime() - enemyTransitionTime > Time(0, 05))
                return;
            enemyTransition = "Unknown";

            auto hasTanks = Players::getCurrentCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0 || Players::getCurrentCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) > 0;
            auto hasGols = Players::getCurrentCount(PlayerState::Enemy, Terran_Goliath) > 0;

            // General factory builds
            if ((Players::getTotalCount(PlayerState::Enemy, Terran_Vulture_Spider_Mine) > 0 && Util::getTime() < Time(6, 00))
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Factory) >= 2 && vultureSpeed)
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 3 && Util::getTime() < Time(5, 30))
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 5 && Util::getTime() < Time(6, 00))
                || (Broodwar->self()->getRace() == Races::Zerg && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) >= 1 && Util::getTime() < Time(5, 0)))
                enemyTransition = "2Fact";

            // PvT
            if (Players::PvT()) {
                if ((Players::getCurrentCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0 && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) == 0)
                    || (enemyFE && Players::getCurrentCount(PlayerState::Enemy, Terran_Machine_Shop) > 0))
                    enemyTransition = "SiegeExpand";
            }

            // ZvT
            if (Players::ZvT()) {

                // RaxFact
                if (enemyBuild == "RaxFact") {
                    if ((!hasTanks && hasGols && enemyFE) || (Util::getTime() < Time(7, 30) && Players::getCurrentCount(PlayerState::Enemy, Terran_Armory) > 0) || (Util::getTime() < Time(8, 00) && Players::getCurrentCount(PlayerState::Enemy, Terran_Goliath) > 0))
                        enemyTransition = "5FactGoliath";
                }

                // 2Rax
                if (enemyBuild == "2Rax") {
                    if (enemyFE && hasTanks && Util::getTime() < Time(10, 30))
                        enemyTransition = "1FactTanks";
                    else if (rushArrivalTime < Time(3, 15) || completesBy(2, Terran_Barracks, Time(2, 35)))
                        enemyTransition = "MarineRush";
                    else if (!enemyFE && (completesBy(1, Terran_Academy, Time(5, 10)) || player.hasTech(TechTypes::Stim_Packs) || arrivesBy(1, Terran_Medic, Time(6, 00)) || arrivesBy(1, Terran_Firebat, Time(6, 00))))
                        enemyTransition = "Academy";
                }

                // RaxCC
                if (enemyBuild == "RaxCC") {
                    if (enemyFE && hasTanks && Util::getTime() < Time(10, 30))
                        enemyTransition = "1FactTanks";
                }
            }

            // TvT
        }

        void enemyProtossBuilds(PlayerInfo& player)
        {
            if (Util::getTime() - enemyBuildTime > Time(0, 05))
                return;
            enemyBuild = "Unknown";

            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // Zealot timing
                if (unit.getType() == Protoss_Zealot) {
                    if (unit.timeArrivesWhen() < rushArrivalTime)
                        rushArrivalTime = unit.timeArrivesWhen();
                }

                // CannonRush
                if (unit.getType() == Protoss_Forge && Scouts::gotFullScout()) {
                    if (unit.getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 200.0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Nexus) <= 1)
                        enemyBuild = "CannonRush";
                }

                // FFE
                if (Util::getTime() < Time(4, 00) && (unit.getType() == Protoss_Photon_Cannon || unit.getType() == Protoss_Forge)) {
                    if (unit.getPosition().getDistance(Position(Terrain::getEnemyNatural()->getChokepoint()->Center())) < 320.0 || unit.getPosition().getDistance(Position(Terrain::getEnemyNatural()->getBase()->Center())) < 320.0) {
                        enemyBuild = "FFE";
                        enemyFE = true;
                    }
                }

                // TODO: Check if this actually works
                if (unit.getType().isWorker() && unit.unit()->exists() && unit.unit()->getOrder() == Orders::HoldPosition)
                    blockedScout = true;
            }

            // 1GateCore - Gas estimation
            if (completesBy(1, Protoss_Assimilator, Time(2, 15)))
                enemyBuild = "1GateCore";

            // 1GateCore - Core estimation
            if (completesBy(1, Protoss_Cybernetics_Core, Time(3, 30)))
                enemyBuild = "1GateCore";

            // 1GateCore - Goon estimation
            if (completesBy(1, Protoss_Dragoon, Time(4, 10)))
                enemyBuild = "1GateCore";

            // 2Gate - Zealot estimation
            if ((Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) >= 5 && Util::getTime() < Time(4, 0))
                || (Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) >= 3 && Util::getTime() < Time(3, 30))
                || arrivesBy(2, Protoss_Zealot, Time(3, 30))
                || arrivesBy(3, Protoss_Zealot, Time(3, 55))
                || arrivesBy(4, Protoss_Zealot, Time(4, 20))
                || completesBy(2, Protoss_Gateway, Time(2, 55))) {
                enemyBuild = "2Gate";
            }

            // 2Gate Proxy - No info estimation
            if (Scouts::gotFullScout() && Util::getTime() < Time(3, 30) && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Nexus) <= 1 && Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) > 0) {
                enemyBuild = "2Gate";
                proxyTiming.first = true;
            }
        }

        void enemyProtossOpeners(PlayerInfo& player)
        {
            if (Util::getTime() - enemyOpenerTime > Time(0, 05))
                return;
            enemyOpener = "Unknown";

            // 2Gate Openers
            if (enemyBuild == "2Gate") {
                if (proxyTiming.first || arrivesBy(1, Protoss_Zealot, Time(2,50)) || arrivesBy(2, Protoss_Zealot, Time(3, 15)) || arrivesBy(4, Protoss_Zealot, Time(3, 50)))
                    enemyOpener = "Proxy";
                else if (enemyFE)
                    enemyOpener = "Natural";
                else
                    enemyOpener = "Main";
            }

            // FFE Openers
            if (enemyBuild == "FFE") {
                if (finishedSooner(Protoss_Nexus, Protoss_Forge))
                    enemyOpener = "Nexus";
                else if (finishedSooner(Protoss_Gateway, Protoss_Forge))
                    enemyOpener = "Gateway";
                else
                    enemyOpener = "Forge";
            }

            // 1Gate Openers
            if (enemyBuild == "1GateCore") {
                if (finishedSooner(Protoss_Dragoon, Protoss_Zealot) || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) > 0))
                    enemyOpener = "0Zealot";
                else if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) == 1)
                    enemyOpener = "1Zealot";
                else if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 && Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 2)
                    enemyOpener = "2Zealot";
            }
        }

        void enemyProtossTransitions(PlayerInfo& player)
        {
            if (Util::getTime() - enemyTransitionTime > Time(0, 05))
                return;
            enemyTransition = "Unknown";

            // 2Gate rush
            if (enemyBuild == "2Gate") {
                if (enemyOpener == "Proxy"
                    || (Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 4 && Util::getTime() < Time(3, 50))
                    || (Players::getTotalCount(PlayerState::Enemy, Protoss_Assimilator) == 0 && Util::getTime() > Time(5, 0))
                    || completesBy(3, Protoss_Gateway, Time(3,30)))
                    enemyTransition = "ZealotRush";
            }

            // 2Gate tech
            if (enemyBuild == "2Gate" || enemyBuild == "1GateCore") {

                // DT
                if ((Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) > 0)
                    || Players::getCurrentCount(PlayerState::Enemy, Protoss_Templar_Archives) >= 1
                    || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 2 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 && Util::getTime() > Time(6, 45)))
                    enemyTransition = "DT";

                // Corsair
                if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Stargate) > 0)
                    enemyTransition = "Corsair";

                // 5ZealotExpand
                if (Players::PvP() && enemyFE && (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 || Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 5) && Util::getTime() < Time(4, 45))
                    enemyTransition = "5ZealotExpand";                
            }

            // 4Gate
            if (Players::PvP()) {
                if ((Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 7 && Util::getTime() < Time(6, 30))
                    || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 11 && Util::getTime() < Time(7, 15))
                    || (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 4 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 && Util::getTime() < Time(5, 30)))
                    enemyTransition = "4Gate";
            }
            if (Players::ZvP()) {
                if (!enemyFE && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 1)
                    enemyTransition = "4Gate";
            }

            // FFE transitions
            if (Players::ZvP() && enemyBuild == "FFE") {
                if ((Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 4 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 1) || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 4 && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) == 0))
                    enemyTransition = "5GateGoon";
                else if ((completesBy(1, Protoss_Stargate, Time(5, 45)) && completesBy(1, Protoss_Citadel_of_Adun, Time(5, 45))) || (Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 && Util::getTime() < Time(6, 15)))
                    enemyTransition = "NeoBisu";
                else if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1)
                    enemyTransition = "ZealotArchon";
            }
        }

        void checkEnemyRush()
        {
            // Rush builds are immediately aggresive builds
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self) >= 60 : Players::getSupply(PlayerState::Self) >= 80;
            rush = !supplySafe && (enemyTransition == "MarineRush" || enemyTransition == "ZealotRush" || enemyTransition == "LingRush" || enemyTransition == "WorkerRush");
        }

        void checkEnemyPressure()
        {
            // Pressure builds are delayed aggresive builds
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self) >= 100 : Players::getSupply(PlayerState::Self) >= 120;
            pressure = !supplySafe && (enemyTransition == "4Gate" || enemyTransition == "2HatchLing" || enemyTransition == "Sparks" || enemyTransition == "2Fact" || enemyTransition == "Academy");
        }

        void checkHoldChoke()
        {
            // UMS Setting
            if (Broodwar->getGameType() == BWAPI::GameTypes::Use_Map_Settings) {
                holdChoke = true;
                return;
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss && Players::getSupply(PlayerState::Self) > 40) {
                holdChoke = BuildOrder::isFastExpand()
                    || vis(Protoss_Dragoon) > 0
                    || com(Protoss_Shield_Battery) > 0
                    || BuildOrder::isWallNat()
                    || (BuildOrder::isHideTech() && !rush)
                    || Players::getSupply(PlayerState::Self) > 60
                    || Players::vT();
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran && Players::getSupply(PlayerState::Self) > 40)
                holdChoke = true;

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                holdChoke = !Players::ZvZ()
                    || (Strategy::getEnemyOpener() == "9Pool" && !rush && !pressure && com(Zerg_Sunken_Colony) == 0 && total(Zerg_Mutalisk) < 3);
            }

            // Other situations
            if (!holdChoke && Units::getImmThreat() > 0.0)
                holdChoke = false;
        }

        void checkNeedDetection()
        {
            // DTs, Vultures, Lurkers
            invis = (Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar) >= 1 || (Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) > 0) || Players::getCurrentCount(PlayerState::Enemy, Protoss_Templar_Archives) >= 1)
                || (Players::getCurrentCount(PlayerState::Enemy, Terran_Ghost) >= 1 || Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) >= 4)
                || (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lurker) >= 1 || (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) <= 0))
                || (enemyBuild == "1HatchLurker" || enemyBuild == "2HatchLurker" || enemyBuild == "1GateDT");

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (com(Protoss_Observer) > 0)
                    invis = false;
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Pneumatized_Carapace))
                    invis = false;
            }

            // Terran
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (com(Terran_Comsat_Station) > 0)
                    invis = false;
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk) > 0 || Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0)
                    invis = true;
            }
        }

        void checkEnemyProxy()
        {
            // If we have seen an enemy Probe before we've scouted the enemy, follow it
            if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Probe) == 1 && com(Protoss_Zealot) < 1) {
                auto &enemyProbe = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getType() == Protoss_Probe;
                });
                proxyCheck = (enemyProbe && !Terrain::getEnemyStartingPosition().isValid() && (enemyProbe->getPosition().getDistance(BWEB::Map::getMainPosition()) < 640.0 || Terrain::isInAllyTerritory(enemyProbe->getTilePosition())));
            }
            else
                proxyCheck = false;

            // Needs to be true for 5 seconds to be a proxy
            if (proxyTiming.first) {
                if (proxyTiming.second == Time(999, 0))
                    proxyTiming.second = Util::getTime();
                if (Util::getTime() - proxyTiming.second < Time(0, 04))
                    proxyTiming.first = false;
            }
            else
                proxyTiming.second = Time(999, 0);

            // Proxy builds are built closer to me than the enemy
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self) >= 40 : Players::getSupply(PlayerState::Self) >= 80;
            proxyTiming.first = !supplySafe && (enemyBuild == "CannonRush" || enemyBuild == "BunkerRush" || proxyTiming.first);
        }

        void checkEnemyAir()
        {
            air = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair)
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout)
                || Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate)
                || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith)
                || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie)
                || Players::getTotalCount(PlayerState::Enemy, Terran_Starport)
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk)
                || Players::getTotalCount(PlayerState::Enemy, Zerg_Spire);
        }

        void updateEnemyBuild()
        {
            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;

                if (player.isEnemy()) {

                    // Find where the enemy scout is
                    auto playersScout = Util::getClosestUnit(BWEB::Map::getMainPosition(), player.getPlayerState(), [&](auto &u) {
                        return u.getType().isWorker();
                    });
                    scoutPosition = playersScout ? playersScout->getPosition() : Positions::Invalid;

                    enemyStrategicInfo(player);
                    if (player.getCurrentRace() == Races::Zerg) {
                        enemyZergBuilds(player);
                        enemyZergOpeners(player);
                        enemyZergTransitions(player);
                    }
                    else if (player.getCurrentRace() == Races::Protoss) {
                        enemyProtossBuilds(player);
                        enemyProtossOpeners(player);
                        enemyProtossTransitions(player);
                    }
                    else if (player.getCurrentRace() == Races::Terran) {
                        enemyTerranBuilds(player);
                        enemyTerranOpeners(player);
                        enemyTerranTransitions(player);
                    }

                    // Detected a build
                    if (enemyBuild != "Unknown") {
                        player.setBuild(enemyBuild);

                        // Set a time if first time detecting it
                        if (enemyBuildTime == Time(999, 0))
                            enemyBuildTime = Util::getTime();

                        // Reset build detection to not force a reaction until we are sure of the build
                        if (Util::getTime() - enemyBuildTime < Time(0, 05))
                            enemyBuild = "Unknown";                        
                    }

                    // Didn't detect a build, clear info
                    else
                        enemyBuildTime = Time(999, 0);

                    // Detected an opener
                    if (enemyOpener != "Unknown") {
                        player.setOpener(enemyOpener);

                        // Set a time if first time detecting it
                        if (enemyOpenerTime == Time(999, 0))
                            enemyOpenerTime = Util::getTime();

                        // Reset opener detection to not force a reaction until we are sure of the opener
                        if (Util::getTime() - enemyOpenerTime < Time(0, 05))
                            enemyOpener = "Unknown";
                    }

                    // Didn't detect an opener, clear info
                    else
                        enemyOpenerTime = Time(999, 0);

                    // Detected a transition
                    if (enemyTransition != "Unknown") {
                        player.setTransition(enemyTransition);

                        // Set a time if first time detecting it
                        if (enemyTransitionTime == Time(999, 0))
                            enemyTransitionTime = Util::getTime();

                        // Reset transition detection to not force a reaction until we are sure of the transition
                        if (Util::getTime() - enemyTransitionTime < Time(0, 05))
                            enemyTransition = "Unknown";
                    }

                    // Didn't detect a transition, clear info
                    else
                        enemyTransitionTime = Time(999, 0);
                }
            }
        }

        void globalReactions()
        {
            checkNeedDetection();
            checkEnemyPressure();
            checkEnemyProxy();
            checkEnemyRush();
            checkEnemyAir();
            checkHoldChoke();
        }
    }

    void onFrame()
    {
        updateEnemyBuild();
        globalReactions();
    }

    string getEnemyBuild() { return enemyBuild; }
    string getEnemyOpener() { return enemyOpener; }
    string getEnemyTransition() { return enemyTransition; }
    Position enemyScoutPosition() { return scoutPosition; }
    Time getEnemyBuildTime() { return enemyBuildTime; }
    Time getEnemyOpenerTime() { return enemyOpenerTime; }
    Time getEnemyTransitionTime() { return enemyTransitionTime; }
    bool enemyAir() { return air; }
    bool enemyFastExpand() { return enemyFE; }
    bool enemyRush() { return rush; }
    bool needDetection() { return invis; }
    bool defendChoke() { return holdChoke; }
    bool enemyPossibleProxy() { return proxyCheck; }
    bool enemyProxy() { return proxyTiming.first; }
    bool enemyGasSteal() { return gasSteal; }
    bool enemyScouted() { return enemyScout; }
    bool enemyBust() { return enemyBuild.find("Hydra") != string::npos; }
    bool enemyPressure() { return pressure; }
    bool enemyBlockedScout() { return blockedScout; }
    Time enemyArrivalTime() { return rushArrivalTime; }
}