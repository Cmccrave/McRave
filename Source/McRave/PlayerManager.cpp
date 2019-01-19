#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Players
{
    namespace {
        map <Player, PlayerInfo> thePlayers;
        map <Race, int> raceCount;
        map <PlayerInfo&, TotalStrength> playerStrengths;

        void update(PlayerInfo& player)
        {
            // Clear race count and recount
            raceCount.clear();
            player.update();
            playerStrengths.clear();

            // Add up the number of each race - HACK: Don't add self for now
            if (!player.isSelf())
                raceCount[player.getCurrentRace()]++;

            // Add up the total strength for this player
            auto &strengths = playerStrengths[player];
            for (auto &[_, unit] : player.getUnits()) {
                if (unit.getType().isWorker())
                    continue;

                if (unit.getType().isBuilding()) {
                    strengths.airDefense += unit.getVisibleAirStrength();
                    strengths.groundDefense += unit.getVisibleGroundStrength();
                }
                else if (unit.getType().isFlyer()) {
                    strengths.airToAir += unit.getVisibleAirStrength();
                    strengths.airToGround += unit.getVisibleGroundStrength();
                }
                else {
                    strengths.groundToAir += unit.getVisibleAirStrength();
                    strengths.groundToGround += unit.getVisibleGroundStrength();
                }
            }
        }
    }

    void onStart()
    {
        // Store all enemy players
        for (auto &player : Broodwar->getPlayers()) {
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
        for (auto &[_, player] : thePlayers)
            update(player);
    }

    TotalStrength getStrength(PlayerState state)
    {
        TotalStrength combined;
        for (auto &[_, player] : thePlayers) {
            if (player.getPlayerState() == state)
                combined += playerStrengths[player];
        }
        return combined;
    }

    TotalStrength getStrength(PlayerInfo& comparePlayer)
    {
        for (auto &[_, player] : thePlayers) {
            if (player == comparePlayer)
                return playerStrengths[player];
        }
    }

    map <Player, PlayerInfo>& getPlayers() { return thePlayers; }
    int getNumberZerg() { return raceCount[Races::Zerg]; }
    int getNumberProtoss() { return raceCount[Races::Protoss]; }
    int getNumberTerran() { return raceCount[Races::Terran]; }
    int getNumberRandom() { return raceCount[Races::Unknown]; }
    bool vP() { return (thePlayers.size() == 2 && raceCount[Races::Protoss] > 0); }
    bool vT() { return (thePlayers.size() == 2 && raceCount[Races::Terran] > 0); }
    bool vZ() { return (thePlayers.size() == 2 && raceCount[Races::Zerg] > 0); }
}