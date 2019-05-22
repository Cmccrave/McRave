#pragma once
#include "McRave.h"
#include "PlayerInfo.h"

namespace McRave::Players
{
    int getSupply(PlayerState);
    int getRaceCount(BWAPI::Race, PlayerState);
    Strength getStrength(PlayerState);

    void addStrength(UnitInfo&);
    void onStart();
    void onFrame();

    std::map <BWAPI::Player, PlayerInfo>& getPlayers();

    bool vP();
    bool vT();
    bool vZ();

    bool PvP();
    bool PvZ();
    bool PvT();
}
