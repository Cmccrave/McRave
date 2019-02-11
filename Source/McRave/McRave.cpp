// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"
#include "EventManager.h"

// *** TODO ***
// Floating units removing mine positions
// BWEB Destination walls not working
// Proxy builds? BWEB proxy block?
// Zerg macro hatchery fixes
// Unit Interface get distance is edge to point, lots of mistakes!
// Corsairs shouldn't engage inside ally territory
// Cannons in main/nat mineral line vs mutas
// Tests busts vs FFE
// Test vs BBS
// 2Gate - opener for GateNexus gets incremented/decremented by 2Gate build and vice versa
// 2Gate at natural shouldn't transition into defensive version, need a different branch
// Cannon rush issues
// Possible to get units stuck between tech buildings
// Checking if a gateway is queuable/buildable causes it to be unassigned?
// Andromeda concaves area dont match, so line up weird
// Units stop doing anything sometimes
// Circuit Breaker top left, no FFE wall?

// *** TOTEST ***
// If an enemy floats a CC to an expansion, we don't consider it "taken"
    // Added a customOnUnitLand in EventManager.h which should take care of this
    // Test lifting buildings
    // Lift in fog / visible
    // Land in fog / visible

// *** Parallel Lines ***
// Destination 12 o clock spawn issues making parallel lines (offset by +y about 64 pixels?)
// Neo Moon 5 o clock not making parallel lines at all (no debug drawings either)
// La Mancha top left

// *** Ideas ***
// Monitor for overkilling units by hp - (2*nextDmgSource) <= 0 (double damage source to account for a potential miss?)
// Walkable grid cached, only check collision at corners + center when looking for walkable positions for a unit
// Use Player pointer instead of BWAPI::Player pointer in UnitInfo, gives advantage of knowing upgrades/tech that are available always
// Use player filters to grab unit set in getClosestUnit template
// Add panic reactions for auto-loss builds (gate first at natural vs 4pool for example)

using namespace BWAPI;
using namespace std;
using namespace McRave;

void McRaveModule::onStart()
{
    Broodwar->enableFlag(Flag::UserInput);
    Broodwar->setCommandOptimizationLevel(0);
    Broodwar->setLatCom(true);
    Broodwar->setLocalSpeed(0);
    Players::onStart();
    Terrain::onStart();
    BWEB::Map::onStart();
    BWEB::Stations::findStations();

    Stations::onStart();
    Grids::onStart();
    BuildOrder::onStart();
    BWEB::Blocks::findBlocks();
    Broodwar->sendText("glhf");
}

void McRaveModule::onEnd(bool isWinner)
{
    BuildOrder::onEnd(isWinner);
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
    Command::addAction(nullptr, target, TechTypes::Nuclear_Strike);
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
