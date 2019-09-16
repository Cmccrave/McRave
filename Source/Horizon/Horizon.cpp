#include "..\McRave\McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Horizon {

    namespace {
        bool ignoreSim = false;
        double minThreshold;
        double maxThreshold;

        void updateThresholds()
        {
            double minWinPercent = 0.5;
            double maxWinPercent = 1.0;

            if (Players::PvP()) {
                minWinPercent = 0.80;
                maxWinPercent = 1.20;
            }
            if (Players::PvZ()) {
                minWinPercent = 0.6;
                maxWinPercent = 1.2;
            }
            if (Players::PvT()) {
                minWinPercent = 0.4;
                maxWinPercent = 1.0;
            }

            minThreshold = max(0.0, log(10000.0 / Broodwar->getFrameCount())) + minWinPercent;
            maxThreshold = max(0.0, log(10000.0 / Broodwar->getFrameCount())) + maxWinPercent;

            if (BuildOrder::isRush()) {
                minThreshold = 0.00;
                maxThreshold = 1.00;
            }
        }
    }

    void simulate(UnitInfo& unit)
    {
        updateThresholds();
        auto enemyLocalGroundStrength = 0.0, allyLocalGroundStrength = 0.0;
        auto enemyLocalAirStrength = 0.0, allyLocalAirStrength = 0.0;
        auto unitToEngage = unit.getSpeed() > 0.0 ? unit.getEngDist() / (24.0 * unit.getSpeed()) : 5.0;
        auto simulationTime = unitToEngage + 5.0;
        auto unitSpeed = unit.getSpeed() * 24.0;
        auto sync = false;
        auto belowGrdLimits = false;
        auto belowAirLimits = false;
        auto unitArea = mapBWEM.GetArea(unit.getTilePosition());
        map<const BWEM::ChokePoint *, pair<int, double>> squeezeFactor;

        const auto shouldIgnoreSim = [&]() {
            // If we have excessive resources, ignore our simulation and engage
            if (!ignoreSim && Broodwar->self()->minerals() >= 2000 && Broodwar->self()->gas() >= 2000 && Players::getSupply(PlayerState::Self) >= 380)
                ignoreSim = true;
            if (ignoreSim && Broodwar->self()->minerals() <= 500 || Broodwar->self()->gas() <= 500 || Players::getSupply(PlayerState::Self) <= 240)
                ignoreSim = false;

            if (ignoreSim) {
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
            if (BuildOrder::isRush()
                || !source.getTilePosition().isValid()
                || !source.getTarget().getTilePosition().isValid()
                || source.hasTransport()
                || (!mapBWEM.GetArea(source.getTilePosition()) && !BWEB::Map::isWalkable(source.getTilePosition()))
                || (!mapBWEM.GetArea(source.getTarget().getTilePosition()) && !BWEB::Map::isWalkable(source.getTarget().getTilePosition())))
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
            // Check if any units are attempting to pass through a chokepoint
            for (auto &a : Units::getUnits(PlayerState::Self)) {
                UnitInfo &ally = *a;

                if (!canAddToSim(ally) || !applySqueezeFactor(ally) || int(ally.getQuickPath().size()) > 3)
                    continue;

                auto targetReach = ally.getType().isFlyer() ? ally.getTarget().getAirReach() : ally.getTarget().getGroundReach();

                if (!ally.getQuickPath().empty()) {

                    // Add their width to each chokepoint it needs to move through
                    for (auto &choke : ally.getQuickPath()) {
                        if (Util::chokeWidth(choke) <= 128.0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach) {
                            squeezeFactor[choke].second+= (double(Util::chokeWidth(choke)) * ally.getSpeed()) / (double(ally.getType().width()) * double(ally.getType().height()));
                            squeezeFactor[choke].first++;
                            break;
                        }
                    }
                }

                else {
                    auto choke = Util::getClosestChokepoint(ally.getEngagePosition());

                    if (choke && Util::chokeWidth(choke) <= 128.0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach) {
                        auto cost = (double(ally.getType().width()) * double(ally.getType().height())) / ((double(Util::chokeWidth(choke)) / 32.0) * ally.getSpeed());
                        squeezeFactor[choke].second+= log(cost);
                        squeezeFactor[choke].first++;
                    }
                }
            }

            for (auto &[choke, squeeze] : squeezeFactor) {
                auto cost = squeeze.second;
                auto count = log(squeeze.first);
                squeeze.second = 1.0 - exp(-20.0 / (cost * count));
            }
        };

        const auto simEnemies = [&]() {
            for (auto &e : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &enemy = *e;
                if (!canAddToSim(enemy))
                    continue;

                auto deadzone = (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.getTarget().getPosition().getDistance(enemy.getPosition()) < 64.0) ? 64.0 : 0.0;
                auto widths = double(enemy.getType().width() + unit.getType().width()) / 2.0;
                auto enemyRange = (unit.getType().isFlyer() ? enemy.getAirRange() : enemy.getGroundRange());

                // If enemy is stationary, it must be in range of the engage position on a diagonal or straight line
                if (enemy.getSpeed() <= 0.0) {
                    auto engageDistance = enemy.getPosition().getDistance(unit.getEngagePosition()) - enemyRange - widths;
                    if (engageDistance > 64.0)
                        continue;
                }

                auto distance = max(0.0, enemy.getPosition().getDistance(unit.getPosition()) - enemyRange - widths + deadzone);
                auto speed = enemy.getSpeed() > 0.0 ? 24.0 * enemy.getSpeed() : 24.0 * unit.getSpeed();
                auto simRatio =  simulationTime - (distance / speed);

                if (simRatio <= 0.0)
                    continue;

                // Situations where an enemy should be treated as stronger than it actually is
                if (enemy.isHidden())
                    simRatio = simRatio * 2.0;
                if (!enemy.getType().isFlyer() && Broodwar->getGroundHeight(enemy.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(unit.getEngagePosition())))
                    simRatio = simRatio * 2.0;

                // Add their values to the simulation
                enemyLocalGroundStrength += enemy.getVisibleGroundStrength() * simRatio;
                enemyLocalAirStrength += enemy.getVisibleAirStrength() * simRatio;
            }
        };

        const auto simSelf = [&]() {
            for (auto &a : Units::getUnits(PlayerState::Self)) {
                UnitInfo &ally = *a;
                if (!canAddToSim(ally))
                    continue;

                auto deadzone = (ally.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.getTarget().getPosition().getDistance(ally.getPosition()) < 64.0) ? 64.0 : 0.0;
                auto engDist = ally.getEngDist();
                auto widths = double(ally.getType().width() + ally.getTarget().getType().width()) / 2.0;
                auto allyRange = (unit.getTarget().getType().isFlyer() ? ally.getAirRange() : ally.getGroundRange());
                auto speed = 24.0 * ally.getSpeed();
                auto distance = max(0.0, engDist - widths + deadzone);
                auto simRatio = simulationTime - (distance / speed);
                auto reach = max(ally.getAirReach(), ally.getGroundReach());

                // If the unit doesn't affect this simulation
                if (ally.localRetreat()
                    || simRatio <= 0.0
                    || (ally.getSpeed() <= 0.0 && ally.getPosition().getDistance(unit.getTarget().getPosition()) - allyRange - widths > 64.0)
                    || (ally.getTarget().getPosition().getDistance(unit.getEngagePosition()) > max(ally.getGroundReach(), ally.getAirReach())))
                    continue;

                // Situations where an ally should be treated as stronger than it actually is
                if (ally.isHidden())
                    simRatio = simRatio * 2.0;
                if (!ally.getType().isFlyer() && Broodwar->getGroundHeight(TilePosition(ally.getEngagePosition())) > Broodwar->getGroundHeight(TilePosition(ally.getTarget().getPosition())))
                    simRatio = simRatio * 2.0;

                if (!sync && simRatio > 0.0 && ((unit.getType().isFlyer() && !ally.getType().isFlyer()) || (!unit.getType().isFlyer() && ally.getType().isFlyer())))
                    sync = true;

                // Check if we need to squeeze these units through a choke

                if (applySqueezeFactor(ally) && int(ally.getQuickPath().size()) <= 3) {
                    auto &path = ally.getQuickPath();
                    auto targetReach = ally.getType().isFlyer() ? ally.getTarget().getAirReach() : ally.getTarget().getGroundReach();

                    for (auto &choke : path) {
                        if (Util::chokeWidth(choke) <= 128.0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach) {
                            simRatio = simRatio * min(1.0, squeezeFactor[choke].second);
                            break;
                        }
                    }

                    if (ally.getQuickPath().empty()) {
                        auto choke = Util::getClosestChokepoint(ally.getPosition());
                        if (choke && Util::chokeWidth(choke) <= 128.0 && ally.getTarget().getPosition().getDistance(Position(choke->Center())) < targetReach)
                            simRatio = simRatio * min(1.0, squeezeFactor[choke].second);
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

        simTerrain();
        simEnemies();
        simSelf();

        const auto attackAirAsAir =         enemyLocalAirStrength > 0.0 ? allyLocalAirStrength / enemyLocalAirStrength : 10.0;
        const auto attackAirAsGround =      enemyLocalGroundStrength > 0.0 ? allyLocalAirStrength / enemyLocalGroundStrength : 10.0;
        const auto attackGroundAsAir =      enemyLocalAirStrength > 0.0 ? allyLocalGroundStrength / enemyLocalAirStrength : 10.0;
        const auto attackGroundasGround =   enemyLocalGroundStrength > 0.0 ? allyLocalGroundStrength / enemyLocalGroundStrength : 10.0;
        const auto belowLimits =            unit.getType().isFlyer() ? (belowAirLimits || (sync && belowGrdLimits)) : (belowGrdLimits || (sync && belowAirLimits));

        // Determine what sim value to use
        if (sync)
            unit.getType().isFlyer() ? unit.setSimValue(min(attackAirAsAir, attackGroundAsAir)) : unit.setSimValue(min(attackAirAsGround, attackGroundasGround));
        else
            unit.getType().isFlyer() ? unit.setSimValue(unit.getTarget().getType().isFlyer() ? attackAirAsAir : attackGroundAsAir) : unit.setSimValue(unit.getTarget().getType().isFlyer() ? attackAirAsGround : attackGroundasGround);

        // If above/below thresholds, it's a sim win/loss
        if (unit.getSimValue() >= maxThreshold && !belowLimits)
            unit.setSimState(SimState::Win);
        else if (unit.getSimValue() <= minThreshold || belowLimits || (unit.getSimState() == SimState::None && unit.getSimValue() < maxThreshold))
            unit.setSimState(SimState::Loss);
    }
}