#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Units {

    namespace {
        map <Unit, UnitInfo> enemyUnits;
        map <Unit, UnitInfo> myUnits;
        map <Unit, UnitInfo> allyUnits;
        map <Unit, UnitInfo> neutrals;
        map <UnitSizeType, int> allySizes;
        map <UnitSizeType, int> enemySizes;
        map <UnitType, int> enemyTypes;
        map <UnitType, int> myTypes;
        map <Role, int> myRoles;
        set<Unit> splashTargets;
        double immThreat, proxThreat;       
        double globalAllyGroundStrength, globalEnemyGroundStrength;
        double globalAllyAirStrength, globalEnemyAirStrength;
        double allyDefense;
        double minThreshold, maxThreshold;
        int supply = 10;
        int scoutDeadFrame = 0;
        bool ignoreSim;
        Position armyCenter;

        void updateUnitSizes()
        {
            allySizes.clear();
            enemySizes.clear();

            for (auto &u : myUnits) {
                auto &unit = u.second;
                if (unit.getRole() == Role::Fighting)
                    allySizes[unit.getType().size()]++;
            }

            for (auto &u : enemyUnits) {
                auto &unit = u.second;
                if (!unit.getType().isBuilding() && !unit.getType().isWorker())
                    enemySizes[unit.getType().size()]++;
            }
        }

        void updateSimulation(UnitInfo& unit)
        {
            /*
            Modified version of Horizon.
            Need to test deadzones and squeeze factors still.
            */

            auto enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
            auto enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
            auto unitToEngage = max(0.0, unit.getEngDist() / (24.0 * unit.getSpeed()));
            auto simulationTime = unitToEngage + 5.0;
            auto unitSpeed = unit.getSpeed() * 24.0;
            auto sync = false;
            auto belowGrdLimits = false;
            auto belowAirLimits = false;
            map<const BWEM::ChokePoint *, double> enemySqueezeFactor;
            map<const BWEM::ChokePoint *, double> selfSqueezeFactor;

            const auto shouldIgnoreSim = [&]() {
                // If we have excessive resources, ignore our simulation and engage
                if (!ignoreSim && Broodwar->self()->minerals() >= 2000 && Broodwar->self()->gas() >= 2000 && supply >= 380)
                    ignoreSim = true;
                if (ignoreSim && Broodwar->self()->minerals() <= 500 || Broodwar->self()->gas() <= 500 || supply <= 240)
                    ignoreSim = false;

                if (ignoreSim || (Terrain::isIslandMap() && !unit.getType().isFlyer())) {
                    unit.setSimState(SimState::Win);
                    unit.setSimValue(10.0);
                    return true;
                }
                if (!unit.hasTarget()) {
                    unit.setSimState(SimState::None);
                    unit.setSimValue(0.0);
                    return true;
                }
                return false;
            };

            const auto applySqueezeFactor = [&](UnitInfo& source) {
                if (source.getPlayer() == Broodwar->self() && (!source.hasTarget() || source.getTarget().getType().isFlyer() || !source.getTarget().getPosition().isValid()))
                    return false;
                if (source.getPlayer() != Broodwar->self() && (!unit.hasTarget() || unit.getTarget().getType().isFlyer() || !unit.getTarget().getPosition().isValid()))
                    return false;
                if (source.getType().isFlyer() || unit.getType().isFlyer() || !source.getPosition().isValid() || source.unit()->isLoaded() || unit.unit()->isLoaded())
                    return false;
                if (!mapBWEM.GetArea(source.getTilePosition()))
                    return false;
                return true;
            };

            const auto addToSim = [&](UnitInfo& u) {
                if (!u.unit() || u.getType().isWorker() || (u.unit() && u.unit()->exists() && (u.unit()->isStasised() || u.unit()->isMorphing())))
                    return false;
                if (u.getRole() != Role::None && u.getRole() != Role::Fighting)
                    return false;
                if (u.getPlayer() == Broodwar->self() && !u.hasTarget())
                    return false;
                return true;
            };

            const auto simTerrain = [&]() {
                // For every ground unit, add a squeeze factor for navigating chokes
                for (auto &e : enemyUnits) {
                    UnitInfo &enemy = e.second;
                    if (applySqueezeFactor(enemy)) {
                        auto path = mapBWEM.GetPath(enemy.getPosition(), unit.getPosition());
                        for (auto &choke : path) {
                            if (enemy.getGroundReach() < enemy.getPosition().getDistance(Position(choke->Center())))
                                enemySqueezeFactor[choke]+= double(enemy.getType().width());
                        }
                    }
                }
                for (auto &a : myUnits) {
                    UnitInfo &ally = a.second;
                    if (applySqueezeFactor(ally)) {
                        auto path = mapBWEM.GetPath(ally.getPosition(), ally.getTarget().getPosition());
                        for (auto &choke : path) {
                            if (ally.getGroundReach() < ally.getPosition().getDistance(Position(choke->Center())))
                                selfSqueezeFactor[choke]+= double(ally.getType().width());
                        }
                    }
                }
                for (auto &choke : enemySqueezeFactor) {
                    choke.second = min(1.0, (double(Util::chokeWidth(choke.first)) / choke.second));
                }
                for (auto &choke : selfSqueezeFactor) {
                    choke.second = min(1.0, (double(Util::chokeWidth(choke.first)) / choke.second));
                }
            };

            const auto simEnemies = [&]() {
                for (auto &e : enemyUnits) {
                    auto &enemy = e.second;
                    if (!addToSim(enemy))
                        continue;

                    auto deadzone = (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.getTarget().getPosition().getDistance(enemy.getPosition()) < 64.0) ? 64.0 : 0.0;
                    auto widths = double(enemy.getType().width() + unit.getType().width()) / 2.0;
                    auto enemyRange = (unit.getType().isFlyer() ? enemy.getAirRange() : enemy.getGroundRange());
                    auto engDist = enemy.getPosition().getDistance(unit.getPosition()) - enemyRange;
                    auto distance = engDist - widths + deadzone;

                    auto speed = enemy.getSpeed() > 0.0 ? 24.0 * enemy.getSpeed() : 24.0 * unit.getSpeed();
                    auto simRatio =  simulationTime - (distance / speed);

                    // If the unit doesn't affect this simulation
                    if (simRatio <= 0.0 || (enemy.getSpeed() <= 0.0 && enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange - widths <= 64.0))
                        continue;

                    // Situations where an enemy should be treated as stronger than it actually is
                    if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected())
                        simRatio = simRatio * 2.0;
                    if (!enemy.getType().isFlyer() && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition())))
                        simRatio = simRatio * 2.0;
                    if (enemy.getLastVisibleFrame() < Broodwar->getFrameCount())
                        simRatio = simRatio * (1.0 + min(1.0, (double(Broodwar->getFrameCount() - enemy.getLastVisibleFrame()) / 1000.0)));

                    // Check if we need to squeeze these units through a choke
                    if (applySqueezeFactor(enemy)) {
                        auto path = mapBWEM.GetPath(enemy.getPosition(), unit.getPosition());
                        for (auto &choke : path) {
                            if (enemy.getGroundReach() < enemy.getPosition().getDistance(Position(choke->Center())))
                                simRatio = simRatio * min(1.0, enemySqueezeFactor[choke]);
                        }
                    }

                    // Add their values to the simulation
                    enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
                    enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;
                }
            };

            const auto simMyUnits = [&]() {
                for (auto &a : myUnits) {
                    auto &ally = a.second;
                    if (!addToSim(ally))
                        continue;

                    auto deadzone = (ally.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.getTarget().getPosition().getDistance(ally.getPosition()) < 64.0) ? 64.0 : 0.0;
                    auto engDist = ally.getEngDist();
                    auto widths = double(ally.getType().width() + ally.getTarget().getType().width()) / 2.0;
                    auto allyRange = (unit.getTarget().getType().isFlyer() ? ally.getAirRange() : ally.getGroundRange());
                    auto speed = ally.hasTransport() ? 24.0 * ally.getTransport().getSpeed() : 24.0 * ally.getSpeed();

                    auto distance = engDist - widths + deadzone;
                    auto simRatio = simulationTime - (distance / speed);

                    // If the unit doesn't affect this simulation
                    if (simRatio <= 0.0 || (ally.getSpeed() <= 0.0 && ally.getPosition().getDistance(unit.getTarget().getPosition()) - allyRange - widths <= 64.0))
                        continue;

                    // HACK: Bunch of hardcoded stuff
                    if (ally.getPosition().getDistance(unit.getTarget().getPosition()) / speed > simulationTime)
                        continue;
                    if ((ally.getType() == UnitTypes::Protoss_Scout || ally.getType() == UnitTypes::Protoss_Corsair) && ally.getShields() < 30)
                        continue;
                    if (ally.getType() == UnitTypes::Terran_Wraith && ally.getHealth() <= 100)
                        continue;
                    if (ally.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT && Broodwar->getFrameCount() < 8000)
                        continue;
                    if (ally.getType() == UnitTypes::Zerg_Mutalisk && Grids::getEAirThreat((WalkPosition)ally.getEngagePosition()) * 5.0 > ally.getHealth() && ally.getHealth() <= 30)
                        continue;

                    // Situations where an ally should be treated as stronger than it actually is
                    if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && !Command::overlapsEnemyDetection(ally.getEngagePosition()))
                        simRatio = simRatio * 2.0;
                    if (!ally.getType().isFlyer() && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTarget().getPosition())))
                        simRatio = simRatio * 2.0;

                    if (!sync && simRatio > 0.0 && ((unit.getType().isFlyer() && !ally.getType().isFlyer()) || (!unit.getType().isFlyer() && ally.getType().isFlyer())))
                        sync = true;

                    if (applySqueezeFactor(ally)) {
                        auto path = mapBWEM.GetPath(ally.getPosition(), ally.getTarget().getPosition());
                        for (auto &choke : path) {
                            if (ally.getGroundReach() < ally.getPosition().getDistance(Position(choke->Center())))
                                simRatio = simRatio * min(1.0, selfSqueezeFactor[choke]);
                        }
                    }

                    allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
                    allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;

                    if (ally.getType().isFlyer() && ally.getSimValue() < minThreshold && ally.getSimValue() != 0.0)
                        belowAirLimits = true;
                    if (!ally.getType().isFlyer() && ally.getSimValue() < minThreshold && ally.getSimValue() != 0.0)
                        belowGrdLimits = true;
                }
            };

            if (!shouldIgnoreSim()) {
                simTerrain();
                simEnemies();
                simMyUnits();
            }

            double attackAirAsAir = enemyLocalAirStrength > 0.0 ? allyLocalAirStrength / enemyLocalAirStrength : 10.0;
            double attackAirAsGround = enemyLocalGroundStrength > 0.0 ? allyLocalAirStrength / enemyLocalGroundStrength : 10.0;
            double attackGroundAsAir = enemyLocalAirStrength > 0.0 ? allyLocalGroundStrength / enemyLocalAirStrength : 10.0;
            double attackGroundasGround = enemyLocalGroundStrength > 0.0 ? allyLocalGroundStrength / enemyLocalGroundStrength : 10.0;

            // If unit is a flyer and no air threat
            if (unit.getType().isFlyer() && enemyLocalAirStrength == 0.0)
                unit.setSimValue(10.0);

            // If unit is not a flyer and no ground threat
            else if (!unit.getType().isFlyer() && enemyLocalGroundStrength == 0.0)
                unit.setSimValue(10.0);

            // If unit has a target, determine what sim value to use
            else if (unit.hasTarget()) {
                if (sync) {
                    if (unit.getType().isFlyer())
                        unit.setSimValue(min(attackAirAsAir, attackGroundAsAir));
                    else
                        unit.setSimValue(min(attackAirAsGround, attackGroundasGround));
                }
                else {
                    if (unit.getType().isFlyer())
                        unit.setSimValue(unit.getTarget().getType().isFlyer() ? attackAirAsAir : attackGroundAsAir);
                    else
                        unit.setSimValue(unit.getTarget().getType().isFlyer() ? attackAirAsGround : attackGroundasGround);
                }
            }

            auto belowLimits = unit.getType().isFlyer() ? (belowAirLimits || (sync && belowGrdLimits)) : (belowGrdLimits || (sync && belowAirLimits));

            // If above/below thresholds, it's a sim win/loss
            if (unit.getSimValue() >= maxThreshold && !belowLimits) {
                unit.setSimState(SimState::Win);
            }
            else if (unit.getSimValue() <= minThreshold || belowLimits || (unit.getSimState() == SimState::None && unit.getSimValue() < maxThreshold)) {
                unit.setSimState(SimState::Loss);
            }
        }

        void updateLocalState(UnitInfo& unit)
        {
            if (unit.hasTarget()) {

                auto fightingAtHome = ((Terrain::isInAllyTerritory(unit.getTilePosition()) && Util::unitInRange(unit)) || Terrain::isInAllyTerritory(unit.getTarget().getTilePosition()));
                auto invisTarget = unit.getTarget().unit()->exists() && (unit.getTarget().unit()->isCloaked() || unit.getTarget().isBurrowed()) && !unit.getTarget().unit()->isDetected();
                auto enemyReach = unit.getType().isFlyer() ? unit.getTarget().getAirReach() : unit.getTarget().getGroundReach();
                auto enemyThreat = unit.getType().isFlyer() ? Grids::getEAirThreat(unit.getEngagePosition()) : Grids::getEGroundThreat(unit.getEngagePosition());

                // Testing
                if (Command::isInDanger(unit, unit.getPosition()) || (Command::isInDanger(unit, unit.getEngagePosition()) && unit.getPosition().getDistance(unit.getEngagePosition()) < SIM_RADIUS))
                    unit.setLocalState(LocalState::Retreating);

                // Force engaging
                else if (!invisTarget && (isThreatening(unit.getTarget())
                    || (fightingAtHome && (!unit.getType().isFlyer() || !unit.getTarget().getType().isFlyer()) && (Strategy::defendChoke() || unit.getGroundRange() > 64.0))))
                    unit.setLocalState(LocalState::Engaging);

                // Force retreating
                else if ((unit.getType().isMechanical() && unit.getPercentTotal() < LOW_MECH_PERCENT_LIMIT)
                    || (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75)
                    || Grids::getESplash(unit.getWalkPosition()) > 0
                    || (invisTarget && (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= enemyReach || enemyThreat))
                    || unit.getGlobalState() == GlobalState::Retreating)
                    unit.setLocalState(LocalState::Retreating);

                // Close enough to make a decision
                else if (unit.getPosition().getDistance(unit.getSimPosition()) <= SIM_RADIUS) {

                    // Retreat
                    if ((unit.getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && !BuildOrder::isProxy() && unit.getTarget().getType() == UnitTypes::Terran_Vulture && Grids::getMobility(unit.getTarget().getWalkPosition()) > 6 && Grids::getCollision(unit.getTarget().getWalkPosition()) < 4)
                        || ((unit.getType() == UnitTypes::Protoss_Scout || unit.getType() == UnitTypes::Protoss_Corsair) && unit.getTarget().getType() == UnitTypes::Zerg_Overlord && Grids::getEAirThreat((WalkPosition)unit.getEngagePosition()) * 5.0 > (double)unit.getShields())
                        || (unit.getType() == UnitTypes::Protoss_Corsair && unit.getTarget().getType() == UnitTypes::Zerg_Scourge && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Corsair) < 6)
                        || (unit.getType() == UnitTypes::Terran_Medic && unit.unit()->getEnergy() <= TechTypes::Healing.energyCost())
                        || (unit.getType() == UnitTypes::Zerg_Mutalisk && Grids::getEAirThreat((WalkPosition)unit.getEngagePosition()) > 0.0 && unit.getHealth() <= 30)
                        || (unit.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT && Broodwar->getFrameCount() < 8000)
                        || (unit.getType() == UnitTypes::Terran_SCV && Broodwar->getFrameCount() > 12000)
                        || (invisTarget && !isThreatening(unit) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) == 0))
                        unit.setLocalState(LocalState::Retreating);

                    // Engage
                    else if ((Broodwar->getFrameCount() > 10000 && (unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit.getTarget().getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < 96.0 || Util::unitInRange(unit)))
                        || ((unit.unit()->isCloaked() || unit.isBurrowed()) && !Command::overlapsEnemyDetection(unit.getEngagePosition()))
                        || (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && Util::unitInRange(unit))
                        || (unit.getSimState() == SimState::Win && unit.getGlobalState() == GlobalState::Engaging))
                        unit.setLocalState(LocalState::Engaging);
                    else
                        unit.setLocalState(LocalState::Retreating);
                }
                else if (unit.getGlobalState() == GlobalState::Retreating) {
                    unit.setLocalState(LocalState::Retreating);
                }
                else {
                    unit.setLocalState(LocalState::None);
                }
            }
            else if (unit.getGlobalState() == GlobalState::Retreating) {
                unit.setLocalState(LocalState::Retreating);
            }
            else {
                unit.setLocalState(LocalState::None);
            }
        }

        void updateGlobalState(UnitInfo& unit)
        {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if ((!BuildOrder::isFastExpand() && Strategy::enemyFastExpand())
                    || (Strategy::enemyProxy() && !Strategy::enemyRush())
                    || BuildOrder::isRush())
                    unit.setGlobalState(GlobalState::Engaging);

                else if ((Strategy::enemyRush() && !Players::vT())
                    || (!Strategy::enemyRush() && BuildOrder::isHideTech() && BuildOrder::isOpener())
                    || unit.getType().isWorker()
                    || (Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
                    || (unit.getType() == UnitTypes::Protoss_Corsair && !BuildOrder::firstReady() && globalEnemyAirStrength > 0.0))
                    unit.setGlobalState(GlobalState::Retreating);
                else
                    unit.setGlobalState(GlobalState::Engaging);
            }
            else
                unit.setGlobalState(GlobalState::Engaging);
        }

        void updateRole(UnitInfo& unit)
        {
            // Don't assign a role to uncompleted units
            if (!unit.unit()->isCompleted()) {
                unit.setRole(Role::None);
                return;
            }

            // Store old role to update counters after
            auto oldRole = unit.getRole();

            // Update default role
            if (unit.getRole() == Role::None) {
                if (unit.getType().isWorker())
                    unit.setRole(Role::Working);
                else if (unit.getType().isBuilding() && unit.getGroundDamage() == 0.0 && unit.getAirDamage() == 0.0)
                    unit.setRole(Role::Producing);
                else if (unit.getType().isBuilding() && unit.getGroundDamage() != 0.0 && unit.getAirDamage() != 0.0)
                    unit.setRole(Role::Defending);
                else if (unit.getType().spaceProvided() > 0)
                    unit.setRole(Role::Transporting);
                else
                    unit.setRole(Role::Fighting);
            }

            // Check if workers should fight or work
            if (unit.getType().isWorker()) {
                if (unit.getRole() == Role::Working && (Util::reactivePullWorker(unit) || Util::proactivePullWorker(unit) || Util::pullRepairWorker(unit)))
                    unit.setRole(Role::Fighting);
                else if (unit.getRole() == Role::Fighting && !Util::reactivePullWorker(unit) && !Util::proactivePullWorker(unit) && !Util::pullRepairWorker(unit))
                    unit.setRole(Role::Working);
            }

            // Check if an overlord should scout or support
            if (unit.getType() == UnitTypes::Zerg_Overlord) {
                if (unit.getRole() == Role::None || myRoles[Role::Scouting] < myRoles[Role::Supporting] + 1)
                    unit.setRole(Role::Scouting);
                else if (myRoles[Role::Supporting] < myRoles[Role::Scouting] + 1)
                    unit.setRole(Role::Supporting);
            }

            // Check if we should scout - TODO: scout count from scout manager
            if (BWEB::Map::getNaturalChoke() && BuildOrder::shouldScout() && getMyRoleCount(Role::Scouting) < 1 && Broodwar->getFrameCount() - scoutDeadFrame > 500) {
                auto type = Broodwar->self()->getRace().getWorker();
                auto scout = Util::getClosestUnit(Position(BWEB::Map::getNaturalChoke()->Center()), Broodwar->self(), type);
                if (scout == &unit) {

                    if (scout->hasResource())
                        scout->getResource().setGathererCount(scout->getResource().getGathererCount() - 1);

                    scout->setRole(Role::Scouting);
                    scout->setResource(nullptr);
                    scout->setBuildingType(UnitTypes::None);
                    scout->setBuildPosition(TilePositions::Invalid);
                }
            }

            // Check if a worker morphed into a building
            if (unit.getRole() == Role::Working && unit.getType().isBuilding()) {
                if (unit.getType().isBuilding() && unit.getGroundDamage() == 0.0 && unit.getAirDamage() == 0.0)
                    unit.setRole(Role::Producing);
                else
                    unit.setRole(Role::Fighting);
            }

            // Detectors and Support roles
            if ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == UnitTypes::Protoss_Arbiter)
                unit.setRole(Role::Supporting);

            // Increment new role counter, decrement old role counter
            auto newRole = unit.getRole();
            if (oldRole != newRole) {
                if (oldRole != Role::None)
                    myRoles[oldRole] --;
                if (newRole != Role::None)
                    myRoles[newRole] ++;
            }
        }

        void updateUnits()
        {
            // Reset calculations and caches
            globalEnemyGroundStrength = 0.0;
            globalEnemyAirStrength = 0.0;
            globalAllyGroundStrength = 0.0;
            globalAllyAirStrength = 0.0;
            immThreat = 0.0;
            proxThreat = 0.0;
            allyDefense = 0.0;
            splashTargets.clear();
            enemyTypes.clear();
            myTypes.clear();

            // PvZ
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::vZ()) {
                    minThreshold = 0.25;
                    maxThreshold = 0.75;
                }

                // PvT
                if (Players::vT()) {
                    minThreshold = 0.25;
                    maxThreshold = 0.75;
                }

                // PvP
                if (Players::vP()) {
                    minThreshold = 0.25;
                    maxThreshold = 0.75;
                }
            }
            else {
                minThreshold = 0.75;
                maxThreshold = 1.25;
            }

            if (BuildOrder::isRush())
                minThreshold = 0.0, maxThreshold = 0.75;

            // Update Enemy Units
            for (auto &u : enemyUnits) {
                UnitInfo &unit = u.second;

                // If this is a flying building that we haven't recognized as being a flyer, remove overlap tiles
                auto flyingBuilding = unit.unit()->exists() && !unit.isFlying() && (unit.unit()->getOrder() == Orders::LiftingOff || unit.unit()->getOrder() == Orders::BuildingLiftOff || unit.unit()->isFlying());

                if (flyingBuilding && unit.getLastTile().isValid()) {
                    for (int x = unit.getLastTile().x; x < unit.getLastTile().x + unit.getType().tileWidth(); x++) {
                        for (int y = unit.getLastTile().y; y < unit.getLastTile().y + unit.getType().tileHeight(); y++) {
                            TilePosition t(x, y);
                            if (!t.isValid())
                                continue;

                            BWEB::Map::removeUsed(t, 1, 1);
                        }
                    }
                }

                if (isThreatening(unit))
                    unit.circleRed();

                // If unit is visible, update it
                if (unit.unit()->exists()) {
                    unit.updateUnit();

                    if (unit.hasTarget() && (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Protoss_Scarab))
                        splashTargets.insert(unit.getTarget().unit());

                    if (unit.getType().isBuilding() && !unit.isFlying() && unit.unit()->exists() && !BWEB::Map::isUsed(unit.getTilePosition()))
                        BWEB::Map::addUsed(unit.getTilePosition(), unit.getType().tileWidth(), unit.getType().tileHeight());
                }

                // Must see a 3x3 grid of Tiles to set a unit to invalid position
                if (!unit.unit()->exists() && (!unit.isBurrowed() || Command::overlapsAllyDetection(unit.getPosition()) || (unit.getWalkPosition().isValid() && Grids::getAGroundCluster(unit.getWalkPosition()) > 0)) && unit.getPosition().isValid()) {
                    bool move = true;
                    for (int x = unit.getTilePosition().x - 1; x < unit.getTilePosition().x + 1; x++) {
                        for (int y = unit.getTilePosition().y - 1; y < unit.getTilePosition().y + 1; y++) {
                            TilePosition t(x, y);
                            if (t.isValid() && !Broodwar->isVisible(t))
                                move = false;
                        }
                    }
                    if (move) {
                        unit.setPosition(Positions::Invalid);
                        unit.setTilePosition(TilePositions::Invalid);
                        unit.setWalkPosition(WalkPositions::Invalid);
                    }
                }

                // If unit has a valid type, update enemy composition tracking
                if (unit.getType().isValid())
                    enemyTypes[unit.getType()] += 1;

                // If unit is not a worker or building, add it to global strength	
                if (!unit.getType().isWorker())
                    unit.getType().isFlyer() ? globalEnemyAirStrength += unit.getVisibleAirStrength() : globalEnemyGroundStrength += unit.getVisibleGroundStrength();

                // If a unit is threatening our position
                if (isThreatening(unit) && (unit.getType().groundWeapon().damageAmount() > 0 || unit.getType() == UnitTypes::Terran_Bunker)) {
                    if (unit.getType().isBuilding())
                        immThreat += 1.50;
                    else
                        immThreat += unit.getVisibleGroundStrength();
                }
            }

            // Update myUnits
            double centerCluster = 0.0;
            for (auto &u : myUnits) {
                auto &unit = u.second;

                unit.updateUnit();
                updateRole(unit);

                if (unit.getRole() == Role::Fighting) {
                    updateSimulation(unit);
                    updateGlobalState(unit);
                    updateLocalState(unit);

                    double g = Grids::getAGroundCluster(unit.getWalkPosition()) + Grids::getAAirCluster(unit.getWalkPosition());
                    if (g > centerCluster) {
                        centerCluster = g;
                        armyCenter = unit.getPosition();
                    }
                }

                auto type = unit.getType() == UnitTypes::Zerg_Egg ? unit.unit()->getBuildType() : unit.getType();
                myTypes[type] ++;

                // If unit is not a building and deals damage, add it to global strength	
                if (!unit.getType().isBuilding())
                    unit.getType().isFlyer() ? globalAllyAirStrength += unit.getVisibleAirStrength() : globalAllyGroundStrength += unit.getVisibleGroundStrength();
            }

            for (auto &u : neutrals) {
                auto &unit = u.second;
                if (!unit.unit() || !unit.unit()->exists())
                    continue;
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateUnitSizes();
        updateUnits();
        Visuals::endPerfTest("Units");
    }

    bool isThreatening(UnitInfo& unit)
    {
        if ((unit.isBurrowed() || (unit.unit() && unit.unit()->exists() && unit.unit()->isCloaked())) && !Command::overlapsAllyDetection(unit.getPosition()))
            return false;

        // Define "close" - TODO: define better
        auto close = unit.getPosition().getDistance(Terrain::getDefendPosition()) < unit.getGroundReach() || unit.getPosition().getDistance(Terrain::getDefendPosition()) < unit.getAirReach();
        auto atHome = Terrain::isInAllyTerritory(unit.getTilePosition());
        auto manner = unit.getPosition().getDistance(Terrain::getMineralHoldPosition()) < 256.0;
        auto exists = unit.unit() && unit.unit()->exists();
        auto attacked = exists && unit.hasAttackedRecently() && unit.hasTarget() && unit.getTarget().getType().isBuilding();
        auto buildingClose = exists && (unit.getPosition().getDistance(Terrain::getDefendPosition()) < 320.0 || close) && (unit.unit()->isConstructing() || unit.unit()->getOrder() == Orders::ConstructingBuilding || unit.unit()->getOrder() == Orders::PlaceBuilding);

        // Situations where a unit should be attacked:
        // 1) Building
        //	- Blocking any of my building spots or expansions
        //	- Has damage and is "close" or "atHome"
        //	- Shield battery and is "close" or "atHome"
        //	- Manner pylon

        if (unit.getType().isBuilding()) {
            if (Buildings::overlapsQueue(unit.getType(), unit.getTilePosition()))
                return true;
            if ((close || atHome) && (unit.getAirDamage() > 0.0 || unit.getGroundDamage() > 0.0))
                return true;
            if ((close || atHome) && unit.getType() == UnitTypes::Protoss_Shield_Battery)
                return true;
            if (manner)
                return true;
        }

        // 2) Worker
        // - SCV building or repairing something "close"
        // - In my territory

        else if (unit.getType().isWorker()) {
            if (buildingClose)
                return true;
            if (close)
                return true;
            if (atHome && Strategy::defendChoke())
                return true;
        }

        // 3) Unit
        // - "close"
        // - Near my shield battery

        else {
            if (close)
                return true;
            if (atHome && Strategy::defendChoke())
                return true;
            if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shield_Battery) > 0) {
                auto battery = Util::getClosestUnit(unit.getPosition(), Broodwar->self(), UnitTypes::Protoss_Shield_Battery);
                if (battery && unit.getPosition().getDistance(battery->getPosition()) <= 128.0)
                    return true;
            }
        }
        return false;
    }

    int getEnemyCount(UnitType t)
    {
        map<UnitType, int>::iterator itr = enemyTypes.find(t);
        if (itr != enemyTypes.end())
            return itr->second;
        return 0;
    }

    void storeUnit(Unit unit)
    {
        auto &info = unit->getPlayer() == Broodwar->self() ? myUnits[unit] : (unit->getPlayer() == Broodwar->enemy() ? enemyUnits[unit] : allyUnits[unit]);
        info.setUnit(unit);
        info.updateUnit();
        
        // TODO: Supply track enemy?
        if (unit->getPlayer() == Broodwar->self() && !unit->isCompleted() && unit->getType().supplyRequired() > 0)
            supply += unit->getType().supplyRequired();
        if (unit->getType() == UnitTypes::Protoss_Pylon)
            Pylons::storePylon(unit);
    }

    void removeUnit(Unit unit)
    {
        BWEB::Map::onUnitDestroy(unit);

        if (myUnits.find(unit) != myUnits.end()) {
            auto &info = myUnits[unit];
            supply -= unit->getType().supplyRequired();

            if (info.hasResource())
                info.getResource().setGathererCount(info.getResource().getGathererCount() - 1);
            if (info.getRole() != Role::None)
                myRoles[info.getRole()]--;
            if (info.getRole() == Role::Scouting)
                scoutDeadFrame = Broodwar->getFrameCount();

            Transports::removeUnit(unit);
            myUnits.erase(unit);
        }
        else if (enemyUnits.find(unit) != enemyUnits.end())
            enemyUnits.erase(unit);
        else if (allyUnits.find(unit) != allyUnits.end())
            allyUnits.erase(unit);
        else if (neutrals.find(unit) != neutrals.end())
            neutrals.erase(unit);
    }
            
    Position getArmyCenter() { return armyCenter; }
    set<Unit>& getSplashTargets() { return splashTargets; }
    map<Unit, UnitInfo>& getMyUnits() { return myUnits; }
    map<Unit, UnitInfo>& getEnemyUnits() { return enemyUnits; }
    map<Unit, UnitInfo>& getNeutralUnits() { return neutrals; }
    map<UnitSizeType, int>& getAllySizes() { return allySizes; }
    map<UnitSizeType, int>& getEnemySizes() { return enemySizes; }
    map<UnitType, int>& getenemyTypes() { return enemyTypes; }
    double getImmThreat() { return immThreat; }
    double getProxThreat() { return proxThreat; }
    double getGlobalAllyGroundStrength() { return globalAllyGroundStrength; }
    double getGlobalEnemyGroundStrength() { return globalEnemyGroundStrength; }
    double getGlobalAllyAirStrength() { return globalAllyAirStrength; }
    double getGlobalEnemyAirStrength() { return globalEnemyAirStrength; }
    double getAllyDefense() { return allyDefense; }
    int getSupply() { return supply; }
    int getMyRoleCount(Role role) { return myRoles[role]; }
    int getMyTypeCount(UnitType type) { return myTypes[type]; }
}