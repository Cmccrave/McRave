#include "Builds/All/BuildOrder.h"
#include "Combat.h"
#include "Horizon.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Simulation {

    namespace {
        bool ignoreSim       = false;
        double minWinPercent = 0.8;
        double maxWinPercent = 1.4;
        int commitFrames     = 0;
    } // namespace

    void updateSimulation(UnitInfo &unit)
    {
        Horizon::simulate(unit);

        auto lastState = unit.getSimState();

        if (unit.getEngDist() == DBL_MAX || !unit.hasTarget()) {
            unit.setSimState(SimState::Loss);
            unit.setSimValue(0.0);
            return;
        }

        // If we have excessive resources, ignore our simulation and engage
        if (!ignoreSim && Broodwar->self()->minerals() >= 4000 && Broodwar->self()->gas() >= 4000 && Players::getSupply(PlayerState::Self, Races::None) >= 380)
            ignoreSim = true;
        if (ignoreSim && (Broodwar->self()->minerals() <= 1000 || Broodwar->self()->gas() <= 1000 || Players::getSupply(PlayerState::Self, Races::None) <= 240))
            ignoreSim = false;

        if (ignoreSim) {
            unit.setSimState(SimState::Win);
            unit.setSimValue(10.0);
            return;
        }

        // If above/below thresholds, it's a sim win/loss
        auto newState = unit.getSimState();
        if (unit.getSimValue() >= maxWinPercent)
            newState = SimState::Win;
        else if (unit.getSimValue() < minWinPercent)
            newState = SimState::Loss;
        unit.setSimState(newState);

        //if (newState != lastState) {
        //    unit.framesCommitted++;
        //    if (unit.framesCommitted >= commitFrames) {
        //        unit.setSimState(newState);
        //        unit.framesCommitted = 0;
        //    }
        //    else {
        //        unit.setSimState(lastState);
        //    }
        //}
        //else {
        //    unit.framesCommitted = 0;
        //    unit.setSimState(newState);
        //}
    }

    void updateThresholds(UnitInfo &unit)
    {
        // P
        if (Players::PvP()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 12;
        }
        if (Players::PvZ()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 12;
        }
        if (Players::PvT()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 12;
        }
        if (Players::PvFFA()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 12;
        }

        // Z
        if (Players::ZvP()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 12;
        }
        if (Players::ZvZ()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 0;
        }
        if (Players::ZvT()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 12;
        }
        if (Players::ZvFFA()) {
            minWinPercent = 0.8;
            maxWinPercent = 1.2;
            commitFrames  = 0;
        }

        // T
        if (Players::TvP()) {
            minWinPercent = 0.7;
            maxWinPercent = 1.3;
            commitFrames  = 12;
        }
        if (Players::TvZ()) {
            minWinPercent = 0.7;
            maxWinPercent = 1.3;
            commitFrames  = 12;
        }
        if (Players::TvT()) {
            minWinPercent = 0.9;
            maxWinPercent = 1.3;
            commitFrames  = 12;
        }
        if (Players::TvFFA()) {
            minWinPercent = 0.9;
            maxWinPercent = 1.3;
            commitFrames  = 12;
        }
    }

    void updateIncentives(UnitInfo &unit)
    {
        // Can't have incentives without a target for now
        if (!unit.hasTarget())
            return;
        auto &target = *unit.getTarget().lock();

        // Adjust winrates if we have static defense that would make the fight easier and we're at home
        if (!unit.isFlying()) {
            const auto defendStation    = Stations::getClosestStationAir(unit.retreatPos, PlayerState::Self);
            const auto furthestDefender = Util::getFurthestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                return u->getType().isBuilding() && u->canAttackGround() && u->isCompleted() && defendStation && Terrain::inArea(defendStation->getBase()->GetArea(), u->getPosition());
            });
            if (furthestDefender && furthestDefender->isWithinRange(target)) {
                minWinPercent = 0.0;
                maxWinPercent = 0.0;
                return;
            }
        }

        // Adjust winrates if we are all-in
        if (BuildOrder::isAllIn() && !Combat::State::isStaticRetreat(unit.getType()) && Util::getTime() < Time(8, 00)) {
            minWinPercent -= 0.10;
            maxWinPercent += 0.10;
        }

        // Adjust winrate if targeting a worker
        if (target.getType().isWorker() || target.getType() == Zerg_Overlord) {
            minWinPercent *= 2.0;
            maxWinPercent *= 2.0;
        }
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
} // namespace McRave::Combat::Simulation