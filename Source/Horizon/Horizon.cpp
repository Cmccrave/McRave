#include "..\McRave\McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Horizon {

    namespace {
        bool ignoreSim = false;
        double minThreshold;
        double maxThreshold;

        void updateThresholds(UnitInfo& unit)
        {
            double minWinPercent = 0.6;
            double maxWinPercent = 1.2;

            // P
            if (Players::PvP()) {
                minWinPercent = 0.80;
                maxWinPercent = 1.20;
            }
            if (Players::PvZ()) {
                minWinPercent = 0.6;
                maxWinPercent = 1.2;
            }
            if (Players::PvT()) {
                minWinPercent = 0.6;
                maxWinPercent = 1.0;
            }
            
            // Z
            if (Players::ZvP()) {
                minWinPercent = 0.80;
                maxWinPercent = 1.20;
            }
            if (Players::ZvZ()) {
                auto enemyMoreHatch = (Players::getCurrentCount(PlayerState::Enemy, UnitTypes::Zerg_Hatchery) + Players::getCurrentCount(PlayerState::Enemy, UnitTypes::Zerg_Lair) + Players::getCurrentCount(PlayerState::Enemy, UnitTypes::Zerg_Hive)
                    > Players::getCurrentCount(PlayerState::Self, UnitTypes::Zerg_Hatchery) + Players::getCurrentCount(PlayerState::Self, UnitTypes::Zerg_Lair) + Players::getCurrentCount(PlayerState::Self, UnitTypes::Zerg_Hive));
                
                if (enemyMoreHatch || Strategy::getEnemyTransition() == "2HatchLing") {
                    minWinPercent = 1.00;
                    maxWinPercent = 1.25;
                }
                else {
                    minWinPercent = 0.90;
                    maxWinPercent = 1.10;
                }            
            }
            if (Players::ZvT()) {
                minWinPercent = 0.8;
                maxWinPercent = 1.2;
            }

            if (BuildOrder::isRush()) {
                minThreshold = 0.25;
                maxThreshold = 0.75;
            }
            else {
                minThreshold = minWinPercent;
                maxThreshold = maxWinPercent;
            }
        }
    }

    void simulate(UnitInfo& unit)
    {
        updateThresholds(unit);
        auto enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
        auto enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
        auto unitToEngage = unit.getSpeed() > 0.0 ? unit.getEngDist() / (24.0 * unit.getSpeed()) : 5.0;
        auto simulationTime = unitToEngage + 5.0;
        auto sync = false;
        auto belowGrdtoGrdLimit = false;
        auto belowGrdtoAirLimit = false;
        auto belowAirtoAirLimit = false;
        auto belowAirtoGrdLimit = false;
        auto unitArea = mapBWEM.GetArea(unit.getTilePosition());
        map<const BWEM::ChokePoint *, double> squeezeFactor;
        auto casterCount = 0;

        // If we have excessive resources, ignore our simulation and engage
        if (!ignoreSim && Broodwar->self()->minerals() >= 2000 && Broodwar->self()->gas() >= 2000 && Players::getSupply(PlayerState::Self) >= 380)
            ignoreSim = true;
        if (ignoreSim && (Broodwar->self()->minerals() <= 500 || Broodwar->self()->gas() <= 500 || Players::getSupply(PlayerState::Self) <= 240))
            ignoreSim = false;

        if (ignoreSim) {
            unit.setSimState(SimState::Win);
            unit.setSimValue(10.0);
            return;
        }
        else if (!unit.hasTarget()) {
            unit.setSimState(SimState::None);
            unit.setSimValue(0.0);
            return;
        }
        else if (unit.getEngDist() == DBL_MAX) {
            unit.setSimState(SimState::Loss);
            unit.setSimValue(0.0);
            return;
        }

        const auto applySqueezeFactor = [&](UnitInfo& source) {
            if (BuildOrder::isRush()
                || !source.getTilePosition().isValid()
                || !source.getTarget().getTilePosition().isValid()
                || source.hasTransport()
                || source.isFlying()
                || (!mapBWEM.GetArea(source.getTilePosition()) && !BWEB::Map::isWalkable(source.getTilePosition(), source.getType()))
                || (!mapBWEM.GetArea(source.getTarget().getTilePosition()) && !BWEB::Map::isWalkable(source.getTarget().getTilePosition(), source.getTarget().getType())))
                return false;
            return true;
        };

        const auto canAddToSim = [&](UnitInfo& u) {
            if (!u.unit()
                || u.getType().isWorker()
                || (!u.unit()->isCompleted() && u.unit()->exists())
                || (u.unit()->exists() && (u.unit()->isStasised() || u.unit()->isMorphing()))
                || (u.getVisibleAirStrength() <= 0.0 && u.getVisibleGroundStrength() <= 0.0)
                || (u.getPlayer() == Broodwar->self() && !u.hasTarget())
                || (u.getRole() != Role::None && u.getRole() != Role::Combat))
                return false;
            return true;
        };

        const auto simTerrain = [&]() {

            if (Players::getSupply(PlayerState::Self) <= 50)
                return;

            // Check if any units are attempting to pass through a chokepoint
            for (auto &a : Units::getUnits(PlayerState::Self)) {
                UnitInfo &ally = *a;

                if (!canAddToSim(ally) || !applySqueezeFactor(ally) || int(ally.getQuickPath().size()) > 3)
                    continue;

                auto targetReach = ally.getType().isFlyer() ? ally.getTarget().getAirReach() : ally.getTarget().getGroundReach();

                if (!ally.getQuickPath().empty()) {

                    // Add their width to each chokepoint it needs to move through
                    for (auto &choke : ally.getQuickPath()) {
                        if (choke->Width() <= 128.0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach) {
                            squeezeFactor[choke]+= (double(choke->Width()) * ally.getSpeed()) / (double(ally.getType().width()) * double(ally.getType().height()));
                            break;
                        }
                    }
                }

                else {
                    auto choke = Util::getClosestChokepoint(ally.getEngagePosition());
                    if (choke && choke->Width() <= 128.0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach) {
                        auto cost = (double(ally.getType().width()) * double(ally.getType().height())) / ((double(choke->Width())) * ally.getSpeed());
                        squeezeFactor[choke]+= log(cost);
                    }
                }
            }

            for (auto &[choke, cost] : squeezeFactor)
                cost = 1.0 - exp(-5.0 / cost);
        };

        const auto simEnemies = [&]() {
            for (auto &e : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &enemy = *e;
                if (!canAddToSim(enemy))
                    continue;

                const auto enemyRange = unit.getType().isFlyer() ? enemy.getAirRange() : enemy.getGroundRange();
                const auto distance = double(max(0, Util::boxDistance(enemy.getType(), enemy.getPosition(), unit.getType(), unit.getEngagePosition())) - enemyRange);
                const auto speed = enemy.getSpeed() > 0.0 ? 24.0 * enemy.getSpeed() : 24.0 * unit.getSpeed();
                auto simRatio = simulationTime - (distance / speed);

                // If the unit doesn't affect this simulation
                if (simRatio <= 0.0
                    || (enemy.getSpeed() <= 0.0 && distance > 0.0)
                    || (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && enemy.getPosition().getDistance(unit.getPosition()) < 64.0))
                    continue;

                // Hidden bonus
                if (enemy.isHidden())
                    simRatio = simRatio * 2.0;

                // High ground bonus
                if (!enemy.getType().isFlyer() && enemyRange > 32.0 && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition())))
                    simRatio = simRatio * 2.0;

                // Add their values to the simulation
                enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
                enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;

                if (simRatio >= 2.5)
                    enemy.isSpellcaster() ? casterCount++ : casterCount--;
            }
        };

        const auto simSelf = [&]() {
            for (auto &a : Units::getUnits(PlayerState::Self)) {
                UnitInfo &ally = *a;
                if (!canAddToSim(ally))
                    continue;

                const auto allyRange = max(ally.getAirRange(), ally.getGroundRange());
                const auto allyReach = max(ally.getAirReach(), ally.getGroundReach());
                const auto widths = double(ally.getType().width() + ally.getTarget().getType().width()) / 2.0;
                const auto distance = max(0.0, ally.getEngDist() - widths);
                const auto speed = 24.0 * ally.getSpeed();
                auto simRatio = simulationTime - (distance / speed);

                // If the unit doesn't affect this simulation
                if (ally.localRetreat()
                    || simRatio <= 0.0
                    || (ally.getSpeed() <= 0.0 && distance > 0.0)
                    || (ally.getEngagePosition().getDistance(unit.getTarget().getPosition()) > allyReach)
                    || (ally.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.getTarget().getPosition().getDistance(ally.getPosition()) < 64.0))
                    continue;

                // Hidden bonus
                if (ally.isHidden())
                    simRatio = simRatio * 2.0;

                // High ground bonus
                if (!ally.getType().isFlyer() && allyRange > 32.0 && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTarget().getPosition())))
                    simRatio = simRatio * 2.0;

                // Check if air/ground sim needs to sync
                if (!sync && simRatio > 0.0 && ((unit.getType().isFlyer() && !ally.getType().isFlyer()) || (!unit.getType().isFlyer() && ally.getType().isFlyer())))
                    sync = true;

                //// Check if we need to squeeze these units through a choke
                //if (applySqueezeFactor(ally) && int(ally.getQuickPath().size()) <= 3) {
                //    auto &path = ally.getQuickPath();
                //    auto targetReach = ally.getType().isFlyer() ? ally.getTarget().getAirReach() : ally.getTarget().getGroundReach();

                //    for (auto &choke : path) {
                //        if (choke->Width() <= 128 && choke->Width() > 0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach) {
                //            simRatio = simRatio * clamp(squeezeFactor[choke], 0.25, 1.0);
                //            break;
                //        }
                //    }

                //    if (ally.getQuickPath().empty()) {
                //        auto choke = Util::getClosestChokepoint(ally.getPosition());
                //        if (choke && choke->Width() <= 128 && choke->Width() > 0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach)
                //            simRatio = simRatio * clamp(squeezeFactor[choke], 0.25, 1.0);
                //    }
                //}

                // Add their values to the simulation
                allyLocalGroundStrength += ally.getVisibleGroundStrength() * simRatio;
                allyLocalAirStrength += ally.getVisibleAirStrength() * simRatio;

                // Check if any ally are below the limit
                if (ally.getSimValue() < minThreshold && ally.getSimValue() != 0.0) {
                    if (ally.isFlying())
                        ally.getTarget().isFlying() ? belowAirtoAirLimit = true : belowAirtoGrdLimit = true;
                    else
                        ally.getTarget().isFlying() ? belowGrdtoAirLimit = true : belowGrdtoGrdLimit = true;
                }
            }
        };

        simTerrain();
        simEnemies();
        simSelf();

        // Assign the sim value
        const auto attackAirAsAir =         enemyLocalAirStrength > 0.0 ? allyLocalAirStrength / enemyLocalAirStrength : 10.0;
        const auto attackAirAsGround =      enemyLocalGroundStrength > 0.0 ? allyLocalAirStrength / enemyLocalGroundStrength : 10.0;
        const auto attackGroundAsAir =      enemyLocalAirStrength > 0.0 ? allyLocalGroundStrength / enemyLocalAirStrength : 10.0;
        const auto attackGroundAsGround =   enemyLocalGroundStrength > 0.0 ? allyLocalGroundStrength / enemyLocalGroundStrength : 10.0;
        unit.getType().isFlyer() ? unit.setSimValue(unit.getTarget().getType().isFlyer() ? attackAirAsAir : attackGroundAsAir) : unit.setSimValue(unit.getTarget().getType().isFlyer() ? attackAirAsGround : attackGroundAsGround);

        // If above/below thresholds, it's a sim win/loss
        if (unit.getSimValue() >= maxThreshold)
            unit.setSimState(SimState::Win);
        else if (unit.getSimValue() <= minThreshold || (unit.getSimState() == SimState::None && unit.getSimValue() < maxThreshold))
            unit.setSimState(SimState::Loss);

        //// Check for hardcoded directional losses
        //if (unit.isFlying()) {
        //    if (unit.getTarget().isFlying() && belowAirtoAirLimit)
        //        unit.setSimState(SimState::Loss);
        //    else if (unit.getTarget().isFlying() && belowAirtoGrdLimit)
        //        unit.setSimState(SimState::Loss);
        //}
        //else {
        //    if (unit.getTarget().isFlying() && belowGrdtoAirLimit)
        //        unit.setSimState(SimState::Loss);
        //    else if (unit.getTarget().isFlying() && belowGrdtoGrdLimit)
        //        unit.setSimState(SimState::Loss);
        //}
    }
}