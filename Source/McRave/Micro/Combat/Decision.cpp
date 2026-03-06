#include "Combat.h"
#include "Info/Unit/Units.h"
#include "Micro/All/Commands.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Decision {

    void updateDecision(UnitInfo &unit)
    {
        // Visuals::drawLine(unit.getPosition(), unit.getDestination(), Colors::Cyan);
        // Visuals::drawLine(unit.getPosition(), unit.getNavigation(), Colors::Orange);
        // if (unit.unit()->isSelected())

        // Iterate commands, if one is executed then don't try to execute other commands
        static const auto commands = {Command::misc,   Command::special, Command::approach, Command::kite,    Command::attack, Command::poke,
                                      Command::defend, Command::explore, Command::escort,   Command::retreat, Command::move};
        for (auto cmd : commands) {
            if (cmd(unit))
                return;
        }
    }

    void updateSharedDecision(UnitInfo &unit, UnitInfo &commander)
    {
        // Determine if we need to slightly nudge the command to better overlap units
        auto cmdPos = commander.getCommandPosition();
        if (unit.isLightAir() && unit.getPosition().getDistance(commander.getPosition()) > 8.0) {
            double dx = commander.getPosition().x - unit.getPosition().x;
            double dy = commander.getPosition().y - unit.getPosition().y;
            double uv = std::sqrt(dx * dx + dy * dy);

            if (uv > 0.0) {
                double nudgeAmount = 32.0 / uv;
                cmdPos             = Position(int(cmdPos.x + dx * nudgeAmount), int(cmdPos.y + dy * nudgeAmount));
            }
        }

        // Use commands that the commander is using if okay to do so
        unit.commandText = "";
        if (commander.getCommandType() == UnitCommandTypes::Attack_Unit && commander.hasTarget())
            unit.setCommand(commander.getCommandType(), *commander.getTarget().lock());
        else if (commander.getCommandType() == UnitCommandTypes::Move && !unit.isTargetedBySplash())
            unit.setCommand(commander.getCommandType(), cmdPos);
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

                auto sharedDecision = cluster.commandShare == CommandShare::Exact && unit->canMirrorCommander(*commander);

                // If it's a shared decision, replicate the commanders command
                if (sharedDecision)
                    updateSharedDecision(*unit, *commander);
                else
                    updateDecision(*unit);
            }
        }
    }
} // namespace McRave::Combat::Decision