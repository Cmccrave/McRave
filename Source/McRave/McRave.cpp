// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"

// --- AUTHOR NOTES ---
// TODO:
// Air unit kiting
// Scout block building spots
// Safe transfering
// Defend position adjustment for probe pulls
// Cannon rush detection
// Rework DT Expand
// Pylon too many early game
// Sim distance
// Check grids, added width to threat grids
// 4gate weird over-saturation of probes
// If expo blocked, skip Nexus and build units
// Fix BWEB doors
// Ovie scouting

void McRaveModule::onStart()
{
	Broodwar->enableFlag(Flag::UserInput);
	Broodwar->setCommandOptimizationLevel(0);
	Broodwar->setLatCom(true);
	Broodwar->setLocalSpeed(0);
	Terrain().onStart();
	Players().onStart();	
	mapBWEB.onStart();
	Grids().onStart();
	BuildOrder().onStart();		
	mapBWEB.findBlocks();
	Broodwar->sendText("glhf");
}

void McRaveModule::onEnd(bool isWinner)
{
	BuildOrder().onEnd(isWinner);
}

void McRaveModule::onFrame()
{	
	Terrain().onFrame();
	Grids().onFrame();
	Resources().onFrame();
	Strategy().onFrame();
	Workers().onFrame();
	Units().onFrame();
	Transport().onFrame();
	Commands().onFrame();
	Buildings().onFrame();
	Production().onFrame();
	BuildOrder().onFrame();
	Stations().onFrame();
	Display().onFrame();
}

void McRaveModule::onSendText(string text)
{
	Display().onSendText(text);
}

void McRaveModule::onReceiveText(Player player, string text)
{
}

void McRaveModule::onPlayerLeft(Player player)
{
	Broodwar->sendText("ggwp");
}

void McRaveModule::onNukeDetect(Position target)
{
	Commands().addCommand(target, TechTypes::Nuclear_Strike);
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
