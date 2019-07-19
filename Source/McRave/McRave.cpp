// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"
#include "EventManager.h"

// *** TODO ***
// Re-power buildings
// Too many pylons on Plasma only?
// Plasma? - bad targeting, remove units from shuttles in range, too many shuttles
// Attacking proxies with scouts
// Add frame timeouts for allowable enemy build detection
// Targeting neutrals isn't necessarily best option

// Critical:
// HT blocking buildings
// Closest cargo to transport

using namespace BWAPI;
using namespace std;
using namespace McRave;

void McRaveModule::onStart()
{
    Players::onStart();
    Terrain::onStart();
    Stations::onStart();
    Grids::onStart();
    Learning::onStart();
    Buildings::onStart();

    Broodwar->enableFlag(Flag::UserInput);
    Broodwar->setCommandOptimizationLevel(0);
    Broodwar->setLatCom(true);
    Broodwar->sendText("glhf");
    Broodwar->setLocalSpeed(Broodwar->getGameType() != BWAPI::GameTypes::Use_Map_Settings ? 0 : 42);
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
