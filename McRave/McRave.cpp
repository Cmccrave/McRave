// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"

// --- AUTHOR NOTES ---
// TODO in testing before AIIDE 2017:
// Check threat grids
// Better engaging
// Scout improvements
// Island check for DistanceGridHome
// Test Recall!
// Test size/damage type
// Anti-stone check
// Cannons / Worker pull
// Test psi storm grid and EMP grid

// One shot stuff:
// Store resources when Nexus created - remove when destroyed
// onUnitMorph - Archons, Eggs, Refineries
// One shot composition storing - requires onMorph usage
// One shot unit scoring? maybe not

// TODO:
// Only remove boulders close to me
// Invis grid for observers to detect stuff
// Archon merging when low energy or fleeing
// Spider mine removal from expansions
// Improve shuttles
// Remove exists check from target aquisition?
// IsSelected to display information
// Move special units into their own area, separate command manager?
// Make target position stuff based on units current command target position?
// Move production buildings to the front of the base, tech to the back
// Dijkstras theory for distance grid
// Move stim research to strategy

// TODO to move to no latency compensation:
// Building idle status stored
// Unit idle status stored?
// Update commands to remove any latency components

void McRaveModule::onStart()
{
	Broodwar->enableFlag(Flag::UserInput);	
	Broodwar->setCommandOptimizationLevel(0);
	Broodwar->setLatCom(true);
	Broodwar->setLocalSpeed(0);
	theMap.Initialize();
	theMap.EnableAutomaticPathAnalysis();
	bool startingLocationsOK = theMap.FindBasesForStartingLocations();
	assert(startingLocationsOK);
	Terrain().onStart();
}

void McRaveModule::onEnd(bool isWinner)
{
}

void McRaveModule::onFrame()
{
	Players().update();
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

void McRaveModule::onSendText(string text)
{
	Display().sendText(text);
}

void McRaveModule::onReceiveText(Player player, string text)
{
}

void McRaveModule::onPlayerLeft(Player player)
{
	Broodwar->sendText("GG %s!", player->getName().c_str());
}

void McRaveModule::onNukeDetect(Position target)
{
}

void McRaveModule::onUnitDiscover(Unit unit)
{
	if (unit->getPlayer()->isEnemy(Broodwar->self()))
	{
		Units().storeEnemy(unit);
	}
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
	Units().onUnitCreate(unit);
}

void McRaveModule::onUnitDestroy(Unit unit)
{
	Units().removeUnit(unit);
}

void McRaveModule::onUnitMorph(Unit unit)
{
	Units().onUnitMorph(unit);
}

void McRaveModule::onUnitRenegade(Unit unit)
{
}

void McRaveModule::onSaveGame(string gameName)
{
}

void McRaveModule::onUnitComplete(Unit unit)
{
	Units().onUnitComplete(unit);
}