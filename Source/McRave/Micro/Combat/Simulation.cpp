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
        Horizon::simulate(unit);

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
        auto unitTarget = unit.getTarget().lock();
        auto selfEngaged = false;
        auto enemyEngaged = false;
        auto allyEngaged = false;

        // Check if any allied unit is below the limit to synchronize sim values
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            UnitInfo &self = *u;

            if (self.hasTarget()) {
                auto selfTarget = self.getTarget().lock();
                if (self.isWithinRange(*selfTarget) && self.getSimState() == SimState::Win)
                    selfEngaged = true;
                if (selfTarget == unitTarget && self.getSimValue() <= minThreshold && self.getSimValue() != 0.0) {
                    self.isFlying() ?
                        (selfTarget->isFlying() ? belowAirtoAirLimit = true : belowAirtoGrdLimit = true) :
                        (selfTarget->isFlying() ? belowGrdtoAirLimit = true : belowGrdtoGrdLimit = true);
                }
            }
        }
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            UnitInfo &enemy = *u;

            if (enemy.hasTarget()) {
                auto enemyTarget = enemy.getTarget().lock();
                if (enemy.isWithinRange(*enemyTarget) && enemy.hasAttackedRecently())
                    enemyEngaged = true;
            }
        }
        for (auto &u : Units::getUnits(PlayerState::Ally)) {
            UnitInfo &ally = *u;

            if (ally.hasTarget()) {
                auto allyTarget = ally.getTarget().lock();
                if (ally.isWithinRange(*allyTarget) && ally.hasAttackedRecently())
                    allyEngaged = true;
                if (ally.getTarget() == unit.getTarget() && ally.getSimValue() <= minThreshold && ally.getSimValue() != 0.0) {
                    ally.isFlying() ?
                        (allyTarget->isFlying() ? belowAirtoAirLimit = true : belowAirtoGrdLimit = true) :
                        (allyTarget->isFlying() ? belowGrdtoAirLimit = true : belowGrdtoGrdLimit = true);
                }
            }
        }

        if (selfEngaged || !enemyEngaged)
            maxThreshold = minThreshold;

        // If above/below thresholds, it's a sim win/loss
        if (unit.getSimValue() >= maxThreshold)
            unit.setSimState(SimState::Win);
        else if (unit.getSimValue() < minThreshold || (unit.getSimState() == SimState::None && unit.getSimValue() < maxThreshold))
            unit.setSimState(SimState::Loss);

        // Check for hardcoded directional losses
        if (unit.getSimValue() < maxThreshold) {
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
            minWinPercent = 1.0;
            maxWinPercent = 1.4;
        }
        if (Players::ZvZ()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
        }
        if (Players::ZvT()) {
            minWinPercent = 1.0;
            maxWinPercent = 1.4;
        }

        // T
        if (Players::TvP()) {
            minWinPercent = 0.7;
            maxWinPercent = 1.1;
        }
        if (Players::TvZ()) {
            minWinPercent = 0.7;
            maxWinPercent = 1.1;
        }
        if (Players::TvT()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
        }
    }

    void updateIncentives(UnitInfo& unit)
    {
        // Can't have incentives without a target for now
        if (!unit.hasTarget())
            return;
        auto &target = *unit.getTarget().lock();

        // Air units are way more powerful when clustered properly
        if (unit.isLightAir() && !target.isThreatening()) {
            auto density = Grids::getAirDensity(unit.getPosition(), PlayerState::Self);
            if (density < 5) {
                minWinPercent += 0.2;
                maxWinPercent += 0.2;
            }
        }

        // Adjust winrates if we have static defense that would make the fight easier
        if (Util::getTime() < Time(8, 00) && !unit.isFlying() && com(Zerg_Sunken_Colony) > 0 && (unit.getGlobalState() == GlobalState::Retreat || unit.getGlobalState() == GlobalState::ForcedRetreat)) {
            const auto defendStation = Stations::getClosestStationAir(unit.retreatPos, PlayerState::Self);
            const auto furthestSunk = Util::getFurthestUnit(unit.retreatPos, PlayerState::Self, [&](auto &u) {
                return u->getType() == Zerg_Sunken_Colony && u->isCompleted() && Terrain::inArea(defendStation->getBase()->GetArea(), u->getPosition());
            });
            if (furthestSunk) {
                if (furthestSunk->isWithinRange(target)) {
                    minWinPercent = 0.0;
                    maxWinPercent = 0.0;
                }
                else {
                    minWinPercent *=2;
                    maxWinPercent *=2;
                }
            }
        }

        // Adjust winrates based on how close to self station we are
        const auto insideDefendingChoke = (Combat::holdAtChoke() || Combat::isDefendNatural()) && Terrain::inTerritory(PlayerState::Self, target.getPosition()) && Terrain::inArea(Combat::getDefendArea(), target.getPosition());
        if (insideDefendingChoke || target.isThreatening()) {
            minWinPercent -= 0.5;
            maxWinPercent -= 0.5;
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
                updateIncentives(unit);
                updateSimulation(unit);
            }
        }
    }
}