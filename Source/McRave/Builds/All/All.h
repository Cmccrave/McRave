#pragma once
#include <BWAPI.h>

#include <set>

#include "Main/Helpers.h"
#include "Micro/Combat/Combat.h"
#include "Info/Player/Players.h"

namespace McRave::BuildOrder::All {

    enum class AllinType { None, Z_3HatchSpeedling, Z_5HatchSpeedling, Z_9HatchCrackling };

    struct Allin {
        std::string name;
        BWAPI::UnitType type;
        int workerCount     = 0;
        int productionCount = 0;
        int typeCount       = 0;

        bool isValid() { return name != ""; }
        bool isActive() { return type != BWAPI::UnitTypes::None && typeCount > 0 && total(type) >= typeCount && !Combat::State::isStaticRetreat(BWAPI::UnitTypes::Zerg_Zergling); }
        bool isPreparing() { return type != BWAPI::UnitTypes::None; }
    };

    inline std::map<BWAPI::UnitType, int> buildQueue;
    inline std::map<BWAPI::TechType, int> techQueue;
    inline std::map<BWAPI::UpgradeType, int> upgradeQueue;
    inline bool inOpening    = true;
    inline bool inTransition = false;

    inline bool wallNat       = false;
    inline bool wallMain      = false;
    inline bool wallThird     = false;
    inline bool scout         = false;
    inline bool productionSat = false;
    inline bool techSat       = false;
    inline bool wantNatural   = false;
    inline bool wantThird     = false;
    inline bool proxy         = false;
    inline bool hideTech      = false;
    inline bool rush          = false;
    inline bool pressure      = false;

    inline bool inBookSupply    = false;
    inline bool transitionReady = false;
    inline bool planEarly       = false;
    inline bool gasTrick        = false;

    inline bool gasDesired    = false;
    inline bool expandDesired = false;
    inline bool rampDesired   = false;
    inline bool techDesired   = false;
    inline bool mineralThird  = false;

    inline std::map<BWAPI::UnitType, int> unitReservations;
    inline std::map<BWAPI::UnitType, bool> unitRush;
    inline std::map<BWAPI::UnitType, bool> unitPressure;
    inline int gasLimit = INT_MAX;
    inline int s        = 0;

    inline bool gas(int amount) { return BWAPI::Broodwar->self()->gas() >= amount; }
    inline bool minerals(int amount) { return BWAPI::Broodwar->self()->minerals() >= amount; }

    inline std::string currentBuild      = "";
    inline std::string currentOpener     = "";
    inline std::string currentTransition = "";

    inline BWAPI::UnitType focusUnit = BWAPI::UnitTypes::None;
    inline BWAPI::UnitType rampType  = BWAPI::UnitTypes::None;
    inline std::vector<BWAPI::UnitType> unitOrder;
    inline std::set<BWAPI::UnitType> focusUnits;

    inline std::set<BWAPI::UnitType> unlockedType;
    inline std::map<BWAPI::UnitType, double> armyComposition;

    inline Allin activeAllin;
    inline AllinType activeAllinType;
} // namespace All