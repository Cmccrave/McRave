#pragma once
#include <BWAPI.h>
#include <sstream>
#include <set>

namespace McRave::BuildOrder {

    enum class AllinType {
        None, Z_5HatchSpeedling, Z_6HatchSpeedling, Z_6HatchCrackling, Z_8HatchCrackling
    };

    struct Allin {
        std::string name;
        BWAPI::UnitType type;
        int workerCount = 0;
        int productionCount = 0;
        int typeCount = 0;
        bool isValid() {
            return name != "";
        }
        bool isActive() {
            return type != BWAPI::UnitTypes::None && typeCount > 0 && total(type) >= typeCount;
        }
        bool isPreparing() {
            return type != BWAPI::UnitTypes::None;
        }
    };

    // Need a namespace to share variables among the various files used
    namespace All {
        inline std::map <BWAPI::UnitType, int> buildQueue;
        inline std::map <BWAPI::TechType, int> techQueue;
        inline std::map <BWAPI::UpgradeType, int> upgradeQueue;
        inline bool inOpening = true;
        inline bool inTransition = false;

        inline bool getTech = false;
        inline bool wallNat = false;
        inline bool wallMain = false;
        inline bool wallThird = false;
        inline bool scout = false;
        inline bool productionSat = false;
        inline bool techSat = false;
        inline bool wantNatural = false;
        inline bool wantThird = false;
        inline bool proxy = false;
        inline bool hideTech = false;
        inline bool rush = false;
        inline bool pressure = false;        

        inline bool inBookSupply = false;
        inline bool transitionReady = false;
        inline bool planEarly = false;
        inline bool gasTrick = false;

        inline bool gasDesired = false;
        inline bool expandDesired = false;
        inline bool rampDesired = false;
        inline bool mineralThird = false;

        inline std::map<BWAPI::UnitType, int> unitLimits;
        inline std::map<BWAPI::UnitType, int> unitReservations;
        inline int gasLimit = INT_MAX;
        inline int s = 0;

        inline std::string currentBuild = "";
        inline std::string currentOpener = "";
        inline std::string currentTransition = "";

        // Focus queue
        inline BWAPI::UnitType focusUnit = BWAPI::UnitTypes::None;
        inline std::vector<BWAPI::UnitType> unitOrder;

        // Focus complete
        inline std::set <BWAPI::UnitType> focusUnits;

        inline BWAPI::UnitType desiredDetection = BWAPI::UnitTypes::None;        
        inline std::set <BWAPI::UnitType> unlockedType;
        inline std::map <BWAPI::UnitType, double> armyComposition;

        inline Allin activeAllin;
        inline AllinType activeAllinType;
    }

    namespace Zerg {
        inline int reserveLarva = 0;
        inline bool pumpLings = false;
        inline bool pumpHydras = false;
        inline bool pumpMutas = false;
        inline bool pumpLurker = false;
        inline bool pumpScourge = false;
        inline bool pumpDefiler = false;

        inline std::map<int, int> baseToHatchRatio;
    }

    namespace Terran {
        inline BWAPI::UnitType rampType = BWAPI::UnitTypes::Terran_Barracks;
    }

    double getCompositionPercentage(BWAPI::UnitType);
    std::map<BWAPI::UnitType, double> getArmyComposition();
    int buildCount(BWAPI::UnitType);
    bool unlockReady(BWAPI::UnitType);

    void onFrame();
    void getNewTech();
    void getTechBuildings();

    int getGasQueued();
    int getMinQueued();
    bool shouldRamp();
    bool shouldExpand();
    bool techComplete();
    bool atPercent(BWAPI::UnitType, double);
    bool atPercent(BWAPI::TechType, double);

    std::map<BWAPI::UnitType, int>& getBuildQueue();
    std::map<BWAPI::UpgradeType, int>& getUpgradeQueue();
    std::map<BWAPI::TechType, int>& getTechQueue();
    BWAPI::UnitType getFirstFocusUnit();
    std::set <BWAPI::UnitType>& getUnlockedList();
    int gasWorkerLimit();
    int getUnitReservation(BWAPI::UnitType);
    bool isAllIn();
    bool isPreparingAllIn();
    bool isUnitUnlocked(BWAPI::UnitType);
    bool isFocusUnit(BWAPI::UnitType);
    bool isOpener();
    bool takeNatural();
    bool takeThird();
    bool shouldScout();
    bool isWallNat();
    bool isWallMain();
    bool isWallThird();
    bool isProxy();
    bool isHideTech();
    bool isRush();
    bool isPressure();
    bool isGasTrick();
    bool isPlanEarly();
    bool mineralThirdDesired();
    std::string getCurrentBuild();
    std::string getCurrentOpener();
    std::string getCurrentTransition();

    void setLearnedBuild(std::string, std::string, std::string);
}