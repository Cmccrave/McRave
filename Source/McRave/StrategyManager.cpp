#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Strategy {

    namespace {

        map <UnitType, double> unitScore;

        bool enemyFE = false;
        bool invis = false;
        bool rush = false;
        bool holdChoke = false;
        bool blockedScout = false;

        bool proxy = false;
        bool gasSteal = false;
        bool enemyScout = false;
        bool pressure = false;
        Position scoutPosition = Positions::Invalid;
        string enemyBuild = "Unknown";
        int poolFrame = INT_MAX;
        int rushFrame = INT_MAX;
        int enemyGas;
        int enemyFrame;

        int inboundScoutFrame;

        bool goonRange = false;
        bool vultureSpeed = false;

        void enemyZergBuilds(PlayerInfo& player)
        {
            auto hatchNum = Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery);
            auto poolNum = Players::getCurrentCount(PlayerState::Enemy, Zerg_Spawning_Pool);

            // 5 Hatch build detection
            if (Stations::getEnemyStations().size() >= 3 || (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) + Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) >= 4 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Drone) >= 14))
                enemyBuild = "5Hatch";

            for (auto &u : player.getUnits()) {
                UnitInfo &unit =*u;

                // Monitor gas intake or gas steal
                if (unit.getType().isRefinery() && unit.unit()->exists()) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()))
                        gasSteal = true;
                    else
                        enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
                }

                // Zergling build detection and pool timing
                if (unit.getType() == UnitTypes::Zerg_Zergling) {

                    // If this is our first time seeing a Zergling or it arrives earlier than we expected before
                    if (rushFrame == 0 || unit.frameArrivesWhen() < rushFrame)
                        rushFrame = unit.frameArrivesWhen();
                }

                if (unit.getType() == Zerg_Spawning_Pool) {

                    // If this is our first time seeing a Spawning Pool, see how soon it completes, if it's not completed
                    if (poolFrame == 0 && !unit.unit()->isCompleted())
                        poolFrame = unit.frameCompletesWhen();                    

                    // If it's already completed, see if we can determine the build based on drone count
                    if (poolFrame == 0 && unit.unit()->isCompleted()) {
                        if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Drone) <= 5)
                            enemyBuild = "4Pool";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Drone) <= 9)
                            enemyBuild = "9Pool";
                    }

                    if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 0) {
                        if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 1 && enemyGas < 148 && enemyGas >= 50 && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > 6)
                            enemyBuild = "9Pool";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 3 && enemyGas < 148 && enemyGas >= 100)
                            enemyBuild = "3HatchLing";
                    }
                }

                // Hydralisk/Lurker build detection
                else if (unit.getType() == Zerg_Hydralisk_Den) {
                    if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Spire) == 0) {
                        if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 3)
                            enemyBuild = "3HatchHydra";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 2)
                            enemyBuild = "2HatchHydra";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 1)
                            enemyBuild = "1HatchHydra";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 2)
                            enemyBuild = "2HatchLurker";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 1 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 0)
                            enemyBuild = "1HatchLurker";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) >= 4)
                            enemyBuild = "5Hatch";
                    }
                    else
                        enemyBuild = "Unknown";
                }

                // Mutalisk build detection
                else if (unit.getType() == Zerg_Spire || unit.getType() == Zerg_Lair) {
                    if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0) {
                        if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 3 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Drone) < 14)
                            enemyBuild = "3HatchMuta";
                        else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) + Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) == 2)
                            enemyBuild = "2HatchMuta";
                    }
                    else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) >= 4)
                        enemyBuild = "5Hatch";
                    else
                        enemyBuild = "Unknown";
                }
            }

            // 4Pool/9Pool Estimation
            if (enemyBuild == "Unknown") {
                if (rushFrame < 4200 || poolFrame < 2400)
                    enemyBuild = "4Pool";
                else if ((poolFrame <= 3000 || rushFrame < 4800) && Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) == 0 && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > 6)
                    enemyBuild = "9Pool";
            }

            // 2HatchMuta Estimation
            if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) > 0 && Util::getTime() < Time(4,00) && Util::getTime() > Time(3, 30) && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0)
                enemyBuild = "2HatchMuta";
        }

        void enemyTerranBuilds(PlayerInfo& player)
        {
            auto enemyStart = Terrain::getEnemyStartingPosition();
            auto enemyNat = Position(Terrain::getEnemyNatural());

            // Shallow two
            if (Broodwar->self()->getRace() == Races::Protoss && Players::getCurrentCount(PlayerState::Enemy, UnitTypes::Terran_Medic) >= 2)
                enemyBuild = "ShallowTwo";

            // Sparks
            if (Broodwar->self()->getRace() == Races::Zerg && Players::getCurrentCount(PlayerState::Enemy, Terran_Academy) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Terran_Engineering_Bay) >= 1)
                enemyBuild = "Sparks";

            // Joyo
            if (Broodwar->getFrameCount() < 9000 && Broodwar->self()->getRace() == Races::Protoss && !enemyFE && (Players::getCurrentCount(PlayerState::Enemy, UnitTypes::Terran_Machine_Shop) >= 2 || (Players::getCurrentCount(PlayerState::Enemy, Terran_Marine) >= 4 && Players::getCurrentCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) >= 3)))
                enemyBuild = "Joyo";

            // BBS
            if ((Players::getCurrentCount(PlayerState::Enemy, Terran_Barracks) >= 2 && Players::getCurrentCount(PlayerState::Enemy, Terran_Refinery) == 0) || (Players::getCurrentCount(PlayerState::Enemy, Terran_Marine) > 5 && Players::getCurrentCount(PlayerState::Enemy, Terran_Bunker) <= 0 && Broodwar->getFrameCount() < 6000))
                enemyBuild = "BBS";

            // 2Fact
            if ((Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture_Spider_Mine) > 0 && Broodwar->getFrameCount() < 9000) || (Players::getCurrentCount(PlayerState::Enemy, Terran_Factory) >= 2 && vultureSpeed))
                enemyBuild = "2Fact";

            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // Monitor gas intake or gas steal
                if (unit.getType().isRefinery() && unit.unit()->exists()) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()))
                        gasSteal = true;
                    else
                        enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
                }

                // Marine timing
                if (unit.getType() == Terran_Marine && Players::getCurrentCount(PlayerState::Enemy, Terran_Marine) >= 2) {
                    if (rushFrame == 0 || unit.frameArrivesWhen() < rushFrame) {
                        rushFrame = unit.frameArrivesWhen();
                        if (rushFrame < 3900)
                            enemyBuild = "BBS";
                    }
                }

                // TSiegeExpand
                if ((unit.getType() == Terran_Siege_Tank_Siege_Mode && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) == 0) || (unit.getType().isResourceDepot() && !Terrain::isStartingBase(unit.getTilePosition()) && Players::getCurrentCount(PlayerState::Enemy, Terran_Machine_Shop) > 0))
                    enemyBuild = "SiegeExpand";

                // Barracks Builds
                if (unit.getType() == Terran_Barracks) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()) || unit.getPosition().getDistance(mapBWEM.Center()) < 1280.0 || (BWEB::Map::getNaturalChoke() && unit.getPosition().getDistance((Position)BWEB::Map::getNaturalChoke()->Center()) < 320))
                        enemyBuild = "BBS";
                }

                // Factory Research
                if (unit.getType() == Terran_Machine_Shop) {
                    if (unit.unit()->exists() && unit.unit()->isUpgrading())
                        vultureSpeed = true;
                }

                // FE Detection
                if (unit.getType().isResourceDepot() && !Terrain::isStartingBase(unit.getTilePosition()))
                    enemyFE = true;
                if (unit.getType() == Terran_Bunker && unit.getPosition().getDistance(enemyStart) < unit.getPosition().getDistance(BWEB::Map::getMainPosition()) && unit.getPosition().getDistance(enemyNat) < 320.0)
                    enemyFE = true;
            }
        }

        void enemyProtossBuilds(PlayerInfo& player)
        {
            auto noGates = Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) == 0;
            auto noGas = Players::getCurrentCount(PlayerState::Enemy, Protoss_Assimilator) == 0;
            auto noExpand = Players::getCurrentCount(PlayerState::Enemy, Protoss_Nexus) <= 1 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Forge) <= 0;

            // Detect missing buildings as a potential 2Gate
            if (Scouts::gotFullScout() && Broodwar->getFrameCount() < 3600) {
                if (noGates && noExpand) {
                    enemyBuild = "2Gate";
                    proxy = true;
                }
                else if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Nexus) <= 1)
                    enemyBuild = "2Gate";
                else
                    enemyBuild = "Unknown";
            }

            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // Monitor the soonest the enemy will scout us
                if (Terrain::isInAllyTerritory(unit.getTilePosition()) || (inboundScoutFrame > 0 && inboundScoutFrame - Broodwar->getFrameCount() < 64))
                    enemyScout = true;
                if (unit.getType().isWorker() && inboundScoutFrame == 0)
                    inboundScoutFrame = unit.frameArrivesWhen();

                // Monitor gas intake or gas steal
                if (unit.getType().isRefinery() && unit.unit()->exists()) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()))
                        gasSteal = true;
                    else
                        enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
                }

                // CannonRush
                if (unit.getType() == Protoss_Forge) {
                    if (unit.getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 320.0 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) == 0)
                        enemyBuild = "CannonRush";
                    else if (enemyBuild == "CannonRush")
                        enemyBuild = "Unknown";
                }

                // FFE
                if ((unit.getType() == Protoss_Photon_Cannon || unit.getType() == Protoss_Forge) && Players::getCurrentCount(PlayerState::Enemy, Protoss_Robotics_Facility) == 0) {
                    if (unit.getPosition().getDistance((Position)Terrain::getEnemyNatural()) < 480.0 && Broodwar->getFrameCount() < 8000) {
                        enemyBuild = "FFE";
                        enemyFE = true;
                    }
                    else if (enemyBuild == "FFE") {
                        enemyBuild = "Unknown";
                        enemyFE = false;
                    }
                }

                // 2 Gate proxy estimation
                if (Broodwar->getFrameCount() < 5000 && unit.getType() == UnitTypes::Protoss_Zealot && Players::getCurrentCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) >= 2 && unit.getPosition().getDistance(Terrain::getDefendPosition()) < 640.0) {
                    enemyBuild = "2Gate";
                    proxy = true;
                }

                // 2 Gate Expand
                if (unit.getType() == Protoss_Nexus) {
                    if (!Terrain::isStartingBase(unit.getTilePosition()) && Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 && Broodwar->getFrameCount() < 5000)
                        enemyBuild = "2GateExpand";
                }

                if (unit.getType().isWorker() && unit.unit()->exists() && unit.unit()->getOrder() == Orders::HoldPosition)
                    blockedScout = true;

                // Proxy Builds
                if (unit.getType() == Protoss_Gateway || unit.getType() == Protoss_Pylon) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition())
                        || unit.getPosition().getDistance(mapBWEM.Center()) < 1280.0
                        || (BWEB::Map::getNaturalChoke() && unit.getPosition().getDistance((Position)BWEB::Map::getNaturalChoke()->Center()) < 480.0)) {

                        proxy = true;

                        if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 && Broodwar->getFrameCount() < 8000)
                            enemyBuild = "2Gate";
                    }
                }

                // 1GateCore
                if (unit.getType() == Protoss_Cybernetics_Core) {
                    if (unit.unit()->isUpgrading() && Util::getTime() < Time(5, 0))
                        goonRange = true;
                }

                // FE Detection
                if (unit.getType().isResourceDepot() && !Terrain::isStartingBase(unit.getTilePosition()))
                    enemyFE = true;
            }

            // 1Gate DT Estimation
            if ((Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) > 0)
                || Players::getCurrentCount(PlayerState::Enemy, Protoss_Templar_Archives) >= 1
                || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 2 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 && !goonRange && Util::getTime() > Time(6, 45)))
                enemyBuild = "1GateDT";

            // 4Gate Estimation
            if ((Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 7 && Util::getTime() < Time(6, 30))
                || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) >= 11 && Util::getTime() < Time(7, 15))
                || (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 4 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 && Util::getTime() < Time(5, 30)))
                enemyBuild = "4Gate";

            // 2Gate Estmation
            if (Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) >= 5 && Util::getTime() < Time(4, 0)
                || Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) >= 3 && Util::getTime() < Time(3, 30))
                enemyBuild = "2Gate";
        }

        void checkEnemyRush()
        {
            // Rush builds are immediately aggresive builds
            rush = Players::getSupply(PlayerState::Self) < 80 && (enemyBuild == "BBS" || enemyBuild == "2Gate" || enemyBuild == "5Pool" || enemyBuild == "4Pool");
        }

        void checkEnemyPressure()
        {
            // Pressure builds are delayed aggresive builds
            pressure = (enemyBuild == "4Gate" || enemyBuild == "9Pool" || enemyBuild == "Sparks" || enemyBuild == "2Fact");
        }

        void checkHoldChoke()
        {
            if (Broodwar->getGameType() == BWAPI::GameTypes::Use_Map_Settings) {
                holdChoke = true;
                return;
            }

            if (!holdChoke && Units::getImmThreat() > 0.0)
                holdChoke = false;
            else if (Players::getSupply(PlayerState::Self) <= 40)
                holdChoke = false;
            else {
                holdChoke = BuildOrder::isFastExpand()
                    || vis(Protoss_Dragoon) > 0
                    || com(Protoss_Shield_Battery) > 0
                    || BuildOrder::isWallNat()
                    || (BuildOrder::isHideTech() && !rush)
                    || Players::getSupply(PlayerState::Self) > 60
                    || Players::vT();
            }
        }

        void checkNeedDetection()
        {
            // DTs, Vultures, Lurkers
            invis = (Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar) >= 1 || (Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) > 0) || Players::getCurrentCount(PlayerState::Enemy, Protoss_Templar_Archives) >= 1)
                || (Players::getCurrentCount(PlayerState::Enemy, Terran_Ghost) >= 1 || Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) >= 4)
                || (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lurker) >= 1 || (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) <= 0))
                || (enemyBuild == "1HatchLurker" || enemyBuild == "2HatchLurker" || enemyBuild == "1GateDT");

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (com(Protoss_Observer) > 0)
                    invis = false;
            }
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (com(Terran_Comsat_Station) > 0)
                    invis = false;
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk) > 0 || Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0)
                    invis = true;
            }
        }

        void checkEnemyProxy()
        {
            // Proxy builds are built closer to me than the enemy
            proxy = proxy
                || (Players::getSupply(PlayerState::Self) < 80 && (enemyBuild == "CannonRush" || enemyBuild == "BunkerRush"));
        }

        void updateEnemyBuild()
        {
            if (Broodwar->getFrameCount() - enemyFrame > 2000 && enemyFrame != 0 && enemyBuild != "Unknown")
                return;

            if (enemyFrame == 0 && enemyBuild != "Unknown")
                enemyFrame = Broodwar->getFrameCount();

            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;

                if (player.isEnemy()) {

                    // Find where the enemy scout is
                    auto playersScout = Util::getClosestUnit(BWEB::Map::getMainPosition(), player.getPlayerState(), [&](auto &u) {
                        return u.getType().isWorker();
                    });
                    scoutPosition = playersScout ? playersScout->getPosition() : Positions::Invalid;

                    if (player.getCurrentRace() == Races::Zerg)
                        enemyZergBuilds(player);
                    else if (player.getCurrentRace() == Races::Protoss)
                        enemyProtossBuilds(player);
                    else if (player.getCurrentRace() == Races::Terran)
                        enemyTerranBuilds(player);

                    // HACK: Not needed for now anyways
                    player.setBuild(enemyBuild);
                }
            }
        }

        void updateProtossUnitScore(UnitType unit, int cnt)
        {
            double size = double(cnt) * double(unit.supplyRequired());

            switch (unit)
            {
            case Enum::Terran_Marine:
                unitScore[Protoss_Zealot]				+= size * 0.35;
                unitScore[Protoss_Dragoon]				+= size * 0.65;
                unitScore[Protoss_High_Templar]			+= size * 0.90;
                unitScore[Protoss_Dark_Templar]			+= size * 0.10;
                break;
            case Enum::Terran_Medic:
                unitScore[Protoss_Zealot]				+= size * 0.35;
                unitScore[Protoss_Dragoon]				+= size * 0.65;
                unitScore[Protoss_High_Templar]			+= size * 0.90;
                unitScore[Protoss_Dark_Templar]			+= size * 0.10;
                break;
            case Enum::Terran_Firebat:
                unitScore[Protoss_Zealot]				+= size * 0.35;
                unitScore[Protoss_Dragoon]				+= size * 0.65;
                unitScore[Protoss_High_Templar]			+= size * 0.90;
                unitScore[Protoss_Dark_Templar]			+= size * 0.10;
                break;
            case Enum::Terran_Vulture:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                unitScore[Protoss_Observer]				+= size * 0.70;
                unitScore[Protoss_Arbiter]				+= size * 0.15;
                break;
            case Enum::Terran_Goliath:
                unitScore[Protoss_Zealot]				+= size * 0.25;
                unitScore[Protoss_Dragoon]				+= size * 0.75;
                unitScore[Protoss_Arbiter]				+= size * 0.70;
                unitScore[Protoss_High_Templar]			+= size * 0.30;
                break;
            case Enum::Terran_Siege_Tank_Siege_Mode:
                unitScore[Protoss_Zealot]				+= size * 0.40;
                unitScore[Protoss_Dragoon]				+= size * 0.60;
                unitScore[Protoss_Arbiter]				+= size * 0.70;
                unitScore[Protoss_High_Templar]			+= size * 0.30;
                break;
            case Enum::Terran_Siege_Tank_Tank_Mode:
                unitScore[Protoss_Zealot]				+= size * 0.40;
                unitScore[Protoss_Dragoon]				+= size * 0.60;
                unitScore[Protoss_Arbiter]				+= size * 0.70;
                unitScore[Protoss_High_Templar]			+= size * 0.30;
                break;
            case Enum::Terran_Wraith:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                break;
            case Enum::Terran_Science_Vessel:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                break;
            case Enum::Terran_Battlecruiser:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                break;
            case Enum::Terran_Valkyrie:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                break;

            case Enum::Zerg_Zergling:
                unitScore[Protoss_Zealot]				+= size * 0.85;
                unitScore[Protoss_Dragoon]				+= size * 0.15;
                unitScore[Protoss_Corsair]				+= size * 0.60;
                unitScore[Protoss_High_Templar]			+= size * 0.30;
                unitScore[Protoss_Archon]				+= size * 0.30;
                unitScore[Protoss_Dark_Templar]			+= size * 0.10;
                break;
            case Enum::Zerg_Hydralisk:
                unitScore[Protoss_Zealot]				+= size * 0.75;
                unitScore[Protoss_Dragoon]				+= size * 0.25;
                unitScore[Protoss_High_Templar]			+= size * 0.80;
                unitScore[Protoss_Dark_Templar]			+= size * 0.20;
                unitScore[Protoss_Reaver]				+= size * 0.30;
                break;
            case Enum::Zerg_Lurker:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                unitScore[Protoss_High_Templar]			+= size * 0.70;
                unitScore[Protoss_Observer]				+= size * 1.00;
                unitScore[Protoss_Reaver]				+= size * 0.30;
                break;
            case Enum::Zerg_Ultralisk:
                unitScore[Protoss_Zealot]				+= size * 0.25;
                unitScore[Protoss_Dragoon]				+= size * 0.75;
                unitScore[Protoss_Reaver]				+= size * 0.80;
                unitScore[Protoss_Dark_Templar]			+= size * 0.20;
                break;
            case Enum::Zerg_Mutalisk:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                unitScore[Protoss_Corsair]				+= size * 1.00;
                break;
            case Enum::Zerg_Guardian:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                unitScore[Protoss_Corsair]				+= size * 1.00;
                break;
            case Enum::Zerg_Devourer:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                break;
            case Enum::Zerg_Defiler:
                unitScore[Protoss_Zealot]				+= size * 1.00;
                unitScore[Protoss_Dark_Templar]			+= size * 0.10;
                unitScore[Protoss_Reaver]				+= size * 0.90;
                break;

            case Enum::Protoss_Zealot:
                unitScore[Protoss_Zealot]				+= size * 0.05;
                unitScore[Protoss_Dragoon]				+= size * 0.95;
                unitScore[Protoss_Reaver]				+= size * 0.90;
                unitScore[Protoss_Dark_Templar]			+= size * 0.05;
                break;
            case Enum::Protoss_Dragoon:
                unitScore[Protoss_Zealot]				+= size * 0.15;
                unitScore[Protoss_Dragoon]				+= size * 0.85;
                unitScore[Protoss_Reaver]				+= size * 0.60;
                unitScore[Protoss_High_Templar]			+= size * 0.30;
                unitScore[Protoss_Dark_Templar]			+= size * 0.05;
                break;
            case Enum::Protoss_High_Templar:
                unitScore[Protoss_Zealot]				+= size * 0.50;
                unitScore[Protoss_Dragoon]				+= size * 0.50;
                unitScore[Protoss_High_Templar]			+= size * 1.00;
                break;
            case Enum::Protoss_Dark_Templar:
                unitScore[Protoss_Zealot]				+= size * 0.50;
                unitScore[Protoss_Dragoon]				+= size * 0.50;
                unitScore[Protoss_Reaver]				+= size * 1.00;
                unitScore[Protoss_Observer]				+= size * 1.00;
                break;
            case Enum::Protoss_Reaver:
                unitScore[Protoss_Zealot]				+= size * 0.50;
                unitScore[Protoss_Dragoon]				+= size * 0.50;
                unitScore[Protoss_Reaver]				+= size * 1.00;
                break;
            case Enum::Protoss_Archon:
                unitScore[Protoss_Zealot]				+= size * 0.50;
                unitScore[Protoss_Dragoon]				+= size * 0.50;
                unitScore[Protoss_High_Templar]			+= size * 1.00;
                break;
            case Enum::Protoss_Dark_Archon:
                unitScore[Protoss_Zealot]				+= size * 0.50;
                unitScore[Protoss_Dragoon]				+= size * 0.50;
                unitScore[Protoss_High_Templar]			+= size * 1.00;
                break;
            case Enum::Protoss_Scout:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                if (Terrain::isIslandMap())
                    unitScore[Protoss_Scout]			+= size * 1.00;
                break;
            case Enum::Protoss_Carrier:
                unitScore[Protoss_Dragoon]				+= size * 1.00;

                if (Terrain::isIslandMap())
                    unitScore[Protoss_Scout]			+= size * 1.00;
                break;
            case Enum::Protoss_Arbiter:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                unitScore[Protoss_High_Templar]			+= size * 1.00;
                break;
            case Enum::Protoss_Corsair:
                unitScore[Protoss_Dragoon]				+= size * 1.00;
                unitScore[Protoss_High_Templar]			+= size * 1.00;
                break;
            case Enum::Protoss_Photon_Cannon:
                unitScore[Protoss_Dragoon]				+= 1.00;
                unitScore[Protoss_Zealot]				+= 0.50;
                break;
            }
        }

        void updateTerranUnitScore(UnitType unit, int count)
        {
            //for (auto &t : unitScore)
            //	if (!BuildOrder::isUnitUnlocked(t.first))
            //		t.second = 0.0;


            //for (auto &t : BuildOrder::getUnlockedList()) {
            //	UnitInfo dummy;
            //	dummy.setType(t);
            //	dummy.setPlayer(Broodwar->self());
            //	dummy.setGroundRange(Util::groundRange(dummy));
            //	dummy.setAirRange(Util::airRange(dummy));
            //	dummy.setGroundDamage(Util::groundDamage(dummy));
            //	dummy.setAirDamage(Util::airDamage(dummy));
            //	dummy.setSpeed(Util::speed(dummy));

            //	double dps = unit.isFlyer() ? Util::airDPS(dummy) : Util::groundDPS(dummy);
            //	if (t == Terran_Medic)
            //		dps = 0.775;

            //	if (t == Terran_Dropship)
            //		unitScore[t] = 10.0;

            //	else if (unitScore[t] <= 0.0)
            //		unitScore[t] += (dps*count / max(1.0, (double)vis(t)));
            //	else
            //		unitScore[t] = (unitScore[t] * (9999.0 / 10000.0)) + ((dps * (double)count) / (10000.0 * max(1.0, (double)vis(t))));
            //}
        }

        void updateMadMixScore()
        {

            using namespace UnitTypes;
            vector<UnitType> allUnits;
            if (Broodwar->self()->getRace() == Races::Protoss) {

            }
            else if (Broodwar->self()->getRace() == Races::Terran)
                allUnits.insert(allUnits.end(), { Terran_Marine, Terran_Medic, Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Goliath, Terran_Wraith, Terran_Dropship, Terran_Science_Vessel, Terran_Valkyrie });
            else if (Broodwar->self()->getRace() == Races::Zerg)
                allUnits.insert(allUnits.end(), { Zerg_Zergling, Zerg_Hydralisk, Zerg_Lurker, Zerg_Mutalisk, Zerg_Scourge });

            // TODO: tier 1,2,3 vectors
            if (Broodwar->getFrameCount() > 20000) {
                if (Broodwar->self()->getRace() == Races::Terran) {
                    allUnits.push_back(Terran_Battlecruiser);
                    allUnits.push_back(Terran_Ghost);
                }
                else if (Broodwar->self()->getRace() == Races::Zerg) {
                    allUnits.push_back(Zerg_Ultralisk);
                    allUnits.push_back(Zerg_Guardian);
                    allUnits.push_back(Zerg_Devourer);
                }
            }

            if (Broodwar->self()->getRace() == Races::Zerg) {
                auto myStrength = Players::getStrength(PlayerState::Self);
                auto enemyStrength = Players::getStrength(PlayerState::Enemy);
                unitScore[Zerg_Drone] = clamp((1.0 + myStrength.groundToGround + myStrength.groundDefense) / (1.0 + enemyStrength.groundToGround - enemyStrength.groundDefense), 1.0, 100.0);

                if (Units::getMyRoleCount(Role::Worker) > Units::getMyRoleCount(Role::Combat) && !BuildOrder::isOpener())
                    unitScore[Zerg_Drone] = 0.0;
            }

            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;
                auto enemyType = unit.getType();

                for (auto &myType : allUnits) {

                    UnitInfo dummy;
                    dummy.createDummy(myType);

                    if (!unit.getPosition().isValid() || myType.isBuilding() || myType.isSpell())
                        continue;

                    double myDPS = enemyType.isFlyer() ? Math::airDPS(dummy) : Math::groundDPS(dummy);
                    double enemyDPS = myType.isFlyer() ? Math::airDPS(unit) : Math::groundDPS(unit);

                    if (unit.getType() == Terran_Medic)
                        enemyDPS = 0.775;

                    double overallMatchup = enemyDPS > 0.0 ? (myDPS, myDPS / enemyDPS) : myDPS;
                    double distTotal = Terrain::getEnemyStartingPosition().isValid() ? BWEB::Map::getMainPosition().getDistance(Terrain::getEnemyStartingPosition()) : 1.0;
                    double distUnit = Terrain::getEnemyStartingPosition().isValid() ? unit.getPosition().getDistance(BWEB::Map::getMainPosition()) / distTotal : 1.0;

                    if (distUnit == 0.0)
                        distUnit = 0.1;
                    if (myType == UnitTypes::Zerg_Zergling)
                        myDPS /= 2;

                    double visible = max(1.0, (double(vis(myType))));

                    if (unitScore[myType] <= 0.0)
                        unitScore[myType] += (overallMatchup / max(1.0, visible * distUnit));
                    else
                        unitScore[myType] = (unitScore[myType] * (999.0 / 1000.0)) + (overallMatchup / (1000.0 * visible * distUnit));
                }
            }
        }

        void updateScoring()
        {
            // Reset unit score for toss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                for (auto &unit : unitScore)
                    unit.second = 0;
            }

            // Unit score based off enemy composition	
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;
                if (unit.getType().isBuilding())
                    continue;

                // For each type, add a score to production based on the unit count divided by our current unit count
                if (Broodwar->self()->getRace() == Races::Protoss)
                    updateProtossUnitScore(unit.getType(), 1);
            }

            bool MadMix = Broodwar->self()->getRace() != Races::Protoss;
            if (MadMix)
                updateMadMixScore();

            if (Broodwar->self()->getRace() == Races::Terran)
                unitScore[Terran_Medic] = unitScore[Terran_Marine];

            if (Broodwar->self()->getRace() == Races::Protoss) {

                for (auto &t : unitScore)
                    t.second = log(t.second);

                unitScore[Protoss_Shuttle] = getUnitScore(Protoss_Reaver);

                if (Players::vP() && BuildOrder::isTechUnit(Protoss_High_Templar)) {
                    unitScore[Protoss_Zealot] = unitScore[Protoss_Dragoon];
                    unitScore[Protoss_Archon] = unitScore[Protoss_Dragoon];
                }

                // DTs worth nothing if enemy has fast Observer or Cannons
                if ((Players::getCurrentCount(PlayerState::Enemy, Protoss_Observer) > 0 || Players::getCurrentCount(PlayerState::Enemy, Protoss_Photon_Cannon) > 0) && Broodwar->getFrameCount() < 15000)
                    unitScore[Protoss_Dark_Templar] = 0.0;
            }
        }

        void updateSituationalBehaviour()
        {
            checkNeedDetection();
            checkEnemyPressure();
            checkEnemyProxy();
            checkEnemyRush();
            checkHoldChoke();
        }
    }

    void onFrame()
    {
        updateSituationalBehaviour();
        updateEnemyBuild();
        updateScoring();
    }

    double getUnitScore(UnitType unit)
    {
        map<UnitType, double>::iterator itr = unitScore.find(unit);
        if (itr != unitScore.end())
            return itr->second;
        return 0.0;
    }

    UnitType getHighestUnitScore()
    {
        double best = 0.0;
        UnitType bestType = None;
        for (auto &unit : unitScore) {
            if (BuildOrder::isUnitUnlocked(unit.first) && unit.second > best) {
                best = unit.second, bestType = unit.first;
            }
        }

        if (bestType == None && Broodwar->self()->getRace() == Races::Zerg)
            return Zerg_Drone;
        return bestType;
    }

    string getEnemyBuild() { return enemyBuild; }
    Position enemyScoutPosition() { return scoutPosition; }
    bool enemyFastExpand() { return enemyFE; }
    bool enemyRush() { return rush; }
    bool needDetection() { return invis; }
    bool defendChoke() { return holdChoke; }
    bool enemyProxy() { return proxy; }
    bool enemyGasSteal() { return gasSteal; }
    bool enemyScouted() { return enemyScout; }
    bool enemyBust() { return enemyBuild.find("Hydra") != string::npos; }
    bool enemyPressure() { return pressure; }
    bool enemyBlockedScout() { return blockedScout; }
    int enemyArrivalFrame() { return rushFrame; }
    map <UnitType, double>& getUnitScores() { return unitScore; }
}