#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Defender {

    namespace {        

        void updateDecision(UnitInfo& unit)
        {
            // Iterate commands, if one is executed then don't try to execute other commands
            static const auto commands ={ Command::misc, Command::attack };
            for (auto cmd : commands) {
                if (cmd(unit))
                    break;
            }
        }

        void updateFormation(UnitInfo& unit)
        {
            // Set formation to closest station chokepoint to align units to
            const auto closestStation = Stations::getClosestStationGround(unit.getPosition(), PlayerState::Self);
            if (closestStation && closestStation->getChokepoint()) {
                auto defendPosition = Stations::getDefendPosition(closestStation);
                unit.setFormation(defendPosition);
            }

            // Add a zone to help with engagements
            Zones::addZone(unit.getPosition(), ZoneType::Defend, 1, int(unit.getGroundRange()));
            Zones::addZone(unit.getPosition(), ZoneType::Defend, 1, int(unit.getAirRange()));
        }

        void updateDefenders()
        {
            // Update all my buildings
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() == Role::Defender && unit.isAvailable()) {
                    updateFormation(unit);
                    updateDecision(unit);
                }

                // HACK: Helps form static formations
                if ((!Players::ZvZ() || unit.getType().isResourceDepot()) && unit.getType().isBuilding() && unit.getRole() == Role::Production) {
                    updateFormation(unit);
                }
            }
        }
    }

    void onFrame()
    {
        updateDefenders();
    }
}