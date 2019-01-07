#pragma once
#include <BWAPI.h>
#include <sstream>
#include <set>

namespace McRave::BuildOrder {
    class Item
    {
        int actualCount, reserveCount;
    public:
        Item() {};

        Item(int actual, int reserve = -1) {
            actualCount = actual, reserveCount = (reserve == -1 ? actual : reserve);
        }

        //Item(int c1, int c2 = 0, int c3 = 0, int c4 = 0, int c5 = 0, int c6 = 0, int c7 = 0) {
        //	int s = Units::getSupply() / 2;
        //	reserveCount = (s >= c1) + (s >= c2) + (s >= c3) + (s >= c4) + (s >= c5) + (s >= c6) + (s >= c7);
        //}

        int const getReserveCount() { return reserveCount; }
        int const getActualCount() { return actualCount; }
    };

    struct Build {
        std::vector <std::string> transitions;
        std::vector <std::string> openers;
    };

    // Need a namespace to share variables among the various files used
    namespace All {
        inline std::map <BWAPI::UnitType, Item> itemQueue;
        inline bool getOpening = true;
        inline bool getTech = false;
        inline bool wallNat = false;
        inline bool wallMain = false;
        inline bool scout = false;
        inline bool productionSat = false;
        inline bool techSat = false;
        inline bool fastExpand = false;
        inline bool proxy = false;
        inline bool hideTech = false;
        inline bool playPassive = false;
        inline bool rush = false;
        inline bool cutWorkers = false; // TODO: Use unlocking
        inline bool lockedTransition = false;
        inline bool gasTrick = false;
        inline bool bookSupply = false;

        inline int satVal, prodVal, techVal, baseVal;
        inline int gasLimit = INT_MAX;
        inline int zealotLimit = INT_MAX;
        inline int dragoonLimit = INT_MAX;
        inline int lingLimit = INT_MAX;
        inline int droneLimit = INT_MAX;

        inline std::map <std::string, Build> myBuilds;
        inline std::stringstream ss;
        inline std::string currentBuild = "";
        inline std::string currentOpener = "";
        inline std::string currentTransition = "";

        inline BWAPI::UpgradeType firstUpgrade = BWAPI::UpgradeTypes::None;
        inline BWAPI::TechType firstTech = BWAPI::TechTypes::None;
        inline BWAPI::UnitType firstUnit = BWAPI::UnitTypes::None;
        inline BWAPI::UnitType techUnit;
        inline BWAPI::UnitType productionUnit;
        inline std::set <BWAPI::UnitType> techList;
        inline std::set <BWAPI::UnitType> unlockedType;
        inline std::vector <std::string> buildNames;
    }

    namespace Protoss {

        void opener();
        void tech();
        void situational();
        void unlocks();
        void island();

        void P1GateCore();
        void PFFE();
        void PNexusGate();
        void PGateNexus();
        void P2Gate();
    }

    namespace Zerg {

        void opener();
        void tech();
        void situational();
        void unlocks();
        void island();

        void PoolHatch();
        void HatchPool();
        void PoolLair();
    }

    void getDefaultBuild();
    int buildCount(BWAPI::UnitType);
    bool firstReady();

    void getNewTech();
    void checkNewTech();
    void checkAllTech();
    void checkExoticTech();
    bool shouldAddProduction();
    bool shouldAddGas();
    bool shouldExpand();
    bool techComplete();

    std::map<BWAPI::UnitType, Item>& getItemQueue();
    BWAPI::UnitType getTechUnit();
    BWAPI::UnitType getFirstUnit();
    BWAPI::UpgradeType getFirstUpgrade();
    BWAPI::TechType getFirstTech();
    std::set <BWAPI::UnitType>& getTechList();
    std::set <BWAPI::UnitType>& getUnlockedList();
    int gasWorkerLimit();
    bool isWorkerCut();
    bool isUnitUnlocked(BWAPI::UnitType unit);
    bool isTechUnit(BWAPI::UnitType unit);
    bool isOpener();
    bool isFastExpand();
    bool shouldScout();
    bool isWallNat();
    bool isWallMain();
    bool isProxy();
    bool isHideTech();
    bool isPlayPassive();
    bool isRush();
    bool isGasTrick();
    std::string getCurrentBuild();
    std::string getCurrentOpener();
    std::string getCurrentTransition();

    void onEnd(bool);
    void onStart();
    void onFrame();
}