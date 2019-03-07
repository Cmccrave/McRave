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

        bool proxy = false;
        bool gasSteal = false;
        bool enemyScout = false;
        bool pressure = false;
        string enemyBuild = "Unknown";
        int poolFrame, rushFrame;
        int enemyGas;
        int enemyFrame;

        int inboundScoutFrame;

        bool goonRange = false;
        bool vultureSpeed = false;

        int frameArrivesWhen(UnitInfo& unit) {
            return Broodwar->getFrameCount() + int(unit.getPosition().getDistance(Terrain::getDefendPosition()) / unit.getSpeed());
        }

        int frameCompletesWhen(UnitInfo& unit) {
            return Broodwar->getFrameCount() + int((1 - unit.getPercentHealth()) * unit.getType().buildTime());
        }

        void enemyZergBuilds(PlayerInfo& player)
        {
            auto hatchNum = Units::getEnemyCount(Zerg_Hatchery);
            auto poolNum = Units::getEnemyCount(Zerg_Spawning_Pool);

            // 5 Hatch build detection
            if (Stations::getEnemyStations().size() >= 3 || (Units::getEnemyCount(Zerg_Hatchery) + Units::getEnemyCount(Zerg_Lair) >= 4 && Units::getEnemyCount(Zerg_Drone) >= 14))
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
                    if (rushFrame == 0 || frameArrivesWhen(unit) < rushFrame) {
                        rushFrame = frameArrivesWhen(unit);
                        if (enemyBuild == "Unknown") {
                            if (rushFrame < 4200)
                                enemyBuild = "4Pool";
                            else if (rushFrame < 4800)
                                enemyBuild = "9Pool";
                        }
                    }
                }

                if (unit.getType() == Zerg_Spawning_Pool) {

                    // If this is our first time seeing a Spawning Pool, see how soon it completes
                    if (poolFrame == 0) {
                        poolFrame = frameCompletesWhen(unit);
                        if (enemyBuild == "Unknown") {
                            if (poolFrame <= 2400)
                                enemyBuild = "4Pool";
                            else if (poolFrame <= 3000)
                                enemyBuild = "9Pool";
                        }
                    }

                    if (Units::getEnemyCount(Zerg_Spire) == 0 && Units::getEnemyCount(Zerg_Hydralisk_Den) == 0 && Units::getEnemyCount(Zerg_Lair) == 0) {
                        if (Units::getEnemyCount(Zerg_Hatchery) == 1 && enemyGas < 148 && enemyGas >= 50 && Units::getEnemyCount(Zerg_Zergling) >= 8)
                            enemyBuild = "9Pool";
                        else if (Units::getEnemyCount(Zerg_Hatchery) == 3 && enemyGas < 148 && enemyGas >= 100)
                            enemyBuild = "3HatchLing";
                    }
                }

                // Hydralisk/Lurker build detection
                else if (unit.getType() == Zerg_Hydralisk_Den) {
                    if (Units::getEnemyCount(Zerg_Spire) == 0) {
                        if (Units::getEnemyCount(Zerg_Hatchery) == 3)
                            enemyBuild = "3HatchHydra";
                        else if (Units::getEnemyCount(Zerg_Hatchery) == 2)
                            enemyBuild = "2HatchHydra";
                        else if (Units::getEnemyCount(Zerg_Hatchery) == 1)
                            enemyBuild = "1HatchHydra";
                        else if (Units::getEnemyCount(Zerg_Lair) + Units::getEnemyCount(Zerg_Hatchery) == 2)
                            enemyBuild = "2HatchLurker";
                        else if (Units::getEnemyCount(Zerg_Lair) == 1 && Units::getEnemyCount(Zerg_Hatchery) == 0)
                            enemyBuild = "1HatchLurker";
                        else if (Units::getEnemyCount(Zerg_Hatchery) >= 4)
                            enemyBuild = "5Hatch";
                    }
                    else
                        enemyBuild = "Unknown";
                }

                // Mutalisk build detection
                else if (unit.getType() == Zerg_Spire || unit.getType() == Zerg_Lair) {
                    if (Units::getEnemyCount(Zerg_Hydralisk_Den) == 0) {
                        if (Units::getEnemyCount(Zerg_Lair) + Units::getEnemyCount(Zerg_Hatchery) == 3 && Units::getEnemyCount(Zerg_Drone) < 14)
                            enemyBuild = "3HatchMuta";
                        else if (Units::getEnemyCount(Zerg_Lair) + Units::getEnemyCount(Zerg_Hatchery) == 2)
                            enemyBuild = "2HatchMuta";
                    }
                    else if (Units::getEnemyCount(Zerg_Hatchery) >= 4)
                        enemyBuild = "5Hatch";
                    else
                        enemyBuild = "Unknown";
                }
            }
        }

        void enemyTerranBuilds(PlayerInfo& player)
        {
            auto enemyStart = Terrain::getEnemyStartingPosition();
            auto enemyNat = Position(Terrain::getEnemyNatural());

            // Shallow two
            if (Broodwar->self()->getRace() == Races::Protoss && Units::getEnemyCount(UnitTypes::Terran_Medic) >= 2)
                enemyBuild = "ShallowTwo";

            // Sparks
            if (Broodwar->self()->getRace() == Races::Zerg && Units::getEnemyCount(Terran_Academy) >= 1 && Units::getEnemyCount(Terran_Engineering_Bay) >= 1)
                enemyBuild = "Sparks";

            // Joyo
            if (Broodwar->getFrameCount() < 9000 && Broodwar->self()->getRace() == Races::Protoss && !enemyFE && (Units::getEnemyCount(UnitTypes::Terran_Machine_Shop) >= 2 || (Units::getEnemyCount(Terran_Marine) >= 4 && Units::getEnemyCount(Terran_Siege_Tank_Tank_Mode) >= 3)))
                enemyBuild = "Joyo";

            // BBS
            if ((Units::getEnemyCount(Terran_Barracks) >= 2 && Units::getEnemyCount(Terran_Refinery) == 0) || (Units::getEnemyCount(Terran_Marine) > 5 && Units::getEnemyCount(Terran_Bunker) <= 0 && Broodwar->getFrameCount() < 6000))
                enemyBuild = "BBS";

            // 2Fact
            if ((Units::getEnemyCount(Terran_Vulture_Spider_Mine) > 0 && Broodwar->getFrameCount() < 9000) || (Units::getEnemyCount(Terran_Factory) >= 2 && vultureSpeed))
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
                if (unit.getType() == Terran_Marine) {
                    if (rushFrame == 0 || frameArrivesWhen(unit) < rushFrame) {
                        rushFrame = frameArrivesWhen(unit);
                        if (rushFrame < 3900)
                            enemyBuild = "BBS";
                    }
                }

                // TSiegeExpand
                if ((unit.getType() == Terran_Siege_Tank_Siege_Mode && Units::getEnemyCount(Terran_Vulture) == 0) || (unit.getType().isResourceDepot() && !Terrain::isStartingBase(unit.getTilePosition()) && Units::getEnemyCount(Terran_Machine_Shop) > 0))
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
                if (unit.getType() == Terran_Bunker && unit.getPosition().getDistance(enemyStart) < unit.getPosition().getDistance(Terrain::getPlayerStartingPosition()) && unit.getPosition().getDistance(enemyNat) < 320.0)
                    enemyFE = true;
            }
        }

        void enemyProtossBuilds(PlayerInfo& player)
        {
            auto noGates = Units::getEnemyCount(Protoss_Gateway) == 0;
            auto noGas = Units::getEnemyCount(Protoss_Assimilator) == 0;
            auto noExpand = Units::getEnemyCount(Protoss_Nexus) <= 1 && Units::getEnemyCount(Protoss_Forge) <= 0;

            // Detect missing buildings as a potential 2Gate
            if (Terrain::getEnemyStartingPosition().isValid() && Broodwar->getFrameCount() > 3000 && Broodwar->isExplored((TilePosition)Terrain::getEnemyStartingPosition())) {

                // Check 2 corners scouted
                auto topLeft = TilePosition(Util::clipToMap(Terrain::getEnemyStartingPosition() - Position(160, 160)));
                auto botRight = TilePosition(Util::clipToMap(Terrain::getEnemyStartingPosition() + Position(160, 160) + Position(128, 96)));
                auto fullScout = Grids::lastVisibleFrame(topLeft) > 0 && Grids::lastVisibleFrame(botRight) > 0;
                auto maybeProxy = noGates && noExpand;

                if (fullScout) {
                    if (maybeProxy) {
                        enemyBuild = "2Gate";
                        proxy = true;
                    }
                    else if (Units::getEnemyCount(Protoss_Gateway) >= 2 && Units::getEnemyCount(Protoss_Nexus) <= 1 && Units::getEnemyCount(Protoss_Cybernetics_Core) <= 0 && Units::getEnemyCount(Protoss_Dragoon) <= 0)
                        enemyBuild = "2Gate";
                    else if (enemyBuild == "2Gate") {
                        enemyBuild = "Unknown";
                        proxy = false;
                    }
                }
            }

            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                if (Terrain::isInAllyTerritory(unit.getTilePosition()) || (inboundScoutFrame > 0 && inboundScoutFrame - Broodwar->getFrameCount() < 64))
                    enemyScout = true;
                if (unit.getType().isWorker() && inboundScoutFrame == 0) {
                    auto dist = unit.getPosition().getDistance(BWEB::Map::getMainPosition());
                    inboundScoutFrame = Broodwar->getFrameCount() + int(dist / unit.getType().topSpeed());
                }

                // Monitor gas intake or gas steal
                if (unit.getType().isRefinery() && unit.unit()->exists()) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()))
                        gasSteal = true;
                    else
                        enemyGas = unit.unit()->getInitialResources() - unit.unit()->getResources();
                }

                // CannonRush
                if (unit.getType() == Protoss_Forge) {
                    if (unit.getPosition().getDistance(Terrain::getEnemyStartingPosition()) < 320.0 && Units::getEnemyCount(Protoss_Gateway) == 0)
                        enemyBuild = "CannonRush";
                    else if (enemyBuild == "CannonRush")
                        enemyBuild = "Unknown";
                }

                // FFE
                if (unit.getType() == Protoss_Photon_Cannon && Units::getEnemyCount(Protoss_Robotics_Facility) == 0) {
                    if (unit.getPosition().getDistance((Position)Terrain::getEnemyNatural()) < 320.0) {
                        enemyBuild = "FFE";
                        enemyFE = true;
                    }
                    else if (enemyBuild == "FFE") {
                        enemyBuild = "Unknown";
                        enemyFE = false;
                    }
                }

                // 2 Gate proxy estimation
                if (Broodwar->getFrameCount() < 5000 && unit.getType() == UnitTypes::Protoss_Zealot && Units::getEnemyCount(UnitTypes::Protoss_Zealot) >= 2 && unit.getPosition().getDistance(Terrain::getDefendPosition()) < 640.0) {
                    enemyBuild = "2Gate";
                    proxy = true;
                }

                // 2 Gate Expand
                if (unit.getType() == Protoss_Nexus) {
                    if (!Terrain::isStartingBase(unit.getTilePosition()) && Units::getEnemyCount(Protoss_Gateway) >= 2)
                        enemyBuild = "2GateExpand";
                }

                // Proxy Builds
                if (unit.getType() == Protoss_Gateway || unit.getType() == Protoss_Pylon) {
                    if (Terrain::isInAllyTerritory(unit.getTilePosition()) || unit.getPosition().getDistance(mapBWEM.Center()) < 1280.0 || (BWEB::Map::getNaturalChoke() && unit.getPosition().getDistance((Position)BWEB::Map::getNaturalChoke()->Center()) < 480.0)) {
                        proxy = true;

                        if (Units::getEnemyCount(Protoss_Gateway) >= 2)
                            enemyBuild = "2Gate";
                    }
                }

                // 1GateCore
                if (unit.getType() == Protoss_Cybernetics_Core) {
                    if (unit.unit()->isUpgrading())
                        goonRange = true;

                    if (Units::getEnemyCount(Protoss_Robotics_Facility) >= 1 && Units::getEnemyCount(Protoss_Gateway) <= 1)
                        enemyBuild = "1GateRobo";
                    else if (Units::getEnemyCount(Protoss_Gateway) >= 4)
                        enemyBuild = "4Gate";
                    else if ((Units::getEnemyCount(Protoss_Citadel_of_Adun) >= 1 && Units::getEnemyCount(Protoss_Zealot) > 0) || Units::getEnemyCount(Protoss_Templar_Archives) >= 1 || (enemyBuild == "Unknown" && !goonRange && Units::getEnemyCount(Protoss_Dragoon) < 2 && Units::getSupply() > 80))
                        enemyBuild = "1GateDT";
                }

                // Pressure checking
                if (Broodwar->self()->visibleUnitCount(Protoss_Gateway) >= 3)
                    pressure = true;

                // Proxy Detection
                if (unit.getType() == Protoss_Pylon && unit.getPosition().getDistance(Terrain::getPlayerStartingPosition()) < 960.0)
                    proxy = true;

                // FE Detection
                if (unit.getType().isResourceDepot() && !Terrain::isStartingBase(unit.getTilePosition()))
                    enemyFE = true;

            }
        }

        void checkEnemyRush()
        {
            // Rush builds are immediately aggresive builds
            rush = Units::getSupply() < 80 && (enemyBuild == "BBS" || enemyBuild == "2Gate" || enemyBuild == "5Pool" || enemyBuild == "4Pool");
        }

        void checkEnemyPressure()
        {
            // Pressure builds are delayed aggresive builds
            pressure = (enemyBuild == "4Gate" || enemyBuild == "9Pool" || enemyBuild == "Sparks" || enemyBuild == "2Fact");
        }

        void checkHoldChoke()
        {
            if (!holdChoke && Units::getImmThreat() > 0.0)
                holdChoke = false;
            else {
                holdChoke = BuildOrder::isFastExpand()
                    || vis(Protoss_Dragoon) > 0
                    || com(Protoss_Shield_Battery) > 0
                    || BuildOrder::isWallNat()
                    || (BuildOrder::isHideTech() && !rush)
                    || Units::getSupply() > 60
                    || Players::vT();
            }
        }

        void checkNeedDetection()
        {
            // DTs, Vultures, Lurkers
            invis = (Units::getEnemyCount(Protoss_Dark_Templar) >= 1 || (Units::getEnemyCount(Protoss_Citadel_of_Adun) >= 1 && Units::getEnemyCount(Protoss_Zealot) > 0) || Units::getEnemyCount(Protoss_Templar_Archives) >= 1)
                || (enemyBuild == "1GateDT")
                || (Units::getEnemyCount(Terran_Ghost) >= 1 || Units::getEnemyCount(Terran_Vulture) >= 4)
                || (Units::getEnemyCount(Zerg_Lurker) >= 1 || (Units::getEnemyCount(Zerg_Lair) >= 1 && Units::getEnemyCount(Zerg_Hydralisk_Den) >= 1 && Units::getEnemyCount(Zerg_Hatchery) <= 0))
                || (enemyBuild == "1HatchLurker" || enemyBuild == "2HatchLurker" || enemyBuild == "1GateDT");

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Broodwar->self()->completedUnitCount(Protoss_Observer) > 0)
                    invis = false;
            }
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (Broodwar->self()->completedUnitCount(Terran_Comsat_Station) > 0)
                    invis = false;
                else if (Units::getEnemyCount(Zerg_Hydralisk) > 0 || Units::getEnemyCount(Zerg_Hydralisk_Den) > 0)
                    invis = true;
            }
        }

        void checkEnemyProxy()
        {
            // Proxy builds are built closer to me than the enemy
            proxy = proxy
                || (Units::getSupply() < 80 && (enemyBuild == "CannonRush" || enemyBuild == "BunkerRush"));
        }

        void updateEnemyBuild()
        {
            if (Broodwar->getFrameCount() - enemyFrame > 2000 && enemyFrame != 0 && enemyBuild != "Unknown")
                return;

            if (enemyFrame == 0 && enemyBuild != "Unknown")
                enemyFrame = Broodwar->getFrameCount();

            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;

                if (!player.isSelf()) {
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

            auto const vis = [&](UnitType t) {
                return max(1.0, (double)Broodwar->self()->visibleUnitCount(t));
            };

            switch (unit)
            {
            case Enum::Terran_Marine:
                unitScore[Protoss_Zealot]				+= (size * 0.35) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.65) / vis(Protoss_Dragoon);
                unitScore[Protoss_High_Templar]			+= (size * 0.90) / vis(Protoss_High_Templar);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
                break;
            case Enum::Terran_Medic:
                unitScore[Protoss_Zealot]				+= (size * 0.35) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.65) / vis(Protoss_Dragoon);
                unitScore[Protoss_High_Templar]			+= (size * 0.90) / vis(Protoss_High_Templar);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
                break;
            case Enum::Terran_Firebat:
                unitScore[Protoss_Zealot]				+= (size * 0.35) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.65) / vis(Protoss_Dragoon);
                unitScore[Protoss_High_Templar]			+= (size * 0.90) / vis(Protoss_High_Templar);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
                break;
            case Enum::Terran_Vulture:
                unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
                unitScore[Protoss_Observer]				+= (size * 0.70) / vis(Protoss_Observer);
                unitScore[Protoss_Arbiter]				+= (size * 0.15) / vis(Protoss_Arbiter);
                break;
            case Enum::Terran_Goliath:
                unitScore[Protoss_Zealot]				+= (size * 0.25) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.75) / vis(Protoss_Dragoon);
                unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
                unitScore[Protoss_High_Templar]			+= (size * 0.30) / (Protoss_High_Templar);
                break;
            case Enum::Terran_Siege_Tank_Siege_Mode:
                unitScore[Protoss_Zealot]				+= (size * 0.75) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.25) / vis(Protoss_Dragoon);
                unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
                unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
                break;
            case Enum::Terran_Siege_Tank_Tank_Mode:
                unitScore[Protoss_Zealot]				+= (size * 0.75) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.25) / vis(Protoss_Dragoon);
                unitScore[Protoss_Arbiter]				+= (size * 0.70) / vis(Protoss_Arbiter);
                unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
                break;
            case Enum::Terran_Wraith:
                unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
                break;
            case Enum::Terran_Science_Vessel:
                unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
                break;
            case Enum::Terran_Battlecruiser:
                unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
                break;
            case Enum::Terran_Valkyrie:
                break;

            case Enum::Zerg_Zergling:
                unitScore[Protoss_Zealot]				+= (size * 0.85) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.15) / vis(Protoss_Dragoon);
                unitScore[Protoss_Corsair]				+= (size * 0.60) / vis(Protoss_Corsair);
                unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
                unitScore[Protoss_Archon]				+= (size * 0.30) / vis(Protoss_Archon);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
                break;
            case Enum::Zerg_Hydralisk:
                unitScore[Protoss_Zealot]				+= (size * 0.75) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.25) / vis(Protoss_Dragoon);
                unitScore[Protoss_High_Templar]			+= (size * 0.80) / vis(Protoss_High_Templar);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.20) / vis(Protoss_Dark_Templar);
                unitScore[Protoss_Reaver]				+= (size * 0.30) / vis(Protoss_Reaver);
                break;
            case Enum::Zerg_Lurker:
                unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
                unitScore[Protoss_High_Templar]			+= (size * 0.70) / vis(Protoss_High_Templar);
                unitScore[Protoss_Observer]				+= (size * 1.00) / vis(Protoss_Observer);
                unitScore[Protoss_Reaver]				+= (size * 0.30) / vis(Protoss_Reaver);
                break;
            case Enum::Zerg_Ultralisk:
                unitScore[Protoss_Zealot]				+= (size * 0.25) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.75) / vis(Protoss_Dragoon);
                unitScore[Protoss_Reaver]				+= (size * 0.80) / vis(Protoss_Reaver);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.20) / vis(Protoss_Dark_Templar);
                break;
            case Enum::Zerg_Mutalisk:
                unitScore[Protoss_Corsair]				+= (size * 1.00) / vis(Protoss_Corsair);
                break;
            case Enum::Zerg_Guardian:
                unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);
                unitScore[Protoss_Corsair]				+= (size * 1.00) / vis(Protoss_Corsair);
                break;
            case Enum::Zerg_Devourer:
                break;
            case Enum::Zerg_Defiler:
                unitScore[Protoss_Zealot]				+= (size * 1.00) / vis(Protoss_Zealot);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
                unitScore[Protoss_Reaver]				+= (size * 0.90) / vis(Protoss_Reaver);
                break;

            case Enum::Protoss_Zealot:
                unitScore[Protoss_Zealot]				+= (size * 0.05) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.95) / vis(Protoss_Dragoon);
                unitScore[Protoss_Reaver]				+= (size * 0.90) / vis(Protoss_Reaver);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
                break;
            case Enum::Protoss_Dragoon:
                unitScore[Protoss_Zealot]				+= (size * 0.15) / vis(Protoss_Zealot);
                unitScore[Protoss_Dragoon]				+= (size * 0.85) / vis(Protoss_Dragoon);
                unitScore[Protoss_Reaver]				+= (size * 0.60) / vis(Protoss_Reaver);
                unitScore[Protoss_High_Templar]			+= (size * 0.30) / vis(Protoss_High_Templar);
                unitScore[Protoss_Dark_Templar]			+= (size * 0.10) / vis(Protoss_Dark_Templar);
                break;
            case Enum::Protoss_High_Templar:
                unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
                break;
            case Enum::Protoss_Dark_Templar:
                unitScore[Protoss_Reaver]				+= (size * 1.00) / vis(Protoss_Reaver);
                unitScore[Protoss_Observer]				+= (size * 1.00) / vis(Protoss_Observer);
                break;
            case Enum::Protoss_Reaver:
                unitScore[Protoss_Reaver]				+= (size * 1.00) / vis(Protoss_Reaver);
                break;
            case Enum::Protoss_Archon:
                unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
                break;
            case Enum::Protoss_Dark_Archon:
                unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
                break;
            case Enum::Protoss_Scout:
                if (Terrain::isIslandMap())
                    unitScore[Protoss_Scout]			+= (size * 1.00) / vis(Protoss_Scout);
                break;
            case Enum::Protoss_Carrier:
                unitScore[Protoss_Dragoon]				+= (size * 1.00) / vis(Protoss_Dragoon);

                if (Terrain::isIslandMap())
                    unitScore[Protoss_Scout]			+= (size * 1.00) / vis(Protoss_Scout);
                break;
            case Enum::Protoss_Arbiter:
                unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
                break;
            case Enum::Protoss_Corsair:
                unitScore[Protoss_High_Templar]			+= (size * 1.00) / vis(Protoss_High_Templar);
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
            //		unitScore[t] += (dps*count / max(1.0, (double)Broodwar->self()->visibleUnitCount(t)));
            //	else
            //		unitScore[t] = (unitScore[t] * (9999.0 / 10000.0)) + ((dps * (double)count) / (10000.0 * max(1.0, (double)Broodwar->self()->visibleUnitCount(t))));
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
                unitScore[Zerg_Drone] = (myStrength.groundToGround + myStrength.groundDefense) / (enemyStrength.groundToGround - enemyStrength.groundDefense);

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

                if (Players::vP() && Broodwar->getFrameCount() >= 20000 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) > 0 && Broodwar->self()->completedUnitCount(Protoss_Templar_Archives) > 0) {
                    unitScore[Protoss_Zealot] = unitScore[Protoss_Dragoon];
                    unitScore[Protoss_Archon] = unitScore[Protoss_Dragoon];
                    unitScore[Protoss_High_Templar] += unitScore[Protoss_Dragoon];
                }
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
    bool enemyFastExpand() { return enemyFE; }
    bool enemyRush() { return rush; }
    bool needDetection() { return invis; }
    bool defendChoke() { return holdChoke; }
    bool enemyProxy() { return proxy; }
    bool enemyGasSteal() { return gasSteal; }
    bool enemyScouted() { return enemyScout; }
    bool enemyBust() { return enemyBuild.find("Hydra") != string::npos; }
    bool enemyPressure() { return pressure; }
    int enemyArrivalFrame() { return rushFrame; }
    map <UnitType, double>& getUnitScores() { return unitScore; }
}