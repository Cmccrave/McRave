#pragma once
#include <BWAPI.h>
#include <set>

using namespace BWAPI;
using namespace std;

namespace McRave
{
    class PlayerInfo
    {
        Player thisPlayer;
        Race startRace, currentRace;
        bool alive;
        TilePosition startLocation;
        set <UpgradeType> playerUpgrades;
        set <TechType> playerTechs;
    public:
        PlayerInfo();
        Race getCurrentRace() { return currentRace; }
        Race getStartRace() { return startRace; }
        bool isAlive() { return alive; }
        bool isEnemy() { return thisPlayer->isEnemy(Broodwar->self()); }
        bool isAlly() { return thisPlayer->isAlly(Broodwar->self()); }
        bool isSelf() { return thisPlayer == Broodwar->self(); }
        TilePosition getStartingLocation() { return startLocation; }

        Player player() { return thisPlayer; }

        void storeUpgrade(UpgradeType upgrade) { playerUpgrades.insert(upgrade); }
        void storeTech(TechType tech) { playerTechs.insert(tech); }

        void setCurrentRace(Race newRace) { currentRace = newRace; }
        void setStartRace(Race newRace) { startRace = newRace; }
        void setAlive(bool newState) { alive = newState; }
        void setPlayer(Player newPlayer) { thisPlayer = newPlayer; }

        bool hasUpgrade(UpgradeType);
        bool hasTech(TechType);
    };

    PlayerInfo::PlayerInfo()
    {
        thisPlayer = nullptr;
        currentRace = Races::None;
        startRace = Races::None;
        alive = true;
    }

    bool PlayerInfo::hasTech(TechType tech)
    {
        if (playerTechs.find(tech) != playerTechs.end())
            return true;
        return false;
    }

    bool PlayerInfo::hasUpgrade(UpgradeType upgrade)
    {
        if (playerUpgrades.find(upgrade) != playerUpgrades.end())
            return true;
        return false;
    }
}