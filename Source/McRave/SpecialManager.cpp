#include "McRave.h"

void CommandManager::updateArbiter(UnitInfo& arbiter)
{
	double scoreBest = 0.0;
	Position posBest(Grids().getArmyCenter());
	WalkPosition start(arbiter.getWalkPosition());

	for (int x = start.x - 20; x <= start.x + 20; x++) {
		for (int y = start.y - 20; y <= start.y + 20; y++) {
			WalkPosition w(x, y);
			Position p(w);
			if (!w.isValid()
				|| !Util().isSafe(w, UnitTypes::Protoss_Arbiter, false, true)
				|| p.getDistance(arbiter.getPosition()) <= 64
				|| Commands().isInDanger(p)
				|| Commands().overlapsCommands(arbiter.getType(), p, 64))
				continue;

			double score = (1.0 + (double)Grids().getAGroundCluster(w) + (double)Grids().getAGroundCluster(w)) / p.getDistance(Grids().getArmyCenter());
			if (score > scoreBest) {
				scoreBest = score;
				posBest = p;
			}
		}
	}

	// Move and update Grids	
	if (posBest.isValid()) {
		arbiter.setEngagePosition(posBest);
		arbiter.unit()->move(posBest);
		Commands().addCommand(posBest, arbiter.getType());
	}
	else {
		UnitInfo* closest = Util().getClosestAllyUnit(arbiter);
		if (closest && closest->getPosition().isValid())
			arbiter.unit()->move(closest->getPosition());
	}

	// If there's a stasis target, cast stasis on it		
	if (arbiter.hasTarget() && arbiter.getTarget().unit()->exists() && arbiter.unit()->getEnergy() >= TechTypes::Stasis_Field.energyCost() && !Commands().overlapsCommands(TechTypes::Psionic_Storm, arbiter.getTarget().getPosition(), 96)) {
		if ((Grids().getEGroundCluster(arbiter.getTarget().getWalkPosition()) + Grids().getEAirCluster(arbiter.getTarget().getWalkPosition())) > STASIS_LIMIT) {
			arbiter.unit()->useTech(TechTypes::Stasis_Field, arbiter.getTarget().unit());
			Commands().addCommand(arbiter.getTarget().getPosition(), TechTypes::Stasis_Field);
			Broodwar->drawTextMap(arbiter.getPosition(), "STASIS");
		}
	}
}

void CommandManager::updateDetector(UnitInfo& detector)
{
	UnitType building = Broodwar->self()->getRace().getResourceDepot();
	Position posBest(Grids().getArmyCenter());
	
	if (detector.getType() == UnitTypes::Zerg_Overlord)
		return;

	if (detector.getType() == UnitTypes::Spell_Scanner_Sweep) {
		Commands().addCommand(detector.getPosition(), UnitTypes::Spell_Scanner_Sweep);
		return;
	}

	// Check if any expansions need detection on them
	if (Broodwar->self()->completedUnitCount(detector.getType()) > 1 && BuildOrder().buildCount(building) > Broodwar->self()->visibleUnitCount(building) && !Commands().overlapsCommands(detector.getType(), (Position)Buildings().getCurrentExpansion(), 320)) {
		posBest = Position(Buildings().getCurrentExpansion());
	}

	// Check if there is a unit that needs revealing
	else {
		Position destination(posBest);
		double scoreBest = 0.0;
		WalkPosition start = detector.getWalkPosition();

		if (detector.hasTarget() && detector.getTarget().getPosition().isValid())
			destination = detector.getTarget().getPosition();

		for (int x = start.x - 20; x <= start.x + 20; x++) {
			for (int y = start.y - 20; y <= start.y + 20; y++) {

				WalkPosition w(x, y);
				Position p(w);

				if (!w.isValid()
					|| Commands().overlapsCommands(detector.getType(), p, 64)
					|| p.getDistance(detector.getPosition()) <= 64
					|| (detector.unit()->isCloaked() && Commands().overlapsEnemyDetection(p) && Grids().getEAirThreat(w) > 0.0)
					|| (!detector.unit()->isCloaked() && Grids().getEAirThreat(w) > 0.0))
					continue;

				double score = ((double)Grids().getAGroundCluster(w) + (double)Grids().getAGroundCluster(w)) / p.getDistance(destination);
				if (score > scoreBest) {
					scoreBest = score;
					posBest = p;
				}
			}
		}
	}

	if (detector.getType() == UnitTypes::Terran_Science_Vessel && detector.unit()->getEnergy() >= TechTypes::Defensive_Matrix) {
		UnitInfo* ally = Util().getClosestAllyUnit(detector, Filter::IsUnderAttack);
		if (ally && ally->getPosition().getDistance(detector.getPosition()) < 640) {
			detector.unit()->useTech(TechTypes::Defensive_Matrix, ally->unit());
			return;
		}
	}

	if (posBest.isValid()) {
		detector.setEngagePosition(posBest);
		detector.unit()->move(posBest);
		Commands().addCommand(posBest, detector.getType());
	}
}

bool CommandManager::shouldUseSpecial(UnitInfo& unit)
{
	Position p(unit.getEngagePosition());

	// Detectors
	if (unit.getType().isDetector()) {
		updateDetector(unit);
		return true;
	}

	// Arbiters
	else if (unit.getType() == UnitTypes::Protoss_Arbiter) {
		updateArbiter(unit);
		return true;
	}

	// Cloak or Uncloak Wraiths	
	else if (unit.getType() == UnitTypes::Terran_Wraith) {
		if (unit.getHealth() >= 120 && !unit.unit()->isCloaked() && unit.unit()->getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Commands().overlapsEnemyDetection(p))
			unit.unit()->useTech(TechTypes::Cloaking_Field);
		else if (unit.getHealth() <= 90 && unit.unit()->isCloaked())
			unit.unit()->useTech(TechTypes::Cloaking_Field);
	}

	// Yamato Gun
	else if (unit.getType() == UnitTypes::Terran_Battlecruiser) {
		if ((unit.unit()->getOrder() == Orders::FireYamatoGun || (Broodwar->self()->hasResearched(TechTypes::Yamato_Gun) && unit.unit()->getEnergy() >= TechTypes::Yamato_Gun.energyCost()) && unit.getTarget().unit()->getHitPoints() >= 80) && unit.hasTarget() && unit.getTarget().unit()->exists()) {
			if ((unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()))
				unit.unit()->useTech(TechTypes::Yamato_Gun, unit.getTarget().unit());

			addCommand(unit.getTarget().getPosition(), TechTypes::Yamato_Gun);
			return true;
		}
	}

	// Nukes
	else if (unit.getType() == UnitTypes::Terran_Ghost) {
		if (!unit.unit()->isCloaked() && unit.unit()->getEnergy() >= 50 && unit.getPosition().getDistance(unit.getEngagePosition()) < 320 && !Commands().overlapsEnemyDetection(p))
			unit.unit()->useTech(TechTypes::Personnel_Cloaking);

		if (Buildings().getNukesAvailable() > 0 && unit.hasTarget() && unit.getTarget().getWalkPosition().isValid() && unit.unit()->isCloaked() && Grids().getEAirCluster(unit.getTarget().getWalkPosition()) + Grids().getEGroundCluster(unit.getTarget().getWalkPosition()) > 5.0 && unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 320 && unit.getPosition().getDistance(unit.getTarget().getPosition()) > 200) {
			if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Use_Tech_Unit || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()) {
				unit.unit()->useTech(TechTypes::Nuclear_Strike, unit.getTarget().unit());
				addCommand(unit.getTarget().getPosition(), TechTypes::Nuclear_Strike);
				return true;
			}
		}
		if (unit.unit()->getOrder() == Orders::NukePaint || unit.unit()->getOrder() == Orders::NukeTrack || unit.unit()->getOrder() == Orders::CastNuclearStrike) {
			addCommand(unit.unit()->getOrderTargetPosition(), TechTypes::Nuclear_Strike);
			return true;
		}
	}

	// Siege or Unsiege Tanks
	else if (unit.getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode) {
		if (unit.hasTarget() && unit.unit()->getDistance(unit.getTarget().getPosition()) <= 400 && unit.unit()->getDistance(unit.getTarget().getPosition()) > 128)
			unit.unit()->siege();
		//if (unit.getGlobalStrategy() == 0 && unit.getPosition().getDistance(Terrain().getDefendPosition()) < 320)
		//	unit.unit()->siege();
	}
	else if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode) {
		if (unit.getGlobalStrategy() == 1 && (!unit.hasTarget() || unit.unit()->getDistance(unit.getTarget().getPosition()) > 400 || unit.unit()->getDistance(unit.getTarget().getPosition()) < 128))
			unit.unit()->unsiege();
		if (unit.getGlobalStrategy() == 0 && unit.getPosition().getDistance(Terrain().getDefendPosition()) >= 320)
			unit.unit()->unsiege();
	}

	// Stim Marine and Firebat
	else if (Broodwar->self()->hasResearched(TechTypes::Stim_Packs) && (unit.getType() == UnitTypes::Terran_Marine || unit.getType() == UnitTypes::Terran_Firebat) && !unit.unit()->isStimmed() && unit.hasTarget() && unit.getTarget().getPosition().isValid() && unit.unit()->getDistance(unit.getTarget().getPosition()) <= unit.getGroundRange())
		unit.unit()->useTech(TechTypes::Stim_Packs);

	// Bunker Loading/Unloading
	else if (unit.getType() == UnitTypes::Terran_Marine && unit.getGlobalStrategy() == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) > 0) {
		BuildingInfo* bunker = Util().getClosestAllyBuilding(unit, Filter::GetType == UnitTypes::Terran_Bunker && Filter::SpaceRemaining > 0);
		if (bunker && bunker->unit() && unit.hasTarget()) {
			if (unit.getTarget().unit()->exists() && unit.getTarget().getPosition().getDistance(unit.getPosition()) <= 320) {
				unit.unit()->rightClick(bunker->unit());
				return true;
			}
			if (unit.unit()->isLoaded() && unit.getTarget().getPosition().getDistance(unit.getPosition()) > 320)
				bunker->unit()->unloadAll();
		}
	}

	// SCV Repair
	else if (unit.getType() == UnitTypes::Terran_SCV) {
		UnitInfo* mech = Util().getClosestAllyUnit(unit, Filter::IsMechanical && Filter::HP_Percent < 100);
		if (!Strategy().enemyRush() && mech && mech->unit() && unit.getPosition().getDistance(mech->getPosition()) <= 320 && Grids().getMobility(mech->getWalkPosition()) > 0) {
			if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != mech->unit())
				unit.unit()->repair(mech->unit());
			return true;
		}

		BuildingInfo* building = Util().getClosestAllyBuilding(unit, Filter::GetPlayer == Broodwar->self() && Filter::IsCompleted && Filter::HP_Percent < 100);
		if (building && building->unit() && (!Strategy().enemyRush() || building->getType() == UnitTypes::Terran_Bunker)) {
			if (!unit.unit()->isRepairing() || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Repair || unit.unit()->getLastCommand().getTarget() != building->unit())
				unit.unit()->repair(building->unit());
			return true;
		}
	}

	// Spider Mine Placement
	else if (unit.getType() == UnitTypes::Terran_Vulture && Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && unit.unit()->getSpiderMineCount() > 0 && unit.getPosition().getDistance(unit.getSimPosition()) <= 400 && Broodwar->getUnitsInRadius(unit.getPosition(), 4, Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine).size() <= 0) {
		if (unit.unit()->getLastCommand().getTechType() != TechTypes::Spider_Mines || unit.unit()->getLastCommand().getTargetPosition().getDistance(unit.getPosition()) > 8)
			unit.unit()->useTech(TechTypes::Spider_Mines, unit.getPosition());
		return true;
	}

	// Scarab Training
	else if (unit.getType() == UnitTypes::Protoss_Reaver && !unit.unit()->isLoaded() && unit.unit()->getScarabCount() < 5 + (5 * Broodwar->self()->getUpgradeLevel(UpgradeTypes::Reaver_Capacity))) {
		unit.unit()->train(UnitTypes::Protoss_Scarab);
		unit.setLastAttackFrame(Broodwar->getFrameCount());	/// Use this to fudge whether a Reaver has actually shot when using shuttles due to cooldown reset
		return false;
	}

	// Interceptor Training
	else if (unit.getType() == UnitTypes::Protoss_Carrier && unit.unit()->getInterceptorCount() < 4 + (4 * Broodwar->self()->getUpgradeLevel(UpgradeTypes::Carrier_Capacity))) {
		unit.unit()->train(UnitTypes::Protoss_Interceptor);
		return false;
	}
	
	// Merge Archons
	else if (unit.getType() == UnitTypes::Protoss_High_Templar && (Strategy().getUnitScore(UnitTypes::Protoss_Archon) > Strategy().getUnitScore(UnitTypes::Protoss_High_Templar) || (unit.unit()->getEnergy() < 75 && Grids().getEGroundThreat(unit.getWalkPosition()) > 0.0))) {
		UnitInfo* templar = Util().getClosestAllyUnit(unit, Filter::GetType == UnitTypes::Protoss_High_Templar && Filter::Energy < TechTypes::Psionic_Storm.energyCost());
		if (templar && templar->unit() && templar->unit()->exists() && (Strategy().getUnitScore(UnitTypes::Protoss_Archon) > Strategy().getUnitScore(UnitTypes::Protoss_High_Templar) || Grids().getEGroundThreat(templar->getWalkPosition()) > 0.0)) {
			if (templar->unit()->getLastCommand().getTechType() != TechTypes::Archon_Warp && unit.unit()->getLastCommand().getTechType() != TechTypes::Archon_Warp) {
				unit.unit()->useTech(TechTypes::Archon_Warp, templar->unit());
				return true;
			}
		}
	}

	// Morph devourers/guardians
	else if (unit.getType() == UnitTypes::Hero_Kukulza_Mutalisk) {
		if (Strategy().getUnitScore(UnitTypes::Zerg_Devourer) > Strategy().getUnitScore(UnitTypes::Zerg_Mutalisk)) {
			unit.unit()->morph(UnitTypes::Zerg_Devourer);
			return true;
		}
		else if (Strategy().getUnitScore(UnitTypes::Zerg_Guardian) > Strategy().getUnitScore(UnitTypes::Zerg_Mutalisk)) {
			unit.unit()->morph(UnitTypes::Zerg_Guardian);
			return true;
		}
	}

	// Disruption Web
	else if (unit.getType() == UnitTypes::Protoss_Corsair && unit.unit()->getEnergy() >= TechTypes::Disruption_Web.energyCost() && Broodwar->self()->hasResearched(TechTypes::Disruption_Web)) {
		double distBest = DBL_MAX;
		Position posBest = Positions::Invalid;
		for (auto&e : Units().getEnemyUnits()) {
			UnitInfo& enemy = e.second;
			double dist = enemy.getPosition().getDistance(unit.getPosition());
			if (dist < distBest && !overlapsCommands(TechTypes::Disruption_Web, enemy.getPosition(), 96) && enemy.unit()->isAttacking() && enemy.getSpeed() <= UnitTypes::Protoss_Reaver.topSpeed())
				distBest = dist, posBest = enemy.getPosition();
		}
		if (posBest.isValid()) {
			if (unit.getPosition().getDistance(posBest) <= 320)
				addCommand(posBest, TechTypes::Disruption_Web);
			unit.unit()->useTech(TechTypes::Disruption_Web, posBest);
			return true;
		}
	}

	// Shield Battery Use
	else if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shield_Battery) > 0 && unit.getType().maxShields() > 0 && (unit.unit()->getShields() < 10 || (unit.unit()->getShields() < unit.getType().maxShields() && unit.unit()->getLastCommand().getType() == UnitCommandTypes::Right_Click_Unit && unit.unit()->getLastCommand().getTarget() && unit.unit()->getLastCommand().getTarget()->getEnergy() >= 10))) {
		BuildingInfo* battery = Util().getClosestAllyBuilding(unit, Filter::GetType == UnitTypes::Protoss_Shield_Battery && Filter::Energy >= 10 && Filter::IsCompleted);
		if (battery && ((unit.getType().isFlyer() && (!unit.hasTarget() || (unit.getTarget().getPosition().getDistance(unit.getPosition()) >= 320))) || unit.unit()->getDistance(battery->getPosition()) < 320)) {
			if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Right_Click_Unit || unit.unit()->getLastCommand().getTarget() != battery->unit())
				unit.unit()->rightClick(battery->unit());
			return true;
		}
	}
	return false;
}