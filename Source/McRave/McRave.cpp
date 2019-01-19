// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"
#include "EventManager.h"

// *** TODO ***
// Scout spam sacrifice is a problem
// Floating units removing mine positions
// Need to fix how the directional check of viable position iterating works
// BWEB Destination walls not working
// Proxy builds? BWEB proxy block?
// Proxy 2 gate reaction 
// Drone scoring based on my strength + defense vs enemy strength - defense
// Zerg macro hatchery fixes
// Check Muta micro to ensure we are overshooting movement to not decel near targets
// Unit Interface get distance is edge to point, lots of mistakes!

// *** TOTEST ***
// All units are stored in PlayerInfo objects
// If an enemy floats a CC to an expansion, we don't consider it "taken"
    /// Added a customOnUnitLand in EventManager.h which should take care of this
    // Test lifting buildings
    // Lift in fog / visible
    // Land in fog / visible
// Each PlayerInfo has a TotalStrength, test all values

// *** Parallel Lines ***
// Destination 12 o clock spawn issues making parallel lines (offset by +y about 64 pixels?)
// Neo Moon 5 o clock not making parallel lines at all (no debug drawings either)
// La Mancha top left

// *** Ideas ***
// Monitor for overkilling units by hp - (2*nextDmgSource) <= 0 (double damage source to account for a potential miss?)
// Walkable grid cached, only check collision at corners + center when looking for walkable positions for a unit
// Use Player pointer instead of BWAPI::Player pointer in UnitInfo, gives advantage of knowing upgrades/tech that are available always
// Use player filters to grab unit set in getClosestUnit template
// PlayerInfo stores all units for each player - big restructure

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
    // Update relevant map information and strategy
    Players::onFrame();
    Terrain::onFrame();
    Resources::onFrame();
    Strategy::onFrame();
    BuildOrder::onFrame();
    Stations::onFrame();

    // Update unit information and grids based on the information
    Units::onFrame();
    Combat::onFrame();
    Grids::onFrame();

    // Update commands
    Goals::onFrame();
    Support::onFrame();
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
    Command::addCommand(nullptr, target, TechTypes::Nuclear_Strike);
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
