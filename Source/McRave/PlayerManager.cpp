#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Players
{
    namespace {
        map <Player, PlayerInfo> thePlayers;
        map <Race, int> raceCount;
        map <PlayerInfo, TotalStrength> playerStrengths;

        void update(PlayerInfo& player)
        {
            // Add up the number of each race - HACK: Don't add self for now
            player.update();
            if (!player.isSelf())
                raceCount[player.getCurrentRace()]++;

            // Add up the total strength for this player
            auto &strengths = playerStrengths[player];
            for (auto &[_, unit] : player.getUnits()) {
                unit.setTarget(nullptr); // HACK: Just in case our target was killed and we don't get an update in UnitManager
                if (unit.getType().isWorker() && unit.getRole() != Role::Combat)
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

            if (player.player() == Broodwar->self()) {
                Broodwar->drawTextScreen(0, 100, "%2f", strengths.groundToGround);
                Broodwar->drawTextScreen(0, 116, "%2f", strengths.groundToAir);
                Broodwar->drawTextScreen(0, 132, "%2f", strengths.groundDefense);
            }
        }
    }

    void onStart()
    {
        // Store all players
        for (auto player : Broodwar->getPlayers()) {
            PlayerInfo &p = thePlayers[player];
            p.setPlayer(player);
            p.setAlive(true);
            p.setStartRace(player->getRace());
            p.setCurrentRace(player->getRace());
            raceCount[p.getCurrentRace()]++;
        }
    }

    void onFrame()
    {
        // Clear race count and recount
        raceCount.clear();
        playerStrengths.clear();

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
        return TotalStrength();
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