#pragma once
#include "McRave.h"

namespace McRave
{
    namespace Players 
    {
        int getVisibleCount(PlayerState, BWAPI::UnitType);
        int getCompleteCount(PlayerState, BWAPI::UnitType);
        int getTotalCount(PlayerState, BWAPI::UnitType);
        bool hasDetection(PlayerState);
        bool hasMelee(PlayerState);
        bool hasRanged(PlayerState);

        int getSupply(PlayerState, BWAPI::Race);
        int getRaceCount(BWAPI::Race, PlayerState);
        Strength getStrength(PlayerState);

        void onStart();
        void onFrame();
        void storeUnit(BWAPI::Unit);
        void removeUnit(BWAPI::Unit);
        void morphUnit(BWAPI::Unit);

        std::map <BWAPI::Player, PlayerInfo>& getPlayers();
        PlayerInfo * getPlayerInfo(BWAPI::Player);

        bool vP();
        bool vT();
        bool vZ();

        inline bool PvP() { return vP() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss; }
        inline bool PvT() { return vT() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss; }
        inline bool PvZ() { return vZ() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss; }

        inline bool TvP() { return vP() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran; }
        inline bool TvT() { return vT() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran; }
        inline bool TvZ() { return vZ() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran; }

        inline bool ZvP() { return vP() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg; }
        inline bool ZvT() { return vT() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg; }
        inline bool ZvZ() { return vZ() && BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg; }
    }

    /// Returns the self owned visible unit count of this UnitType
    static int vis(BWAPI::UnitType t) {
        return Players::getVisibleCount(PlayerState::Self, t);
    }

    /// Returns the self owned completed unit count of this UnitType
    static int com(BWAPI::UnitType t) {
        return Players::getCompleteCount(PlayerState::Self, t);
    }

    /// Returns the self total unit count of this UnitType
    static int total(BWAPI::UnitType t) {
        return Players::getTotalCount(PlayerState::Self, t);
    }
}
