#include "UnitInfo.h"

// Testing using anonymous namespaces for some functions

namespace McRave {

	namespace Math {

		double getMaxGroundStrength(UnitInfo& unit);

		double getVisibleGroundStrength(UnitInfo& unit);

		double getMaxAirStrength(UnitInfo& unit);

		double getVisibleAirStrength(UnitInfo& unit);

		double getPriority(UnitInfo& unit);

		double groundDPS(UnitInfo& unit);

		double airDPS(UnitInfo& unit);

		double gWeaponCooldown(UnitInfo& unit);

		double aWeaponCooldown(UnitInfo& unit);

		double splashModifier(UnitInfo& unit);

		double effectiveness(UnitInfo& unit);

		double survivability(UnitInfo& unit);

		double groundRange(UnitInfo& unit);

		double airRange(UnitInfo& unit);

		double groundDamage(UnitInfo& unit);

		double airDamage(UnitInfo& unit);

		double speed(UnitInfo& unit);

		int getMinStopFrame(UnitType unitType);

		WalkPosition getWalkPosition(Unit unit);
	}
}