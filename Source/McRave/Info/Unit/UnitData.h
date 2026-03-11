#pragma once
#include "Main/Common.h"

namespace McRave {
    class UnitData {
    protected:
        double visibleGroundStrength = 0.0;
        double visibleAirStrength    = 0.0;
        double maxGroundStrength     = 0.0;
        double maxAirStrength        = 0.0;
        double percentHealth         = 0.0;
        double percentShield         = 0.0;
        double percentTotal          = 0.0;
        double groundRange           = 0.0;
        double groundReach           = 0.0;
        double groundDamage          = 0.0;
        double airRange              = 0.0;
        double airReach              = 0.0;
        double airDamage             = 0.0;
        double speed                 = 0.0;
        double priority              = 0.0;

        int shields     = 0;
        int health      = 0;
        int armor       = 0;
        int shieldArmor = 0;
        int energy      = 0;
        int splash      = 0;
        int walkWidth   = 0;
        int walkHeight  = 0;

    public:
        double getPercentTotal() { return percentTotal; }
        double getVisibleGroundStrength() { return visibleGroundStrength; }
        double getMaxGroundStrength() { return maxGroundStrength; }
        double getVisibleAirStrength() { return visibleAirStrength; }
        double getMaxAirStrength() { return maxAirStrength; }
        double getGroundRange() { return groundRange; }
        double getGroundReach() { return groundReach; }
        double getGroundDamage() { return groundDamage; }
        double getAirRange() { return airRange; }
        double getAirReach() { return airReach; }
        double getAirDamage() { return airDamage; }
        double getSpeed() { return speed; }
        double getPriority() { return priority; }

        int getArmor() { return armor; }
        int getShields() { return shields; }
        int getHealth() { return health; }
        int getEnergy() { return energy; }
        int getSplashRadius() { return splash; }
        int getWalkWidth() { return walkWidth; }
        int getWalkHeight() { return walkHeight; }

        bool isMelee() { return groundDamage > 0.0 && groundRange < 64.0; }

        void updateUnitData(UnitInfo &unit);
    };
} // namespace McRave