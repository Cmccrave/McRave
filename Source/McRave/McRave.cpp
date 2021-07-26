// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "Header.h"
#include "McRave.h"
#include "EventManager.h"
#include <chrono>
#include "../BWEM/BaseFinder/BaseFinder.h"

// TODO:
// -- Protoss --
// Add pylons to nexuses when needed (monitor for pylon spam though)
// Manner pylon self (neo moon 12)
// FFE vs 4pool, probes defend minerals
// Prefer pylons that power locations we need

// Not needed for CoG:
// Defilers get stuck on blue storm - dont even use them anyways, so it's fine
// Look at gas income for knowing how much larva we can save, spend remaining
// Add drilling (move to furthest mineral, attack only when in range and clustered)
// Defilers in ZvP
// Need better swarm placement on own hydras
// Sunks vs 5 fact gol moveout, plenty of minerals still
// Add 4pool in
// Eggs on pathfinder marked as blockers

using namespace BWAPI;
using namespace std;
using namespace McRave;

static chrono::time_point<chrono::system_clock> start;

void McRaveModule::onStart()
{
    Players::onStart();
    Terrain::onStart();
    Walls::onStart();
    Buildings::onStart();
    Stations::onStart();
    Grids::onStart();
    Learning::onStart();
    Util::onStart();
    Combat::onStart();
    Resources::onStart();
    Scouts::onStart();

    start = std::chrono::system_clock::now();

    Broodwar->enableFlag(Flag::UserInput);
    Broodwar->setCommandOptimizationLevel(0);
    Broodwar->setLatCom(true);
    Broodwar->sendText("glhf");
    Broodwar->setLocalSpeed(Broodwar->getGameType() != BWAPI::GameTypes::Use_Map_Settings ? 0 : 42);

    McRave::easyWrite("New game on " + Broodwar->mapName());
    BWEB::Pathfinding::clearCacheFully();
}

void McRaveModule::onEnd(bool isWinner)
{
    Learning::onEnd(isWinner);
    Resources::onEnd();
    Broodwar->sendText("ggwp");
    BWEB::Pathfinding::clearCacheFully();
}

void McRaveModule::onFrame()
{
    if (Broodwar->getGameType() != GameTypes::Use_Map_Settings && Broodwar->isPaused())
        return;

    // Update game state
    Util::onFrame();

    // Update ingame information
    Players::onFrame();
    Units::onFrame();
    Grids::onFrame();
    Pylons::onFrame();

    // Update relevant map information and strategy
    Terrain::onFrame();
    Walls::onFrame();
    Resources::onFrame();
    Strategy::onFrame();
    BuildOrder::onFrame();
    Stations::onFrame();

    // Update gameplay of the bot
    Actions::onFrame();
    Goals::onFrame();
    Support::onFrame();
    Scouts::onFrame();
    Combat::onFrame();
    Workers::onFrame();
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
    Actions::addAction(nullptr, target, TechTypes::Nuclear_Strike, PlayerState::Neutral);
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
