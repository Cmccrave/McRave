#include "McRave.h"

void TerrainManager::onStart()
{
	mapBWEM.Initialize();
	mapBWEM.EnableAutomaticPathAnalysis();
	bool startingLocationsOK = mapBWEM.FindBasesForStartingLocations();
	assert(startingLocationsOK);
	playerStartingTilePosition = Broodwar->self()->getStartLocation();
	playerStartingPosition = Position(playerStartingTilePosition) + Position(64, 48);

	// Check if the map is an island map
	islandMap = false;
	for (auto &start : mapBWEM.StartingLocations()) {
		if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(playerStartingTilePosition)))
			islandMap = true;
	}

	// HACK: Play Plasma as an island map
	if (Broodwar->mapFileName().find("Plasma") != string::npos)
		islandMap = true;

	// Store non island bases	
	for (auto &area : mapBWEM.Areas()) {
		if (!islandMap && area.AccessibleNeighbours().size() == 0)
			continue;
		for (auto &base : area.Bases())
			allBases.insert(&base);
	}
}

void TerrainManager::onFrame()
{
	findEnemyStartingPosition();
	findEnemyNatural();
	findEnemyNextExpand();
	findAttackPosition();
	findDefendPosition();

	updateConcavePositions();
	updateAreas();
}

void TerrainManager::findEnemyStartingPosition()
{
	if (enemyStartingPosition.isValid())
		return;

	// Find closest enemy building
	for (auto &u : Units().getEnemyUnits()) {
		UnitInfo &unit = u.second;
		double distBest = 1280.0;
		TilePosition tileBest = TilePositions::Invalid;
		if (!unit.getType().isBuilding() || !unit.getTilePosition().isValid())
			continue;

		for (auto &start : Broodwar->getStartLocations()) {
			double dist = start.getDistance(unit.getTilePosition());
			if (dist < distBest)
				distBest = dist, tileBest = start;
		}
		if (tileBest.isValid() && tileBest != playerStartingTilePosition) {
			enemyStartingPosition = Position(tileBest) + Position(64, 48);
			enemyStartingTilePosition = tileBest;
		}
	}
}

void TerrainManager::findEnemyNatural()
{
	if (enemyNatural.isValid())
		return;

	// Find enemy natural area
	double distBest = DBL_MAX;
	for (auto &area : mapBWEM.Areas()) {
		for (auto &base : area.Bases()) {

			if (base.Geysers().size() == 0
				|| area.AccessibleNeighbours().size() == 0
				|| base.Center().getDistance(enemyStartingPosition) < 128)
				continue;

			double dist = BWEB::Map::getGroundDistance(base.Center(), enemyStartingPosition);
			if (dist < distBest) {
				distBest = dist;
				enemyNatural = base.Location();
			}
		}
	}
}

void TerrainManager::findEnemyNextExpand()
{
	double best = 0.0;
	for (auto &station : BWEB::Stations::getStations()) {

		UnitType shuttle = Broodwar->self()->getRace().getTransport();

		// Shuttle check for island bases, check enemy owned bases
		if (!station.BWEMBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea()) && Units().getEnemyCount(shuttle) <= 0)
			continue;
		if (BWEB::Map::getUsedTiles().find(station.BWEMBase()->Location()) != BWEB::Map::getUsedTiles().end())
			continue;
		if (Terrain().getEnemyStartingTilePosition() == station.BWEMBase()->Location())
			continue;

		// Get value of the expansion
		double value = 0.0;
		for (auto &mineral : station.BWEMBase()->Minerals())
			value += double(mineral->Amount());
		for (auto &gas : station.BWEMBase()->Geysers())
			value += double(gas->Amount());
		if (station.BWEMBase()->Geysers().size() == 0)
			value = value / 10.0;

		// Get distance of the expansion
		double distance;
		if (!station.BWEMBase()->GetArea()->AccessibleFrom(BWEB::Map::getMainArea()))
			distance = log(station.BWEMBase()->Center().getDistance(Terrain().getEnemyStartingPosition()));
		else if (!Players().vT())
			distance = BWEB::Map::getGroundDistance(Terrain().getEnemyStartingPosition(), station.BWEMBase()->Center()) / (BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), station.BWEMBase()->Center()));
		else
			distance = BWEB::Map::getGroundDistance(Terrain().getEnemyStartingPosition(), station.BWEMBase()->Center());

		double score = value / distance;

		if (score > best) {
			best = score;
			enemyExpand = (TilePosition)station.BWEMBase()->Center();
		}
	}
}

void TerrainManager::findAttackPosition()
{
	// HACK: Hitchhiker has issues pathing to enemy expansions, just attack the main for now
	if (Broodwar->mapFileName().find("Hitchhiker") != string::npos)
		attackPosition = enemyStartingPosition;

	// Attack possible enemy expansion location
	else if (enemyExpand.isValid() && !Broodwar->isExplored(enemyExpand) && !Broodwar->isExplored(enemyExpand + TilePosition(4, 3)))
		attackPosition = (Position)enemyExpand;

	// Attack furthest enemy station
	else if (MyStations().getEnemyStations().size() > 0) {
		double distBest = 0.0;
		Position posBest;

		for (auto &station : MyStations().getEnemyStations()) {
			auto &s = *station.second;
			double dist = Players().vP() ? 1.0 / enemyStartingPosition.getDistance(s.BWEMBase()->Center()) : enemyStartingPosition.getDistance(s.BWEMBase()->Center());
			if (dist >= distBest) {
				distBest = dist;
				posBest = s.BWEMBase()->Center();
			}
		}
		attackPosition = posBest;
	}

	// Attack enemy main
	else if (enemyStartingPosition.isValid() && !Broodwar->isExplored(enemyStartingTilePosition))
		attackPosition = enemyStartingPosition;
	else
		attackPosition = Positions::Invalid;
}

void TerrainManager::findDefendPosition()
{
	UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
	Position oldDefendPosition = defendPosition;
	reverseRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) < Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
	flatRamp = Broodwar->getGroundHeight(BWEB::Map::getMainTile()) == Broodwar->getGroundHeight(BWEB::Map::getNaturalTile());
	narrowNatural = BWEB::Map::getNaturalChoke() ? int(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end1).getDistance(BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end2)) / 4) <= 2 : false;
	defendNatural = BWEB::Map::getNaturalChoke() ? BuildOrder().buildCount(baseType) > 1 || Broodwar->self()->visibleUnitCount(baseType) > 1 || defendPosition == Position(BWEB::Map::getNaturalChoke()->Center()) || reverseRamp : false;

	if (islandMap) {
		defendPosition = BWEB::Map::getMainPosition();
		return;
	}

	// Defend our main choke if we're hiding tech
	if (BuildOrder().isHideTech() && BWEB::Map::getMainChoke() && Broodwar->self()->visibleUnitCount(baseType) == 1) {
		defendPosition = (Position)BWEB::Map::getMainChoke()->Center();
		return;
	}

	// Set mineral holding positions
	double distBest = DBL_MAX;
	for (auto &station : MyStations().getMyStations()) {
		auto &s = *station.second;
		double dist;
		if (Terrain().getEnemyStartingPosition().isValid())
			dist = BWEB::Map::getGroundDistance(Terrain().getEnemyStartingPosition(), Position(s.ResourceCentroid()));
		else
			dist = mapBWEM.Center().getDistance(Position(s.ResourceCentroid()));

		if (dist < distBest) {
			distBest = dist;
			mineralHold = (Position(s.ResourceCentroid()) + s.BWEMBase()->Center()) / 2;
			backMineralHold = (Position(s.ResourceCentroid()) - Position(s.BWEMBase()->Center())) + Position(s.ResourceCentroid());
		}
	}

	// Natural defending
	if (naturalWall) {
		Position door(naturalWall->getDoor());
		defendPosition = door.isValid() ? door : naturalWall->getCentroid();
		allyTerritory.insert(BWEB::Map::getNaturalArea());
		defendNatural = true;
	}
	else if (defendNatural) {
		defendPosition = Position(BWEB::Map::getNaturalChoke()->Center());
		allyTerritory.insert(BWEB::Map::getNaturalArea());

		// Move the defend position on maps like destination
		while (!Broodwar->isBuildable((TilePosition)defendPosition)) {
			double distBest = DBL_MAX;
			TilePosition test = (TilePosition)defendPosition;
			for (int x = test.x - 1; x <= test.x + 1; x++) {
				for (int y = test.y - 1; y <= test.y + 1; y++) {
					TilePosition t(x, y);
					if (!t.isValid())
						continue;

					double dist = Position(t).getDistance((Position)BWEB::Map::getNaturalArea()->Top());

					if (dist < distBest) {
						distBest = dist;
						defendPosition = (Position)t;
					}
				}
			}
		}
	}

	// Main defending
	else if (mainWall) {
		Position door(mainWall->getDoor());
		defendPosition = door.isValid() ? door : mainWall->getCentroid();
	}
	else
		defendPosition = Position(BWEB::Map::getMainChoke()->Center());

	// If enemy non 2 gate proxy, defend natural choke
	if (Strategy().enemyProxy() && Strategy().getEnemyBuild() != "P2Gate" && BWEB::Map::getNaturalChoke()) {
		defendPosition = Position(BWEB::Map::getNaturalChoke()->Center());
		allyTerritory.insert(BWEB::Map::getNaturalArea());
	}

	// If this isn't the same as the last position, make new concave positions
	if (defendPosition != oldDefendPosition)
		chokePositions.clear();
}

void TerrainManager::updateConcavePositions()
{
	for (auto tile : chokePositions) {
		//Broodwar->drawCircleMap(Position(tile), 8, Colors::Blue);

	}

	if (!chokePositions.empty() || Broodwar->getFrameCount() < 100 || defendPosition == mineralHold)
		return;

	const auto addPlacements =[&](Line line, BWEM::ChokePoint const * choke, double gap) {

		auto xStart = 4 + (choke->Pos(choke->end1).x <= choke->Pos(choke->end2).x ? Position(choke->Pos(choke->end1)).x : Position(choke->Pos(choke->end2)).x);
		int yStart = int(line.y(xStart));
		bool firstPlaced = false;
		auto current = Position(xStart, yStart);
		Position last = Positions::Invalid;

		// Doesn't work for vertical lines because we increment by 1
		while (current.isValid() && (mapBWEM.GetMiniTile(WalkPosition(current)).Walkable() || !firstPlaced)) {
			WalkPosition w(current);

			if (last.getDistance(current) > gap && mapBWEM.GetMiniTile(w).Walkable()) {
				Broodwar->drawCircleMap(current, 4, Colors::Green);
				last = current;
				firstPlaced = true;
			}
			//Broodwar->drawCircleMap(current, 1, Colors::Red, true);

			xStart += 1;
			yStart = int(line.y(xStart));
			current = Position(xStart, yStart);
		}
		return;
	};


	// Testing formations - broken as fuck at the moment
	if (false) {
		auto choke = defendNatural ? BWEB::Map::getNaturalChoke() : BWEB::Map::getMainChoke();
		auto width = Util().chokeWidth(choke);

		// 1) Get line of best fit
		auto line = Util().lineOfBestFit(choke);

		// 2) Make 2 parallel lines
		auto line2 = Util().parallelLine(line, 16.0);
		auto line3 = Util().parallelLine(line, -16.0);

		// 3) Add placements
		addPlacements(line, choke, 19.0);
		addPlacements(line2, choke, 19.0);
		addPlacements(line3, choke, 19.0);
	}
	else {
		// Setup parameters
		int min = 0;
		int max = 480;
		double distBest = DBL_MAX;
		WalkPosition center = WalkPosition(defendPosition);
		Position bestPosition = Positions::None;

		const auto tooClose =[&](Position p) {
			for (auto position : chokePositions) {
				if (position.getDistance(p) < 32.0)
					return true;
			}
			return false;
		};

		const auto checkValid = [&](WalkPosition w) {
			TilePosition t(w);
			Position p = Position(w) + Position(4, 4);

			double dist = p.getDistance(Position(center)) / log(p.getDistance(BWEB::Map::getMainPosition()));

			if (!w.isValid()
				|| !Util().isWalkable(w, w, UnitTypes::Protoss_Dragoon)
				|| p.getDistance(Position(center)) < min
				|| p.getDistance(Position(center)) > max
				|| tooClose(p)
				|| (!isInAllyTerritory(t))
				|| p.getDistance(mineralHold) < 128.0
				|| BWEB::Map::getGroundDistance(p, BWEB::Map::getMainPosition()) > BWEB::Map::getGroundDistance(Position(center), BWEB::Map::getMainPosition()))
				return false;
			return true;
		};

		// Find a position around the center that is suitable
		for (int x = center.x - 40; x <= center.x + 40; x++) {
			for (int y = center.y - 40; y <= center.y + 40; y++) {
				WalkPosition w(x, y);
				if (checkValid(w))
					chokePositions.push_back(Position(w));
			}
		}
	}
}

void TerrainManager::updateAreas()
{
	// Squish areas
	if (BWEB::Map::getNaturalArea()) {

		// Add "empty" areas (ie. Andromeda areas around main)	
		for (auto &area : BWEB::Map::getNaturalArea()->AccessibleNeighbours()) {
			for (auto &choke : area->ChokePoints()) {
				if (choke->Center() == BWEB::Map::getMainChoke()->Center())
					allyTerritory.insert(area);
			}
		}

		UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
		if ((BuildOrder().buildCount(baseType) >= 2 || BuildOrder().isWallNat()) && BWEB::Map::getNaturalArea())
			allyTerritory.insert(BWEB::Map::getNaturalArea());

		// HACK: Add to my territory if chokes are shared
		if (BWEB::Map::getMainChoke() == BWEB::Map::getNaturalChoke())
			allyTerritory.insert(BWEB::Map::getNaturalArea());
	}
}



bool TerrainManager::findNaturalWall(vector<UnitType>& types, const vector<UnitType>& defenses)
{
	// Hack: Make a bunch of walls as Zerg for testing - disabled atm
	if (Broodwar->self()->getRace() == Races::Zerg) {
		for (auto &area : mapBWEM.Areas()) {

			// Only make walls at gas bases that aren't starting bases
			bool invalidBase = false;
			for (auto &base : area.Bases()) {
				if (base.Starting())
					invalidBase = true;
			}
			if (invalidBase)
				continue;

			const ChokePoint * bestChoke = nullptr;
			double distBest = DBL_MAX;
			for (auto &choke : area.ChokePoints()) {
				auto dist = Position(choke->Center()).getDistance(mapBWEM.Center());
				if (dist < distBest) {
					distBest = dist;
					bestChoke = choke;
				}
			}
			BWEB::Walls::createWall(types, &area, bestChoke, UnitTypes::None, defenses);

			if (&area == BWEB::Map::getNaturalArea())
				naturalWall = BWEB::Walls::getWall(BWEB::Map::getNaturalArea());
		}
		return true;
	}

	else {
		if (naturalWall)
			return true;

		UnitType wallTight;
		bool reservePath = Broodwar->self()->getRace() != Races::Terran;

		if (Broodwar->self()->getRace() == Races::Terran && Players().vP())
			wallTight = UnitTypes::Protoss_Zealot;
		else if (Players().vZ())
			wallTight = UnitTypes::Zerg_Zergling;
		else
			wallTight = UnitTypes::None;

		// Create a wall
		BWEB::Walls::createWall(types, BWEB::Map::getNaturalArea(), BWEB::Map::getNaturalChoke(), wallTight, defenses, reservePath);
		naturalWall = BWEB::Walls::getWall(BWEB::Map::getNaturalArea());

		if (naturalWall)
			return true;
	}
	return false;
}

bool TerrainManager::findMainWall(vector<UnitType>& types, const vector<UnitType>& defenses)
{
	if (BWEB::Walls::getWall(BWEB::Map::getMainArea(), BWEB::Map::getMainChoke()) || BWEB::Walls::getWall(BWEB::Map::getNaturalArea(), BWEB::Map::getMainChoke()))
		return true;
	UnitType wallTight = Broodwar->enemy()->getRace() == Races::Protoss ? UnitTypes::Protoss_Zealot : UnitTypes::Zerg_Zergling;

	BWEB::Walls::createWall(types, BWEB::Map::getMainArea(), BWEB::Map::getMainChoke(), wallTight, defenses, false, true);
	BWEB::Walls::Wall * wallB = BWEB::Walls::getWall(BWEB::Map::getMainArea(), BWEB::Map::getMainChoke());
	if (wallB) {
		mainWall = wallB;
		return true;
	}

	BWEB::Walls::createWall(types, BWEB::Map::getNaturalArea(), BWEB::Map::getMainChoke(), wallTight, defenses, false, true);
	BWEB::Walls::Wall * wallA = BWEB::Walls::getWall(BWEB::Map::getNaturalArea(), BWEB::Map::getMainChoke());
	if (wallA) {
		mainWall = wallA;
		return true;
	}
	return false;
}

bool TerrainManager::isInAllyTerritory(TilePosition here)
{
	// Find the area of this tile position and see if it exists in ally territory
	if (here.isValid() && mapBWEM.GetArea(here) && allyTerritory.find(mapBWEM.GetArea(here)) != allyTerritory.end())
		return true;
	return false;
}

bool TerrainManager::isInEnemyTerritory(TilePosition here)
{
	// Find the area of this tile position and see if it exists in enemy territory
	if (here.isValid() && mapBWEM.GetArea(here) && enemyTerritory.find(mapBWEM.GetArea(here)) != enemyTerritory.end())
		return true;
	return false;
}

bool TerrainManager::isStartingBase(TilePosition here)
{
	for (auto tile : Broodwar->getStartLocations()) {
		if (here.getDistance(tile) < 4)
			return true;
	}
	return false;
}

Position TerrainManager::closestUnexploredStart() {
	double distBest = DBL_MAX;
	Position posBest = Positions::None;
	for (auto &tile : mapBWEM.StartingLocations()) {
		Position center = Position(tile) + Position(64, 48);
		double dist = center.getDistance(BWEB::Map::getMainPosition());

		if (!Broodwar->isExplored(tile) && dist < distBest) {
			distBest = dist;
			posBest = center;
		}
	}
	return posBest;
}

Position TerrainManager::randomBasePosition()
{
	int random = rand() % (Terrain().getAllBases().size());
	int i = 0;
	for (auto &base : Terrain().getAllBases()) {
		if (i == random)
			return base->Center();
		else
			i++;
	}
	return BWEB::Map::getMainPosition();
}