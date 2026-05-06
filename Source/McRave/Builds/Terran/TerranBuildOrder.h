#pragma once
#include "Main/Common.h"
#include "Builds/All/BuildOrder.h"

namespace McRave::BuildOrder::Terran {

    inline std::map<BWAPI::UnitType, bool> terranUnitPump;

    void opener();
    void tech();
    void situational();
    void composition();
    void unlocks();

    void TvA();

    void TvZ();
    void TvZ_2Rax();
    void TvZ_RaxFact();

    void TvP();
    void TvP_RaxFact();

    void TvT();
    void TvT_RaxFact();
}
