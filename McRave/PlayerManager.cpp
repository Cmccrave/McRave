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