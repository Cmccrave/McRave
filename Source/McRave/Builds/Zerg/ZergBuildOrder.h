#pragma once

namespace McRave::BuildOrder::Zerg {

    inline int reserveLarva = 0;
    inline std::map<BWAPI::UnitType, bool> zergUnitPump;

    void opener();
    void tech();
    void situational();
    void composition();
    void unlocks();

    void ZvT_HP();
    void ZvT_PH();

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
}
