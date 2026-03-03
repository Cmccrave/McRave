#pragma once
#include "Builds/All/BuildOrder.h"
#include "Main/Common.h"

namespace McRave::BuildOrder::Zerg {

    inline int reserveLarva = 0;
    inline std::map<BWAPI::UnitType, bool> zergUnitPump;

    // ZvP
    inline const std::vector<BWAPI::UnitType> mutaling  = {BWAPI::UnitTypes::Zerg_Mutalisk, BWAPI::UnitTypes::Zerg_Zergling};
    inline const std::vector<BWAPI::UnitType> hydralurk = {BWAPI::UnitTypes::Zerg_Hydralisk, BWAPI::UnitTypes::Zerg_Lurker, BWAPI::UnitTypes::Zerg_Mutalisk};
    inline const std::vector<BWAPI::UnitType> mutalurk  = {BWAPI::UnitTypes::Zerg_Mutalisk, BWAPI::UnitTypes::Zerg_Hydralisk, BWAPI::UnitTypes::Zerg_Lurker};

    // ZvT
    inline const std::vector<BWAPI::UnitType> mutalingdefiler = {BWAPI::UnitTypes::Zerg_Mutalisk, BWAPI::UnitTypes::Zerg_Zergling, BWAPI::UnitTypes::Zerg_Defiler};
    inline const std::vector<BWAPI::UnitType> ultraling       = {BWAPI::UnitTypes::Zerg_Mutalisk, BWAPI::UnitTypes::Zerg_Ultralisk, BWAPI::UnitTypes::Zerg_Zergling, BWAPI::UnitTypes::Zerg_Defiler};
    inline const std::vector<BWAPI::UnitType> defilerling     = {BWAPI::UnitTypes::Zerg_Mutalisk, BWAPI::UnitTypes::Zerg_Defiler, BWAPI::UnitTypes::Zerg_Zergling, BWAPI::UnitTypes::Zerg_Ultralisk};

    void opener();
    void tech();
    void situational();
    void composition();
    void unlocks();

    void ZvT_HP();
    void ZvT_PH();
    void ZvT_PL();

    void ZvP_HP();
    void ZvP_PH();

    void ZvZ_PH();
    void ZvZ_PL();

    void ZvFFA_HP();

    bool lingSpeed();
    bool hydraSpeed();
    bool hydraRange();
    bool gas(int);
    bool minerals(int);
    int capGas(int);
    int hatchCount();

    int lingsNeeded_ZvFFA();
    int lingsNeeded_ZvP();
    int lingsNeeded_ZvZ();
    int lingsNeeded_ZvT();

    void defaultZvT();
    void defaultZvP();
    void defaultZvZ();
    void defaultZvFFA();

    void ZvT();
    void ZvP();
    void ZvZ();
    void ZvFFA();
} // namespace McRave::BuildOrder::Zerg
