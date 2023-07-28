#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Destination {

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

    void updateRetreat(UnitInfo& unit)
    {
        unit.setRetreat(Positions::Invalid);
        auto retreat = Stations::getClosestRetreatStation(unit);

        if (Stations::getStations(PlayerState::Self).size() == 1)
            unit.setRetreat(Terrain::getMainPosition());
        else if (retreat)
            unit.setRetreat(retreat->getBase()->Center());
        else
            unit.setRetreat(Terrain::getMainPosition());
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
            else if (unit.getInterceptPosition().isValid()) {
                unit.setDestination(unit.getInterceptPosition());
            }
            else if (unit.getSurroundPosition().isValid()) {
                unit.setDestination(unit.getSurroundPosition());
            }
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
                //Visuals::drawLine(unit.getPosition(), unit.getGoal(), Colors::Yellow);
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

        //Visuals::drawLine(unit.getPosition(), unit.getDestination(), Colors::Cyan);
        //Visuals::drawLine(unit.getPosition(), unit.getNavigation(), Colors::Orange);
    }

    void onFrame()
    {
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;
            if (unit.getRole() == Role::Combat) {
                updateRetreat(unit);
                updateDestination(unit);
            }
        }
    }
}