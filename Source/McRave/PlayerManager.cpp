#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Players
{
    namespace {
        map <Player, PlayerInfo> thePlayers;
        map <Race, int> raceCount;

        void update(PlayerInfo& player)
        {
            // Add up the number of each race - HACK: Don't add self for now
            player.update();
            if (!player.isSelf())
                raceCount[player.getCurrentRace()]++;
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
        for (auto &[_, player] : thePlayers)
            update(player);
    }

    int getSupply(PlayerState state)
    {
        auto combined = 0;
        for (auto &[_, player] : thePlayers) {
            if (player.getPlayerState() == state)
                combined += player.getSupply();
        }
        return combined;
    }

    int getRaceCount(Race race, PlayerState state)
    {
        auto combined = 0;
        for (auto &[_, player] : thePlayers) {
            if (player.getCurrentRace() == race && player.getPlayerState() == state)
                combined += 1;
        }
        return combined;
    }

    Strength getStrength(PlayerState state)
    {
        Strength combined;
        for (auto &[_, player] : thePlayers) {
            if (player.getPlayerState() == state)
                combined += player.getStrength();
        }
        return combined;
    }

    void addStrength(UnitInfo& unit)
    {
        // TODO: When UnitInfo stores PlayerInfo, fix this
        auto &pInfo = thePlayers[unit.getPlayer()];
        auto &strengths = pInfo.getStrength();
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

    bool vP() { return (thePlayers.size() == 3 && raceCount[Races::Protoss] > 0); }
    bool vT() { return (thePlayers.size() == 3 && raceCount[Races::Terran] > 0); }
    bool vZ() { return (thePlayers.size() == 3 && raceCount[Races::Zerg] > 0); }

    bool PvP() {
        return vP() && Broodwar->self()->getRace() == Races::Protoss;
    }
    bool PvT() {
        return vT() && Broodwar->self()->getRace() == Races::Protoss;
    }
    bool PvZ() {
        return vZ() && Broodwar->self()->getRace() == Races::Protoss;
    }

    bool TvP() {
        return vP() && Broodwar->self()->getRace() == Races::Terran;
    }
    bool TvT() {
        return vT() && Broodwar->self()->getRace() == Races::Terran;
    }
    bool TvZ() {
        return vZ() && Broodwar->self()->getRace() == Races::Terran;
    }

    bool ZvP() {
        return vP() && Broodwar->self()->getRace() == Races::Zerg;
    }
    bool ZvT() {
        return vT() && Broodwar->self()->getRace() == Races::Zerg;
    }
    bool ZvZ() {
        return vZ() && Broodwar->self()->getRace() == Races::Zerg;
    }
}