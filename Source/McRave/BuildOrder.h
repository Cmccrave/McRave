#pragma once
#include <BWAPI.h>
#include <sstream>
#include <set>

namespace McRave::BuildOrder {

    // Need a namespace to share variables among the various files used
    namespace All {
        inline std::map <BWAPI::UnitType, int> buildQueue;
        inline bool inOpeningBook = true;
        inline bool getTech = false;
        inline bool wallNat = false;
        inline bool wallMain = false;
        inline bool scout = false;
        inline bool productionSat = false;
        inline bool techSat = false;
        inline bool wantNatural = false;
        inline bool wantThird = false;
        inline bool proxy = false;
        inline bool hideTech = false;
        inline bool playPassive = false;
        inline bool rush = false;
        inline bool pressure = false;
        inline bool lockedTransition = false;
        inline bool gasTrick = false;
        inline bool inBookSupply = false;
        inline bool transitionReady = false;
        inline bool planEarly = false;

        inline bool gasDesired = false;
        inline bool expandDesired = false;
        inline bool rampDesired = false;
        // inline bool techDesired = false;

        inline std::map<BWAPI::UnitType, int> unitLimits;
        inline int gasLimit = INT_MAX;
        inline int s = 0;

        inline std::string currentBuild = "";
        inline std::string currentOpener = "";
        inline std::string currentTransition = "";

        inline BWAPI::UpgradeType firstUpgrade = BWAPI::UpgradeTypes::None;
        inline BWAPI::TechType firstTech = BWAPI::TechTypes::None;
        inline BWAPI::UnitType firstUnit = BWAPI::UnitTypes::None;
        inline BWAPI::UnitType techUnit = BWAPI::UnitTypes::None;
        inline BWAPI::UnitType desiredDetection = BWAPI::UnitTypes::None;
        inline std::vector<BWAPI::UnitType> techOrder;
        inline std::set <BWAPI::UnitType> techList;
        inline std::set <BWAPI::UnitType> unlockedType;

        inline std::map <BWAPI::UnitType, double> armyComposition;
    }

    namespace Protoss {

        void opener();
        void tech();
        void situational();
        void composition();
        void unlocks();

        void PvP_1GC();
        void PvP_2G();

        void PvT_1GC();
        void PvT_2G();
        void PvT_2B();

        void PvZ_1GC();
        void PvZ_2G();
        void PvZ_FFE();

        void PvFFA_1GC();

        bool goonRange();
        bool enemyMoreZealots();
        bool enemyMaybeDT();
        void defaultPvP();
        void defaultPvT();
        void defaultPvZ();

        void PvT();
        void PvP();
        void PvZ();
        void PvFFA();
    }

    namespace Terran {
        void opener();
        void tech();
        void situational();
        void composition();
        void unlocks();

        void TvA();
    }

    namespace Zerg {

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
        bool gas(int);
        int gasMax();
        int capGas(int);
        int hatchCount();
        int colonyCount();

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

    double getCompositionPercentage(BWAPI::UnitType);
    std::map<BWAPI::UnitType, double> getArmyComposition();
    int buildCount(BWAPI::UnitType);
    bool firstReady();
    bool unlockReady(BWAPI::UnitType);

    void onFrame();
    void getNewTech();
    void getTechBuildings();

    void checkExpand();
    void checkRamp();

    int getGasQueued();
    int getMinQueued();
    bool shouldRamp();
    bool shouldAddGas();
    bool shouldExpand();
    bool techComplete();
    bool atPercent(BWAPI::UnitType, double);
    bool atPercent(BWAPI::TechType, double);

    std::map<BWAPI::UnitType, int>& getBuildQueue();
    BWAPI::UnitType getTechUnit();
    BWAPI::UpgradeType getFirstUpgrade();
    BWAPI::TechType getFirstTech();
    std::set <BWAPI::UnitType>& getTechList();
    std::set <BWAPI::UnitType>& getUnlockedList();
    int gasWorkerLimit();
    int getUnitLimit(BWAPI::UnitType);
    bool isUnitUnlocked(BWAPI::UnitType);
    bool isTechUnit(BWAPI::UnitType);
    bool isOpener();
    bool takeNatural();
    bool takeThird();
    bool shouldScout();
    bool isWallNat();
    bool isWallMain();
    bool isProxy();
    bool isHideTech();
    bool isPlayPassive();
    bool isRush();
    bool isPressure();
    bool isGasTrick();
    bool isPlanEarly();
    std::string getCurrentBuild();
    std::string getCurrentOpener();
    std::string getCurrentTransition();

    void setLearnedBuild(std::string, std::string, std::string);
}