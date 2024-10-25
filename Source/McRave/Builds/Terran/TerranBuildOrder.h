#pragma once

namespace McRave::BuildOrder::Terran {
    inline BWAPI::UnitType rampType = BWAPI::UnitTypes::Terran_Barracks;

    inline std::map<BWAPI::UnitType, bool> terranUnitPump;

    void opener();
    void tech();
    void situational();
    void composition();
    void unlocks();

    void TvA();

    void TvZ();
    void TvZ_2Rax();

    void TvP();
    void TvP_RaxFact();
}
