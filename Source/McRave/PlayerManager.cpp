#include "McRave.h"

namespace McRave
{
    void PlayerManager::onStart()
    {
        // Store all enemy players
        for (auto &player : Broodwar->enemies()) {
            PlayerInfo &p = thePlayers[player];

            p.setAlive(true);
            p.setStartRace(player->getRace());
            p.setCurrentRace(player->getRace());
            p.setPlayer(player);
            raceCount[p.getCurrentRace()]++;
        }
    }

    void PlayerManager::onFrame()
    {
        for (auto &player : thePlayers)
            update(player.second);
    }

    void PlayerManager::update(PlayerInfo& player)
    {
        // Clear race count and recount
        raceCount.clear();

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

        // Add up the number of each race
        if (player.isAlive()) {
            player.setCurrentRace(player.player()->getRace());
            raceCount[player.getCurrentRace()]++;
        }
    }
}