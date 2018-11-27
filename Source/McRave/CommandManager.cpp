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
		if (!unit.unit() || !unit.unit()->exists()																							// Prevent crashes			
			|| unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())	// If the unit is locked down, maelstrommed, stassised, or not completed
			return;

		// Convert our commands to strings to display what the unit is doing for debugging
		map<int, string> commandNames{
			make_pair(0, "Misc"),
			make_pair(1, "Special"),
			make_pair(2, "Attack"),
			make_pair(3, "Approach"),
			make_pair(4, "Kite"),
			make_pair(5, "Defend"),
			make_pair(6, "Hunt"),
			make_pair(7, "Escort"),
			make_pair(8, "Retreat"),
			make_pair(9, "Move")
		};

		int i = 0;
		int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
		for (auto cmd : commands) {
			if ((this->*cmd)(unit)) {
				Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
				break;
			}
			i++;
		}
	}

	bool CommandManager::misc(UnitInfo& unit)
	{
		// Unstick a unit
		if (unit.isStuck() && unit.unit()->isMoving()) {
			unit.unit()->stop();
			return true;
		}

		// DT hold position vs spider mines
		else if (unit.getType() == UnitTypes::Protoss_Dark_Templar && unit.hasTarget() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getTarget().hasTarget() && unit.getTarget().getTarget().unit() == unit.unit() && !unit.getTarget().isBurrowed()) {
			if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Hold_Position)
				unit.unit()->holdPosition();
			return true;
		}

		// If unit has a transport, load into it if we need to
		else if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
			if (unit.getType() == UnitTypes::Protoss_Reaver && (unit.unit()->getScarabCount() != MAX_SCARAB || unit.getLocalState() == LocalState::Retreating)) {
				unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
				return true;
			}
			else if (unit.getType() == UnitTypes::Protoss_High_Templar && (unit.getEnergy() < 75 || unit.getLocalState() == LocalState::Retreating)) {
				unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
				return true;
			}
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
		auto shouldAttack = unit.hasTarget() && unit.getTarget().unit()->exists() && unit.getLocalState() == LocalState::Engaging;

		// If unit should be attacking
		if (shouldAttack) {
			auto canAttack = unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() < Broodwar->getRemainingLatencyFrames() : unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames();

			if (canAttack) {

				// Flyers don't want to decel when out of range, so we move to the target then attack when in range
				if (unit.getType().isFlyer() && unit.hasTarget() && !Util().unitInRange(unit))
					unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition());
				else
					unit.command(UnitCommandTypes::Attack_Unit, &unit.getTarget());
				return true;
			}
		}
		return false;
	}

	bool CommandManager::approach(UnitInfo& unit)
	{
		auto airHarasser = unit.getType() == UnitTypes::Protoss_Corsair || unit.getType() == UnitTypes::Zerg_Mutalisk || unit.getType() == UnitTypes::Terran_Wraith;
		auto shouldApproach = unit.getLocalState() == LocalState::Engaging;

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

		if (shouldApproach && canApproach() && unit.getTarget().getPosition().isValid()) {
			unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition());
			return true;
		}
		return false;
	}

	bool CommandManager::move(UnitInfo& unit)
	{
		function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
			Position p = Position(w) + Position(4, 4);
			double distance = (unit.getType().isFlyer() || Terrain().isIslandMap()) ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination());
			double threat = Util().getHighestThreat(w, unit);
			double grouping = 1.0 + (unit.getType().isFlyer() ? Grids().getAAirCluster(w) : 1.0 / Grids().getAGroundCluster(w));
			double score = grouping / (threat * distance);
			return score;
		};

		if (unit.hasTarget() && Util().unitInRange(unit))
			return false;

		if (unit.getLocalState() == LocalState::Retreating)
			return false;

		if (unit.getDestination().isValid()) {
			if (!Terrain().isInEnemyTerritory((TilePosition)unit.getDestination())) {
				Position bestPosition = Util().getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getDestination())));

				if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None)) {
					if (unit.unit()->getLastCommand().getTargetPosition() != Position(bestPosition) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
						unit.unit()->move(Position(bestPosition));
				}
			}
			else if (unit.unit()->getLastCommand().getTargetPosition() != Position(unit.getDestination()) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
				unit.unit()->move(unit.getDestination());

			return true;
		}

		// If unit has a transport, move to it or load into it
		else if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
			unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
			return true;
		}

		// If target doesn't exist, move towards it
		else if (unit.hasTarget() && unit.getTarget().getPosition().isValid() && Grids().getMobility(WalkPosition(unit.getEngagePosition())) > 0 && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < 320.0 || unit.getType().isFlyer())) {
			unit.setDestination(unit.getTarget().getPosition());
		}

		else if (Terrain().getAttackPosition().isValid()) {
			unit.setDestination(Terrain().getAttackPosition());
		}

		// If no target and no enemy bases, move to a base location (random if we have found the enemy once already)
		else if (unit.unit()->isIdle()) {
			if (Terrain().getEnemyStartingPosition().isValid()) {
				unit.setDestination(Terrain().randomBasePosition());
			}
			else {
				for (auto &start : Broodwar->getStartLocations()) {
					if (start.isValid() && !Broodwar->isExplored(start) && !Commands().overlapsCommands(unit.unit(), unit.getType(), Position(start), 32)) {
						unit.setDestination(Position(start));
					}
				}
			}
		}

		Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Purple);

		if (unit.getDestination().isValid()) {
			auto bestPosition = findViablePosition(unit, scoreFunction);
			if (bestPosition.isValid()) {
				Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Green);
				unit.command(UnitCommandTypes::Move, bestPosition);
				return true;
			}
		}

		return false;
	}

	bool CommandManager::kite(UnitInfo& unit)
	{
		const auto shouldKite = [&]() {
			auto airHarasser = unit.getType() == UnitTypes::Protoss_Corsair || unit.getType() == UnitTypes::Zerg_Mutalisk || unit.getType() == UnitTypes::Terran_Wraith;

			// Units that never kite based on their target
			if (unit.hasTarget() && unit.getLocalState() == LocalState::Engaging) {
				auto allyRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
				auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

				if (airHarasser
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
			}
			return false;
		};

		// TODO: Make this a bit more readable
		function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
			Position p = Position(w) + Position(4, 4);
			double distance;
			if (!Strategy().defendChoke() && unit.getType() == UnitTypes::Protoss_Zealot && unit.hasTarget() && Terrain().isInAllyTerritory(unit.getTarget().getTilePosition()))
				distance = p.getDistance(Terrain().getMineralHoldPosition());
			else if (unit.hasTarget() && (Terrain().isInAllyTerritory(unit.getTarget().getTilePosition()) || Terrain().isInAllyTerritory(unit.getTilePosition())))
				distance = 1.0 / (32.0 + p.getDistance(unit.getTarget().getPosition()));
			else
				distance = (unit.getType().isFlyer() || Terrain().isIslandMap()) ? p.getDistance(BWEB::Map::getMainPosition()) : Grids().getDistanceHome(w);

			double threat = Util().getHighestThreat(w, unit);
			double grouping = 1.0 + (unit.getType().isFlyer() ? Grids().getAAirCluster(w) : 1.0 / Grids().getAGroundCluster(w));
			double score = grouping / (threat * distance);
			return score;
		};

		if (!shouldKite() && !unit.getType().isWorker())
			return false;

		// If we found a valid position, move to it
		auto bestPosition = findViablePosition(unit, scoreFunction);
		if (bestPosition.isValid()) {
			unit.command(UnitCommandTypes::Move, bestPosition);
			return true;
		}
		return false;
	}

	bool CommandManager::defend(UnitInfo& unit)
	{
		bool closeToDefend = Terrain().getDefendPosition().getDistance(unit.getPosition()) < 320.0 || Terrain().isInAllyTerritory(unit.getTilePosition()) || Terrain().isInAllyTerritory((TilePosition)unit.getDestination()) || (!unit.getType().isFlyer() && !unit.hasTransport() && !mapBWEM.GetArea(unit.getTilePosition()));
		if (!closeToDefend || unit.getLocalState() != LocalState::Retreating)
			return false;

		// Probe Cannon surround
		if (unit.getType().isWorker() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) {
			auto cannon = Util().getClosestUnit(mapBWEM.Center(), Broodwar->self(), UnitTypes::Protoss_Photon_Cannon);
			auto distBest = DBL_MAX;
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

			if (walkBest.isValid() && unit.unit()->getLastCommand().getTargetPosition() != Position(walkBest)) {
				unit.unit()->move(Position(walkBest));
				return true;
			}
		}

		// HACK: Flyers defend above a base
		// TODO: Choose a base instead of closest to enemy, sometimes fly over a base I dont own
		if (unit.getType().isFlyer()) {
			if (Terrain().getEnemyStartingPosition().isValid() && BWEB::Map::getMainPosition().getDistance(Terrain().getEnemyStartingPosition()) < BWEB::Map::getNaturalPosition().getDistance(Terrain().getEnemyStartingPosition()))
				unit.unit()->move(BWEB::Map::getMainPosition());
			else
				unit.unit()->move(BWEB::Map::getNaturalPosition());
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

	bool CommandManager::hunt(UnitInfo& unit)
	{
		function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
			Position p = Position(w) + Position(4, 4);
			double threat = Util().getHighestThreat(w, unit);
			double distance = (unit.getType().isFlyer() ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination()));
			double visited = log(min(500.0, double(Broodwar->getFrameCount() - Grids().lastVisitedFrame(w))));
			double grouping = exp((unit.getType().isFlyer() ? Grids().getAAirCluster(w) : 1.0 / Grids().getAGroundCluster(w)));
			double score = grouping * visited / distance;
			if (threat == MIN_THREAT || (unit.unit()->isCloaked() && !overlapsEnemyDetection(p)))
				return score;
			return 0.0;
		};

		// Hunting is only valid with workers, flyers or dark templars
		auto shouldHunt = unit.getType().isWorker() || unit.getType().isFlyer() || unit.getType() == UnitTypes::Protoss_Dark_Templar;
		if (!shouldHunt)
			return false;

		// HACK: If unit has no destination, give target as a destination
		if (!unit.getDestination().isValid()) {
			if (unit.hasTarget())
				unit.setDestination(unit.getTarget().getPosition());
			else if (Terrain().getAttackPosition().isValid())
				unit.setDestination(Terrain().getAttackPosition());
		}

		// If we found a valid position
		auto bestPosition = findViablePosition(unit, scoreFunction);

		// Check if we can get free attacks
		if (unit.hasTarget() && unit.getPercentShield() >= LOW_SHIELD_PERCENT_LIMIT && Util().getHighestThreat(WalkPosition(unit.getEngagePosition()), unit) == MIN_THREAT && Util().unitInRange(unit)) {
			attack(unit);
			return true;
		}
		// Move to the position
		else if (bestPosition.isValid() && bestPosition != unit.getDestination()) {
			Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Grey);
			unit.command(UnitCommandTypes::Move, bestPosition);
			return true;
		}
		return false;
	}

	bool CommandManager::retreat(UnitInfo& unit)
	{
		// Low distance, low threat, high clustering
		function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
			Position p = Position(w) + Position(4, 4);
			double distance = (unit.getType().isFlyer() || Terrain().isIslandMap()) ? p.getDistance(BWEB::Map::getMainPosition()) : Grids().getDistanceHome(w);
			double threat = Util().getHighestThreat(w, unit);
			double grouping = 1.0 + (unit.getType().isFlyer() ? Grids().getAAirCluster(w) : 1.0 / Grids().getAGroundCluster(w));
			double score = grouping / (threat * distance);
			return score;
		};

		// Retreating is only valid when local state is retreating
		auto shouldRetreat = unit.getLocalState() == LocalState::Retreating || unit.getRole() == Role::Scouting;
		if (!shouldRetreat)
			return false;

		// If we found a valid position, move to it
		auto bestPosition = findViablePosition(unit, scoreFunction);
		unit.circleBlack();
		if (bestPosition.isValid()) {
			Broodwar->drawLineMap(unit.getPosition(), bestPosition, Colors::Red);
			unit.command(UnitCommandTypes::Move, bestPosition);
			return true;
		}
		return false;
	}

	bool CommandManager::escort(UnitInfo& unit)
	{
		// Low distance, low threat
		function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
			Position p = Position(w) + Position(4, 4);
			double threat = Util().getHighestThreat(w, unit);
			double distance = 1.0 + (unit.getType().isFlyer() ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination()));
			double score = 1.0 / (threat * distance);
			return score;
		};

		// Escorting
		auto shouldEscort = false;
		if (!shouldEscort)
			return false;

		// If we found a valid position, move to it
		auto bestPosition = findViablePosition(unit, scoreFunction);
		if (bestPosition.isValid()) {
			unit.command(UnitCommandTypes::Move, bestPosition);
			return true;
		}
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

	bool CommandManager::isInDanger(UnitInfo& unit, Position here)
	{
		int halfWidth = int(ceil(unit.getType().width() / 2));
		int halfHeight = int(ceil(unit.getType().height() / 2));

		if (here == Positions::Invalid)
			here = unit.getPosition();

		auto uTopLeft = here + Position(-halfWidth, -halfHeight);
		auto uTopRight = here + Position(halfWidth, -halfHeight);
		auto uBotLeft = here + Position(-halfWidth, halfHeight);
		auto uBotRight = here + Position(halfWidth, halfHeight);

		const auto checkCorners = [&](Position cTopLeft, Position cBotRight) {
			if (Util().rectangleIntersect(cTopLeft, cBotRight, uTopLeft)
				|| Util().rectangleIntersect(cTopLeft, cBotRight, uTopRight)
				|| Util().rectangleIntersect(cTopLeft, cBotRight, uBotLeft)
				|| Util().rectangleIntersect(cTopLeft, cBotRight, uBotRight))
				return true;
			return false;
		};

		// Check that we're not in danger of Storm, DWEB, EMP
		for (auto &command : myCommands) {
			if (command.tech == TechTypes::Psionic_Storm
				|| command.tech == TechTypes::Disruption_Web
				|| command.tech == TechTypes::EMP_Shockwave) {
				auto cTopLeft = command.pos - Position(48, 48);
				auto cBotRight = command.pos + Position(48, 48);
				
				if (checkCorners(cTopLeft, cBotRight))
					return true;
			}

			if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
				auto cTopLeft = command.pos - Position(320, 320);
				auto cBotRight = command.pos + Position(320, 320);

				if (checkCorners(cTopLeft, cBotRight))
					return true;
			}
		}

		for (auto &command : enemyCommands) {
			if (command.tech == TechTypes::Psionic_Storm
				|| command.tech == TechTypes::Disruption_Web
				|| command.tech == TechTypes::EMP_Shockwave) {
				auto cTopLeft = command.pos - Position(48, 48);
				auto cBotRight = command.pos + Position(48, 48);

				if (checkCorners(cTopLeft, cBotRight))
					return true;
			}
			if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
				auto cTopLeft = command.pos - Position(320, 320);
				auto cBotRight = command.pos + Position(320, 320);

				if (checkCorners(cTopLeft, cBotRight))
					return true;
			}
		}
		return false;
	}

	Position CommandManager::findViablePosition(UnitInfo& unit, function<double(WalkPosition)> score)
	{
		// Radius for checking tiles
		auto start = unit.getWalkPosition();
		auto radius = 4 + int(unit.getSpeed());
		auto unitHeight = int(unit.getType().height() / 8.0);
		auto unitWidth = int(unit.getType().width() / 8.0);
		auto mapWidth = Broodwar->mapWidth() * 4;
		auto mapHeight = Broodwar->mapHeight() * 4;
		auto offset = unit.getType().isFlyer() ? 12 : 0;

		// Boundaries
		auto left = max(start.x - radius, offset);
		auto right = min(start.x + radius + unitWidth, mapWidth - offset);
		auto top = max(start.y - radius, offset);
		auto bot = min(start.y + radius + unitHeight, mapHeight - offset);

		const auto viablePosition = [&](WalkPosition here) {
			Position p = Position(here) + Position(4, 4);

			// If not a flyer and position blocks a building, has collision or a splash threat
			if (!unit.getType().isFlyer() &&
				(Buildings().overlapsQueuedBuilding(unit.getType(), unit.getTilePosition()) || Grids().getESplash(here) > 0))
				return false;

			// If too close of a command, is in danger or isn't walkable
			if (p.getDistance(unit.getPosition()) > radius * 8
				|| Commands().isInDanger(unit, p)
				|| !Util().isWalkable(unit.getWalkPosition(), here, unit.getType()))
				return false;
			return true;
		};

		auto bestPosition = Positions::Invalid;
		auto best = 0.0;
		for (int x = left; x < right; x++) {
			for (int y = top; y < bot; y++) {
				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);

				//if (viablePosition(w))
					//Broodwar->drawBoxMap(Position(w), Position(w) + Position(9, 9), Colors::Black);

				auto current = score(w);
				if (current > best && viablePosition(w)) {
					bestPosition = p;
					best = current;
				}
			}
		}
		return bestPosition;
	}
}