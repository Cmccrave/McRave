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
		bool attackCooldown = Broodwar->getFrameCount() - unit.getLastAttackFrame() <= unit.getMinStopFrame() - Broodwar->getLatencyFrames();
		bool latencyCooldown =	Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0;

		if (!unit.unit() || !unit.unit()->exists()																							// Prevent crashes	
			|| unit.getType() == UnitTypes::Protoss_Interceptor																				// Interceptors don't need commands
			|| unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted()		// If the unit is locked down, maelstrommed, stassised, or not completed	
			|| (unit.getType() != UnitTypes::Protoss_Carrier && attackCooldown)																// If the unit is not ready to perform an action after an attack (certain units have minimum frames after an attack before they can receive a new command)
			|| latencyCooldown)
			return;
		
		for (auto cmd : commands) {
			if ((this->*cmd)(unit))
				break;
		}
	}

	bool CommandManager::misc(UnitInfo& unit)
	{
		// Unstick a unit
		if (unit.isStuck() && unit.unit()->isMoving()) {
			unit.unit()->stop();
			return true;
		}

		// Units targeted by splash need to move away from the army
		// TODO: Maybe move this to their respective functions
		else if (Units().getSplashTargets().find(unit.unit()) != Units().getSplashTargets().end()) {
			if (unit.hasTarget() && unit.unit()->getGroundWeaponCooldown() <= 0 && unit.getTarget().unit()->exists())
				attack(unit);
			else
				approach(unit);
			return true;
		}
		return false;
	}

	bool CommandManager::attack(UnitInfo& unit)
	{
		unit.circleGreen();
		auto newOrder = Broodwar->getFrameCount() - unit.unit()->getLastCommandFrame() > Broodwar->getRemainingLatencyFrames();
		auto canAttack = (unit.hasTarget() && unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() : unit.unit()->getGroundWeaponCooldown()) < Broodwar->getRemainingLatencyFrames();
		auto shouldAttack = unit.getLocalState() == LocalState::Engaging;

		// If unit can't attack
		if (!unit.hasTarget() || !canAttack || !shouldAttack)
			return false;

		// HACK: Flyers don't want to decel when out of range, so we move to the target then attack when in range
		if (unit.getType().isFlyer() && !Util().unitInRange(unit)) {
			unit.unit()->move(unit.getTarget().getPosition());
			return true;
		}

		// DT hold position vs spider mines
		else if (unit.getType() == UnitTypes::Protoss_Dark_Templar && unit.hasTarget() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getTarget().hasTarget() && unit.getTarget().getTarget().unit() == unit.unit() && !unit.getTarget().isBurrowed()) {
			if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Hold_Position)
				unit.unit()->holdPosition();
			return true;
		}

		// Medic's just attack move
		else if (unit.getType() == UnitTypes::Terran_Medic) {
			if (!isLastCommand(unit, UnitCommandTypes::Attack_Move, unit.getTarget().getPosition()))
				unit.unit()->attack(unit.getTarget().getPosition());
			return true;
		}

		// If unit is currently not attacking his assigned target, or last command isn't attack
		else if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Attack_Unit || unit.unit()->getLastCommand().getTarget() != unit.getTarget().unit()) {
			unit.unit()->attack(unit.getTarget().unit());
			return true;
		}

		// If unit can receive a new order and current order is wrong, re-issue
		else if (newOrder && unit.unit()->getOrderTarget() != unit.getTarget().unit()) {
			unit.unit()->attack(unit.getTarget().unit());
			return true;
		}
		return (canAttack && Util().unitInRange(unit));
	}

	bool CommandManager::approach(UnitInfo& unit)
	{		
		const auto canApproach = [&]() {
			
			// No target, Carrier or Muta
			if (!unit.hasTarget() || unit.getType() == UnitTypes::Protoss_Carrier || unit.getType() == UnitTypes::Zerg_Mutalisk)
				return false;

			// Dominant fight, lower range, Lurker or non Scourge targets
			if ((unit.getSimValue() >= 10.0 && unit.getType() != UnitTypes::Protoss_Reaver && (!unit.getTarget().getType().isWorker() || unit.getGroundRange() <= 32))
				|| (unit.getGroundRange() < 32 && unit.getTarget().getType().isWorker())
				|| unit.getType() == UnitTypes::Zerg_Lurker
				|| (unit.getGroundRange() < unit.getTarget().getGroundRange() && !unit.getTarget().getType().isBuilding() && Grids().getMobility(WalkPosition(unit.getEngagePosition())) > 0)																					// Approach slower units with higher range
				|| (unit.getType() != UnitTypes::Terran_Battlecruiser && unit.getType() != UnitTypes::Zerg_Guardian && unit.getType().isFlyer() && unit.getTarget().getType() != UnitTypes::Zerg_Scourge))																												// Small flying units approach other flying units except scourge
				return true;
			return false;
		};

		
		if (canApproach() && unit.getTarget().getPosition().isValid()) {
			if (!isLastCommand(unit, UnitCommandTypes::Move, unit.getTarget().getPosition()))
				unit.unit()->move(unit.getTarget().getPosition());

			unit.circleBlue();
			return true;
		}
		return false;
	}

	bool CommandManager::move(UnitInfo& unit)
	{
		unit.circleYellow();

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
						return true;
					}
				}
				// Catch in case no valid movement positions
				unit.unit()->move(Terrain().closestUnexploredStart());
			}
		}
	}

	bool CommandManager::defend(UnitInfo& unit)
	{
		unit.circleBlack();
		bool closeToDefend = Terrain().getDefendPosition().getDistance(unit.getPosition()) < 320.0 || Terrain().isInAllyTerritory(unit.getTilePosition()) || Terrain().isInAllyTerritory((TilePosition)unit.getDestination()) || (!unit.getType().isFlyer() && !unit.hasTransport() && !mapBWEM.GetArea(unit.getTilePosition()));
		if (!closeToDefend)
			return false;

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
							if (dist < distBest && Util().isWalkable(unit.getWalkPosition(), w, unit.getType())) {
								distBest = dist;
								walkBest = w;
							}
						}
					}

					if (walkBest.isValid() && unit.unit()->getLastCommand().getTargetPosition() != Position(walkBest))
						unit.unit()->move(Position(walkBest));
					return true;
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
				return true;
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
				return true;
			}

			Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Purple);
		}
		return false;
	}

	bool CommandManager::kite(UnitInfo& unit)
	{
		unit.circleRed();
		const auto canKite = [&]() {
			// Units that never kite based on their target
			if (unit.hasTarget() && unit.getLocalState() == LocalState::Engaging) {
				auto allyRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
				auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

				if (unit.getType() == UnitTypes::Protoss_Corsair
					|| (unit.getTarget().getType().isBuilding() && !unit.getType().isFlyer()))
					return false;

				if (unit.getType() == UnitTypes::Protoss_Reaver
					|| (unit.getType() == UnitTypes::Terran_Vulture)
					|| (unit.getType() == UnitTypes::Zerg_Mutalisk)
					|| (unit.getType() == UnitTypes::Protoss_Carrier)
					|| (allyRange >= 32.0 && unit.unit()->isUnderAttack() && allyRange >= enemyRange)
					|| (unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine && !unit.getTarget().isBurrowed())
					|| ((enemyRange <= allyRange && unit.unit()->getDistance(unit.getTarget().getPosition()) <= allyRange - enemyRange)))
					return true;
				return false;
			}
			return true;
		};

		// Check if a unit with a transport should load into it
		if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
			if (unit.getType() == UnitTypes::Protoss_Reaver && unit.unit()->getScarabCount() != MAX_SCARAB) {
				unit.unit()->rightClick(unit.getTransport().unit());
				return true;
			}
			else if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75) {
				unit.unit()->rightClick(unit.getTransport().unit());
				return true;
			}
		}

		// Check if a worker should mineral walk
		else if (unit.hasResource() && unit.getResource().unit()->exists() && Terrain().isInAllyTerritory(unit.getTilePosition()) && Grids().getEGroundThreat(WalkPosition(unit.getResource().getPosition())) == 0.0) {
			unit.unit()->gather(unit.getResource().unit());
			return true;
		}

		else if (canKite()) {
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
						|| Grids().getCollision(w) > 0
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
					double grouping = 1.0 + (unit.getType().isFlyer() ? double(Grids().getAAirCluster(w)) : 0.0);
					double score = grouping / (threat * distance);

					// If position is valid and better score than current, set as current best
					if (score > best && Util().isWalkable(start, w, unit.getType())) {
						posBest = p;
						best = score;
					}
				}
			}

			if (!isLastCommand(unit, UnitCommandTypes::Move, posBest)) {
				unit.unit()->move(posBest);
				return true;
			}
			Broodwar->drawLineMap(unit.getPosition(), posBest, Colors::Red);
		}
		return false;
	}
	
	

	bool CommandManager::safeMove(UnitInfo& unit)
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
						|| !Util().isWalkable(start, w, unit.getType())
						|| isInDanger(p))
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

	bool CommandManager::hunt(UnitInfo& unit)
	{
		if (!unit.getType().isFlyer() && unit.getType() != UnitTypes::Protoss_Dark_Templar)
			return false;

		// Not doing this
		return false;

		// If unit has no destination, give target as a destination
		if (!unit.getDestination().isValid()) {
			if (unit.hasTarget())
				unit.setDestination(unit.getTarget().getPosition());
			else if (Terrain().getAttackPosition().isValid())
				unit.setDestination(Terrain().getAttackPosition());
		}

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
					|| !Util().isWalkable(start, w, unit.getType())
					|| p.getDistance(unit.getPosition()) < 64.0
					|| distToP > radius * 8
					|| isInDanger(p))
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
			if (true)
				attack(unit);
			else
				approach(unit);
			return true;
		}
		else if (bestPos != unit.getDestination()) {
			Broodwar->drawLineMap(unit.getPosition(), bestPos, Colors::Grey);
			if (!isLastCommand(unit, UnitCommandTypes::Move, bestPos))
				unit.unit()->move(bestPos);
			return true;
		}
		else {
			Broodwar->drawLineMap(unit.getPosition(), bestPos, Colors::Grey);
			kite(unit);
			return true;
		}
		return false;
	}

	bool CommandManager::escort(UnitInfo& unit)
	{
		// Not doing this atm
		return false;

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
					|| !Util().isWalkable(start, w, unit.getType())
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
		return true;
	}



	bool CommandManager::isLastCommand(UnitInfo& unit, UnitCommandType thisCommand, Position thisPosition)
	{
		if (unit.unit()->getLastCommand().getType() == thisCommand && unit.unit()->getLastCommand().getTargetPosition() == thisPosition)
			return true;
		return false;
	}

	bool CommandManager::overlapsCommands(Unit unit, TechType tech, Position here, int radius)
	{
		// TechType checks use a rectangular check
		for (auto &command : myCommands) {
			auto topLeft = command.pos - Position(radius, radius);
			auto botRight = command.pos + Position(radius, radius);
			if (command.unit != unit && command.tech == tech && Util().rectangleIntersect(topLeft, botRight, here))
				return true;
		}
		return false;
	}

	bool CommandManager::overlapsCommands(Unit unit, UnitType type, Position here, int radius)
	{
		// UnitType checks use a radial check
		for (auto &command : myCommands) {
			if (command.unit != unit && command.type == type && command.pos.getDistance(here) <= radius * 2)
				return true;
		}
		return false;
	}

	bool CommandManager::overlapsAllyDetection(Position here)
	{
		// Detection checks use a radial check
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
		// Detection checks use a radial check
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
		// Check that we're not in danger of Storm, DWEB, EMP
		for (auto &command : myCommands) {
			if (command.tech == TechTypes::Psionic_Storm
				|| command.tech == TechTypes::Disruption_Web
				|| command.tech == TechTypes::EMP_Shockwave) {
				auto topLeft = command.pos - Position(48, 48);
				auto botRight = command.pos + Position(48, 48);
				
				return Util().rectangleIntersect(topLeft, botRight, here);
			}

			if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
				auto topLeft = command.pos - Position(320, 320);
				auto botRight = command.pos + Position(320, 320);

				return Util().rectangleIntersect(topLeft, botRight, here);
			}
		}

		for (auto &command : enemyCommands) {
			if (command.tech == TechTypes::Psionic_Storm
				|| command.tech == TechTypes::Disruption_Web
				|| command.tech == TechTypes::EMP_Shockwave) {
				auto topLeft = command.pos - Position(48, 48);
				auto botRight = command.pos + Position(48, 48);

				return Util().rectangleIntersect(topLeft, botRight, here);
			}
			if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
				auto topLeft = command.pos - Position(320, 320);
				auto botRight = command.pos + Position(320, 320);

				return Util().rectangleIntersect(topLeft, botRight, here);
			}
		}
		return false;
	}

	bool CommandManager::isInDanger(UnitInfo& unit)
	{
		return isInDanger(unit.getPosition());
	}
}