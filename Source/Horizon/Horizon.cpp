#include "Builds/All/BuildOrder.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Horizon {

    namespace {

        struct SimStrength {
            double airToAir       = 0.0001;
            double airToGround    = 0.0001;
            double groundToAir    = 0.0001;
            double groundToGround = 0.0001;
            double combined       = 0.0001;
        };

        bool addToSim(UnitInfo &u)
        {
            if (u.getPlayer() != Broodwar->self()) {
                if (u.getType().isWorker() && !u.isProxy() && ((u.unit()->getOrder() != Orders::AttackUnit && !u.hasAttackedRecently()) || Util::getTime() > Time(6, 00)))
                    return false;
            }

            if (!u.unit() || (u.isStunned()) || (u.getVisibleAirStrength() <= 0.0 && u.getVisibleGroundStrength() <= 0.0) ||
                (u.getRole() != Role::None && u.getRole() != Role::Combat && u.getRole() != Role::Defender) || (u.getRole() == Role::Combat && u.getGlobalState() == GlobalState::Retreat) ||
                (!u.hasTarget() && !u.hasSimTarget()))
                return false;
            return true;
        }

        void addBonus(UnitInfo &u, UnitInfo &t, double &simRatio)
        {
            if (u.isHidden() && u.isWithinRange(t))
                simRatio *= 2.0;
            if (!u.isFlying() && !t.isFlying() && u.getGroundRange() > 32.0 && Broodwar->getGroundHeight(u.getTilePosition()) > Broodwar->getGroundHeight(TilePosition(t.getEngagePosition())))
                simRatio *= 1.15;
            if (u.getType().isWorker() && (!u.hasAttackedRecently() || BuildOrder::isRush()))
                simRatio /= 10.0;
            return;
        }

        void addSimStrength(SimStrength &sim, UnitInfo &unit, double ratio)
        {
            auto grd = unit.getVisibleGroundStrength() * ratio;
            auto air = unit.getVisibleAirStrength() * ratio;
            sim.combined += max(grd, air);

            if (unit.isFlying()) {
                sim.airToGround += grd;
                sim.airToAir += air;
            }
            else {
                sim.groundToGround += grd;
                sim.groundToAir += air;
            }
        }

        double addPrepTime(UnitInfo &unit)
        {
            if (unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode)
                return 65.0 / 24.0;
            if (unit.getType() == UnitTypes::Zerg_Lurker && !unit.isBurrowed())
                return 36.0 / 24.0;
            return 0.0;
        }
    } // namespace

    void simulate(UnitInfo &unit)
    {
        if (!unit.hasTarget())
            return;

        auto &unitTarget = unit.getTarget().lock();

        // Determine when to start tracking engagement times, if nothing is in range, it cannot
        // count towards simulation time
        const auto maxRange          = max({unit.getGroundRange(), unit.getAirRange(), unitTarget->getGroundRange(), unitTarget->getAirRange()});
        const auto maxSpeed          = max(unit.getSpeed(), unitTarget->getSpeed()) * 24.0;
        const auto rangeDisplacement = (Util::boxDistance(unit.getType(), unit.getPosition(), unitTarget->getType(), unitTarget->getPosition()) - maxRange) / maxSpeed;
        const auto timePad           = Util::getTime().minutes / 6;
        const auto unitToEngage      = unit.getSpeed() > 0.0 ? unit.getEngDist() / (24.0 * unit.getSpeed()) : 5.0;

        const auto extendDuration     = (unit.isLightAir() || Players::ZvZ()) ? 2.0 : 4.0;
        const auto simulationTime     = unitToEngage + extendDuration + addPrepTime(unit) /*- rangeDisplacement*/;
        const auto targetDisplacement = 0.0; // unitToEngage * unitTarget->getSpeed() * 24.0;
        map<Player, SimStrength> simStrengthPerPlayer;
        map<PlayerState, SimStrength> simStrengthPerState;

        auto combinedSim = false;

        for (auto &e : Units::getUnits(PlayerState::Enemy)) {
            UnitInfo &enemy = *e;
            if (!addToSim(enemy))
                continue;

            auto &enemyTarget      = enemy.hasTarget() ? enemy.getTarget().lock() : enemy.getSimTarget().lock();
            auto simRatio          = 0.0;
            const auto distUnknown = min(double(unit.getType().sightRange()), (Broodwar->getFrameCount() - enemy.getLastVisibleFrame()) * enemy.getSpeed());
            const auto distTarget  = max(0.0, double(Util::boxDistance(enemy.getType(), enemy.getPosition(), unit.getType(), unit.getPosition())));
            const auto distEngage  = max(0.0, double(Util::boxDistance(enemy.getType(), enemy.getPosition(), unit.getType(), unit.getEngagePosition())));
            const auto enemyRange  = max(enemy.getAirRange(), enemy.getGroundRange());
            const auto enemyReach  = max(enemy.getAirReach(), enemy.getGroundReach());

            // If the unit doesn't affect this simulation
            if ((enemy.getSpeed() <= 0.0 && distEngage - targetDisplacement > enemyRange + 32.0 && distTarget - targetDisplacement > enemyRange + 32.0) ||
                (enemy.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && distTarget < 64.0) ||
                (enemy.getSpeed() <= 0.0 && distTarget - targetDisplacement > enemyRange && enemyTarget->getSpeed() <= 0.0) ||
                (enemy.targetsFriendly() && unit.hasTarget() && enemy.getPosition().getDistance(unitTarget->getPosition()) >= enemyReach))
                continue;

            // If enemy doesn't move, calculate how long it will remain in range once in range
            if (enemy.getSpeed() <= 0.0) {
                const auto distance   = min(distTarget, distEngage);
                const auto speed      = enemyTarget->getSpeed() * 24.0;
                const auto engageTime = max(0.0, (distance - enemyRange) / speed);
                simRatio              = max(0.0, simulationTime - engageTime);
            }

            // If enemy can move, calculate how quickly it can engage
            else {
                const auto distance   = min(distTarget, distEngage) - distUnknown;
                const auto speed      = enemy.getSpeed() * 24.0;
                const auto engageTime = max(0.0, (distance - enemyRange) / speed);
                simRatio              = max(0.0, simulationTime - engageTime);
            }

            // Add their values to the simulation
            addBonus(enemy, *enemyTarget, simRatio);
            addSimStrength(simStrengthPerPlayer[enemy.getPlayer()], enemy, simRatio);
            addSimStrength(simStrengthPerState[PlayerState::Enemy], enemy, simRatio);

            //if (unit.unit()->isSelected())
            //    Broodwar->drawTextMap(enemy.getPosition(), "%.2f", simRatio);
        }

        for (auto &a : Units::getUnits(PlayerState::Self)) {
            UnitInfo &self = *a;
            if (!addToSim(self))
                continue;

            auto &selfTarget      = self.hasTarget() ? self.getTarget().lock() : self.getSimTarget().lock();
            const auto range      = max(self.getAirRange(), self.getGroundRange());
            const auto reach      = max(self.getAirReach(), self.getGroundReach());
            const auto distance   = self.getEngDist();
            const auto speed      = self.getSpeed() > 0.0 ? self.getSpeed() * 24.0 : unit.getSpeed() * 24.0;
            const auto engageTime = max(0.0, ((distance - range) / speed) - unitToEngage);
            auto simRatio         = max(0.0, simulationTime - engageTime - addPrepTime(self));

            // If the unit doesn't affect this simulation
            if ((self.getSpeed() <= 0.0 && self.getEngDist() > -16.0) || (self.getEngagePosition().getDistance(unitTarget->getPosition()) > reach * 2) ||
                (self.getGlobalState() == GlobalState::Retreat) || (Combat::State::isStaticRetreat(self.getType()) && !unitTarget->isThreatening()))
                continue;

            if (selfTarget->isFlying() != unitTarget->isFlying())
                combinedSim = true;

            // Add their values to the simulation
            addBonus(self, *selfTarget, simRatio);
            addSimStrength(simStrengthPerPlayer[self.getPlayer()], self, simRatio);
            addSimStrength(simStrengthPerState[PlayerState::Self], self, simRatio);

            //if (unit.unit()->isSelected())
            //    Broodwar->drawTextMap(self.getPosition(), "%.2f", simRatio);
        }

        for (auto &a : Units::getUnits(PlayerState::Ally)) {
            UnitInfo &ally = *a;
            if (!addToSim(ally))
                continue;

            auto &allyTarget      = ally.hasTarget() ? ally.getTarget().lock() : ally.getSimTarget().lock();
            const auto range      = max(ally.getAirRange(), ally.getGroundRange());
            const auto reach      = max(ally.getAirReach(), ally.getGroundReach());
            const auto distance   = double(Util::boxDistance(ally.getType(), ally.getPosition(), unit.getType(), unitTarget->getPosition()));
            const auto speed      = ally.getSpeed() > 0.0 ? ally.getSpeed() * 24.0 : unit.getSpeed() * 24.0;
            const auto engageTime = max(0.0, ((distance - range) / speed) - unitToEngage);
            auto simRatio         = max(0.0, simulationTime - engageTime - addPrepTime(ally));

            // If the unit doesn't affect this simulation
            if ((ally.getSpeed() <= 0.0 && ally.getEngDist() > -16.0) || (unit.hasTarget() && ally.hasTarget() && ally.getEngagePosition().getDistance(unitTarget->getPosition()) > reach))
                continue;

            if (unit.unit()->isSelected())
                Broodwar->drawTextMap(ally.getPosition(), "%.2f", simRatio);

            // Add their values to the simulation
            addBonus(ally, *allyTarget, simRatio);
            addSimStrength(simStrengthPerPlayer[ally.getPlayer()], ally, simRatio);
            addSimStrength(simStrengthPerState[PlayerState::Ally], ally, simRatio);
        }

        auto enemyStrength = simStrengthPerState[PlayerState::Enemy];
        auto selfStrength  = simStrengthPerState[PlayerState::Self];
        auto allyStrength  = simStrengthPerState[PlayerState::Ally];

        // Determine sim value based on max of enemy forces and max of self/ally forces
        auto addForces = Broodwar->getGameType() == GameTypes::Top_vs_Bottom;
        if (addForces) {
            // TODO
        }
        else {

            // Check if both raw engagement wins (flyer engages just a2a + g2a, ground engages just
            // a2g + g2g) Check if combined engagement wins If raw engagement is a stronger win than
            // combined, engage only if part of the raw engagement type i.e. Mutas engage alone vs
            // Tanks, they significantly outpower, while Lings would just be suicdal to engage Mutas
            // engaged vs Goliaths could make use of Lings however
            auto enemyVsAir = (simStrengthPerState[PlayerState::Enemy].airToAir + simStrengthPerState[PlayerState::Enemy].groundToAir);
            auto enemyVsGrd = (simStrengthPerState[PlayerState::Enemy].airToGround + simStrengthPerState[PlayerState::Enemy].groundToGround);
            auto combined   = simStrengthPerState[PlayerState::Self].combined / simStrengthPerState[PlayerState::Enemy].combined;

            // Occasionally solo engagements might be better than a combined engagement
            auto solo = 0.0;
            if (unitTarget->getType().isFlyer()) {
                if (unit.isFlying())
                    solo = simStrengthPerState[PlayerState::Self].airToAir / enemyVsAir;
                else
                    solo = simStrengthPerState[PlayerState::Self].groundToAir / enemyVsGrd;
            }
            else {
                if (unit.isFlying())
                    solo = simStrengthPerState[PlayerState::Self].airToGround / enemyVsAir;
                else
                    solo = simStrengthPerState[PlayerState::Self].groundToGround / enemyVsGrd;
            }

            // Assign the highest value
            if (combinedSim)
                unit.setSimValue(combined);
            else
                unit.setSimValue(solo);
        }
    }
} // namespace McRave::Horizon