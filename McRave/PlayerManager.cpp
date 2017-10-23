#include "McRave.h"

void PlayerTrackerClass::onStart()
{
	// Store enemy races
	for (auto &player : Broodwar->enemies())
	{
		thePlayers[player].setAlive(true);
		thePlayers[player].setRace(player->getRace());

		if (player->getRace() == Races::Zerg)
		{
			eZerg++;
		}
		else if (player->getRace() == Races::Protoss)
		{
			eProtoss++;
		}
		else if (player->getRace() == Races::Terran)
		{
			eTerran++;
		}
		else
		{
			eRandom++;
		}
	}
	return;
}

void PlayerTrackerClass::update()
{
	for (auto &player : thePlayers)
	{
		updateUpgrades(player.second);
	}
}

void PlayerTrackerClass::updateUpgrades(PlayerInfo& player)
{
	
}