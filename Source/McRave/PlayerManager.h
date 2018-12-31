#pragma once
#include "McRave.h"
#include "PlayerInfo.h"

namespace McRave::Players
{
    void onStart();
    void onFrame();

    std::map <BWAPI::Player, PlayerInfo>& getPlayers();
    int getNumberZerg();
    int getNumberProtoss();
    int getNumberTerran();
    int getNumberRandom();
    bool vP();
    bool vT();
    bool vZ();
}
