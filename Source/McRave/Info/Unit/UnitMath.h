#pragma once
#include "Main/Common.h"

namespace McRave::Math {
    double calcMaxGroundStrength(UnitInfo &);
    double calcVisibleGroundStrength(UnitInfo &);
    double calcMaxAirStrength(UnitInfo &);
    double calcVisibleAirStrength(UnitInfo &);
    double calcPriority(UnitInfo &);
    double calcRelativeCost(UnitInfo &);
    double calcGroundDPS(UnitInfo &);
    double calcAirDPS(UnitInfo &);
    double calcGroundCooldown(UnitInfo &);
    double calcAirCooldown(UnitInfo &);
    double calcSplashModifier(UnitInfo &);
    double calcGrdEffectiveness(UnitInfo &);
    double calcAirEffectiveness(UnitInfo &);
    double calcSurvivability(UnitInfo &);
    double calcGroundRange(UnitInfo &);
    double calcAirRange(UnitInfo &);
    double calcGroundReach(UnitInfo &);
    double calcAirReach(UnitInfo &);
    double calcGroundDamage(UnitInfo &);
    double calcAirDamage(UnitInfo &);
    double calcMoveSpeed(UnitInfo &);
    double calcSimRadius(UnitInfo &);
    int realisticMineralCost(BWAPI::UnitType);
    int realisticGasCost(BWAPI::UnitType);
    int stopAnimationFrames(BWAPI::UnitType);
    int firstAttackAnimationFrames(BWAPI::UnitType);
    int contAttackAnimationFrames(BWAPI::UnitType);
} // namespace McRave::Math
