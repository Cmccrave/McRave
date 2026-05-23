#include "Defender.h"

#include "Info/Unit/Units.h"
#include "Map/Stations/Stations.h"
#include "Micro/All/Commands.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Defender {

    namespace {

        void updateDecision(UnitInfo &unit)
        {
            if (!Units::commandAllowed(unit))
                return;

            // Iterate commands, if one is executed then don't try to execute other commands
            static const auto commands = {Command::misc, Command::attack};
            for (auto cmd : commands) {
                if (cmd(unit))
                    break;
            }
        }

        void updateFormation(UnitInfo &unit)
        {
            // Set formation to closest station chokepoint to align units to
            const auto closestStation = Stations::getClosestStationGround(unit.getPosition(), PlayerState::Self);
            if (closestStation) {
                auto defendPosition = Stations::getDefendPosition(closestStation);
                unit.setFormation(defendPosition);
            }
        }

        void updateDefenders()
        {
            // Update all my buildings
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() == Role::Defender) {
                    updateFormation(unit);
                    updateDecision(unit);
                }

                // HACK: Helps form static formations
                if (unit.getType().isResourceDepot()) {
                    const auto closestStation = Stations::getClosestStationGround(unit.getPosition(), PlayerState::Self);
                    auto stationDepot         = closestStation && unit.getTilePosition() == closestStation->getBase()->Location();
                    if (stationDepot)
                        updateFormation(unit);
                }

                //
                if (unit.getType().isBuilding() && unit.getType().getRace() != Races::Zerg)
                    updateFormation(unit);
            }
        }
    } // namespace

    void onFrame()
    {
        updateDefenders();
        Visuals::endPerfTest("Defenders");
    }
} // namespace McRave::Defender