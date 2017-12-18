// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"

// --- AUTHOR NOTES ---
// TODO before SSCAIT:
// Disable DT Expand
// Fix chasing scouts
// Deny expansions
// Add repairing scvs to sim

// TODO after:
// Production sat based on active mining bases instead
// Create building clustering/simcity
// Rework reserved/queued resources
// Rework pylon building timings (note: overbuilding when not producing units fast enough to keep up)

void McRaveModule::onStart()
{
	Broodwar->enableFlag(Flag::UserInput);
	Broodwar->setCommandOptimizationLevel(0);
	Broodwar->setLatCom(true);
	Broodwar->setLocalSpeed(0);
	Terrain().onStart();
	Players().onStart();
	BuildOrder().onStart();
}

void McRaveModule::onEnd(bool isWinner)
{
	BuildOrder().onEnd(isWinner);
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
	Units().onUnitRenegade(unit);
}

void McRaveModule::onSaveGame(string gameName)
{
}

void McRaveModule::onUnitComplete(Unit unit)
{
	Units().onUnitComplete(unit);
}