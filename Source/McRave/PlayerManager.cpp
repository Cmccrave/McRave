#include "McRave.h"

namespace McRave
{
	void PlayerManager::onStart()
	{
		// Store all enemy players
		for (auto &player : Broodwar->enemies()) {
			PlayerInfo &p = thePlayers[player];

			p.setAlive(true);
			p.setRace(player->getRace());
			p.setPlayer(player);

			if (player->getRace() == Races::Zerg)
				eZerg++;			
			else if (player->getRace() == Races::Protoss)
				eProtoss++;			
			else if (player->getRace() == Races::Terran)
				eTerran++;			
			else
				eRandom++;
		
		}
	}

	void PlayerManager::onFrame()
	{
		for (auto &player : thePlayers)
			update(player.second);		
	}

	void PlayerManager::update(PlayerInfo& player)
	{
		// Store any upgrades this player has
		for (auto &upgrade : UpgradeTypes::allUpgradeTypes()) {
			if (player.player()->getUpgradeLevel(upgrade) > 0)
				player.storeUpgrade(upgrade);
		}

		// Store any tech this player has
		for (auto &tech : TechTypes::allTechTypes()) {
			if (player.player()->hasResearched(tech))
				player.storeTech(tech);
		}
	}
}