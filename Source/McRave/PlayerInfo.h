#pragma once
#include <BWAPI.h>
#include <set>

namespace McRave
{
    class PlayerInfo
    {
        BWAPI::Player thisPlayer;
        BWAPI::Race startRace, currentRace;
        bool alive;
        BWAPI::TilePosition startLocation;
        std::set <BWAPI::UpgradeType> playerUpgrades;
        std::set <BWAPI::TechType> playerTechs;
    public:
        PlayerInfo() {
            thisPlayer = nullptr;
            currentRace = BWAPI::Races::None;
            startRace = BWAPI::Races::None;
            alive = true;
        }

        BWAPI::Race getCurrentRace() { return currentRace; }
        BWAPI::Race getStartRace() { return startRace; }
        bool isAlive() { return alive; }
        bool isEnemy() { return thisPlayer->isEnemy(BWAPI::Broodwar->self()); }
        bool isAlly() { return thisPlayer->isAlly(BWAPI::Broodwar->self()); }
        bool isSelf() { return thisPlayer == BWAPI::Broodwar->self(); }
        BWAPI::TilePosition getStartingLocation() { return startLocation; }

        BWAPI::Player player() { return thisPlayer; }

        void storeUpgrade(BWAPI::UpgradeType upgrade) { playerUpgrades.insert(upgrade); }
        void storeTech(BWAPI::TechType tech) { playerTechs.insert(tech); }

        void setCurrentRace(BWAPI::Race newRace) { currentRace = newRace; }
        void setStartRace(BWAPI::Race newRace) { startRace = newRace; }
        void setAlive(bool newState) { alive = newState; }
        void setPlayer(BWAPI::Player newPlayer) { thisPlayer = newPlayer; }

        bool hasUpgrade(BWAPI::UpgradeType upgrade) {
            if (playerUpgrades.find(upgrade) != playerUpgrades.end())
                return true;
            return false;
        }
        bool hasTech(BWAPI::TechType tech) {
            if (playerTechs.find(tech) != playerTechs.end())
                return true;
            return false;
        }
    };
}