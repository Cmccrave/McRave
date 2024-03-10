#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Bearings {

    void getCleanupPosition(UnitInfo& unit)
    {
        // Finish off positions that are old
        auto &list = unit.isFlying() ? Terrain::getAirCleanupPositions() : Terrain::getGroundCleanupPositions();
        auto posBest = Positions::Invalid;
        if (!list.empty()) {
            auto distBest = DBL_MAX;
            for (auto &pos : list) {
                const auto dist = pos.getDistance(unit.getPosition());
                if (dist < distBest) {
                    distBest = dist;
                    posBest = pos;
                }
            }
        }
        if (posBest.isValid())
            unit.setDestination(posBest);
        else
            unit.setDestination(Terrain::getMainPosition());
    }

    // What is the "backward" bearing for this unit
    void updateRetreat(UnitInfo& unit)
    {
        auto retreat = Stations::getClosestRetreatStation(unit);

        if (retreat)
            unit.retreatPos = retreat->getBase()->Center();
        else
            unit.retreatPos = Terrain::getMainPosition();
    }

    // What is the "forward" bearing for this unit
    void updateMarch(UnitInfo& unit)
    {
        // TODO: March station (depending on goal), like retreat       
        if (unit.getLocalState() == LocalState::Retreat || unit.getGlobalState() == GlobalState::Retreat || unit.getGlobalState() == GlobalState::ForcedRetreat) {
            auto retreat = Stations::getClosestRetreatStation(unit);
            unit.marchPos = Stations::getDefendPosition(retreat);
        }
        else if (unit.hasTarget()) {
            unit.marchPos = unit.getTarget().lock()->getPosition();
        }
        else if (Combat::getAttackPosition().isValid() && unit.canAttackGround()) {
            unit.marchPos = Combat::getAttackPosition();
        }
        else {
            unit.marchPos = unit.getDestination();
            Broodwar->drawLineMap(unit.getPosition(), unit.marchPos, Colors::Green);
        }
    }

    void updateDestination(UnitInfo& unit)
    {
        auto retreat = Stations::getClosestRetreatStation(unit);

        if (unit.getGoal().isValid() && unit.getGoalType() == GoalType::Explore) {
            unit.setDestination(unit.getGoal());
        }

        // If attacking and target is close, set as destination
        else if (unit.getLocalState() == LocalState::Attack || unit.getLocalState() == LocalState::ForcedAttack) {
            if (unit.attemptingRunby()) {
                unit.setDestination(unit.getEngagePosition());
            }
            //else if (unit.getInterceptPosition().isValid()) {
            //    unit.setDestination(unit.getInterceptPosition());
            //}
            //else if (unit.getSurroundPosition().isValid()) {
            //    unit.setDestination(unit.getSurroundPosition());
            //}
            else if (!unit.isFlying() && unit.getEngagePosition().isValid()) {
                unit.setDestination(unit.getEngagePosition());
            }
            else if (unit.hasTarget()) {
                unit.setDestination(unit.getTarget().lock()->getPosition());
            }
        }
        else if (unit.getLocalState() == LocalState::Retreat || unit.getLocalState() == LocalState::ForcedRetreat || unit.getGlobalState() == GlobalState::Retreat || unit.getGlobalState() == GlobalState::ForcedRetreat) {
            if (unit.getGoal().isValid() && unit.getGoalType() == GoalType::Defend) {
                unit.setDestination(unit.getGoal());
            }
            else if (unit.getGlobalState() != GlobalState::ForcedRetreat && unit.attemptingRegroup()) {
                unit.setDestination(unit.getCommander().lock()->getPosition());
            }
            else if (retreat && unit.isFlying()) {
                unit.setDestination(retreat->getBase()->Center());
            }
            else if (retreat) {
                unit.setDestination(Stations::getDefendPosition(retreat));
            }
            else {
                unit.setDestination(Position(Terrain::getMainChoke()->Center()));
            }
        }
        else {
            if (unit.getGoal().isValid()) {
                unit.setDestination(unit.getGoal());
            }
            else if (unit.attemptingRegroup()) {
                unit.setDestination(unit.getCommander().lock()->getPosition());
            }
            else if (unit.attemptingHarass()) {
                unit.setDestination(Combat::getHarassPosition());
            }
            else if (unit.hasTarget()) {
                unit.setDestination(unit.getTarget().lock()->getPosition());
            }
            else if (Combat::getAttackPosition().isValid() && unit.canAttackGround()) {
                unit.setDestination(Combat::getAttackPosition());
            }
            else {
                getCleanupPosition(unit);
            }
        }
    }

    void onFrame()
    {
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;
            if (unit.getRole() == Role::Combat) {
                updateDestination(unit);
                updateRetreat(unit);
                updateMarch(unit);
            }
        }
    }
}