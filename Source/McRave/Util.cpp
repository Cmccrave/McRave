#include "McRave.h"

bool UtilManager::isWalkable(WalkPosition start, WalkPosition end, UnitType unitType)
{
	int walkWidth = (int)ceil(unitType.width() / 8.0) + 1;
	int walkHeight = (int)ceil(unitType.height() / 8.0) + 1;
	int halfW = walkWidth / 2;
	int halfH = walkHeight / 2;

	if (unitType.isFlyer()) {
		halfW += 2;
		halfH += 2;
	}

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
			if (!unitType.isFlyer()) {
				if (overlapsUnit(x, y) && Grids().getMobility(w) > 0)
					continue;
				if (Grids().getMobility(w) <= 0 || Grids().getCollision(w) > 0)
					return false;
			}
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


bool UtilManager::proactivePullWorker(UnitInfo& unit)
{
	if (Broodwar->self()->getRace() == Races::Protoss) {
		int completedDefenders = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot);
		int visibleDefenders = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot);

		if (unit.getType() == UnitTypes::Protoss_Probe && unit.getShields() < 20)
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
	else if (Broodwar->self()->getRace() == Races::Zerg)
		return false;

	if (Strategy().enemyProxy() && Strategy().getEnemyBuild() != "P2Gate" && Units().getImmThreat() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense())
		return true;

	return false;
}

bool UtilManager::reactivePullWorker(UnitInfo& unit)
{
	if (Units().getEnemyCount(UnitTypes::Terran_Vulture) > 0)
		return false;

	if (unit.getType() == UnitTypes::Protoss_Probe) {
		if (unit.getShields() < 8)
			return false;
	}

	const BWEB::Station * station = mapBWEB.getClosestStation(unit.getTilePosition());
	if (station && station->ResourceCentroid().getDistance(unit.getPosition()) < 160.0) {
		if (Terrain().isInAllyTerritory(unit.getTilePosition()) && Grids().getEGroundThreat(unit.getWalkPosition()) > 0.0 && Broodwar->getFrameCount() < 10000)
			return true;
	}

	// If we have no combat units and there is a threat
	if (Units().getImmThreat() > Units().getGlobalAllyGroundStrength() + Units().getAllyDefense()) {
		if (Broodwar->self()->getRace() == Races::Protoss) {
			if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) == 0)
				return true;
		}
		else if (Broodwar->self()->getRace() == Races::Zerg) {
			if (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Zergling) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Mutalisk) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Hydralisk) == 0)
				return true;
		}
		else if (Broodwar->self()->getRace() == Races::Terran) {
			if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture) == 0)
				return true;
		}
	}
	return false;
}

bool UtilManager::pullRepairWorker(UnitInfo& unit)
{
	if (Broodwar->self()->getRace() == Races::Terran) {
		int mechUnits = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Goliath)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Wraith)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Battlecruiser)
			+ Broodwar->self()->completedUnitCount(UnitTypes::Terran_Valkyrie);

		//if ((mechUnits > 0 && Units().getRepairWorkers() < Units().getSupply() / 30)
		//	|| (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) > 0 && BuildOrder().isFastExpand() && Units().getRepairWorkers() < 2))
		//	return true;
	}
	return false;
}


double UtilManager::getHighestThreat(WalkPosition here, UnitInfo& unit)
{
	// Determine highest threat possible here
	auto t = unit.getType();
	auto highest = MIN_THREAT;
	auto dx = int(ceil(t.width() / 16.0));		// Half walk resolution width
	auto dy = int(ceil(t.height() / 16.0));		// Half walk resolution height

	for (int x = here.x - dx; x < here.x + dx; x++) {
		for (int y = here.y - dy; y < here.y + dy; y++) {
			WalkPosition w(x, y);
			if (!w.isValid())
				continue;

			auto grd = Grids().getEGroundThreat(w);
			auto air = Grids().getEAirThreat(w);
			auto current = unit.getType().isFlyer() ? air : grd;
			highest = (current > highest) ? current : highest;
		}
	}
	return highest;
}

bool UtilManager::accurateThreatOnPath(UnitInfo& unit, Path& path)
{
	if (path.getTiles().empty())
		return false;

	for (auto &tile : path.getTiles()) {
		auto w = WalkPosition(tile);
		auto threat = unit.getType().isFlyer() ? Grids().getEAirThreat(w) > 0.0 : Grids().getEGroundThreat(w) > 0.0;
		if (threat)
			return true;
	}
	return false;
}

bool UtilManager::rectangleIntersect(Position topLeft, Position botRight, Position target)
{
	if (target.x >= topLeft.x
		&& target.x < botRight.x
		&& target.y >= topLeft.y
		&& target.y < botRight.y)
		return true;
	return false;
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

UnitInfo * UtilManager::getClosestUnit(UnitInfo& source, Player p, UnitType t) {
	double distBest = DBL_MAX;
	UnitInfo* best = nullptr;
	auto &units = (p == Broodwar->self()) ? Units().getMyUnits() : Units().getEnemyUnits();

	for (auto &u : units) {
		UnitInfo &unit = u.second;

		if (!unit.unit() || source.unit() == unit.unit() || (t != UnitTypes::None && unit.getType() != t))
			continue;

		double dist = source.getPosition().getDistance(unit.getPosition());
		if (dist < distBest) {
			best = &unit;
			distBest = dist;
		}
	}
	return best;
}

UnitInfo * UtilManager::getClosestThreat(UnitInfo& unit)
{
	double distBest = DBL_MAX;
	UnitInfo* best = nullptr;
	auto &units = (unit.getPlayer() == Broodwar->self()) ? Units().getEnemyUnits() : Units().getMyUnits();

	for (auto &t : units) {
		UnitInfo &threat = t.second;
		auto canAttack = unit.getType().isFlyer() ? threat.getAirDamage() > 0.0 : threat.getGroundDamage() > 0.0;

		if (!threat.unit() || !canAttack)
			continue;

		double dist = threat.getPosition().getDistance(unit.getPosition());
		if (dist < distBest) {
			best = &threat;
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

		if (!unit.unit() || unit.getRole() != Role::Working || unit.getBuildPosition().isValid() || (unit.hasResource() && !unit.getResource().getType().isMineralField()))
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

void UtilManager::lineOfbestFit(const BWEM::ChokePoint * choke)
{
	auto sumX = 0.0, sumY = 0.0;
	auto sumXY = 0.0, sumX2 = 0.0;
	for (auto geo : choke->Geometry()) {
		Position p = Position(geo) + Position(4, 4);
		sumX += p.x;
		sumY += p.y;
		sumXY += p.x * p.y;
		sumX2 += p.x * p.x;
	}
	double xMean = sumX / choke->Geometry().size();
	double yMean = sumY / choke->Geometry().size();
	double denominator = sumX2 - sumX * xMean;

	double slope = (sumXY - sumX * yMean) / denominator;
	double yInt = yMean - slope * xMean;

	// y = mx + b
	// solve for x and y

	// end 1
	int x1 = Position(choke->Pos(choke->end1)).x;
	int y1 = int(ceil(x1 * slope)) + yInt;
	Position p1 = Position(x1, y1);

	// end 2
	int x2 = Position(choke->Pos(choke->end2)).x;
	int y2 = int(ceil(x2 * slope)) + yInt;
	Position p2 = Position(x2, y2);

	Broodwar->drawLineMap(p1, p2, Colors::Red);
}

Position UtilManager::getConcavePosition(UnitInfo& unit, BWEM::Area const * area, Position here)
{
	// Setup parameters
	int min = int(unit.getGroundRange());
	double distBest = DBL_MAX;
	WalkPosition center = WalkPositions::None;
	Position bestPosition = Positions::None;

	// HACK: I found my reavers getting picked off too often when they held too close
	if (unit.getType() == UnitTypes::Protoss_Reaver)
		min += 64;

	// Finds which position we are forming the concave at
	const auto getConcaveCenter = [&]() {
		if (here.isValid())
			center = (WalkPosition)here;
		else if (area == mapBWEB.getNaturalArea() && mapBWEB.getNaturalChoke())
			center = mapBWEB.getNaturalChoke()->Center();
		else if (area == mapBWEB.getMainArea() && mapBWEB.getMainChoke())
			center = mapBWEB.getMainChoke()->Center();

		else if (area) {
			for (auto &c : area->ChokePoints()) {
				double dist = mapBWEB.getGroundDistance(Position(c->Center()), Terrain().getEnemyStartingPosition());
				if (dist < distBest) {
					distBest = dist;
					center = c->Center();
				}
			}
		}
	};

	const auto checkbest = [&](WalkPosition w) {
		TilePosition t(w);
		Position p = Position(w) + Position(4, 4);

		double dist = p.getDistance(Position(center)) * log(p.getDistance(mapBWEB.getMainPosition()));

		if (!w.isValid()
			|| !Util().isWalkable(unit.getWalkPosition(), w, unit.getType())
			|| (here != Terrain().getDefendPosition() && area && mapBWEM.GetArea(t) != area)
			|| (unit.getGroundRange() > 32.0 && p.getDistance(Position(center)) < min)
			|| Buildings().overlapsQueuedBuilding(unit.getType(), t)
			|| dist > distBest
			|| Commands().overlapsCommands(unit.unit(), UnitTypes::None, p, 8)
			|| (unit.getType() == UnitTypes::Protoss_Reaver && Terrain().isDefendNatural() && mapBWEM.GetArea(w) != mapBWEB.getNaturalArea()))
			return false;

		bestPosition = p;
		distBest = dist;
		return true;
	};

	// Find the center
	getConcaveCenter();

	// If this is the defending position, grab from a vector we already made
	// TODO: generate a vector for every choke and store as a map<Choke, vector<Position>>?
	if (here == Terrain().getDefendPosition()) {
		for (auto &position : Terrain().getChokePositions()) {
			checkbest(WalkPosition(position));
		}
	}

	// Find a position around the center that is suitable
	else {
		for (int x = center.x - 40; x <= center.x + 40; x++) {
			for (int y = center.y - 40; y <= center.y + 40; y++) {
				WalkPosition w(x, y);
				checkbest(w);
			}
		}
	}
	return bestPosition;
}
