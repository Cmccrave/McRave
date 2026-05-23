#pragma once
#include <BWAPI.h>

#include <set>
#include <sstream>

#include "Main/Helpers.h"

namespace McRave::BuildOrder {

    double getCompositionPercentage(BWAPI::UnitType);
    std::map<BWAPI::UnitType, double> getArmyComposition();
    int buildCount(BWAPI::UnitType);
    bool unlockReady(BWAPI::UnitType);

    void onFrame();
    void getNewTech();
    void getTechBuildings(BWAPI::UnitType);

    int getGasQueued();
    int getMinQueued();
    bool shouldRamp();
    bool shouldExpand();
    bool techComplete();
    bool atPercent(BWAPI::UnitType, double);
    bool atPercent(BWAPI::TechType, double);

    std::map<BWAPI::UnitType, int> &getBuildQueue();
    std::map<BWAPI::UpgradeType, int> &getUpgradeQueue();
    std::map<BWAPI::TechType, int> &getTechQueue();
    BWAPI::UnitType getRampType();
    std::set<BWAPI::UnitType> &getUnlockedList();
    int gasWorkerLimit();
    int gasMax();
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
    bool isRush(BWAPI::UnitType type = BWAPI::UnitTypes::None);
    bool isPressure(BWAPI::UnitType type = BWAPI::UnitTypes::None);
    bool isGasTrick();
    bool isPlanEarly();
    bool mineralThirdDesired();
    std::string getCurrentBuild();
    std::string getCurrentOpener();
    std::string getCurrentTransition();

    void setLearnedBuild(std::string_view, std::string_view, std::string_view);
} // namespace McRave::BuildOrder