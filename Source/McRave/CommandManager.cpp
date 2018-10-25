#include "McRave.h"

namespace McRave
{
	void CommandManager::onFrame()
	{
		Display().startClock();
		updateEnemyCommands();
		updateUnits();
		Display().performanceTest(__FUNCTION__);
	}

	void CommandManager::updateEnemyCommands()
	{
		// Clear cache
		enemyCommands.clear();

		// Store bullets as enemy units if they can affect us too
		for (auto &b : Broodwar->getBullets()) {
			if (b && b->exists()) {
				if (b->getType() == BulletTypes::Psionic_Storm)
					addCommand(nullptr, b->getPosition(), TechTypes::Psionic_Storm, true);

				if (b->getType() == BulletTypes::EMP_Missile)
					addCommand(nullptr, b->getTargetPosition(), TechTypes::EMP_Shockwave, true);
			}
		}

		// Store enemy detection and assume casting orders
		for (auto &u : Units().getEnemyUnits()) {
			UnitInfo& unit = u.second;

			if (!unit.unit() || (unit.unit()->exists() && (unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())))
				continue;

			if (unit.getType().isDetector())
				Commands().addCommand(unit.unit(), unit.getPosition(), unit.getType(), true);

			if (unit.unit()->exists()) {
				Order enemyOrder = unit.unit()->getOrder();
				Position enemyTarget = unit.unit()->getOrderTargetPosition();

				if (enemyOrder == Orders::CastEMPShockwave)
					Commands().addCommand(unit.unit(), enemyTarget, TechTypes::EMP_Shockwave, true);
				if (enemyOrder == Orders::CastPsionicStorm)
					Commands().addCommand(unit.unit(), enemyTarget, TechTypes::Psionic_Storm, true);
			}
		}

		// Store nuke dots
		for (auto &dot : Broodwar->getNukeDots())
			Commands().addCommand(nullptr, dot, TechTypes::Nuclear_Strike, true);
	}

	void CommandManager::updateUnits()
	{
		myCommands.clear();
		for (auto &u : Units().getMyUnits()) {
			auto &unit = u.second;
			if (unit.getRole() == Role::Fighting)
				updateDecision(unit);
		}
	}

	void CommandManager::updateDecision(UnitInfo& unit)
	{
		bool attackCooldown = Broodwar->getFrameCount() - unit.getLastAttackFrame() <= unit.getMinStopFrame() - Broodwar->getRemainingLatencyFrames();
		bool latencyCooldown =	Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0;

		if (!unit.unit() || !unit.unit()->exists()																							// Prevent crashes	
			|| unit.getType() == UnitTypes::Protoss_Interceptor																				// Interceptors don't need commands
			|| unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted()		// If the unit is locked down, maelstrommed, stassised, or not completed	
			|| (unit.getType() != UnitTypes::Protoss_Carrier && attackCooldown)																// If the unit is not ready to perform an action after an attack (certain units have minimum frames after an attack before they can receive a new command)
			|| latencyCooldown)
			return;

		// Unstick a unit
		if (unit.isStuck() && unit.unit()->isMoving())
			unit.unit()->stop();

		// Units targeted by splash need to move away from the army
		else if (Units().getSplashTargets().find(unit.unit()) != Units().getSplashTargets().end()) {
			if (unit.hasTarget() && unit.unit()->getGroundWeaponCooldown() <= 0 && unit.getTarget().unit()->exists())
				attack(unit);
			else
				approach(unit);
		}

		// If this unit should use a special ability that requires a command
		else if (shouldUseSpecial(unit))
			return;

		// If this unit should engage
		else if (unit.getCombatState() == CombatState::Engaging) {
			unit.circleGreen();
			if (shouldAttack(unit))
				attack(unit);
			else if (shouldApproach(unit))
				approach(unit);
			else if (shouldKite(unit))
				kite(unit);
			else
				move(unit);			
		}

		// If this unit should retreat
		else if (unit.getCombatState() == CombatState::Retreating) {
			unit.circleRed();
			if (shouldDefend(unit))
				defend(unit);
			else if (unit.getType().isFlyer() && unit.getDestination().isValid())	// should escort - temp not using
				escort(unit);
			else if (shouldHunt(unit))				
				hunt(unit);			
			else
				kite(unit);
		}


	}

	bool CommandManager::shouldAttack(UnitInfo& unit)
	{
		// If no target target or a Lurker
		if (!unit.hasTarget()
			|| unit.getType() == UnitTypes::Zerg_Lurker)
			return false;

		// Carrier attack command issuing
		if (unit.getType() == UnitTypes::Protoss_Carrier) {
			for (auto &i : unit.unit()->getInterceptors()) {
				if (!i || !i->isCompleted()) continue;
				UnitInfo interceptor = Units().getMyUnits()[i];
				if (Broodwar->getFrameCount() - interceptor.getLastAttackFrame() > 100)
					return true;
			}
			return false;
		}

		// If attack not on cooldown
		if ((!unit.getTarget().getType().isFlyer() && unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames())
			|| (unit.getTarget().getType().isFlyer() && unit.unit()->getAirWeaponCooldown() < Broodwar->getRemainingLatencyFrames())
			|| unit.getType() == UnitTypes::Terran_Medic)
			return true;
		return false;
	}

	bool CommandManager::shouldKite(UnitInfo& unit)
	{
		// Mutas and Carriers
		if (unit.getType() == UnitTypes::Protoss_Carrier
			|| unit.getType() == UnitTypes::Zerg_Mutalisk)
			return true;

		// Corsairs or Zealots at a wall
		else if (unit.getType() == UnitTypes::Protoss_Corsair
			|| (unit.getType() == UnitTypes::Protoss_Zealot && Terrain().getNaturalWall() && Position(Terrain().getNaturalWall()->getDoor()).getDistance(unit.getPosition()) < 96.0))
			return false;

		if (unit.hasTarget()) {

			auto widths = unit.getTarget().getType().tileWidth() * 16.0 + unit.getType().tileWidth() * 16.0;
			auto allyRange = widths + (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
			auto enemyRange = widths + (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

			// Within defending distance or attacking a building
			if ((unit.getPosition().getDistance(Terrain().getDefendPosition()) < 128.0 && allyRange <= 64.0 && Strategy().defendChoke())
				|| (unit.getTarget().getType().isBuilding()))
				return false;
						
			if (((unit.getTarget().isBurrowed() || unit.getTarget().unit()->isCloaked()) && !unit.getTarget().unit()->isDetected())		// Invisible Unit
				|| (unit.getType() == UnitTypes::Protoss_Reaver)																		// Reavers always kite
				|| (unit.getType() == UnitTypes::Terran_Vulture)																		// Vultures always kite
				|| (unit.getType() == UnitTypes::Zerg_Mutalisk)																			// Mutas always kite
				|| (unit.getType() == UnitTypes::Protoss_Carrier)
				|| (allyRange >= 32.0 && unit.unit()->isUnderAttack() && allyRange >= enemyRange)										// Ranged unit under attack by unit with lower range
				|| ((enemyRange <= allyRange && unit.unit()->getDistance(unit.getTarget().getPosition()) <= allyRange - enemyRange)))	// Ranged unit fighting lower range unit and not at max range
				return true;
		}
		return false;
	}

	bool CommandManager::shouldApproach(UnitInfo& unit)
	{
		// No target, Carrier or Muta
		if (!unit.hasTarget()
			|| unit.getType() == UnitTypes::Protoss_Carrier
			|| unit.getType() == UnitTypes::Zerg_Mutalisk)
			return false;

		// Dominant fight, lower range, Lurker or non Scourge targets
		if ((unit.getSimValue() >= 10.0 && unit.getType() != UnitTypes::Protoss_Reaver && (!unit.getTarget().getType().isWorker() || unit.getGroundRange() <= 32))
			|| (unit.getGroundRange() < 32 && unit.getTarget().getType().isWorker())
			|| unit.getType() == UnitTypes::Zerg_Lurker
			|| (unit.getGroundRange() < unit.getTarget().getGroundRange() && !unit.getTarget().getType().isBuilding() && Grids().getMobility(WalkPosition(unit.getEngagePosition())) > 0)																					// Approach slower units with higher range
			|| (unit.getType() != UnitTypes::Terran_Battlecruiser && unit.getType() != UnitTypes::Zerg_Guardian && unit.getType().isFlyer() && unit.getTarget().getType() != UnitTypes::Zerg_Scourge))																												// Small flying units approach other flying units except scourge
			return true;
		return false;
	}

	bool CommandManager::shouldDefend(UnitInfo& unit)
	{
		bool isSafe = (!unit.getType().isFlyer() && Grids().getEGroundThreat(unit.getWalkPosition()) == 0.0) || (unit.getType().isFlyer() && Grids().getEAirThreat(unit.getWalkPosition()) == 0.0);
		bool closeToDefend = Terrain().getDefendPosition().getDistance(unit.getPosition()) < 320.0 || Terrain().isInAllyTerritory(unit.getTilePosition()) || Terrain().isInAllyTerritory((TilePosition)unit.getDestination()) || (!unit.getType().isFlyer() && !mapBWEM.GetArea(unit.getTilePosition()));
		if (/*(!unit.hasTarget() || !Units().isThreatening(unit.getTarget())) &&*/ isSafe && closeToDefend)
			return true;
		return false;
	}
	
	bool CommandManager::shouldHunt(UnitInfo& unit)
	{
		if (unit.hasTarget() && (unit.getType().isFlyer() || unit.getType() == UnitTypes::Protoss_Dark_Templar))
			return true;
		return false;
	}



	void CommandManager::move(UnitInfo& unit)
	{
		// MOVE TO SPECIAL COMMANDS: If it's a tank, make sure we're unsieged before moving - TODO: Check that target has velocity and > 512 or no velocity and < tank range
		if (unit.hasTarget() && unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && unit.unit()->getDistance(unit.getTarget().getPosition()) > 512 && unit.getTarget().getSpeed() > 0.0)
			unit.unit()->unsiege();

		// Concave at destination
		else if (unit.getDestination().isValid()) {
			if (!Terrain().isInEnemyTerritory((TilePosition)unit.getDestination())) {
				Position bestPosition = Util().getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getDestination())));

				if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None)) {
					if (unit.unit()->getLastCommand().getTargetPosition() != Position(bestPosition) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
						unit.unit()->move(Position(bestPosition));
				}
			}
			else if (unit.unit()->getLastCommand().getTargetPosition() != Position(unit.getDestination()) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
				unit.unit()->move(unit.getDestination());
		}

		// If unit has a transport, move to it or load into it
		else if (unit.hasTransport() && unit.getTransport().unit()->exists())
			unit.unit()->rightClick(unit.getTransport().unit());

		// If target doesn't exist, move towards it
		else if (unit.hasTarget() && unit.getTarget().getPosition().isValid() && Grids().getMobility(WalkPosition(unit.getEngagePosition())) > 0 && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < 320.0 || unit.getType().isFlyer())) {
			if (!isLastCommand(unit, UnitCommandTypes::Move, unit.getTarget().getPosition()))
				unit.unit()->move(unit.getTarget().getPosition());
		}

		else if (Terrain().getAttackPosition().isValid()) {
			if (!isLastCommand(unit, UnitCommandTypes::Move, Terrain().getAttackPosition()))
				unit.unit()->move(Terrain().getAttackPosition());
		}
		
		// If no target and no enemy bases, move to a base location (random if we have found the enemy once already)
		else if (unit.unit()->isIdle()) {
			if (Terrain().getEnemyStartingPosition().isValid())
				unit.unit()->move(Terrain().randomBasePosition());
			else {
				for (auto &start : Broodwar->getStartLocations()) {
					if (start.isValid() && !Broodwar->isExplored(start) && !Commands().overlapsCommands(unit.unit(), unit.getType(), Position(start), 32)) {
						unit.unit()->move(Position(start));
						Commands().addCommand(unit.unit(), Position(start), unit.getType());
						return;
					}
				}
				// Catch in case no valid movement positions
				unit.unit()->move(Terrain().closestUnexploredStart());
			}
		}
	}

	void CommandManager::defend(UnitInfo& unit)
	{
		// HACK: Hardcoded cannon surround, testing
		if (unit.getType().isWorker() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) {
			auto enemy = Util().getClosestUnit(unit.getPosition(), Broodwar->enemy());
			if (enemy) {
				auto cannon = Util().getClosestUnit(enemy->getPosition(), Broodwar->self(), UnitTypes::Protoss_Photon_Cannon);
				if (cannon) {
					double distBest = DBL_MAX;
					auto walkBest = WalkPositions::Invalid;
					auto start = cannon->getWalkPosition();
					for (int x = start.x - 2; x < start.x + 10; x++) {
						for (int y = start.y - 2; y < start.y + 10; y++) {
							WalkPosition w(x, y);
							double dist = Position(w).getDistance(mapBWEM.Center());
							if (dist < distBest && Util().isMobile(unit.getWalkPosition(), w, unit.getType())) {
								distBest = dist;
								walkBest = w;
							}
						}
					}

					if (walkBest.isValid() && unit.unit()->getLastCommand().getTargetPosition() != Position(walkBest))
						unit.unit()->move(Position(walkBest));
					return;
				}
			}
		}

		// HACK: Flyers defend above a base
		// TODO: Choose a base instead of closest to enemy, sometimes fly over a base I dont own
		if (unit.getType().isFlyer()) {
			if (Terrain().getEnemyStartingPosition().isValid() && mapBWEB.getMainPosition().getDistance(Terrain().getEnemyStartingPosition()) < mapBWEB.getNaturalPosition().getDistance(Terrain().getEnemyStartingPosition()))
				unit.unit()->move(mapBWEB.getMainPosition());
			else
				unit.unit()->move(mapBWEB.getNaturalPosition());
		}

		else if (unit.getDestination().isValid()) {
			Position bestPosition = Util().getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getDestination())));

			if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None)) {
				if (unit.unit()->getLastCommand().getTargetPosition() != bestPosition || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
					unit.unit()->move(bestPosition);
			}
		}

		else {
			Position bestPosition = Util().getConcavePosition(unit, nullptr, Terrain().getDefendPosition());
			if (bestPosition.isValid()) {
				if (unit.getPosition().getDistance(bestPosition) > 16.0)
					unit.unit()->move(bestPosition);
				else
					addCommand(unit.unit(), bestPosition, UnitTypes::None);
				unit.setDestination(bestPosition);
			}

			Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Purple);
		}
	}

	void CommandManager::kite(UnitInfo& unit)
	{
		// If unit has a transport, move to it or load into it - TODO: add scarab count upgrade check
		// HACK: This is just for Reavers atm, add HT before AIIDE hopefully
		if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
			if (unit.getType() == UnitTypes::Protoss_Reaver && unit.unit()->getScarabCount() != 5)
				unit.unit()->rightClick(unit.getTransport().unit());
			else if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75)
				unit.unit()->rightClick(unit.getTransport().unit());
		}

		// Workers use mineral fields to help with drilling
		else if (unit.hasResource() && unit.getResource().unit()->exists() && Terrain().isInAllyTerritory(unit.getTilePosition()) && Grids().getEGroundThreat(WalkPosition(unit.getResource().getPosition())) == 0.0)
			unit.unit()->gather(unit.getResource().unit());

		else {
			auto start = unit.getWalkPosition();
			auto best = 0.0;
			auto posBest = Positions::Invalid;

			for (int x = start.x - 6; x < start.x + 10; x++) {
				for (int y = start.y - 6; y < start.y + 10; y++) {
					WalkPosition w(x, y);
					Position p = Position(w) + Position(4, 4);

					if (!w.isValid()
						|| p.getDistance(unit.getPosition()) < 32.0
						|| Commands().isInDanger(p)
						|| !Util().isMobile(start, w, unit.getType())
						|| Grids().getESplash(w) > 0
						|| Buildings().overlapsQueuedBuilding(unit.getType(), unit.getTilePosition()))
						continue;

					double distance;
					if (!Strategy().defendChoke() && unit.getType() == UnitTypes::Protoss_Zealot && unit.hasTarget() && Terrain().isInAllyTerritory(unit.getTarget().getTilePosition()))
						distance = p.getDistance(Terrain().getMineralHoldPosition());
					else if (unit.hasTarget() && (Terrain().isInAllyTerritory(unit.getTarget().getTilePosition()) || Terrain().isInAllyTerritory(unit.getTilePosition())))
						distance = 1.0 / (32.0 + p.getDistance(unit.getTarget().getPosition()));
					else
						distance = (unit.getType().isFlyer() || Terrain().isIslandMap()) ? p.getDistance(mapBWEB.getMainPosition()) : Grids().getDistanceHome(w);

					double mobility = 1.0; // HACK: Test ignoring mobility

					double threat = Util().getHighestThreat(w, unit);
					double grouping = 1.0 + (unit.getType().isFlyer() ? double(Grids().getAAirCluster(w)): 0.0);
					double score = grouping / (threat * distance);

					// If position is valid and better score than current, set as current best
					if (score > best) {
						posBest = p;
						best = score;
					}
				}
			}

			if (!isLastCommand(unit, UnitCommandTypes::Move, posBest))
				unit.unit()->move(posBest);
		}
	}

	void CommandManager::attack(UnitInfo& unit)
	{
		bool newOrder = Broodwar->getFrameCount() - unit.unit()->getLastCommandFrame() > Broodwar->getRemainingLatencyFrames();

		// HACK: Flyers don't want to decel when out of range, so we move to the target then attack when in range
		if (unit.getType().isFlyer() && !Util().unitInRange(unit))
			unit.unit()->move(unit.getTarget().getPosition());


		// DT hold position vs spider mines
		else if (unit.getType() == UnitTypes::Protoss_Dark_Templar && unit.hasTarget() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getTarget().hasTarget() && unit.getTarget().getTarget().unit() == unit.unit() && !unit.getTarget().isBurrowed()) {
			if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Hold_Position)
				unit.unit()->holdPosition();
		}

		// TODO: Move to special - If close enough, set the current area as a psi storm area so units move and storms don't overlap
		else if (unit.getType() == UnitTypes::Protoss_High_Templar) {
			if (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= 640 && !Commands().overlapsCommands(unit.unit(), TechTypes::Psionic_Storm, unit.getTarget().getPosition(), 96) && unit.unit()->getEnergy() >= 75 && (Grids().getEGroundCluster(unit.getTarget().getWalkPosition()) + Grids().getEAirCluster(unit.getTarget().getWalkPosition())) >= STORM_LIMIT) {
				unit.unit()->useTech(TechTypes::Psionic_Storm, unit.getTarget().unit());
				addCommand(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm);
			}
		}

		// Medic's just attack move
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
		if (unit.hasTarget() && unit.getTarget().getPosition().isValid() && !isLastCommand(unit, UnitCommandTypes::Move, unit.getTarget().getPosition()))
			unit.unit()->move(unit.getTarget().getPosition());
	}

	void CommandManager::safeMove(UnitInfo& unit)
	{
		// Low threat, low distance		
		auto start = unit.getWalkPosition();
		auto best = 0.0;
		auto bestPos = unit.getDestination();
		UnitInfo* enemy = Util().getClosestThreat(unit);

		if (enemy) {
			for (int x = start.x - 12; x < start.x + 16; x++) {
				for (int y = start.y - 12; y < start.y + 16; y++) {
					WalkPosition w(x, y);
					Position p = Position(w) + Position(4, 4);

					if (!w.isValid()
						|| !Util().isMobile(start, w, unit.getType()))
						continue;

					double threat = Util().getHighestThreat(w, unit);
					double distance = 1.0 + (unit.getType().isFlyer() ? p.getDistance(unit.getDestination()) : mapBWEB.getGroundDistance(p, unit.getDestination()));

					double score = 1.0 / (threat * distance);
					if (score >= best) {
						best = score;
						bestPos = Position(w);
					}
				}
			}
		}

		if (!isLastCommand(unit, UnitCommandTypes::Move, bestPos))
			unit.unit()->move(bestPos);
	}

	void CommandManager::hunt(UnitInfo& unit)
	{
		// If unit has no destination, give target as a destination
		if (unit.hasTarget() && !unit.getDestination().isValid())
			unit.setDestination(unit.getTarget().getPosition());

		// No threat, low visibility, low distance, attack if possible
		auto start = unit.getWalkPosition();
		auto best = 0.0;
		auto bestPos = unit.getDestination();

		// Some testing stuff
		auto radius = 12 + int(unit.getSpeed());
		auto height = int(unit.getType().height() / 8.0);
		auto width = int(unit.getType().width() / 8.0);
		auto currentDist = (unit.getType().isFlyer() ? unit.getPosition().getDistance(unit.getDestination()) : mapBWEB.getGroundDistance(unit.getPosition(), unit.getDestination()));

		for (int x = start.x - radius; x < start.x + radius + width; x++) {
			for (int y = start.y - radius; y < start.y + radius + height; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);
				TilePosition t(w);
				double distToP = unit.getType().isFlyer() ? unit.getPosition().getDistance(p) : mapBWEB.getGroundDistance(p, unit.getPosition());
				double grdDist = mapBWEB.getGroundDistance(p, unit.getDestination());
				double airDist = p.getDistance(unit.getDestination());

				if (!w.isValid()
					|| !Util().isMobile(start, w, unit.getType())
					|| p.getDistance(unit.getPosition()) < 32.0
					|| distToP > radius * 8)
					continue;

				double threat = Util().getHighestThreat(w, unit);
				double distance = (unit.getType().isFlyer() ? airDist : grdDist);
				double visited = log(min(500.0, double(Broodwar->getFrameCount() - Grids().lastVisitedFrame(w))));
				double grouping = exp((unit.getType().isFlyer() ? double(Grids().getAAirCluster(w)) : 0.0));

				double score = grouping * visited / (threat * distance);

				if (score >= best && threat == MIN_THREAT) {
					best = score;
					bestPos = Position(w);
				}
			}
		}

		if (unit.hasTarget() && Util().getHighestThreat(WalkPosition(unit.getEngagePosition()), unit) == MIN_THREAT && Util().unitInRange(unit)) {
			if (shouldAttack(unit))
				attack(unit);
			else
				approach(unit);
		}
		else if (bestPos != unit.getDestination()) {
			Broodwar->drawLineMap(unit.getPosition(), bestPos, Colors::Grey);
			if (!isLastCommand(unit, UnitCommandTypes::Move, bestPos))
				unit.unit()->move(bestPos);
		}
		else
			kite(unit);
	}

	void CommandManager::escort(UnitInfo& unit)
	{
		// Low distance, attack if possible
		auto start = unit.getWalkPosition();
		auto best = 0.0;
		auto bestPos = unit.getDestination();

		Broodwar->drawTextMap(unit.getPosition(), "Escort");
		for (int x = start.x - 12; x < start.x + 16; x++) {
			for (int y = start.y - 12; y < start.y + 16; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);
				TilePosition t(w);

				if (!w.isValid()
					|| !Util().isMobile(start, w, unit.getType())
					|| p.getDistance(unit.getPosition()) <= 32.0)
					continue;

				double threat = Util().getHighestThreat(w, unit);
				double distance = 1.0 + (unit.getType().isFlyer() ? p.getDistance(unit.getDestination()) : mapBWEB.getGroundDistance(p, unit.getDestination()));

				double score = 1.0 / (threat * distance);
				if (score >= best && threat < 2.0) {
					best = score;
					bestPos = Position(w);
				}
			}
		}

		if (!isLastCommand(unit, UnitCommandTypes::Move, bestPos))
			unit.unit()->move(bestPos);
	}



	bool CommandManager::isLastCommand(UnitInfo& unit, UnitCommandType thisCommand, Position thisPosition)
	{
		if (unit.unit()->getLastCommand().getType() == thisCommand && unit.unit()->getLastCommand().getTargetPosition() == thisPosition)
			return true;
		return false;
	}

	bool CommandManager::overlapsCommands(Unit unit, TechType tech, Position here, int radius)
	{
		for (auto &command : myCommands) {
			if (command.unit != unit && command.tech == tech && command.pos.getDistance(here) <= radius * 2)
				return true;
		}
		return false;
	}

	bool CommandManager::overlapsCommands(Unit unit, UnitType type, Position here, int radius)
	{
		for (auto &command : myCommands) {
			if (command.unit != unit && command.type == type && command.pos.getDistance(here) <= radius * 2)
				return true;
		}
		return false;
	}

	bool CommandManager::overlapsAllyDetection(Position here)
	{
		for (auto &command : myCommands) {
			if (command.type == UnitTypes::Spell_Scanner_Sweep) {
				double range = 420.0;
				if (command.pos.getDistance(here) < range)
					return true;
			}
			else if (command.type.isDetector()) {
				double range = command.type.isBuilding() ? 256.0 : 360.0;
				if (command.pos.getDistance(here) < range)
					return true;
			}
		}
		return false;
	}

	bool CommandManager::overlapsEnemyDetection(Position here)
	{
		for (auto &command : enemyCommands) {
			if (command.type == UnitTypes::Spell_Scanner_Sweep) {
				double range = 420.0;
				double ff = 32;
				if (command.pos.getDistance(here) < range + ff)
					return true;
			}

			else if (command.type.isDetector()) {
				double range = command.type.isBuilding() ? 256.0 : 360.0;
				double ff = 32;
				if (command.pos.getDistance(here) < range + ff)
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