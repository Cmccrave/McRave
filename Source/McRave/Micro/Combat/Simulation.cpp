#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Simulation {

    namespace {
        bool ignoreSim = false;
        double minWinPercent = 0.6;
        double maxWinPercent = 1.2;
        double minThreshold = 0.0;
        double maxThreshold = 0.0;
    }

    void updateSimulation(UnitInfo& unit)
    {
        if (unit.getEngDist() == DBL_MAX || !unit.hasTarget()) {
            unit.setSimState(SimState::Loss);
            unit.setSimValue(0.0);
            return;
        }

        // If we have excessive resources, ignore our simulation and engage
        if (!ignoreSim && Broodwar->self()->minerals() >= 2000 && Broodwar->self()->gas() >= 2000 && Players::getSupply(PlayerState::Self, Races::None) >= 380)
            ignoreSim = true;
        if (ignoreSim && (Broodwar->self()->minerals() <= 500 || Broodwar->self()->gas() <= 500 || Players::getSupply(PlayerState::Self, Races::None) <= 240))
            ignoreSim = false;

        if (ignoreSim) {
            unit.setSimState(SimState::Win);
            unit.setSimValue(10.0);
            return;
        }

        auto belowGrdtoGrdLimit = false;
        auto belowGrdtoAirLimit = false;
        auto belowAirtoAirLimit = false;
        auto belowAirtoGrdLimit = false;
        auto engagedAlreadyOffset = unit.getSimState() == SimState::Win ? 0.2 : 0.0;
        auto unitTarget = unit.getTarget().lock();

        // Check if any allied unit is below the limit to synchronize sim values
        for (auto &a : Units::getUnits(PlayerState::Self)) {
            UnitInfo &self = *a;

            if (self.hasTarget()) {
                auto selfTarget = self.getTarget().lock();
                if (selfTarget == unitTarget && self.getSimValue() <= minThreshold && self.getSimValue() != 0.0) {
                    self.isFlying() ?
                        (selfTarget->isFlying() ? belowAirtoAirLimit = true : belowAirtoGrdLimit = true) :
                        (selfTarget->isFlying() ? belowGrdtoAirLimit = true : belowGrdtoGrdLimit = true);
                }
            }
        }
        for (auto &a : Units::getUnits(PlayerState::Ally)) {
            UnitInfo &ally = *a;

            if (ally.hasTarget()) {
                auto allyTarget = ally.getTarget().lock();
                if (ally.getTarget() == unit.getTarget() && ally.getSimValue() <= minThreshold && ally.getSimValue() != 0.0) {
                    ally.isFlying() ?
                        (allyTarget->isFlying() ? belowAirtoAirLimit = true : belowAirtoGrdLimit = true) :
                        (allyTarget->isFlying() ? belowGrdtoAirLimit = true : belowGrdtoGrdLimit = true);
                }
            }
        }

        // If above/below thresholds, it's a sim win/loss
        if (unit.getSimValue() >= maxThreshold - engagedAlreadyOffset)
            unit.setSimState(SimState::Win);
        else if (unit.getSimValue() < minThreshold - engagedAlreadyOffset || (unit.getSimState() == SimState::None && unit.getSimValue() < maxThreshold))
            unit.setSimState(SimState::Loss);

        // Check for hardcoded directional losses
        if (unit.getSimValue() < maxThreshold - engagedAlreadyOffset) {
            if (unit.isFlying()) {
                if (unitTarget->isFlying() && belowAirtoAirLimit)
                    unit.setSimState(SimState::Loss);
                else if (!unitTarget->isFlying() && belowAirtoGrdLimit)
                    unit.setSimState(SimState::Loss);
            }
            else {
                if (unitTarget->isFlying() && belowGrdtoAirLimit)
                    unit.setSimState(SimState::Loss);
                else if (!unitTarget->isFlying() && belowGrdtoGrdLimit)
                    unit.setSimState(SimState::Loss);
            }
        }
    }

    void updateThresholds(UnitInfo& unit)
    {
        if (!unit.hasTarget())
            return;
        auto unitTarget = unit.getTarget().lock();

        // P
        if (Players::PvP()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
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
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
        }
        if (Players::ZvZ()) {
            minWinPercent = 1.0;
            maxWinPercent = 1.4;
        }
        if (Players::ZvT()) {
            minWinPercent = 1.0;
            maxWinPercent = 1.4;
        }

        // Z
        if (Players::TvP()) {
            minWinPercent = 1.0;
            maxWinPercent = 1.4;
        }
        if (Players::TvZ()) {
            minWinPercent = 1.0;
            maxWinPercent = 1.4;
        }
        if (Players::TvT()) {
            minWinPercent = 1.0;
            maxWinPercent = 1.4;
        }

        // Adjust winrates based on game duration
        // Early on we want confident engagements, later on we can be lenient
        // TODO

        // Adjust winrates based on how close to a station we are
        auto closestSelf = unit.isFlying() ? Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self) : Stations::getClosestStationGround(unit.getPosition(), PlayerState::Self);
        //auto closestEnemy = unit.isFlying() ? Stations::getClosestStationAir(unit.getPosition(), PlayerState::Enemy) : Stations::getClosestStationGround(unit.getPosition(), PlayerState::Enemy);

        if (closestSelf /*&& closestEnemy*/) {
            const auto distSelf = unit.isFlying() ? min(unit.getPosition().getDistance(closestSelf->getBase()->Center()), unitTarget->getPosition().getDistance(closestSelf->getBase()->Center()))
                : min(BWEB::Map::getGroundDistance(unit.getPosition(), closestSelf->getBase()->Center()), BWEB::Map::getGroundDistance(unitTarget->getPosition(), closestSelf->getBase()->Center()));
            //const auto distEnemy = unit.isFlying() ? min(unit.getPosition().getDistance(closestEnemy->getBase()->Center()), unitTarget->getPosition().getDistance(closestEnemy->getBase()->Center()))
            //    : min(BWEB::Map::getGroundDistance(unit.getPosition(), closestEnemy->getBase()->Center()), BWEB::Map::getGroundDistance(unitTarget->getPosition(), closestEnemy->getBase()->Center()));

            const auto dist = clamp(200.0, Util::getTime().minutes * 50.0, 640.0);
            const auto diffAllowed = 0.50;
            const auto diffSelf = diffAllowed * (1.0 - exp(max(0.0, dist - distSelf) / dist));
            //const auto diffEnemy = diffAllowed * exp(max(0.0, dist - distEnemy) / dist);
            minWinPercent = minWinPercent /*+ diffEnemy*/ - diffSelf;
            maxWinPercent = maxWinPercent /*+ diffEnemy*/ - diffSelf;
        }

        minThreshold = minWinPercent;
        maxThreshold = maxWinPercent;
    }

    void onFrame()
    {
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;
            if (unit.getRole() == Role::Combat) {
                updateThresholds(unit);
                Horizon::simulate(unit);
                updateSimulation(unit);
            }
        }
    }
}