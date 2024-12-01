#pragma once

namespace McRave::BuildOrder::Zerg {

    inline int reserveLarva = 0;
    inline std::map<BWAPI::UnitType, bool> zergUnitPump;

    inline const vector<UnitType> mutaling ={ Zerg_Mutalisk, Zerg_Zergling };
    inline const vector<UnitType> hydralurk ={ Zerg_Hydralisk, Zerg_Lurker, Zerg_Mutalisk };
    inline const vector<UnitType> mutalurk ={ Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker };

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
}
