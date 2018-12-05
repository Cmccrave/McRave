// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"

// *** Coding ***
// Move 4gate vs Z to 2 gate category
// Use Player pointer instead of BWAPI::Player pointer in UnitInfo, gives advantage of knowing upgrades/tech that are available always
// Floating units removing mine positions
// If an enemy floats a CC to an expansion, we don't consider it "taken"
	// - Should we check all stations for being taken? How? UsedGrid? If so, we need to verify that buildings that land have used tiles
// Pylon walls
// JPS timeout if path unreachable

// *** SSCAIT2018 Goals ***
// Unit formations
// Build detection improvements
// Observer building timings

void McRaveModule::onStart()
{
	Broodwar->enableFlag(Flag::UserInput);
	Broodwar->setCommandOptimizationLevel(0);
	Broodwar->setLatCom(true);
	Broodwar->setLocalSpeed(0);
	Terrain().onStart();
	Players().onStart();
	BWEB::Map::onStart();
	MyStations().onStart();
	Grids().onStart();
	BuildOrder().onStart();
	BWEB::Blocks::findBlocks();
	Broodwar->sendText("glhf");
}

void McRaveModule::onEnd(bool isWinner)
{
	BuildOrder().onEnd(isWinner);
}

void McRaveModule::onFrame()
{
	// Update relevant map information and strategy
	Terrain().onFrame();
	Resources().onFrame();
	Strategy().onFrame();
	BuildOrder().onFrame();
	MyStations().onFrame();

	// Update unit information and grids based on the information
	Units().onFrame();
	Grids().onFrame();

	// Update commands
	Goals().onFrame();
	Support::onFrame();
	Commands().onFrame();
	Workers().onFrame();
	Scouts().onFrame();
	Transport().onFrame();
	Buildings().onFrame();
	Production().onFrame();	

	// Display information from this frame
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
	Commands().addCommand(nullptr, target, TechTypes::Nuclear_Strike);
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
