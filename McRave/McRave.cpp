// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"

// --- AUTHOR NOTES ---
// Critical TODOS:
// Fix HT storming ally units and merging
// Secondary scout, see what sort of tech we are against

// TODOS:
// Move production buildings to the front of the base, tech to the back
// Dijkstras theory for distance grid
// Move stim research to strategy
// One time increases: supply, sizes

// TODOS to move to no latency compensation:
// Building idle status stored
// Unit idle status stored?
// Update commands to remove any latency components

// Testing:
// Spider mine removal from expansions

// Possibility:
// Take angles into account for micro? (Distance to target)
// Player class to track upgrades/race/supply/strength?
// One time supply increase instead of resetting?

void McRaveModule::onStart()
{
	// Enable the UserInput flag, which allows us to control the bot and type messages.
	Broodwar->enableFlag(Flag::UserInput);

	// Set the command optimization level so that common commands can be grouped and reduce the bot's APM (Actions Per Minute).
	Broodwar->setCommandOptimizationLevel(0);

	Broodwar->setLatCom(true);
	Broodwar->setLocalSpeed(0);

	theMap.Initialize();
	theMap.EnableAutomaticPathAnalysis();
	bool startingLocationsOK = theMap.FindBasesForStartingLocations();
	assert(startingLocationsOK);
}

void McRaveModule::onEnd(bool isWinner)
{
}

void McRaveModule::onFrame()
{
	Terrain().update();
	Grids().update();
	Resources().update();
	Strategy().update();
	Workers().update();
	Units().update();
	SpecialUnits().update();
	Transport().update();
	Commands().update();
	Buildings().update();
	Production().update();
	BuildOrder().update();
	Bases().update();
	Display().update();
}

void McRaveModule::onSendText(std::string text)
{
	// Else send the text to the game if it is not being processed
	Broodwar->sendText("%s", text.c_str());
}

void McRaveModule::onReceiveText(BWAPI::Player player, std::string text)
{
}

void McRaveModule::onPlayerLeft(BWAPI::Player player)
{
	Broodwar->sendText("GG %s!", player->getName().c_str());
}

void McRaveModule::onNukeDetect(BWAPI::Position target)
{
}

void McRaveModule::onUnitDiscover(BWAPI::Unit unit)
{
}

void McRaveModule::onUnitEvade(BWAPI::Unit unit)
{
}

void McRaveModule::onUnitShow(BWAPI::Unit unit)
{
}

void McRaveModule::onUnitHide(BWAPI::Unit unit)
{
}

void McRaveModule::onUnitCreate(BWAPI::Unit unit)
{
}

void McRaveModule::onUnitDestroy(BWAPI::Unit unit)
{
	Units().decayUnit(unit);
	Buildings().removeBuilding(unit);
	SpecialUnits().removeUnit(unit);
	Workers().removeWorker(unit);
	Resources().removeResource(unit);
	Terrain().removeTerritory(unit);
}

void McRaveModule::onUnitMorph(BWAPI::Unit unit)
{
}

void McRaveModule::onUnitRenegade(BWAPI::Unit unit)
{
}

void McRaveModule::onSaveGame(std::string gameName)
{
}

void McRaveModule::onUnitComplete(BWAPI::Unit unit)
{
}