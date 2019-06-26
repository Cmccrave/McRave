#pragma once
#include <BWAPI.h>

namespace McRave::Math {

    double getMaxGroundStrength(UnitInfo&);
    double getVisibleGroundStrength(UnitInfo&);
    double getMaxAirStrength(UnitInfo&);
    double getVisibleAirStrength(UnitInfo&);
    double getPriority(UnitInfo&);
    double relativeCost(UnitInfo&);
    double groundDPS(UnitInfo&);    
    double airDPS(UnitInfo&);
    double gWeaponCooldown(UnitInfo&);
    double aWeaponCooldown(UnitInfo&);
    double splashModifier(UnitInfo&);
    double effectiveness(UnitInfo&);
    double survivability(UnitInfo&);
    double groundRange(UnitInfo&);
    double airRange(UnitInfo&);
    double groundDamage(UnitInfo&);
    double airDamage(UnitInfo&);
    double speed(UnitInfo&);
    int stopAnimationFrames(BWAPI::UnitType);
    int firstAttackAnimationFrames(BWAPI::UnitType);
    int contAttackAnimationFrames(BWAPI::UnitType);

    BWAPI::WalkPosition getWalkPosition(BWAPI::Unit unit);
}