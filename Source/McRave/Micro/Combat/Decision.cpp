#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Decision {

    constexpr tuple commands{ Command::misc, Command::special, Command::attack, Command::approach, Command::kite, Command::defend, Command::explore, Command::escort, Command::retreat, Command::move };

    void updateDecision(UnitInfo& unit)
    {
        // Convert our commands to strings to display what the unit is doing for debugging
        map<int, string> commandNames{
            make_pair(0, "Misc"),
            make_pair(1, "Special"),
            make_pair(2, "Attack"),
            make_pair(3, "Approach"),
            make_pair(4, "Kite"),
            make_pair(5, "Defend"),
            make_pair(6, "Explore"),
            make_pair(7, "Escort"),
            make_pair(8, "Retreat"),
            make_pair(9, "Move")
        };

        // Iterate commands, if one is executed then don't try to execute other commands
        int height = unit.getType().height() / 2;
        int width = unit.getType().width() / 2;
        int i = Util::iterateCommands(commands, unit);
        auto startText = unit.getPosition() + Position(-4 * int(commandNames[i].length() / 2), height);
        Broodwar->drawTextMap(startText, "%c%s", Text::White, commandNames[i].c_str());
    }

    void onFrame()
    {
        for (auto &cluster : Clusters::getClusters()) {

            // Update the commander first
            auto commander = cluster.commander.lock();
            if (commander)
                updateDecision(*commander);

            // Update remaining units
            for (auto &unit : cluster.units) {
                if (unit == &*commander)
                    continue;

                auto sharedDecision = cluster.commandShare == CommandShare::Exact && !unit->localRetreat() && !unit->globalRetreat() && !unit->isNearSuicide()
                    && !unit->attemptingRegroup() && (unit->getType() == commander->getType() || unit->getLocalState() != LocalState::Attack);

                // If it's a shared decision, replicate the commanders command
                if (sharedDecision) {
                    if (commander->getCommandType() == UnitCommandTypes::Attack_Unit && commander->hasTarget())
                        unit->command(commander->getCommandType(), *commander->getTarget().lock());
                    else if (commander->getCommandType() == UnitCommandTypes::Move && !unit->isTargetedBySplash())
                        unit->command(commander->getCommandType(), commander->getCommandPosition());
                    else if (commander->getCommandType() == UnitCommandTypes::Right_Click_Position && !unit->isTargetedBySplash())
                        unit->command(UnitCommandTypes::Right_Click_Position, commander->getCommandPosition());
                    else
                        updateDecision(*unit);
                }
                else
                    updateDecision(*unit);
            }
        }
    }
}