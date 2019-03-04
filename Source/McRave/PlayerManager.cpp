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
            for (auto &u : player.getUnits()) {
                auto &unit = *u;
                if (unit.getType().isWorker() && unit.getRole() != Role::Combat)
                    continue;
                addStrength(unit);
                unit.getTargetedBy().clear();
                unit.setTarget(nullptr);
            }
        }
    }

    void onStart()
    {
        // Store all players
        for (auto player : Broodwar->getPlayers()) {
            auto race = player->isNeutral() ? Races::None : player->getRace(); // BWAPI returns Zerg for neutral race

            PlayerInfo &p = thePlayers[player];
            p.setPlayer(player);
            p.setStartRace(race);
            p.update();
           
            if (!p.isSelf())
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

    void addStrength(UnitInfo& unit)
    {
        // TODO: When UnitInfo stores PlayerInfo, fix this
        auto &pInfo = thePlayers[unit.getPlayer()];
        auto &strengths = playerStrengths[pInfo];
        for (auto &[p, player] : thePlayers) {
            if (p == unit.getPlayer()) {
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

    map <Player, PlayerInfo>& getPlayers() { return thePlayers; }
    int getNumberZerg() { return raceCount[Races::Zerg]; }
    int getNumberProtoss() { return raceCount[Races::Protoss]; }
    int getNumberTerran() { return raceCount[Races::Terran]; }
    int getNumberRandom() { return raceCount[Races::Unknown]; }
    bool vP() { return (thePlayers.size() == 3 && raceCount[Races::Protoss] > 0); }
    bool vT() { return (thePlayers.size() == 3 && raceCount[Races::Terran] > 0); }
    bool vZ() { return (thePlayers.size() == 3 && raceCount[Races::Zerg] > 0); }
}