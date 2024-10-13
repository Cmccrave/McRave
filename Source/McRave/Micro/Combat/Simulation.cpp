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

        auto lastState = unit.getSimState();

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

        // If above/below thresholds, it's a sim win/loss
        if (unit.getSimValue() >= maxThreshold)
            unit.setSimState(SimState::Win);
        else if (unit.getSimValue() < minThreshold || (unit.getSimState() == SimState::None && unit.getSimValue() < maxThreshold))
            unit.setSimState(SimState::Loss);

        // Reset counter if we're losing
        if (unit.getSimState() == SimState::Loss)
            unit.framesCommitted = 0;

        // Only commit to a win after some debouncing
        if (unit.getSimState() == SimState::Win) {
            unit.framesCommitted++;
            if (unit.framesCommitted < 80)
                unit.setSimState(SimState::Loss);
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
            minWinPercent = 0.8;
            maxWinPercent = 1.4;
        }
        if (Players::ZvZ()) {
            minWinPercent = 0.7;
            maxWinPercent = 1.3;
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

        // Adjust winrates if we have static defense that would make the fight easier and we're at home
        if (Util::getTime() < Time(8, 00) && !unit.isFlying() && com(Zerg_Sunken_Colony) > 0 && Combat::State::isStaticRetreat(unit.getType())) {
            const auto defendStation = Stations::getClosestStationAir(unit.retreatPos, PlayerState::Self);
            const auto furthestSunk = Util::getFurthestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                return u->getType() == Zerg_Sunken_Colony && u->isCompleted() && Terrain::inArea(defendStation->getBase()->GetArea(), u->getPosition());
            });
            if (furthestSunk) {
                if (furthestSunk->isWithinRange(target)) {
                    minWinPercent = 0.0;
                    maxWinPercent = 0.0;
                }
                else {
                    minWinPercent *=4;
                    maxWinPercent *=4;
                }
            }
        }

        // Adjust winrates if we are all-in
        if (BuildOrder::isAllIn() && !Combat::State::isStaticRetreat(unit.getType()) && Util::getTime() < Time(8, 00)) {
            minWinPercent -= 0.20;
            maxWinPercent -= 0.10;
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

        for (auto &u : Units::getUnits(PlayerState::Self)) {
            UnitInfo &self = *u;
            if (!u->hasCommander())
                continue;

            if (u->hasCommander() && !u->isLightAir()) {
                u->setSimValue(u->getCommander().lock()->getSimValue());
                u->setSimState(u->getCommander().lock()->getSimState());
            }
        }
    }
}