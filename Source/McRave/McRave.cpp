// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"
#include "EventManager.h"

// *** TODO ***
// Floating units removing mine positions
// BWEB Destination walls not working - only north one
// BWAPI::Unit getDistance is edge to point
// Corsairs shouldn't engage inside ally territory
// Test vs BBS
// Reavers aren't picked up if we cant afford scarabs
// Check storming burrowed lurkers
// Prevent HT walking into enemies when storm isn't suitable
// Add pathing to attack enemy buildings so our sim actually works
// Re-power buildings
// Burrowed units that aren't threatening cause defending units to eat free hits because they're targeting it
// Add playerstate to actions
// Add 3rd stargate on 3 base with carriers
// Seeing an extractor trick causes a crash

// Need a target based isThreatening
// Examples:
// - Melee shouldn't engage ranged on ramps, back up instead
// - Engage when in range of defenses for FFE

// Current BASIL Losses:
// Add logic to detect a large worker pull early on - Stone/PurpleSpirit
// Better kiting near edge of maps - Andrey/Velicro/Wuli etc
// Better scouting of never visited tiles to check for builds - Tyr DT rush
// Worker drilling - 4pools
// Gas steal transition - Zur/Locutus

// Scout targets
// - Check for Nexus when we see no gateways in PvP (find timing for this and check after this time based on last visible frame on our grid)
// - Check for 3rd against Z when we see 2 hatchery and no gas

// *** TOTEST ***
// If an enemy floats a CC to an expansion, we don't consider it "taken"
    // Added a customOnUnitLand in EventManager.h which should take care of this
    // Test lifting buildings
    // Lift in fog / visible
    // Land in fog / visible

// *** Ideas ***
// Monitor for overkilling units by hp - (2*nextDmgSource) <= 0 (double damage source to account for a potential miss?)
// Use Player pointer instead of BWAPI::Player pointer in UnitInfo, gives advantage of knowing upgrades/tech that are available always
// Use player filters to grab unit set in getClosestUnit template
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
    Players::onStart();
    Terrain::onStart();
    BWEB::Map::onStart();
    BWEB::Stations::findStations();

    if (Broodwar->getGameType() != BWAPI::GameTypes::Use_Map_Settings)
        Broodwar->setLocalSpeed(0);

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
