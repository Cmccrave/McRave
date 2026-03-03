// McRave is made by Christian McCrave
// Twitch nicknamed it McRave \o/
// For any questions, email christianmccrave@gmail.com
// Bot started 01/03/2017

#include "BaseFinder/BaseFinder.h"
#include "Builds/All/BuildOrder.h"
#include "Builds/All/Learning.h"
#include "Events.h"
#include "Header.h"
#include "Info/Building/Buildings.h"
#include "Info/Resource/Resources.h"
#include "Info/Roles.h"
#include "Info/Unit/Pathing.h"
#include "Info/Unit/Targeting.h"
#include "Info/Unit/Units.h"
#include "Macro/Expanding/Expanding.h"
#include "Macro/Planning/Pylons.h"
#include "Macro/Producing/Producing.h"
#include "Macro/Researching/Researching.h"
#include "Macro/Upgrading/Upgrading.h"
#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Map/Walls/Walls.h"
#include "Micro/Combat/Combat.h"
#include "Micro/Defender/Defender.h"
#include "Micro/Scout/Scouts.h"
#include "Micro/Support/Support.h"
#include "Micro/Transport/Transports.h"
#include "Micro/Worker/Workers.h"
#include "Strategy/Actions/Actions.h"
#include "Strategy/Goals/Goals.h"
#include "Strategy/Spy/Spy.h"
#include "Strategy/Zones/Zones.h"
#include "Util.h"

using namespace BWAPI;
using namespace std;
using namespace McRave;

void McRaveModule::onStart()
{
    Util::onStart();
    Players::onStart();
    Terrain::onStart();
    Walls::onStart();
    Planning::onStart();
    Stations::onStart();
    Expansion::onStart();
    Grids::onStart();
    Learning::onStart();
    Resources::onStart();
    Scouts::onStart();
    Combat::onStart();
    Goals::onStart();

    Broodwar->enableFlag(Flag::UserInput);
    Broodwar->setCommandOptimizationLevel(0);
    Broodwar->setLatCom(true);
    Broodwar->sendText("glhf");
    Broodwar->setLocalSpeed(Broodwar->getGameType() != BWAPI::GameTypes::Use_Map_Settings ? 0 : 42);

    BWEB::Pathfinding::clearCacheFully();
}

void McRaveModule::onEnd(bool isWinner)
{
    // @McRave ☑ has taken "drop all your weapons at the end of a csgo match" to the ai scene
    for (auto &unit : Units::getUnits(PlayerState::Self))
        unit->setMarkForDeath(true);

    Learning::onEnd(isWinner);
    Resources::onEnd();
    Broodwar->sendText("ggwp");
    BWEB::Pathfinding::clearCacheFully();
}

void McRaveModule::onFrame()
{
    if (Broodwar->getGameType() != GameTypes::Use_Map_Settings && Broodwar->isPaused())
        return;

    // if (Util::getTime() > Time(29, 59))
    //    Broodwar->leaveGame();

    // auto mousePos = WalkPosition(Broodwar->getScreenPosition() + Broodwar->getMousePosition());
    // auto grid = Grids::getAirThreat(mousePos, PlayerState::Enemy);
    // Broodwar << grid << endl;

    // Update game state
    Util::onFrame();

    // Update ingame information
    Players::onFrame();
    Units::onFrame();
    Roles::onFrame();
    Targets::onFrame();
    Pathing::onFrame();
    Grids::onFrame();
    Pylons::onFrame();

    // Update relevant map information and strategy
    Terrain::onFrame();
    Walls::onFrame();
    Resources::onFrame();
    Spy::onFrame();
    BuildOrder::onFrame();
    Stations::onFrame();

    // Update gameplay of the bot
    Actions::onFrame();
    Goals::onFrame();
    Support::onFrame();
    Scouts::onFrame();
    Defender::onFrame();
    Combat::onFrame();
    Workers::onFrame();
    Transports::onFrame();
    Expansion::onFrame();
    Planning::onFrame();
    Buildings::onFrame();
    Upgrading::onFrame();
    Researching::onFrame();
    Producing::onFrame();
    Zones::onFrame();

    // Display information from this frame
    Visuals::onFrame();

    BWEB::Pathfinding::testCache();
}

void McRaveModule::onSendText(string text) { Visuals::onSendText(text); }

void McRaveModule::onReceiveText(Player player, string text) {}

void McRaveModule::onPlayerLeft(Player player) {}

void McRaveModule::onNukeDetect(Position target) { Actions::addAction(nullptr, target, TechTypes::Nuclear_Strike, PlayerState::Neutral, Util::getCastRadius(TechTypes::Nuclear_Strike)); }

void McRaveModule::onUnitDiscover(Unit unit) { Events::onUnitDiscover(unit); }

void McRaveModule::onUnitEvade(Unit unit) {}

void McRaveModule::onUnitShow(Unit unit) {}

void McRaveModule::onUnitHide(Unit unit) {}

void McRaveModule::onUnitCreate(Unit unit) { Events::onUnitCreate(unit); }

void McRaveModule::onUnitDestroy(Unit unit) { Events::onUnitDestroy(unit); }

void McRaveModule::onUnitMorph(Unit unit) { Events::onUnitMorph(unit); }

void McRaveModule::onUnitRenegade(Unit unit) { Events::onUnitRenegade(unit); }

void McRaveModule::onSaveGame(string gameName) {}

void McRaveModule::onUnitComplete(Unit unit) { Events::onUnitComplete(unit); }
