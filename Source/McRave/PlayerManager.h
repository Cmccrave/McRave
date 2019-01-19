#pragma once
#include "McRave.h"
#include "PlayerInfo.h"

namespace McRave::Players
{
    struct TotalStrength {
        double airToAir = 0.0;
        double airToGround = 0.0;
        double groundToAir = 0.0;
        double groundToGround = 0.0;
        double airDefense = 0.0;
        double groundDefense = 0.0;

        void operator+= (const TotalStrength &second) {
            airToAir += second.airToAir;
            airToGround += second.airToGround;
            groundToAir += second.groundToAir;
            groundToGround += second.groundToGround;
            airDefense += second.airDefense;
            groundDefense += second.groundDefense;
        }
    };

    TotalStrength getStrength(PlayerState);
    TotalStrength getStrength(PlayerInfo);

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
    
    int getSupply(PlayerInfo&);

    double strengthCompare(CombatType);
}
