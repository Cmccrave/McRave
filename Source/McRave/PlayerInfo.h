#pragma once
#include <BWAPI.h>
#include <set>

namespace McRave
{
    class PlayerInfo
    {
        std::set <BWAPI::UpgradeType> playerUpgrades;
        std::set <BWAPI::TechType> playerTechs;
        std::set <std::shared_ptr<UnitInfo>> units;

        BWAPI::Player thisPlayer;
        BWAPI::Race startRace, currentRace;
        BWAPI::TilePosition startLocation;
        std::string build;

        Strength pStrength;
        PlayerState pState;

        bool alive;
        int supply;
    public:
        PlayerInfo() {
            thisPlayer = nullptr;
            currentRace = BWAPI::Races::None;
            startRace = BWAPI::Races::None;
            alive = true;
            pState = PlayerState::None;
            build = "Unknown";

            supply = 0;
        }

        bool operator== (PlayerInfo const& p) const {
            return thisPlayer == p.player();
        }

        bool operator< (PlayerInfo const& p) const {
            return thisPlayer < p.player();
        }

        void update();

        BWAPI::TilePosition getStartingLocation() { return startLocation; }
        BWAPI::Race getCurrentRace() { return currentRace; }
        BWAPI::Race getStartRace() { return startRace; }
        BWAPI::Player player() const { return thisPlayer; }
        PlayerState getPlayerState() { return pState; }
        Strength& getStrength() { return pStrength; }
        std::set <std::shared_ptr<UnitInfo>>& getUnits() { return units; }
        std::string getBuild() { return build; }

        bool isAlive() { return alive; }
        bool isEnemy() { return pState == PlayerState::Enemy; }
        bool isAlly() { return pState == PlayerState::Ally; }
        bool isSelf() { return pState == PlayerState::Self; }
        bool isNeutral() { return pState == PlayerState::Neutral; }
        int getSupply() { return supply; }

        void setCurrentRace(BWAPI::Race newRace) { currentRace = newRace; }
        void setStartRace(BWAPI::Race newRace) { startRace = newRace; }
        void setPlayer(BWAPI::Player newPlayer) { thisPlayer = newPlayer; }
        void setAlive(bool newState) { alive = newState; }
        void setBuild(std::string newBuild) { build = newBuild; }

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