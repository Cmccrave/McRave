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
        bool proxy = false;
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
        int enemyFrame = 0;
        int inboundScoutFrame = 0;

        bool goonRange = false;
        bool vultureSpeed = false;

        Time rushArrivalTime;
        map<UnitType, Time> firstFinishedTime;

        bool finishedSooner(UnitType t1, UnitType t2)
        {
            if (firstFinishedTime.find(t1) == firstFinishedTime.end())
                return false;
            return (firstFinishedTime.find(t2) == firstFinishedTime.end() || firstFinishedTime.at(t1) < firstFinishedTime.at(t2));
        }

        void enemyStrategicInfo(PlayerInfo& player)
        {
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

                // Monitor if this is the first of its type to be scene and estimate the finishing frame
                if (firstFinishedTime[unit.getType()] == Time(999, 0) && (!unit.getType().isResourceDepot() || !Terrain::isStartingBase(unit.getTilePosition())))
                    firstFinishedTime[unit.getType()] = unit.timeCompletesWhen();

                if (workerNearUs > 3 && Util::getTime() < Time(2, 30))
                    enemyTransition = "WorkerRush";

                // Monitor for a fast expand
                if (unit.getType().isResourceDepot()) {
                    for (auto &base : Terrain::getAllBases()) {
                        if (!base->Starting() && base->Center().getDistance(unit.getPosition()) < 64.0)
                            enemyFE = true;
                    }
                }
            }
        }

        void enemyZergBuilds(PlayerInfo& player)
        {
            auto enemyHatchCount = Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hive);

            for (auto &u : player.getUnits()) {
                UnitInfo &unit =*u;

                // If this is our first time seeing a Zergling, see how soon until it arrives
                if (unit.getType() == UnitTypes::Zerg_Zergling) {
                    if (rushArrivalTime == Time(999, 00) || unit.timeArrivesWhen() < rushArrivalTime)
                        rushArrivalTime = unit.timeArrivesWhen();
                }
            }

            // Zerg builds
            if ((enemyFE || enemyHatchCount > 1) && firstFinishedTime[Zerg_Hatchery] < Time(3, 15))
                enemyBuild = "HatchPool";
            else if ((enemyFE || enemyHatchCount > 1) && firstFinishedTime[Zerg_Hatchery] < Time(4, 00))
                enemyBuild = "PoolHatch";
            else if ((!enemyFE && enemyHatchCount == 1) && Util::getTime() > Time(4, 00))
                enemyBuild = "PoolLair";
        }

        void enemyZergOpeners(PlayerInfo& player)
        {
            auto enemyHatchCount = Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hive);

            // Pool timing
            if (enemyOpener == "Unknown") {
                if (rushArrivalTime < Time(2, 45) || firstFinishedTime[Zerg_Zergling] < Time(2, 00) || firstFinishedTime[Zerg_Spawning_Pool] < Time(1, 30))
                    enemyOpener = "4Pool";
                else if (rushArrivalTime < Time(3, 20) || firstFinishedTime[Zerg_Zergling] < Time(2, 30) || firstFinishedTime[Zerg_Spawning_Pool] < Time(2, 00))
                    enemyOpener = "9Pool";
                else if (enemyFE && (rushArrivalTime < Time(3, 50) || firstFinishedTime[Zerg_Zergling] < Time(3, 00) || firstFinishedTime[Zerg_Spawning_Pool] < Time(2, 00)))
                    enemyOpener = "12Pool";
            }

            // Hatch timing
        }

        void enemyZergTransitions(PlayerInfo& player)
        {
            auto enemyHatchCount = Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair)
                + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hive);

            // Ling rush detection
            if (enemyOpener == "4Pool" || (enemyOpener == "9Pool" && Util::getTime() < Time(3, 30) && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 12 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 0))
                enemyTransition = "LingRush";

            // Zergling all-in transitions
            if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 0) {
                if ((Players::ZvZ() && !enemyFE && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 2 && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 8) || (Util::getTime() < Time(4, 30) && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 14))
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
            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // Marine timing
                if (unit.getType() == Terran_Marine) {
                    if (rushArrivalTime == Time(999, 00) || unit.timeArrivesWhen() < rushArrivalTime)
                        rushArrivalTime = unit.timeArrivesWhen();
                }

                // Factory Research
                if (unit.getType() == Terran_Machine_Shop) {
                    if (unit.unit()->exists() && unit.unit()->isUpgrading())
                        vultureSpeed = true;
                }

                // FE Detection
                if (unit.getType() == Terran_Bunker && unit.getPosition().getDistance(Position(Terrain::getEnemyNatural())) < 320.0)
                    enemyFE = true;
            }

            // 2Rax
            if (rushArrivalTime < Time(2, 30)
                || firstFinishedTime[Terran_Barracks] < Time(1, 45)
                || (Util::getTime() < Time(2, 55) && Players::getTotalCount(PlayerState::Enemy, Terran_Barracks) >= 2)
                || (Util::getTime() < Time(2, 40) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 2)
                || (Util::getTime() < Time(2, 55) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 3)
                || (Util::getTime() < Time(3, 15) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 5)
                || (Util::getTime() < Time(3, 35) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 7)
                || (Util::getTime() < Time(3, 55) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 9)
                || (Util::getTime() < Time(4, 10) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 11))
                enemyBuild = "2Rax";

            // RaxCC
            if ((Util::getTime() < Time(3, 00) && enemyFE))
                enemyBuild = "RaxCC";

            // RaxFact
            if (Util::getTime() < Time(3, 00) && Players::getTotalCount(PlayerState::Enemy, Terran_Refinery) > 0)
                enemyBuild = "RaxFact";
        }

        void enemyTerranOpeners(PlayerInfo& player)
        {
            // Bio builds
            if (enemyBuild == "2Rax") {
                // Main, Proxy, Expand
            }

            // Mech builds
            if (enemyBuild == "RaxFact") {
                // Tank, Vulture, Goliath
            }

            // Expand builds
            if (enemyBuild == "RaxCC") {
                // idk
            }
        }

        void enemyTerranTransitions(PlayerInfo& player)
        {
            auto hasTanks = Players::getCurrentCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0 || Players::getCurrentCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) > 0;

            // Academy timing
            if (Players::ZvT() && enemyBuild == "2Rax") {

                if (enemyFE)
                    enemyTransition = "Expand";
                else if (rushArrivalTime < Time(3, 15) || firstFinishedTime[Terran_Barracks] < Time(2,15))
                    enemyTransition = "MarineRush";
                else if (Util::getTime() > Time(4, 30))
                    enemyTransition = "Academy";
            }

            // Factory builds
            if ((Players::getTotalCount(PlayerState::Enemy, Terran_Vulture_Spider_Mine) > 0 && Util::getTime() < Time(6, 00))
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Factory) >= 2 && vultureSpeed)
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 3 && Util::getTime() < Time(5, 30))
                || (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 5 && Util::getTime() < Time(6, 00))
                || (Broodwar->self()->getRace() == Races::Zerg && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) >= 1 && Util::getTime() < Time(5, 0)))
                enemyTransition = "2Fact";

            // Siege Expand
            if (Players::PvT()) {
                if ((Players::getCurrentCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0 && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) == 0)
                    || (enemyFE && Players::getCurrentCount(PlayerState::Enemy, Terran_Machine_Shop) > 0))
                    enemyTransition = "SiegeExpand";
            }

            // 1 Fact Tanks
            if (Players::ZvT()) {
                if ((hasTanks && Util::getTime() < Time(10, 00)) || (enemyFE && Players::getCurrentCount(PlayerState::Enemy, Terran_Machine_Shop) > 0 && Util::getTime() < Time(9, 00)))
                    enemyTransition = "1FactTanks";
            }
        }

        void enemyProtossBuilds(PlayerInfo& player)
        {
            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // Zealot timing
                if (unit.getType() == Protoss_Zealot) {
                    if (rushArrivalTime == Time(999, 00) || unit.timeArrivesWhen() < rushArrivalTime)
                        rushArrivalTime = unit.timeArrivesWhen();
                }

                // 1GateCore
                if (unit.getType() == Protoss_Assimilator) {
                    if (unit.timeCompletesWhen() < Time(2, 15))
                        enemyBuild = "1GateCore";
                }
                if (unit.getType() == Protoss_Cybernetics_Core) {
                    if (unit.timeCompletesWhen() < Time(3, 30))
                        enemyBuild = "1GateCore";
                }
                if (unit.getType() == Protoss_Dragoon) {
                    if (firstFinishedTime[unit.getType()] < Time(4, 10))
                        enemyBuild = "1GateCore";
                }

                // CannonRush
                if (unit.getType() == Protoss_Forge && Scouts::gotFullScout()) {
                    if (unit.getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 320.0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Nexus) <= 1)
                        enemyBuild = "CannonRush";
                }

                // FFE
                if ((unit.getType() == Protoss_Photon_Cannon || unit.getType() == Protoss_Forge)) {
                    if (unit.getPosition().getDistance((Position)Terrain::getEnemyNatural()) < 320.0 && Broodwar->getFrameCount() < 8000) {
                        enemyBuild = "FFE";
                        enemyFE = true;
                    }
                }

                // TODO: Check if this actually works
                if (unit.getType().isWorker() && unit.unit()->exists() && unit.unit()->getOrder() == Orders::HoldPosition)
                    blockedScout = true;

                // Proxy detection
                if (unit.getType() == Protoss_Gateway || unit.getType() == Protoss_Pylon) {
                    auto closestStation = BWEB::Stations::getClosestStation(unit.getTilePosition());
                    if (closestStation && !closestStation->isMain() && !closestStation->isNatural())
                        proxy = true;

                    if (Terrain::isInAllyTerritory(unit.getTilePosition())
                        || unit.getPosition().getDistance(mapBWEM.Center()) < 1280.0
                        || (BWEB::Map::getNaturalChoke() && unit.getPosition().getDistance((Position)BWEB::Map::getNaturalChoke()->Center()) < 480.0)) {

                        proxy = true;
                    }
                }
            }

            // 2Gate - Zealot estimation
            if ((Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) >= 5 && Util::getTime() < Time(4, 0))
                || (Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) >= 3 && Util::getTime() < Time(3, 30))
                || rushArrivalTime < Time(2, 50)) {
                enemyBuild = "2Gate";
            }

            // 2Gate - Gateway estimation
            if (Util::getTime() < Time(3, 30)) {
                if (Scouts::gotFullScout() && Util::getTime() < Time(3, 30) && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) == 0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Nexus) <= 1 && Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) > 0) {
                    enemyBuild = "2Gate";
                    proxy = true;
                }
                else if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Nexus) <= 1) {
                    enemyBuild = "2Gate";
                    proxy = false;
                }
            }
        }

        void enemyProtossOpeners(PlayerInfo& player)
        {
            // 2Gate Openers
            if (enemyBuild == "2Gate") {
                if (proxy || rushArrivalTime < Time(2, 50))
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
            // ZealotRush
            if (enemyBuild == "2Gate" && enemyOpener == "Proxy")
                enemyTransition = "ZealotRush";

            // 5ZealotExpand
            if (enemyFE && (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 || Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 5) && Util::getTime() < Time(4, 45)) {
                enemyTransition = "5ZealotExpand";
                proxy = false;
            }

            // DT
            if ((Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) > 0)
                || Players::getCurrentCount(PlayerState::Enemy, Protoss_Templar_Archives) >= 1
                || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 2 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 && Util::getTime() > Time(6, 45)))
                enemyTransition = "DT";

            // Corsair
            if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Stargate) > 0)
                enemyTransition = "Corsair";

            // 4Gate
            if (Players::PvP()) {
                if ((Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 7 && Util::getTime() < Time(6, 30))
                    || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 11 && Util::getTime() < Time(7, 15))
                    || (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 4 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 && Util::getTime() < Time(5, 30)))
                    enemyTransition = "4Gate";
            }
            if (Players::ZvP()) {
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 1)
                    enemyTransition = "4Gate";
            }

            // FFE transitions
            if (Players::ZvP() && enemyBuild == "FFE") {
                if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 4 && Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 1)
                    enemyTransition = "5GateGoon";
                else if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Stargate) >= 1)
                    enemyTransition = "NeoBisu";
                else if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1)
                    enemyTransition = "ZealotArchon";
            }
        }

        void checkEnemyRush()
        {
            // Rush builds are immediately aggresive builds
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self) >= 40 : Players::getSupply(PlayerState::Self) >= 80;
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
                holdChoke = !Players::ZvZ() && (BuildOrder::isFastExpand()
                    || BuildOrder::isWallNat()
                    || !rush
                    || Players::vT());
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

            // Proxy builds are built closer to me than the enemy
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self) >= 40 : Players::getSupply(PlayerState::Self) >= 80;
            proxy = !supplySafe && (enemyBuild == "CannonRush" || enemyBuild == "BunkerRush" || proxy);
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
            if (Broodwar->getFrameCount() - enemyFrame > 120 && enemyFrame != 0 && enemyTransition != "Unknown")
                return;

            if (enemyFrame == 0 && enemyTransition != "Unknown")
                enemyFrame = Broodwar->getFrameCount();

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

                    // TODO: We don't use this at the moment
                    player.setBuild(enemyBuild);
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
        globalReactions();
        updateEnemyBuild();
    }

    string getEnemyBuild() { return enemyBuild; }
    string getEnemyOpener() { return enemyOpener; }
    string getEnemyTransition() { return enemyTransition; }
    Position enemyScoutPosition() { return scoutPosition; }
    bool enemyAir() { return air; }
    bool enemyFastExpand() { return enemyFE; }
    bool enemyRush() { return rush; }
    bool needDetection() { return invis; }
    bool defendChoke() { return holdChoke; }
    bool enemyPossibleProxy() { return proxyCheck; }
    bool enemyProxy() { return proxy; }
    bool enemyGasSteal() { return gasSteal; }
    bool enemyScouted() { return enemyScout; }
    bool enemyBust() { return enemyBuild.find("Hydra") != string::npos; }
    bool enemyPressure() { return pressure; }
    bool enemyBlockedScout() { return blockedScout; }
    Time enemyArrivalTime() { return rushArrivalTime; }
}