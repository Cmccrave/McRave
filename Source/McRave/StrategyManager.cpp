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
            if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Lair) > 0 && Util::getTime() < Time(4, 00) && Util::getTime() > Time(3, 30) && Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0)
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
            if ((Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture_Spider_Mine) > 0 && Broodwar->getFrameCount() < 9000)
                || (Players::getCurrentCount(PlayerState::Enemy, Terran_Factory) >= 2 && vultureSpeed)
                || (Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) >= 3 && Util::getTime() < Time(5,0))
                || (Broodwar->self()->getRace() == Races::Zerg && Players::getCurrentCount(PlayerState::Enemy, Terran_Vulture) >= 1 && Util::getTime() < Time(4, 30)))
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
                    if (unit.getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 320.0 && noGates && noExpand)
                        enemyBuild = "CannonRush";
                    else if (enemyBuild == "CannonRush")
                        enemyBuild = "Unknown";
                }

                // FFE
                if ((unit.getType() == Protoss_Photon_Cannon || unit.getType() == Protoss_Forge)) {
                    if (unit.getPosition().getDistance((Position)Terrain::getEnemyNatural()) < 480.0 && Broodwar->getFrameCount() < 8000) {
                        enemyBuild = "FFE";
                        enemyFE = true;
                    }
                    else if (enemyBuild == "FFE") {
                        enemyBuild = "Unknown";
                        enemyFE = false;
                    }
                }

                if (unit.getType().isWorker() && unit.unit()->exists() && unit.unit()->getOrder() == Orders::HoldPosition)
                    blockedScout = true;

                // Proxy detection
                if (unit.getType() == Protoss_Gateway || unit.getType() == Protoss_Pylon) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition())
                        || unit.getPosition().getDistance(mapBWEM.Center()) < 1280.0
                        || (BWEB::Map::getNaturalChoke() && unit.getPosition().getDistance((Position)BWEB::Map::getNaturalChoke()->Center()) < 480.0)) {

                        proxy = true;
                    }
                }

                // FE Detection
                if (unit.getType().isResourceDepot() && !Terrain::isStartingBase(unit.getTilePosition()))
                    enemyFE = true;
            }

            // 2Gate Detection
            if (Util::getTime() < Time(3, 30)) {
                if (Scouts::gotFullScout() && Util::getTime() < Time(3, 30) && noGates && noExpand && Players::getTotalCount(PlayerState::Enemy, UnitTypes::Protoss_Zealot) > 0) {
                    enemyBuild = "2Gate";
                    proxy = true;
                }
                else if (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 && noExpand) {
                    enemyBuild = "2Gate";
                    proxy = false;
                }
            }

            // 2GateExpand Detection
            if (enemyFE && (Players::getCurrentCount(PlayerState::Enemy, Protoss_Gateway) >= 2 || Players::getTotalCount(PlayerState::Enemy, Protoss_Zealot) >= 5) && Util::getTime() < Time(4, 45)) {
                enemyBuild = "2GateExpand";
                proxy = false;
            }

            // 1Gate DT Estimation
            if ((Players::getCurrentCount(PlayerState::Enemy, Protoss_Citadel_of_Adun) >= 1 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) > 0)
                || Players::getCurrentCount(PlayerState::Enemy, Protoss_Templar_Archives) >= 1
                || (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) < 2 && Players::getCurrentCount(PlayerState::Enemy, Protoss_Cybernetics_Core) >= 1 && Util::getTime() > Time(6, 45)))
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
            auto supplySafe = Broodwar->self()->getRace() == Races::Zerg ? Players::getSupply(PlayerState::Self) < 40 : Players::getSupply(PlayerState::Self) < 80;

            // Rush builds are immediately aggresive builds
            rush = supplySafe && (enemyBuild == "BBS" || enemyBuild == "2Gate" || enemyBuild == "5Pool" || enemyBuild == "4Pool");
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

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                holdChoke = BuildOrder::isFastExpand()
                    || vis(Protoss_Dragoon) > 0
                    || com(Protoss_Shield_Battery) > 0
                    || BuildOrder::isWallNat()
                    || (BuildOrder::isHideTech() && !rush)
                    || Players::getSupply(PlayerState::Self) > 60
                    || Players::vT();

                if (Players::getSupply(PlayerState::Self) <= 40)
                    holdChoke = false;
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                holdChoke = !rush || BuildOrder::isFastExpand();
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
            proxy = proxy
                || (Players::getSupply(PlayerState::Self) < 80 && (enemyBuild == "CannonRush" || enemyBuild == "BunkerRush"));
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
    int enemyArrivalFrame() { return rushFrame; }
}