#include "Combat.h"
#include "Info/Unit/Units.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Bearings {

    void getCleanupPosition(UnitInfo &unit)
    {
        // Scourge with nothing to do should stay close to home or a flying commander
        if (unit.getType() == Zerg_Scourge) {
            const auto closestMuta = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) { return u->getType() == Zerg_Mutalisk && u->getGlobalState() == GlobalState::Attack; });
            if (closestMuta) {
                unit.setDestination(closestMuta->getPosition());
                return;
            }
            else {
                const auto closestStation = Stations::getClosestStationAir(unit.getPosition(), PlayerState::Self);
                if (closestStation) {
                    unit.setDestination(closestStation->getBase()->Center());
                    return;
                }
            }
        }

        // Finish off positions that are old
        auto &list   = unit.isFlying() ? Terrain::getAirCleanupPositions() : Terrain::getGroundCleanupPositions();
        auto posBest = Positions::Invalid;
        if (!list.empty()) {
            auto distBest = DBL_MAX;
            for (auto &pos : list) {
                const auto dist = pos.getDistance(unit.getPosition());
                if (dist < distBest) {
                    distBest = dist;
                    posBest  = pos;
                }
            }
        }
        if (posBest.isValid())
            unit.setDestination(posBest);
        else
            unit.setDestination(Terrain::getMainPosition());
    }

    // What is the "backward" bearing for this unit
    void updateRetreat(UnitInfo &unit) {}

    // What is the "forward" bearing for this unit
    void updateMarch(UnitInfo &unit) {}

    void updateDestination(UnitInfo &unit)
    {
        if (unit.getDestination().isValid()) {
            Broodwar->drawTextMap(unit.getPosition(), "z_stale");
            return;
        }

        auto retreat    = Stations::getClosestRetreatStation(unit);
        unit.marchPos   = Stations::getDefendPosition(retreat);
        unit.retreatPos = Stations::getHoldPosition(retreat);

        // If attacking and target is close, set as destination
        if (unit.getLocalState() == LocalState::Attack) {
            if (unit.attemptingRegroup()) {
                unit.setDestination(unit.getCommander().lock()->getPosition());
                Broodwar->drawTextMap(unit.getPosition(), "a_regrp");
            }
            else if (unit.getInterceptPosition().isValid()) {
                unit.setDestination(unit.getInterceptPosition());
                Broodwar->drawTextMap(unit.getPosition(), "a_intercept");
            }
            else if (unit.attemptingTrap()) {
                unit.setDestination(unit.getTrapPosition());
                Broodwar->drawTextMap(unit.getPosition(), "a_trap");
            }
            else if (unit.attemptingSurround()) {
                unit.setDestination(unit.getSurroundPosition());
                Broodwar->drawTextMap(unit.getPosition(), "a_surround");
            }
            else if (unit.hasTarget()) {
                unit.setDestination(unit.getTarget().lock()->getPosition());
                Broodwar->drawTextMap(unit.getPosition(), "a_target");
            }
            else {
                Broodwar << "no dest" << endl;
            }
            unit.marchPos = unit.getDestination();
        }
        else if (unit.getLocalState() == LocalState::Retreat || unit.getGlobalState() == GlobalState::Retreat) {
            if (unit.getGoal().isValid() && unit.getGoalType() == GoalType::Defend) {
                unit.setDestination(unit.getGoal());
                Broodwar->drawTextMap(unit.getPosition(), "r_goal");
            }
            else if (unit.getType() == Terran_Wraith && unit.saveUnit) {
                auto closestRepair = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) { return u->getType().isWorker() && u->getRole() == Role::Combat; });
                if (closestRepair) {
                    unit.setDestination(closestRepair->getPosition());
                    Broodwar->drawTextMap(unit.getPosition(), "r_repair");
                }
            }
            else if (retreat) {
                unit.setDestination(retreat->getBase()->Center());
                Broodwar->drawTextMap(unit.getPosition(), "r_retreat");
            }
            else {
                unit.setDestination(Position(Terrain::getMainChoke()->Center()));
                Broodwar->drawTextMap(unit.getPosition(), "r_choke");
            }
        }
        else {
            if (unit.getGoal().isValid()) {
                unit.setDestination(unit.getGoal());
                Broodwar->drawTextMap(unit.getPosition(), "z_goal");
            }
            else if (unit.attemptingRegroup()) {
                unit.setDestination(unit.getCommander().lock()->getPosition());
                Broodwar->drawTextMap(unit.getPosition(), "z_regrp");
            }
            else if (Combat::getHarassPosition().isValid() && unit.attemptingHarass()) {
                unit.setDestination(Combat::getHarassPosition());
                Broodwar->drawTextMap(unit.getPosition(), "z_harass");
            }
            else if (unit.hasTarget()) {
                unit.setDestination(unit.getTarget().lock()->getPosition());
                Broodwar->drawTextMap(unit.getPosition(), "z_target");
            }
            else if (Combat::getAttackPosition().isValid() && unit.canAttackGround()) {
                unit.setDestination(Combat::getAttackPosition());
                Broodwar->drawTextMap(unit.getPosition(), "z_atkpos");
            }
            else {
                getCleanupPosition(unit);
                Broodwar->drawTextMap(unit.getPosition(), "z_clean");
            }
            unit.marchPos = unit.getDestination();
        }

        Visuals::drawLine(unit.getPosition(), unit.getDestination(), Colors::Cyan);
    }

    void onFrame()
    {
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;
            if (unit.getRole() == Role::Combat) {
                updateDestination(unit);
                updateRetreat(unit);
                updateMarch(unit);
                Broodwar << unit.getDestination() << endl;
            }
        }
    }
} // namespace McRave::Combat::Bearings