// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"
#include "EventManager.h"

// *** TODO ***
// BWEB Destination south walls not working
// Re-power buildings
// Change UnitInfo::Command to take an Action
// Add stargates gradually in carrier builds
// Add frame timeouts for allowable enemy build detection
// Move command doesn't care about threat, probably should
// When choosing a tech or creating a building, allow certain unlocks/research
// Adjust Production to check for unlocks
// HT/DT reserving causes production stall
// Storm drops and storm cast limit testing
// Mines at chokepoints causes unit suicide with defend command
// DTs get stuck between core/gate/pylon
// Only produce one unit per building at a time to prevent overspending
// Obs/Arbiter positioning
// Units think they cant reach an enemy between buildings
// 2Gate reaction doesnt tech
// Proxy bunker targeting
// Path workers to point not blocked
// Scanned obs just pause
// Worker suicide
// Lifted units not targetable
// Allow retreating inside own territory
// Fix making obs AND forge for detection - 2 gate robo seems like it can make obs first

// Remove createWallPath, remove BFS, change to JPS and use incrementing between points to reserve path

// Antigas list
// NeoBisu build is off

// Need a target based isThreatening
// Examples:
// - Melee shouldn't engage ranged on ramps, back up instead
// - Engage when in range of defenses for FFE

// Scout targets
// - Check for Nexus when we see no gateways in PvP (find timing for this and check after this time based on last visible frame on our grid)
// - Check for 3rd against Z when we see 2 hatchery and no gas

// *** Ideas/Small fixes ***
// Floating units removing mine positions
// Monitor for overkilling units by hp - (2*nextDmgSource) <= 0 (double damage source to account for a potential miss?)
// Use Player pointer instead of BWAPI::Player pointer in UnitInfo, gives advantage of knowing upgrades/tech that are available always
// Upgrade/Research unlock list
// Add panic reactions for auto-loss builds
// - Gate FE vs 4pool
// - Nexus first vs 6rax/BBS

using namespace BWAPI;
using namespace std;
using namespace McRave;

void McRaveModule::onStart()
{
    Broodwar->enableFlag(Flag::UserInput);
    Broodwar->setCommandOptimizationLevel(0);
    Broodwar->setLatCom(true);

    if (Broodwar->getGameType() != BWAPI::GameTypes::Use_Map_Settings)
        Broodwar->setLocalSpeed(0);

    Players::onStart();
    Terrain::onStart();
    BWEB::Map::onStart();
    BWEB::Stations::findStations();

    Stations::onStart();
    Grids::onStart();
    Learning::onStart();
    BWEB::Blocks::findBlocks();
    Broodwar->sendText("glhf");
}

void McRaveModule::onEnd(bool isWinner)
{
    Learning::onEnd(isWinner);
    Broodwar->sendText("ggwp");
}

void McRaveModule::onFrame()
{
    // Update unit information and grids based on the information
    Players::onFrame();
    Units::onFrame();
    Grids::onFrame();

    // Update relevant map information and strategy    
    Terrain::onFrame();
    Resources::onFrame();
    Strategy::onFrame();
    BuildOrder::onFrame();
    Stations::onFrame();

    // Update commands
    Goals::onFrame();
    Support::onFrame();
    Combat::onFrame();
    Command::onFrame();
    Workers::onFrame();
    Scouts::onFrame();
    Transports::onFrame();
    Buildings::onFrame();
    Production::onFrame();

    // Display information from this frame
    Visuals::onFrame();
}

void McRaveModule::onSendText(string text)
{
    Visuals::onSendText(text);
}

void McRaveModule::onReceiveText(Player player, string text)
{
}

void McRaveModule::onPlayerLeft(Player player)
{
}

void McRaveModule::onNukeDetect(Position target)
{
    Command::addAction(nullptr, target, TechTypes::Nuclear_Strike, PlayerState::Neutral);
}

void McRaveModule::onUnitDiscover(Unit unit)
{
    Events::onUnitDiscover(unit);
}

void McRaveModule::onUnitEvade(Unit unit)
{
}

void McRaveModule::onUnitShow(Unit unit)
{
}

void McRaveModule::onUnitHide(Unit unit)
{
}

void McRaveModule::onUnitCreate(Unit unit)
{
    Events::onUnitCreate(unit);
}

void McRaveModule::onUnitDestroy(Unit unit)
{
    Events::onUnitDestroy(unit);
}

void McRaveModule::onUnitMorph(Unit unit)
{
    Events::onUnitMorph(unit);
}

void McRaveModule::onUnitRenegade(Unit unit)
{
    Events::onUnitRenegade(unit);
}

void McRaveModule::onSaveGame(string gameName)
{
}

void McRaveModule::onUnitComplete(Unit unit)
{
    Events::onUnitComplete(unit);
}
