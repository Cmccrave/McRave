#pragma once
#include <BWAPI.h>

struct UnitData
{
    double visibleGroundStrength = 0.0;
    double visibleAirStrength = 0.0;
    double maxGroundStrength = 0.0;
    double maxAirStrength = 0.0;
    double percentHealth = 0.0;
    double percentShield = 0.0;
    double percentTotal = 0.0;
    double groundRange = 0.0;
    double groundReach = 0.0;
    double groundDamage = 0.0;
    double airRange = 0.0;
    double airReach = 0.0;
    double airDamage = 0.0;
    double speed = 0.0;
    double priority = 0.0;

    int shields = 0;
    int health = 0;
    int armor = 0;
    int shieldArmor = 0;
    int minStopFrame = 0;
    int energy = 0;
    int walkWidth = 0;
    int walkHeight = 0;
};

namespace McRave::Math {

    double maxGroundStrength(UnitInfo&);
    double visibleGroundStrength(UnitInfo&);
    double maxAirStrength(UnitInfo&);
    double visibleAirStrength(UnitInfo&);
    double priority(UnitInfo&);
    double relativeCost(UnitInfo&);
    double groundDPS(UnitInfo&);
    double airDPS(UnitInfo&);
    double groundCooldown(UnitInfo&);
    double airCooldown(UnitInfo&);
    double splashModifier(UnitInfo&);
    double grdEffectiveness(UnitInfo&);
    double airEffectiveness(UnitInfo&);
    double survivability(UnitInfo&);
    double groundRange(UnitInfo&);
    double airRange(UnitInfo&);
    double groundReach(UnitInfo&);
    double airReach(UnitInfo&);
    double groundDamage(UnitInfo&);
    double airDamage(UnitInfo&);
    double moveSpeed(UnitInfo&);
    double simRadius(UnitInfo&);
    int realisticMineralCost(BWAPI::UnitType);
    int realisticGasCost(BWAPI::UnitType);
    int stopAnimationFrames(BWAPI::UnitType);
    int firstAttackAnimationFrames(BWAPI::UnitType);
    int contAttackAnimationFrames(BWAPI::UnitType);
}