#include "McRave.h"

namespace McRave
{
	void CommandManager::onFrame()
	{
		Display().startClock();
		updateEnemyCommands();
		updateGoals();
		updateAlliedUnits();
		Display().performanceTest(__FUNCTION__);
		return;
	}

	void CommandManager::updateEnemyCommands()
	{
		// Clear cache
		enemyCommands.clear();

		// Store bullets as enemy units if they can affect us too
		for (auto &b : Broodwar->getBullets()) {
			if (b && b->exists()) {
				if (b->getType() == BulletTypes::Psionic_Storm)
					addCommand(b->getPosition(), TechTypes::Psionic_Storm, true);
				
				if (b->getType() == BulletTypes::EMP_Missile)
					addCommand(b->getTargetPosition(), TechTypes::EMP_Shockwave, true);
			}
		}

		// Store enemy detection and assume casting orders
		for (auto &u : Units().getEnemyUnits()) {
			UnitInfo& unit = u.second;
			
			if (!unit.unit() || (unit.unit()->exists() && (unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())))
				continue;

			if (unit.getType().isDetector())
				Commands().addCommand(unit.getPosition(), unit.getType(), true);

			if (unit.unit()->exists()) {
				Order enemyOrder = unit.unit()->getOrder();
				Position enemyTarget = unit.unit()->getOrderTargetPosition();

				if (enemyOrder == Orders::CastEMPShockwave)
					Commands().addCommand(enemyTarget, TechTypes::EMP_Shockwave, true);
				if (enemyOrder == Orders::CastPsionicStorm)
					Commands().addCommand(enemyTarget, TechTypes::Psionic_Storm, true);
			}
		}

		// Store nuke dots
		for (auto &dot : Broodwar->getNukeDots())
			Commands().addCommand(dot, TechTypes::Nuclear_Strike, true);
	}

	void CommandManager::updateGoals()
	{
		// Goal: Defend my expansions
		for (auto &s : Stations().getMyStations()) {
			Station station = s.second;
			BuildingInfo& base = Buildings().getMyBuildings().at(s.first);

			if (station.BWEMBase()->Location() != mapBWEB.getNaturalTile() && station.BWEMBase()->Location() != mapBWEB.getMainTile() && Grids().getDefense(station.BWEMBase()->Location()) == 0) {

				myGoals[station.ResourceCentroid()] = 0.1;
				UnitInfo* rangedUnit = Util().getClosestAllyUnit(base, Filter::GetType == UnitTypes::Protoss_Dragoon || Filter::GetType == UnitTypes::Terran_Siege_Tank_Tank_Mode);

				if (rangedUnit)
					rangedUnit->setDestination(station.ResourceCentroid());
			}
			else if (myGoals.find(station.ResourceCentroid()) != myGoals.end())
				myGoals.erase(station.ResourceCentroid());
		}

		// Goal: Deny enemy expansions
		if (Players().getPlayers().size() == 1) {
			if (Broodwar->self()->getRace() == Races::Protoss && Broodwar->enemy()->getRace() == Races::Terran && Stations().getMyStations().size() >= 3 && Stations().getMyStations().size() > Stations().getEnemyStations().size() && Terrain().getEnemyExpand().isValid()) {
				map<double, UnitInfo*> unitByDist;

				for (auto &u : Units().getMyUnits()) {
					UnitInfo &unit = u.second;

					if (unit.unit() && unit.getType() == UnitTypes::Protoss_Dragoon)
						unitByDist[unit.getPosition().getDistance(Position(Terrain().getEnemyExpand()))] = &u.second;
				}

				int i = 0;
				for (auto &u : unitByDist) {
					if (!u.second->getDestination().isValid()) {
						u.second->setDestination(Position(Terrain().getEnemyExpand()));
						myGoals[Position(Terrain().getEnemyExpand())] += u.second->getVisibleGroundStrength();
						i++;
					}
					if (i == 4)
						break;
				}
			}
		}

		// Goal: Runby static defenses

		// Goal: Secure our own future expansion position


	}

	void CommandManager::updateAlliedUnits()
	{
		myCommands.clear();
		for (auto &u : Units().getMyUnits()) {
			UnitInfo &unit = u.second;

			bool attackCooldown = (Broodwar->getFrameCount() - unit.getLastAttackFrame() <= unit.getMinStopFrame() - Broodwar->getLatencyFrames());

			if (!unit.unit() || !unit.unit()->exists()																							// Prevent crashes	
				|| unit.getType() == UnitTypes::Protoss_Shuttle || unit.getType() == UnitTypes::Terran_Dropship 								// Transports have their own commands	
				|| unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted()		// If the unit is locked down, maelstrommed, stassised, or not completed	
				|| (unit.getType() != UnitTypes::Protoss_Carrier && attackCooldown))															// If the unit is not ready to perform an action after an attack (certain units have minimum frames after an attack before they can receive a new command)
				continue;
			
			// Temp variables to use
			if (unit.hasTarget()) {
				widths = unit.getTarget().getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
				allyRange = widths + (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
				enemyRange = widths + (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());
			}

			// Units targeted by splash need to move away from the army
			if (Units().getSplashTargets().find(unit.unit()) != Units().getSplashTargets().end()) {
				if (unit.hasTarget() && unit.unit()->getGroundWeaponCooldown() <= 0 && unit.hasTarget() && unit.getTarget().unit()->exists())
					attack(unit);
				else
					approach(unit);
			}
			
			// If this unit should use a special ability that requires a command
			else if (shouldUseSpecial(unit))
				continue;

			// If this unit should engage
			else if (unit.shouldEngage()) {
				if (shouldAttack(unit))
					attack(unit);
				else if (shouldApproach(unit))
					approach(unit);
				else if (shouldKite(unit))
					kite(unit);
			}

			// If this unit should retreat
			else if (unit.shouldRetreat()) {
				if (shouldDefend(unit))
					defend(unit);
				else
					kite(unit);
			}

			else
				move(unit);			
		}
		return;
	}

	bool CommandManager::shouldAttack(UnitInfo& unit)
	{
		if (unit.getType() == UnitTypes::Protoss_Carrier) {
			for (auto &i : unit.unit()->getInterceptors()) {
				if (!i || !i->isCompleted()) continue;
				UnitInfo interceptor = Units().getMyUnits()[i];
				if (Broodwar->getFrameCount() - interceptor.getLastAttackFrame() > 300)
					return true;				
			}
			return false;
		}

		if ((!unit.getTarget().getType().isFlyer() && unit.unit()->getGroundWeaponCooldown() <= 0)
			|| (unit.getTarget().getType().isFlyer() && unit.unit()->getAirWeaponCooldown() <= 0)
			|| unit.getType() == UnitTypes::Terran_Medic)
			return true;
		return false;
	}

	bool CommandManager::shouldKite(UnitInfo& unit)
	{
		if (unit.getType() == UnitTypes::Protoss_Carrier || unit.getType() == UnitTypes::Zerg_Mutalisk)
			return true;

		else if (unit.getType() == UnitTypes::Protoss_Corsair)
			return false;

		else if (unit.getType() == UnitTypes::Protoss_Zealot && Terrain().getWall() && Position(Terrain().getWall()->getDoor()).getDistance(unit.getPosition()) < 96.0)
			return false;

		if ((!unit.getTarget().getType().isFlyer() && unit.unit()->getGroundWeaponCooldown() <= 0)
			|| (unit.getTarget().getType().isFlyer() && unit.unit()->getAirWeaponCooldown() <= 0)
			|| (Terrain().isInAllyTerritory(unit.getTilePosition()) && allyRange <= 32)
			|| (unit.getTarget().getType().isBuilding()))
			return false;

		if (((unit.getTarget().isBurrowed() || unit.getTarget().unit()->isCloaked()) && !unit.getTarget().unit()->isDetected())		// Invisible Unit
			|| (unit.getType() == UnitTypes::Protoss_Reaver && unit.getTarget().getGroundRange() < unit.getGroundRange())			// Reavers always kite units with lower range
			|| (unit.getType() == UnitTypes::Terran_Vulture)																		// Vultures always kite
			|| (unit.getType() == UnitTypes::Zerg_Mutalisk)																			// Mutas always kite
			|| (unit.getType() == UnitTypes::Protoss_Carrier)
			|| (allyRange > 32.0 && unit.unit()->isUnderAttack() && allyRange >= enemyRange)										// Ranged unit under attack by unit with lower range
			|| ((enemyRange <= allyRange && unit.unit()->getDistance(unit.getTarget().getPosition()) <= allyRange - enemyRange)))	// Ranged unit fighting lower range unit and not at max range
			return true;
		return false;
	}

	bool CommandManager::shouldApproach(UnitInfo& unit)
	{
		if (unit.getType() == UnitTypes::Protoss_Carrier || unit.getType() == UnitTypes::Zerg_Mutalisk)
			return false;

		if ((!unit.getTarget().getType().isFlyer() && unit.unit()->getGroundWeaponCooldown() <= 0)
			|| (unit.getTarget().getType().isFlyer() && unit.unit()->getAirWeaponCooldown() <= 0)
			|| (!unit.getTarget().unit()->exists()))
			return false;

		if (unit.getGroundRange() < 64.0 && unit.getTarget().getType().isWorker())
			return true;

		if ((unit.getGroundRange() > 32 && unit.getGroundRange() < unit.getTarget().getGroundRange() && !unit.getTarget().getType().isBuilding() && unit.getSpeed() > unit.getTarget().getSpeed() && Grids().getMobility(WalkPosition(unit.getEngagePosition())) > 0)	// Approach slower units with higher range
			|| (unit.getType() != UnitTypes::Terran_Battlecruiser && unit.getType().isFlyer() && unit.getTarget().getType() != UnitTypes::Zerg_Scourge))																												// Small flying units approach other flying units except scourge
			return true;
		return false;
	}

	bool CommandManager::shouldDefend(UnitInfo& unit)
	{
		if ((!unit.hasTarget() || !Units().isThreatening(unit.getTarget())) && ((!unit.getType().isFlyer() && Grids().getEGroundThreat(unit.getWalkPosition()) == 0.0 && unit.getGlobalStrategy() == 0)
			|| (unit.getType().isFlyer() && Grids().getEAirThreat(unit.getWalkPosition()) == 0.0 && unit.getGlobalStrategy() == 0)))
			return true;
		return false;
	}

	void CommandManager::move(UnitInfo& unit)
	{
		// If it's a tank, make sure we're unsieged before moving - TODO: Check that target has velocity and > 512 or no velocity and < tank range
		if (unit.hasTarget() && unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.unit()->getDistance(unit.getTarget().getPosition()) > 512 && unit.getTarget().getSpeed() > 0.0)		
			unit.unit()->unsiege();		

		else if (unit.getDestination().isValid()) {
			Position bestPosition = Util().getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getDestination())));

			if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None)) {
				if (unit.unit()->getLastCommand().getTargetPosition() != Position(bestPosition) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
					unit.unit()->move(Position(bestPosition));
			}
		}

		// If unit has a transport, move to it or load into it
		else if (unit.getTransport() && unit.getTransport()->exists()) {
			if (!isLastCommand(unit, UnitCommandTypes::Right_Click_Unit, unit.getTransport()->getPosition()))
				unit.unit()->rightClick(unit.getTransport());
		}

		// If target doesn't exist, move towards it
		else if (unit.hasTarget() && unit.getTarget().getPosition().isValid() && Grids().getMobility(WalkPosition(unit.getEngagePosition())) > 0 && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < 400.0 || unit.getType().isFlyer())) {
			if (!isLastCommand(unit, UnitCommandTypes::Move, unit.getTarget().getPosition()))
				unit.unit()->move(unit.getTarget().getPosition());
			Broodwar->drawCircleMap(unit.getPosition(), 4, Colors::Blue);
		}

		else if (Terrain().getAttackPosition().isValid() && !unit.getType().isFlyer()) {
			if (!isLastCommand(unit, UnitCommandTypes::Move, Terrain().getAttackPosition()))
				unit.unit()->move(Terrain().getAttackPosition());
		}

		else if (unit.getType().isFlyer() && !unit.hasTarget() && unit.getPosition().getDistance(unit.getSimPosition()) < 640.0)
			kite(unit);

		// If no target and no enemy bases, move to a random base
		else if (unit.unit()->isIdle()) {
			int random = rand() % (Terrain().getAllBases().size());
			int i = 0;
			for (auto &base : Terrain().getAllBases()) {
				if (i == random) {
					unit.unit()->move(base->Center());
					return;
				}
				else i++;
			}
		}
	}

	void CommandManager::defend(UnitInfo& unit)
	{
		// Flyers defend at natural
		if (unit.getType().isFlyer())
			unit.unit()->move(mapBWEB.getNaturalPosition());

		else if (unit.getDestination().isValid()) {
			Position bestPosition = Util().getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getDestination())));

			if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None)) {
				if (unit.unit()->getLastCommand().getTargetPosition() != Position(bestPosition) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
					unit.unit()->move(Position(bestPosition));
			}
		}

		// Early on, defend mineral line
		else if (!Strategy().defendChoke()) {
			if (unit.getType() == UnitTypes::Protoss_Dragoon && Terrain().getBackMineralHoldPosition().isValid()) {
				if (!isLastCommand(unit, UnitCommandTypes::Move, Terrain().getBackMineralHoldPosition()))
					unit.unit()->move(Terrain().getBackMineralHoldPosition());
			}
			else {
				if (!isLastCommand(unit, UnitCommandTypes::Move, Terrain().getMineralHoldPosition()))
					unit.unit()->move(Terrain().getMineralHoldPosition());
			}
		}
		else {

			Position bestPosition;
			UnitType baseType = Broodwar->self()->getRace().getResourceDepot();

			if (BuildOrder().buildCount(baseType) >= 2)
				bestPosition = Util().getConcavePosition(unit, nullptr, Terrain().getDefendPosition());
			else
				bestPosition = Util().getConcavePosition(unit, nullptr, Terrain().getDefendPosition());

			if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None)) {
				if (unit.unit()->getLastCommand().getTargetPosition() != Position(bestPosition) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
					unit.unit()->move(Position(bestPosition));
				unit.setDestination(Position(bestPosition));
			}
		}
	}

	void CommandManager::kite(UnitInfo& unit)
	{
		// Workers use mineral fields to help with drilling
		if (unit.getType().isWorker() && Terrain().isInAllyTerritory(unit.getTilePosition())) {
			Unit mineral = unit.unit()->getClosestUnit(Filter::IsMineralField);
			if (mineral) {
				unit.unit()->gather(mineral);
				return;
			}
		}

		// Mechanical units run to SCVs to be repaired
		if (unit.getType().isMechanical() && unit.getPercentHealth() < 1.00 && unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) > 400) {
			UnitInfo* scv = Util().getClosestAllyUnit(unit, Filter::GetType == UnitTypes::Terran_SCV);
			if (scv && unit.getPosition().getDistance(scv->getPosition()) > 32) {
				unit.unit()->move(scv->getPosition());
				return;
			}
		}

		// If unit has a transport, don't run away, run to it TODO
		if (unit.getTransport() && unit.getTransport()->exists())
			return;

		WalkPosition start = unit.getWalkPosition();
		Position posBest = Positions::Invalid;
		double best = 0.0;
		int radius = 16;

		int walkWidth = (int)ceil(unit.getType().width() / 8.0);
		bool evenW = walkWidth % 2 == 0;

		int walkHeight = (int)ceil(unit.getType().height() / 8.0);
		bool evenH = walkHeight % 2 == 0;

		// Search a grid around the unit
		for (int x = start.x - radius; x <= start.x + radius + walkWidth; x++) {
			for (int y = start.y - radius; y <= start.y + radius + walkHeight; y++) {
				WalkPosition w(x, y);
				Position p(w);

				p = p + Position(4 * evenW, 4 * evenH);

				if (!w.isValid())
					continue;
				if (isInDanger(p) || Grids().getESplash(w) > 0)
					continue;

				if (!Util().isMobile(start, w, unit.getType()))
					continue;
				if (p.getDistance(unit.getPosition()) > radius * 8)
					continue;

				double distance;
				if (unit.hasTarget() && Terrain().isInAllyTerritory(unit.getTilePosition()))
					distance = 1.0 / (32.0 + p.getDistance(unit.getTarget().getPosition()));
				else
					distance = (unit.getType().isFlyer() || Terrain().isIslandMap()) ? pow(p.getDistance(mapBWEB.getMainPosition()), 2.0) : pow(Grids().getDistanceHome(w), 2.0); // Distance value	

				double mobility = unit.getType().isFlyer() ? 1.0 : double(Grids().getMobility(w));												// If unit is a flyer, ignore mobility
				double threat = unit.getType().isFlyer() ? max(0.1, Grids().getEAirThreat(w)) : max(0.1, Grids().getEGroundThreat(w));			// If unit is a flyer, use air threat
				double grouping = (unit.getType().isFlyer() && double(Grids().getAAirCluster(w)) > 1.0) ? 2.0 : 1.0;							// Flying units should try to cluster
				double score = grouping * mobility / (threat * distance);

				// If position is valid and better score than current, set as current best
				if (score > best) {
					posBest = p;
					best = score;
				}
			}
		}

		// If a valid position was found that isn't the starting position
		if (posBest.isValid() && posBest != unit.getPosition()) {
			Grids().updateAllyMovement(unit.unit(), (WalkPosition)posBest);
			unit.setDestination(posBest);

			//Broodwar->drawLineMap(unit.getPosition(), posBest, Colors::Blue);

			if (!isLastCommand(unit, UnitCommandTypes::Move, posBest))
				unit.unit()->move(posBest);
		}
	}

	void CommandManager::attack(UnitInfo& unit)
	{
		bool newOrder = Broodwar->getFrameCount() - unit.unit()->getLastCommandFrame() > Broodwar->getLatencyFrames();

		if (unit.getTarget().unit() && !unit.getTarget().unit()->exists())
			move(unit);
		
		// DT hold position vs spider mines
		else if (unit.getType() == UnitTypes::Protoss_Dark_Templar && unit.hasTarget() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getTarget().hasTarget() && unit.getTarget().getTarget().unit() == unit.unit() && !unit.getTarget().isBurrowed()) {
			if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Hold_Position)
				unit.unit()->holdPosition();
		}

		// If close enough, set the current area as a psi storm area so units move and storms don't overlap
		else if (unit.getType() == UnitTypes::Protoss_High_Templar) {
			if (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 400 && !Commands().overlapsCommands(TechTypes::Psionic_Storm, unit.getTarget().getPosition(), 96) && unit.unit()->getEnergy() >= 75 && (Grids().getEGroundCluster(unit.getTarget().getWalkPosition()) + Grids().getEAirCluster(unit.getTarget().getWalkPosition())) > STORM_LIMIT) {
				unit.unit()->useTech(TechTypes::Psionic_Storm, unit.getTarget().unit());

				if (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 360)
					addCommand(unit.getTarget().getPosition(), TechTypes::Psionic_Storm);
			}
			else if (unit.getPosition().getDistance(unit.getTarget().getPosition()) > 640.0) {
				move(unit);
			}
			else
				kite(unit);
		}

		else if (unit.getType() == UnitTypes::Terran_Medic) {
			if (!isLastCommand(unit, UnitCommandTypes::Attack_Move, unit.getTarget().getPosition()))
				unit.unit()->attack(unit.getTarget().getPosition());
		}

		// If unit is currently not attacking his assigned target, or last command isn't attack
		else if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Attack_Unit || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit())
			unit.unit()->attack(unit.getTarget().unit());

		// If unit can receive a new order and current order is wrong, re-issue
		else if (newOrder && unit.unit()->getOrderTarget() != unit.getTarget().unit())
			unit.unit()->attack(unit.getTarget().unit());

	}

	void CommandManager::approach(UnitInfo& unit)
	{
		if (unit.getTarget().getPosition().isValid() && !isLastCommand(unit, UnitCommandTypes::Move, unit.getTarget().getPosition()))
			unit.unit()->move(unit.getTarget().getPosition());
	}

	bool CommandManager::isLastCommand(UnitInfo& unit, UnitCommandType thisCommand, Position thisPosition)
	{
		if (unit.unit()->getLastCommand().getType() == thisCommand && unit.unit()->getLastCommand().getTargetPosition() == thisPosition)
			return true;
		return false;
	}

	bool CommandManager::overlapsCommands(TechType tech, Position here, int radius)
	{
		for (auto &command : myCommands) {
			if (command.tech == tech && command.pos.getDistance(here) <= radius * 2)
				return true;
		}
		return false;
	}

	bool CommandManager::overlapsCommands(UnitType unit, Position here, int radius)
	{
		for (auto &command : myCommands) {
			if (command.unit == unit && command.pos.getDistance(here) <= radius * 2)
				return true;
		}
		return false;
	}

	bool CommandManager::overlapsAllyDetection(Position here)
	{
		for (auto &command : myCommands) {
			if (command.unit == UnitTypes::Spell_Scanner_Sweep) {
				double range = 420.0;
				if (command.pos.getDistance(here) < range)
					return true;
			}
			else if (command.unit.isDetector()) {
				double range = command.unit.isBuilding() ? 256.0 : 360.0;
				if (command.pos.getDistance(here) < range)
					return true;
			}			
		}
		return false;
	}

	bool CommandManager::overlapsEnemyDetection(Position here)
	{
		for (auto &command : enemyCommands) {
			if (command.unit == UnitTypes::Spell_Scanner_Sweep) {
				double range = 420.0;
				if (command.pos.getDistance(here) < range)
					return true;
			}

			else if (command.unit.isDetector()) {
				double range = command.unit.isBuilding() ? 256.0 : 360.0;
				if (command.pos.getDistance(here) < range)
					return true;
			}			
		}
		return false;
	}

	bool CommandManager::isInDanger(Position here)
	{
		for (auto &command : myCommands) {
			if (command.tech == TechTypes::Psionic_Storm && here.getDistance(command.pos) <= 128)
				return true;
			if (command.tech == TechTypes::Disruption_Web && here.getDistance(command.pos) <= 128)
				return true;
			if (command.tech == TechTypes::EMP_Shockwave && here.getDistance(command.pos) <= 128)
				return true;
			if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640)
				return true;
		}

		for (auto &command : enemyCommands) {
			if (command.tech == TechTypes::Psionic_Storm && here.getDistance(command.pos) <= 128)
				return true;
			if (command.tech == TechTypes::Disruption_Web && here.getDistance(command.pos) <= 128)
				return true;
			if (command.tech == TechTypes::EMP_Shockwave && here.getDistance(command.pos) <= 128)
				return true;
			if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640)
				return true;
		}

		return false;
	}

	bool CommandManager::isInDanger(UnitInfo& unit)
	{
		return isInDanger(unit.getPosition());
	}
}