#include "McRave.h"

namespace McRave::Players
{
    namespace {
        map <Player, PlayerInfo> thePlayers;
        map <Race, int> raceCount;

        void update(PlayerInfo& player)
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

    void onStart()
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

    void onFrame()
    {
        for (auto &player : thePlayers)
            update(player.second);
    }

    map <Player, PlayerInfo>& getPlayers() { return thePlayers; }
    int getNumberZerg() { return raceCount[Races::Zerg]; }
    int getNumberProtoss() { return raceCount[Races::Protoss]; }
    int getNumberTerran() { return raceCount[Races::Terran]; }
    int getNumberRandom() { return raceCount[Races::Unknown]; }
    bool vP() { return (thePlayers.size() == 1 && raceCount[Races::Protoss] > 0); }
    bool vT() { return (thePlayers.size() == 1 && raceCount[Races::Terran] > 0); }
    bool vZ() { return (thePlayers.size() == 1 && raceCount[Races::Zerg] > 0); }
}