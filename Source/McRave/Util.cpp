#include "McRave.h"

double UtilManager::getPercentHealth(UnitInfo& unit)
{
	// Returns the percent of health for a unit, with higher emphasis on health over shields
	return double(unit.unit()->getHitPoints() + (unit.unit()->getShields() / 2)) / double(unit.getType().maxHitPoints() + (unit.getType().maxShields() / 2));
}

double UtilManager::getMaxGroundStrength(UnitInfo& unit)
{
	// HACK: Some hardcoded values
	if (unit.getType() == UnitTypes::Terran_Medic)
		return 5.0;
	else if (unit.getType() == UnitTypes::Protoss_Scarab || unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Zerg_Egg || unit.getType() == UnitTypes::Zerg_Larva || unit.getGroundRange() <= 0.0)
		return 0.0;
	else if (unit.getType() == UnitTypes::Protoss_Interceptor)
		return 2.0;
	else if (unit.getType() == UnitTypes::Protoss_Carrier) {
		double cnt = 0.0;
		for (auto &i : unit.unit()->getInterceptors()) {
			if (i && !i->exists()) {
				cnt += 2.0;
			}
		}
		return cnt;
	}

	double dps, surv, eff;
	dps = groundDPS(unit);
	surv = log(survivability(unit));
	eff = effectiveness(unit);

	return dps * surv * eff;
}

double UtilManager::getVisibleGroundStrength(UnitInfo& unit)
{
	if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised())
		return 0.0;
	return unit.getPercentHealth() * unit.getMaxGroundStrength();
}

double UtilManager::getMaxAirStrength(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Protoss_Scarab || unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Zerg_Egg || unit.getType() == UnitTypes::Zerg_Larva || unit.getAirRange() <= 0.0)
		return 0.0;

	else if (unit.getType() == UnitTypes::Protoss_Interceptor)
		return 2.0;

	else if (unit.getType() == UnitTypes::Protoss_Carrier) {
		double cnt = 0.0;
		for (auto &i : unit.unit()->getInterceptors()) {
			if (i && !i->exists()) {
				cnt += 2.0;
			}
		}
		return cnt;
	}

	double dps, surv, eff;
	dps = airDPS(unit);
	surv = log(survivability(unit));
	eff = effectiveness(unit);

	return dps * surv * eff;
}

double UtilManager::getVisibleAirStrength(UnitInfo& unit)
{
	if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised())
		return 0.0;
	return unit.getPercentHealth() * unit.getMaxAirStrength();
}

double UtilManager::getPriority(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Terran_Science_Vessel)
		return 3.0;
	if ((unit.unit()->isRepairing() || unit.unit()->isConstructing()) && Units().isThreatening(unit))
		return 100.0;
	if (Broodwar->getFrameCount() < 6000 && Strategy().enemyProxy() && unit.getType() == UnitTypes::Protoss_Pylon)
		return -5.0;

	const auto area = mapBWEM.GetArea(unit.getTilePosition());
	if (area && Terrain().isInAllyTerritory(unit.getTilePosition())) {
		for (auto &gas : area->Geysers()) {
			if (gas->TopLeft().getDistance(unit.getTilePosition()) < 2 && !unit.unit()->isInvincible())
				return 100.0;
		}
	}

	double mineral, gas;

	if (unit.getType() == UnitTypes::Protoss_Archon)
		mineral = 100.0, gas = 300.0;
	else if (unit.getType() == UnitTypes::Protoss_Dark_Archon)
		mineral = 250.0, gas = 200.0;
	else if (unit.getType() == UnitTypes::Zerg_Sunken_Colony || unit.getType() == UnitTypes::Zerg_Spore_Colony)
		mineral = 175.0, gas = 0.0;
	else
		mineral = unit.getType().mineralPrice(), gas = unit.getType().gasPrice();

	double dps = 1.00 + max({ groundDPS(unit), airDPS(unit), 1.0 });
	double cost = max(log((mineral * 0.33) + (gas * 0.66)) * (double)max(unit.getType().supplyRequired(), 1), 5.00);
	double surv = survivability(unit);

	return log(10.0 + (dps * cost / surv));
}

WalkPosition UtilManager::getWalkPosition(Unit unit)
{
	if (!unit->getType().isBuilding())
		return WalkPosition(Position(unit->getLeft(), unit->getTop()));
	else
		return WalkPosition(unit->getTilePosition());
	return WalkPositions::None;
}

bool UtilManager::isSafe(WalkPosition end, UnitType unitType, bool groundCheck, bool airCheck)
{
	int walkWidth = int(ceil(unitType.width() / 8.0));
	int walkHeight = int(ceil(unitType.height() / 8.0));
	int halfW = int(ceil(walkWidth / 2));
	int halfH = int(ceil(walkHeight / 2));

	for (int x = end.x - halfW; x <= end.x + halfW; x++) {
		for (int y = end.y - halfH; y <= end.y + halfH; y++) {
			WalkPosition w(x, y);
			if (!w.isValid())
				continue;

			if ((groundCheck && Grids().getEGroundThreat(w) != 0.0) || (airCheck && Grids().getEAirThreat(w) != 0.0))
				return false;
		}
	}
	return true;
}

bool UtilManager::isMobile(WalkPosition start, WalkPosition end, UnitType unitType)
{
	int walkWidth = (int)ceil(unitType.width() / 8.0) + 1;
	int walkHeight = (int)ceil(unitType.height() / 8.0) + 1;
	int halfW = walkWidth / 2;
	int halfH = walkHeight / 2;

	// If WalkPosition shared with WalkPositions under unit, ignore it
	auto const overlapsUnit = [&](const int x, const int y) {
		if (x >= start.x && x < start.x + walkWidth && y >= start.y && y < start.y + walkHeight)
			return true;
		return false;
	};

	for (int x = end.x - halfW; x < end.x + halfW; x++) {
		for (int y = end.y - halfH; y < end.y + halfH; y++) {
			WalkPosition w(x, y);
			if (!w.isValid())
				return false;
			if (!unitType.isFlyer() && overlapsUnit(x, y) && Grids().getMobility(w) > 0)
				continue;
			if (!unitType.isFlyer() && (Grids().getMobility(w) <= 0 || Grids().getCollision(w) > 0))
				return false;
		}
	}
	return true;
}

bool UtilManager::unitInRange(UnitInfo& unit)
{
	if (!unit.hasTarget())
		return false;

	double widths = unit.getTarget().getType().width() + unit.getType().width();
	double allyRange = (widths / 2) + (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());

	// HACK: Reavers have a weird ground distance range, try to reduce the amount of times Reavers try to engage
	if (unit.getType() == UnitTypes::Protoss_Reaver)
		allyRange -= 96.0;

	if (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= allyRange)
		return true;
	return false;
}

bool UtilManager::proactivePullWorker(Unit unit)
{
	if (Broodwar->self()->getRace() == Races::Protoss) {
		int completedDefenders = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot);
		int visibleDefenders = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot);

		if (unit->getType() == UnitTypes::Protoss_Probe && unit->getShields() < 20)
			return false;

		if (BuildOrder().isHideTech() && completedDefenders == 1 && Units().getMyUnits().size() == 1)
			return true;

		if (BuildOrder().getCurrentBuild() == "PFFE") {
			if (Units().getEnemyCount(UnitTypes::Zerg_Zergling) >= 5) {
				if (Strategy().getEnemyBuild() == "Z5Pool" && Units().getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 2 && visibleDefenders >= 1)
					return true;
				if (Strategy().getEnemyBuild() == "Z9Pool" && Units().getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 5 && visibleDefenders >= 2)
					return true;
				if (!Terrain().getEnemyStartingPosition().isValid() && Strategy().getEnemyBuild() == "Unknown" && Units().getGlobalAllyGroundStrength() < 2.00 && completedDefenders < 1 && visibleDefenders > 0)
					return true;
				if (Units().getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 2 && visibleDefenders >= 1)
					return true;
			}
			else {
				if (Strategy().getEnemyBuild() == "Z5Pool" && Units().getGlobalAllyGroundStrength() < 1.00 && completedDefenders < 2 && visibleDefenders >= 2)
					return true;
				if (!Terrain().getEnemyStartingPosition().isValid() && Strategy().getEnemyBuild() == "Unknown" && Units().getGlobalAllyGroundStrength() < 2.00 && completedDefenders < 1 && visibleDefenders > 0)
					return true;
			}
		}
		else if (BuildOrder().getCurrentBuild() == "P2GateExpand") {
			if (Strategy().getEnemyBuild() == "Z5Pool" && Units().getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 2)
				return true;
			if (Strategy().getEnemyBuild() == "Z9Pool" && Units().getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 3)
				return true;
		}
	}
	else if (Broodwar->self()->getRace() == Races::Terran && BuildOrder().isWallNat()) {
		if (Strategy().enemyRush() && BuildOrder().isFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) < 1)
			return true;
	}

	if (Strategy().enemyProxy() && Strategy().getEnemyBuild() != "P2Gate" && Units().getImmThreat() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense())
		return true;

	return false;
}

bool UtilManager::reactivePullWorker(Unit unit)
{
	if (Units().getEnemyCount(UnitTypes::Terran_Vulture) > 0)
		return false;

	if (Broodwar->self()->getRace() == Races::Protoss) {
		if (unit->getShields() < 8)
			return false;
	}

	const BWEB::Station * station = mapBWEB.getClosestStation(unit->getTilePosition());
	if (station && station->ResourceCentroid().getDistance(unit->getPosition()) < 160.0) {
		if (Terrain().isInAllyTerritory(unit->getTilePosition()) && Grids().getEGroundThreat(getWalkPosition(unit)) > 0.0 && Broodwar->getFrameCount() < 10000)
			return true;
	}

	// If we have no combat units and there is a threat
	if (Units().getImmThreat() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense()) {
		if (Broodwar->self()->getRace() == Races::Protoss) {
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) == 0)
				return true;
		}
		else if (Broodwar->self()->getRace() == Races::Zerg) {
			if (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Zergling) == 0)
				return true;
		}
		else if (Broodwar->self()->getRace() == Races::Terran) {
			if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture) == 0)
				return true;
		}
	}
	return false;
}

bool UtilManager::pullRepairWorker(Unit unit)
{
	if (Broodwar->self()->getRace() == Races::Terran) {
		int mechUnits = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Goliath)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Wraith)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Battlecruiser)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Valkyrie);

		if ((mechUnits > 0 && Units().getRepairWorkers() < Units().getSupply() / 30)
			|| (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) > 0 && BuildOrder().isFastExpand() && Units().getRepairWorkers() < 2))
			return true;
	}
	return false;
}

double UtilManager::groundDPS(UnitInfo& unit)
{
	double splash, damage, range, cooldown;
	splash = splashModifier(unit);
	damage = unit.getGroundDamage();
	range = log(unit.getGroundRange());
	cooldown = gWeaponCooldown(unit);
	if (damage <= 0)
		return 0.0;
	return splash * damage * range / cooldown;
}

double UtilManager::airDPS(UnitInfo& unit)
{
	double splash, damage, range, cooldown;
	splash = splashModifier(unit);
	damage = unit.getAirDamage();
	range = log(unit.getAirRange());
	cooldown = aWeaponCooldown(unit);
	if (damage <= 0)
		return 0.0;
	return splash * damage * range / cooldown;
}

double UtilManager::gWeaponCooldown(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Terran_Bunker) return 15.0;
	else if (unit.getType() == UnitTypes::Protoss_Reaver) return 60.0;
	else if (unit.getType() == UnitTypes::Protoss_High_Templar) return 224.0;
	else if (unit.getType() == UnitTypes::Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Adrenal_Glands)) return 6.0;
	else if (unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->hasResearched(TechTypes::Stim_Packs)) return 7.5;
	return unit.getType().groundWeapon().damageCooldown();
}

double UtilManager::aWeaponCooldown(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Terran_Bunker) return 15.0;
	else if (unit.getType() == UnitTypes::Protoss_High_Templar) return 224.0;
	else if (unit.getType() == UnitTypes::Zerg_Scourge) return 110.0;
	else if (unit.getType() == UnitTypes::Zerg_Infested_Terran) return 500.0;
	else if (unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->hasResearched(TechTypes::Stim_Packs)) return 7.5;
	return unit.getType().airWeapon().damageCooldown();
}

double UtilManager::splashModifier(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Protoss_Archon || unit.getType() == UnitTypes::Terran_Firebat) return 1.25;
	if (unit.getType() == UnitTypes::Protoss_Reaver) return 1.25;
	if (unit.getType() == UnitTypes::Protoss_High_Templar) return 6.00;
	if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode) return 2.50;
	if (unit.getType() == UnitTypes::Terran_Valkyrie || unit.getType() == UnitTypes::Zerg_Mutalisk) return 1.50;
	if (unit.getType() == UnitTypes::Zerg_Lurker) return 2.00;
	return 1.00;
}

double UtilManager::effectiveness(UnitInfo& unit)
{
	double effectiveness = 1.0;
	//double aLarge = double(Units().getAllySizes()[UnitSizeTypes::Large]);
	//double aMedium = double(Units().getAllySizes()[UnitSizeTypes::Medium]);
	//double aSmall = double(Units().getAllySizes()[UnitSizeTypes::Small]);

	//double eLarge = double(Units().getEnemySizes()[UnitSizeTypes::Large]);
	//double eMedium = double(Units().getEnemySizes()[UnitSizeTypes::Medium]);
	//double eSmall = double(Units().getEnemySizes()[UnitSizeTypes::Small]);

	//if (unit.getPlayer()->isEnemy(Broodwar->self())) {
	//	if (unit.getType().groundWeapon().damageType() == DamageTypes::Explosive)
	//		effectiveness = ((aLarge*1.0) + (aMedium*0.75) + (aSmall*0.5)) / max(1.0, aLarge + aMedium + aSmall);
	//	else if (unit.getType().groundWeapon().damageType() == DamageTypes::Concussive)
	//		effectiveness = ((aLarge*0.25) + (aMedium*0.5) + (aSmall*1.0)) / max(1.0, aLarge + aMedium + aSmall);
	//}

	//else {
	//	if (unit.getType().groundWeapon().damageType() == DamageTypes::Explosive)
	//		effectiveness = ((eLarge*1.0) + (eMedium*0.75) + (eSmall*0.5)) / max(1.0, eLarge + eMedium + eSmall);
	//	else if (unit.getType().groundWeapon().damageType() == DamageTypes::Concussive)
	//		effectiveness = ((eLarge*0.25) + (eMedium*0.5) + (eSmall*1.0)) / max(1.0, eLarge + eMedium + eSmall);
	//}
	return effectiveness;
}

double UtilManager::survivability(UnitInfo& unit)
{
	double speed, armor, health;
	speed = (unit.getType().isBuilding()) ? 0.5 : max(1.0, log(unit.getSpeed()));
	armor = 2.0 + double(unit.getType().armor() + unit.getPlayer()->getUpgradeLevel(unit.getType().armorUpgrade()));
	health = log(((double)unit.getType().maxHitPoints() + (double)unit.getType().maxShields()) / 20.0);
	return speed * armor * health;
}

double UtilManager::groundRange(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge)) return 192.0;
	if ((unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines))) return 160.0;
	if (unit.getType() == UnitTypes::Protoss_High_Templar) return 288.0;
	if (unit.getType() == UnitTypes::Protoss_Reaver) return 256.0;
	if (unit.getType() == UnitTypes::Terran_Bunker) {
		if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) return 192.0;
		return 160.0;
	}
	return double(unit.getType().groundWeapon().maxRange());
}

double UtilManager::airRange(UnitInfo& unit)
{
	if (unit.getType() == UnitTypes::Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge)) return 192.0;
	if ((unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines))) return 160.0;
	if (unit.getType() == UnitTypes::Terran_Goliath && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Charon_Boosters)) return 256.0;
	if (unit.getType() == UnitTypes::Protoss_High_Templar) return 288.0;
	if (unit.getType() == UnitTypes::Terran_Bunker) {
		if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) return 192.0;
		return 160.0;
	}
	return double(unit.getType().airWeapon().maxRange());
}

double UtilManager::groundDamage(UnitInfo& unit)
{
	// TODO Check Reaver upgrade type functional here or if needed hardcoding
	int upLevel = unit.getPlayer()->getUpgradeLevel(unit.getType().groundWeapon().upgradeType());
	if (unit.getType() == UnitTypes::Protoss_Reaver) {
		if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Scarab_Damage)) return 125.00;
		else return 100.00;
	}
	if (unit.getType() == UnitTypes::Terran_Bunker) return 24.0 + (4.0 * upLevel);
	if (unit.getType() == UnitTypes::Terran_Firebat || unit.getType() == UnitTypes::Protoss_Zealot) return 16.0 + (2.0 * upLevel);
	if (unit.getType() == UnitTypes::Protoss_High_Templar) return 112.0;
	return unit.getType().groundWeapon().damageAmount() + (unit.getType().groundWeapon().damageBonus() * upLevel);
}

double UtilManager::airDamage(UnitInfo& unit)
{
	int upLevel = unit.getPlayer()->getUpgradeLevel(unit.getType().airWeapon().upgradeType());
	if (unit.getType() == UnitTypes::Terran_Bunker)	return 24.0 + (4.0 * upLevel);
	if (unit.getType() == UnitTypes::Protoss_Scout)	return 28.0 + (2.0 * upLevel);
	if (unit.getType() == UnitTypes::Terran_Valkyrie) return 48.0 + (8.0 * upLevel);
	if (unit.getType() == UnitTypes::Protoss_High_Templar) return 112.0;
	return unit.getType().airWeapon().damageAmount() + (unit.getType().airWeapon().damageBonus() * upLevel);
}

double UtilManager::speed(UnitInfo& unit)
{
	double speed = unit.getType().topSpeed();

	if ((unit.getType() == UnitTypes::Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments)) || (unit.getType() == UnitTypes::Zerg_Ultralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Anabolic_Synthesis)) || (unit.getType() == UnitTypes::Protoss_Shuttle && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Drive)) || (unit.getType() == UnitTypes::Protoss_Observer && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Boosters)) || (unit.getType() == UnitTypes::Protoss_Zealot && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements)) || (unit.getType() == UnitTypes::Terran_Vulture && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)))
		return speed * 1.5;
	if (unit.getType() == UnitTypes::Zerg_Overlord && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace)) return speed * 4.01;
	if (unit.getType() == UnitTypes::Protoss_Scout && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments)) return speed * 1.33;
	if (unit.getType().isBuilding()) return 0.0;
	if (unit.isBurrowed()) return 0.0;
	return speed;
}

int UtilManager::getMinStopFrame(UnitType unitType)
{
	if (unitType == UnitTypes::Protoss_Dragoon)	return 9;
	else if (unitType == UnitTypes::Zerg_Devourer) return 11;
	return 0;
}

const BWEM::ChokePoint * UtilManager::getClosestChokepoint(Position here)
{
	double distBest = DBL_MAX;
	const BWEM::ChokePoint * closest = nullptr;
	for (auto &area : mapBWEM.Areas()) {
		for (auto &choke : area.ChokePoints()) {
			double dist = Position(choke->Center()).getDistance(here);
			if (dist < distBest) {
				distBest = dist;
				closest = choke;
			}
		}
	}
	return closest;
}

UnitInfo * UtilManager::getClosestUnit(Position here, Player p, UnitType t) {
	double distBest = DBL_MAX;
	UnitInfo* best = nullptr;
	auto &units = (p == Broodwar->self()) ? Units().getMyUnits() : Units().getEnemyUnits();

	for (auto &u : units) {
		UnitInfo &unit = u.second;

		if (!unit.unit() || (t != UnitTypes::None && unit.getType() != t))
			continue;

		double dist = here.getDistance(unit.getPosition());
		if (dist < distBest) {
			best = &unit;
			distBest = dist;
		}
	}
	return best;
}

UnitInfo * UtilManager::getClosestBuilder(Position here)
{
	double distBest = DBL_MAX;
	UnitInfo* best = nullptr;
	auto &units = Units().getMyUnits();

	for (auto &u : units) {
		UnitInfo &unit = u.second;

		if (!unit.unit() || !unit.getType().isWorker())
			continue;

		double dist = here.getDistance(unit.getPosition());
		if (dist < distBest) {
			best = &unit;
			distBest = dist;
		}
	}
	return best;
}

int UtilManager::chokeWidth(const BWEM::ChokePoint * choke)
{
	if (!choke)
		return 0;
	return int(choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2))) * 8;
}