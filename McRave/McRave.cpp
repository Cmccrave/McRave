// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"

// --- AUTHOR NOTES ---
// TODO:
// Prevent overlapping buildings
// Mine removal from expansions - important for Ice/Iron

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
	BuildOrder().loadConfig();
}

void McRaveModule::onEnd(bool isWinner)
{
	if (isWinner)
	{
		BuildOrder().recordWinningBuild(isWinner);
	}
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
	Units().onUnitDiscover(unit);
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
	Units().onUnitDestroy(unit);
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