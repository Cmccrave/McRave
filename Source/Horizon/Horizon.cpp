#include "..\McRave\McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Horizon {

    namespace {
        bool ignoreSim = false;
    }

    void simulate(UnitInfo& unit)
    {
        /*
        Modified version of Horizon.
        Need to test deadzones and squeeze factors still.
        */

        auto minThreshold = max(0.0, log(10000.0 / Broodwar->getFrameCount())) + 0.50;
        auto maxThreshold = max(0.0, log(10000.0 / Broodwar->getFrameCount())) + 0.80;
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

        // Figure out whether it's likely the unit will move horizontally or vertically to its target
        const auto straightEngagePosition =[&](UnitInfo& u) {
            auto dx = unit.getPosition().x - unit.getEngagePosition().x;
            auto dy = unit.getPosition().y - unit.getEngagePosition().y;

            if (abs(dx) < abs(dy))
                return unit.getEngagePosition() + Position(dx, 0);
            if (abs(dx) > abs(dy))
                return unit.getEngagePosition() + Position(0, dy);
            return Positions::Invalid;
        };

        auto unitStraightEngage = straightEngagePosition(unit);

        const auto shouldIgnoreSim = [&]() {
            // If we have excessive resources, ignore our simulation and engage
            if (!ignoreSim && Broodwar->self()->minerals() >= 2000 && Broodwar->self()->gas() >= 2000 && Units::getSupply() >= 380)
                ignoreSim = true;
            if (ignoreSim && Broodwar->self()->minerals() <= 500 || Broodwar->self()->gas() <= 500 || Units::getSupply() <= 240)
                ignoreSim = false;

            if (ignoreSim || (Terrain::isIslandMap() && !unit.getType().isFlyer())) {
                unit.setSimState(SimState::Win);
                unit.setSimValue(10.0);
                return true;
            }
            else if (!unit.hasTarget()) {
                unit.setSimState(SimState::None);
                unit.setSimValue(0.0);
                return true;
            }
            else if (unit.getEngDist() == DBL_MAX) {
                unit.setSimState(SimState::Loss);
                unit.setSimValue(0.0);
                return true;
            }
            return false;
        };

        const auto applySqueezeFactor = [&](UnitInfo& source) {
            return false; // Too slow atm

            if (source.getPlayer() == Broodwar->self() && (!source.hasTarget() || source.getTarget().getType().isFlyer() || !source.getTarget().getPosition().isValid()))
                return false;
            if (source.getPlayer() != Broodwar->self() && (!unit.hasTarget() || unit.getTarget().getType().isFlyer() || !unit.getTarget().getPosition().isValid()))
                return false;
            if (unit.getType().isFlyer() || source.getType().isFlyer() || !source.getPosition().isValid() || source.unit()->isLoaded() || unit.unit()->isLoaded())
                return false;
            if (source.hasTarget() && !source.getTarget().getPosition().isValid())
                return false;
            if (!mapBWEM.GetArea(source.getTilePosition()))
                return false;
            return true;
        };

        const auto addToSim = [&](UnitInfo& u) {
            if (u.getVisibleAirStrength() <= 0.0 && u.getVisibleGroundStrength() <= 0.0)
                return false;
            if (u.getPlayer() == Broodwar->self() && !u.hasTarget())
                return false;
            if (!u.unit() || u.getType().isWorker() || (u.unit() && u.unit()->exists() && (u.unit()->isStasised() || u.unit()->isMorphing())))
                return false;
            if (u.getRole() != Role::None && u.getRole() != Role::Combat)
                return false;
            return true;
        };

        const auto simTerrain = [&]() {
            // For every ground unit, add a squeeze factor for navigating chokes
            for (auto &e : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &enemy = *e;
                if (applySqueezeFactor(enemy)) {
                    auto path = enemy.sameTile() ? enemy.getQuickPath() : mapBWEM.GetPath(enemy.getPosition(), unit.getPosition());
                    enemy.setQuickPath(path);
                    for (auto &choke : path) {
                        if (enemy.getGroundReach() < enemy.getPosition().getDistance(Position(choke->Center())))
                            enemySqueezeFactor[choke]+= double(enemy.getType().width());
                    }
                }
            }
            for (auto &a : Units::getUnits(PlayerState::Self)) {
                UnitInfo &ally = *a;
                if (applySqueezeFactor(ally)) {
                    auto path = ally.sameTile() ? ally.getQuickPath() : mapBWEM.GetPath(ally.getPosition(), ally.getTarget().getPosition());
                    ally.setQuickPath(path);
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
            for (auto &e : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &enemy = *e;
                if (!addToSim(enemy))
                    continue;

                auto deadzone = (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.getTarget().getPosition().getDistance(enemy.getPosition()) < 64.0) ? 64.0 : 0.0;
                auto widths = double(enemy.getType().width() + unit.getType().width()) / 2.0;
                auto enemyRange = (unit.getType().isFlyer() ? enemy.getAirRange() : enemy.getGroundRange());

                // If enemy is stationary, it must be in range of the engage position on a diagonal or straight line
                if (enemy.getSpeed() <= 0.0) {
                    auto diEngageDistance = enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange - widths /*+ deadzone*/;
                    //auto stEngageDistance = unitStraightEngage.getDistance(enemy.getPosition()) - enemyRange - widths + deadzone;
                    if (diEngageDistance > 64.0/* && stEngageDistance > 64.0*/)
                        continue;
                }

                auto distance = max(0.0, enemy.getPosition().getDistance(unit.getPosition()) - enemyRange - widths /*+ deadzone*/);
                auto speed = enemy.getSpeed() > 0.0 ? 24.0 * enemy.getSpeed() : 24.0 * unit.getSpeed();
                auto simRatio =  simulationTime - (distance / speed);

                if (simRatio <= 0.0)
                    continue;

                // Situations where an enemy should be treated as stronger than it actually is
                if (enemy.unit()->exists() && (enemy.unit()->isBurrowed() || enemy.unit()->isCloaked()) && !enemy.unit()->isDetected())
                    simRatio = simRatio * 2.0;
                if (!enemy.getType().isFlyer() && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition())))
                    simRatio = simRatio * 2.0;

                // Check if we need to squeeze these units through a choke
                // Disabled atm while Terrain analysis is simulated better
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
            for (auto &a : Units::getUnits(PlayerState::Self)) {
                UnitInfo &ally = *a;
                if (!addToSim(ally))
                    continue;

                auto deadzone = (ally.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.getTarget().getPosition().getDistance(ally.getPosition()) < 64.0) ? 64.0 : 0.0;
                auto engDist = ally.getEngDist();
                auto widths = double(ally.getType().width() + ally.getTarget().getType().width()) / 2.0;
                auto allyRange = (unit.getTarget().getType().isFlyer() ? ally.getAirRange() : ally.getGroundRange());
                auto speed = ally.hasTransport() ? 24.0 * ally.getTransport().getSpeed() : 24.0 * ally.getSpeed();

                auto distance = engDist - widths + deadzone;
                auto simRatio = simulationTime - (distance / speed);

                auto reach = max(ally.getAirReach(), ally.getGroundReach());

                // If the unit doesn't affect this simulation
                if (simRatio <= 0.0 || (ally.getSpeed() <= 0.0 && ally.getPosition().getDistance(unit.getTarget().getPosition()) - allyRange - widths > 64.0))
                    continue;

                // HACK: Bunch of hardcoded stuff
                if (ally.getTarget().getPosition().getDistance(unit.getTarget().getPosition()) > reach)
                    continue;
                if ((ally.getType() == UnitTypes::Protoss_Scout || ally.getType() == UnitTypes::Protoss_Corsair) && ally.getShields() < 30)
                    continue;
                if (ally.getType() == UnitTypes::Terran_Wraith && ally.getHealth() <= 100)
                    continue;
                if (ally.getType().maxShields() > 0 && ally.getPercentShield() < LOW_SHIELD_PERCENT_LIMIT && Broodwar->getFrameCount() < 8000)
                    continue;
                if (ally.getType().getRace() == Races::Zerg && ally.getPercentTotal() < LOW_BIO_PERCENT_LIMIT)
                    continue;
                if (ally.getType() == UnitTypes::Zerg_Mutalisk && Grids::getEAirThreat((WalkPosition)ally.getEngagePosition()) > 0.0 && ally.getHealth() <= 30)
                    continue;

                // Situations where an ally should be treated as stronger than it actually is
                if ((ally.unit()->isCloaked() || ally.unit()->isBurrowed()) && !Command::overlapsDetection(ally.unit(), ally.getEngagePosition(), PlayerState::Enemy))
                    simRatio = simRatio * 2.0;
                if (!ally.getType().isFlyer() && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTarget().getPosition())))
                    simRatio = simRatio * 2.0;

                if (!sync && simRatio > 0.0 && ((unit.getType().isFlyer() && !ally.getType().isFlyer()) || (!unit.getType().isFlyer() && ally.getType().isFlyer())))
                    sync = true;

                // Disabled atm while Terrain analysis is simulated better
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

        if (shouldIgnoreSim())
            return;

        //simTerrain();
        simEnemies();
        simMyUnits();

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
}