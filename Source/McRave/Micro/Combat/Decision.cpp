#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Decision {

    void updateDecision(UnitInfo& unit)
    {
        //Visuals::drawLine(unit.getPosition(), unit.getDestination(), Colors::Cyan);
        //Visuals::drawLine(unit.getPosition(), unit.getNavigation(), Colors::Orange);

        // Iterate commands, if one is executed then don't try to execute other commands
        static const auto commands ={ Command::misc, Command::special, Command::approach, Command::kite, Command::attack, Command::defend, Command::explore, Command::escort, Command::retreat, Command::move };
        for (auto cmd : commands) {
            if (cmd(unit))
                return;
        }
    }

    void updateSharedDecision(UnitInfo& unit, UnitInfo& commander)
    {
        // Use commands that the commander is using if okay to do so
        if (commander.getCommandType() == UnitCommandTypes::Attack_Unit && commander.hasTarget())
            unit.setCommand(commander.getCommandType(), *commander.getTarget().lock());
        else if (commander.getCommandType() == UnitCommandTypes::Move && !unit.isTargetedBySplash())
            unit.setCommand(commander.getCommandType(), commander.getCommandPosition());
        else if (commander.getCommandType() == UnitCommandTypes::Right_Click_Position && !unit.isTargetedBySplash())
            unit.setCommand(UnitCommandTypes::Right_Click_Position, commander.getCommandPosition());
        else
            updateDecision(unit);
    }

    void onFrame()
    {
        for (auto &cluster : Clusters::getClusters()) {

            // Update the commander first
            auto commander = cluster.commander.lock();
            if (commander && commander->isLightAir() && Units::commandAllowed(*commander))
                updateDecision(*commander);

            // Update remaining units
            for (auto &unit : cluster.units) {
                if (unit == &*commander && commander->isLightAir())
                    continue;
                if (!Units::commandAllowed(*unit))
                    continue;

                auto sharedDecision = cluster.commandShare == CommandShare::Exact && unit->getLocalState() != LocalState::ForcedRetreat && unit->getGlobalState() != GlobalState::ForcedRetreat && !unit->isNearSuicide()
                    && !unit->attemptingRegroup() && (unit->getType() == commander->getType() || unit->getLocalState() != LocalState::Attack || unit->getLocalState() != LocalState::ForcedAttack);

                // If it's a shared decision, replicate the commanders command
                if (sharedDecision)
                    updateSharedDecision(*unit, *commander);
                else
                    updateDecision(*unit);
            }
        }
    }
}