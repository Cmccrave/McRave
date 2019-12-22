#pragma once
#include "McRave.h"
#include "PlayerInfo.h"

namespace McRave::Players
{
    int getCurrentCount(PlayerState, BWAPI::UnitType);
    int getTotalCount(PlayerState, BWAPI::UnitType);
    bool hasDetection(PlayerState);
    bool hasMelee(PlayerState);
    bool hasRanged(PlayerState);

    int getSupply(PlayerState);
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

    bool PvP();
    bool PvZ();
    bool PvT();

    bool TvP();
    bool TvZ();
    bool TvT();

    bool ZvP();
    bool ZvZ();
    bool ZvT();
}
