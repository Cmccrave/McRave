#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Destination {

    namespace {

        vector<BWEB::Station*> combatScoutOrder;
    }

    void getCleanupPosition(UnitInfo& unit)
    {
        // Finish off positions that are old
        if (Util::getTime() > Time(1, 00)) {
            auto &list = unit.isFlying() ? Terrain::getAirCleanupPositions() : Terrain::getGroundCleanupPositions();
            if (!list.empty()) {
                unit.setDestination(*list.begin());
                list.erase(remove_if(list.begin(), list.end(), [&](auto &p) {
                    return unit.getPosition().getDistance(p) < 160.0;
                }), list.end());
            }
        }
    }

    void updateRetreat(UnitInfo& unit)
    {
        unit.setRetreat(Positions::Invalid);
        const auto &retreat = Stations::getClosestRetreatStation(unit);
        const auto lowGroundCount = (vis(Zerg_Zergling) < 12 && vis(Zerg_Hydralisk) < 6);

        // 1 Base - Retreat is Main
        // 2 Base - Retreat first is main choke
        // 2 Base - Retreat second is natural
        // x Base - Closest retreat station
        if (Stations::getStations(PlayerState::Self).size() == 1)
            unit.setRetreat(Terrain::getMainPosition());
        else if (Stations::getStations(PlayerState::Self).size() >= 2 && lowGroundCount)
            unit.setRetreat(Position(Terrain::getMainChoke()->Center()));
        else if (retreat)
            unit.setRetreat(retreat->getBase()->Center());
        else
            unit.setRetreat(Terrain::getMainPosition());
    }

    void updateDestination(UnitInfo& unit)
    {
        if (unit.getGoal().isValid() && unit.getGoalType() == GoalType::Explore) {
            unit.setDestination(unit.getGoal());
        }

        // If attacking and target is close, set as destination
        else if (unit.getLocalState() == LocalState::Attack) {
            if (unit.attemptingRunby()) {
                unit.setDestination(unit.getEngagePosition());
            }
            else if (unit.getInterceptPosition().isValid()) {
                unit.setDestination(unit.getInterceptPosition());
            }
            else if (unit.getSurroundPosition().isValid()) {
                unit.setDestination(unit.getSurroundPosition());
            }
            else if (unit.getEngagePosition().isValid()) {
                unit.setDestination(unit.getEngagePosition());
            }
            else if (unit.hasTarget()) {
                unit.setDestination(unit.getTarget().lock()->getPosition());
            }
        }
        else if (unit.getLocalState() == LocalState::Retreat || unit.getGlobalState() == GlobalState::Retreat) {
            const auto &retreat = Stations::getClosestRetreatStation(unit);

            if (!unit.globalRetreat() && unit.attemptingRegroup()) {
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
                unit.setDestination(Terrain::getHarassPosition());
            }
            else if (unit.hasTarget()) {
                unit.setDestination(unit.getTarget().lock()->getPosition());
            }
            else if (Terrain::getAttackPosition().isValid() && unit.canAttackGround()) {
                unit.setDestination(Terrain::getAttackPosition());
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