#include "McRave.h"
#include "CombatUnitManager.h"

void CombatUnitManager::onFrame() {

	for (auto u : enemyCombatUnits) {
		auto &unit = u.second;

		// If unit is visible, update it
		if (unit.info()->unit()->exists()) {
			if (unit.hasTarget() && (unit.info()->getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.info()->getType() == UnitTypes::Protoss_Scarab))
				splashTargets.insert(unit.getTarget().unit());
		}
	}
}