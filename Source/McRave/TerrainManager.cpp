#include "McRave.h"

void TerrainManager::onFrame()
{
	Display().startClock();
	updateTerrain();
	Display().performanceTest(__FUNCTION__);
	return;
}

void TerrainManager::updateTerrain()
{
	if (!enemyStartingPosition.isValid())
		findEnemyStartingPosition();

	else if (!enemyNatural.isValid())
		findEnemyNatural();

	findAttackPosition();
	findDefendPosition();
	findEnemyNextExpand();

	// Squish areas
	if (mapBWEB.getNaturalArea()) {

		// Add "empty" areas (ie. Andromeda areas around main)	
		for (auto &area : mapBWEB.getNaturalArea()->AccessibleNeighbours()) {
			for (auto &choke : area->ChokePoints()) {
				if (choke->Center() == mapBWEB.getMainChoke()->Center() && allyTerritory.find(area) == allyTerritory.end())
					allyTerritory.insert(area);
			}
		}

		UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
		if ((BuildOrder().buildCount(baseType) >= 2 || BuildOrder().isWallNat()) && mapBWEB.getNaturalArea())
			allyTerritory.insert(mapBWEB.getNaturalArea());

		if (mapBWEB.getMainChoke() == mapBWEB.getNaturalChoke())
			allyTerritory.insert(mapBWEB.getNaturalArea());
	}

	// Check if enemy is walled
	if (enemyStartingTilePosition.isValid() && enemyNatural.isValid() && !enemyWall) {
		return;

		auto &enemyPath = mapBWEB.findPath(mapBWEB, enemyStartingTilePosition, enemyNatural, true, true, false, false);
		if (enemyPath.empty())
			enemyWall = true;

		if (enemyWall)
			Broodwar << "Detected enemy wall" << endl;
	}

}

void TerrainManager::findEnemyStartingPosition()
{
	// Find closest enemy building
	for (auto &u : Units().getEnemyUnits()) {
		UnitInfo &unit = u.second;
		double distBest = DBL_MAX;
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
	// Find enemy natural area
	double distBest = DBL_MAX;
	for (auto &area : mapBWEM.Areas()) {
		for (auto &base : area.Bases()) {

			if (base.Geysers().size() == 0
				|| area.AccessibleNeighbours().size() == 0
				|| base.Center().getDistance(enemyStartingPosition) < 128)
				continue;

			double dist = mapBWEB.getGroundDistance(base.Center(), enemyStartingPosition);
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
	for (auto &station : mapBWEB.Stations()) {

		UnitType shuttle = Broodwar->self()->getRace().getTransport();

		// Shuttle check for island bases, check enemy owned bases
		if (!station.BWEMBase()->GetArea()->AccessibleFrom(mapBWEB.getMainArea()) && Units().getEnemyCount(shuttle) <= 0)
			continue;
		if (mapBWEB.getUsedTiles().find(station.BWEMBase()->Location()) != mapBWEB.getUsedTiles().end())
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
		if (!station.BWEMBase()->GetArea()->AccessibleFrom(mapBWEB.getMainArea()))
			distance = log(station.BWEMBase()->Center().getDistance(Terrain().getEnemyStartingPosition()));
		else if (Broodwar->enemy()->getRace() != Races::Terran)
			distance = mapBWEB.getGroundDistance(Terrain().getEnemyStartingPosition(), station.BWEMBase()->Center()) / (mapBWEB.getGroundDistance(mapBWEB.getMainPosition(), station.BWEMBase()->Center()));
		else
			distance = mapBWEB.getGroundDistance(Terrain().getEnemyStartingPosition(), station.BWEMBase()->Center());

		double score = value / distance;

		if (score > best) {
			best = score;
			enemyExpand = (TilePosition)station.BWEMBase()->Center();
		}
	}
}

void TerrainManager::findAttackPosition()
{
	// Attack possible enemy expansion location
	if (enemyExpand.isValid() && !Broodwar->isExplored(enemyExpand) && !Broodwar->isExplored(enemyExpand + TilePosition(4, 3)))
		attackPosition = (Position)enemyExpand;

	// Attack furthest enemy station
	else if (Stations().getEnemyStations().size() > 0) {
		double closestD = 0.0;
		Position closestP;
		for (auto &station : Stations().getEnemyStations()) {
			Station s = station.second;
			if (enemyStartingPosition.getDistance(s.BWEMBase()->Center()) >= closestD) {
				closestD = enemyStartingPosition.getDistance(s.BWEMBase()->Center());
				closestP = s.BWEMBase()->Center();
			}
		}
		attackPosition = closestP;
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

	if (islandMap) {
		defendPosition = mapBWEB.getMainPosition();
		return;
	}

	// Set mineral holding positions
	double distBest = DBL_MAX;
	for (auto &station : Stations().getMyStations()) {
		Station s = station.second;
		double dist;
		if (Terrain().getEnemyStartingPosition().isValid())
			dist = mapBWEB.getGroundDistance(Terrain().getEnemyStartingPosition(), Position(s.ResourceCentroid()));
		else
			dist = mapBWEB.getGroundDistance(mapBWEM.Center(), Position(s.ResourceCentroid()));

		if (dist < distBest) {
			distBest = dist;
			mineralHold = (Position(s.ResourceCentroid()) + s.BWEMBase()->Center()) / 2;
			backMineralHold = (Position(s.ResourceCentroid()) - Position(s.BWEMBase()->Center())) + Position(s.ResourceCentroid());
		}
	}

	if (Strategy().enemyProxy()) {
		defendPosition = Position(mapBWEB.getNaturalChoke()->Center());
	}

	else if (wall = mapBWEB.getWall(mapBWEB.getNaturalArea())) {
		/*Position door(wall->getDoor());
		if (door.isValid())
			defendPosition = door;
		else 	*/
		defendPosition = wall->getCentroid();
	}
	else if (!Strategy().defendChoke())
		defendPosition = mineralHold;
	else if (mapBWEB.getMainChoke() && Strategy().defendChoke() && BuildOrder().buildCount(baseType) <= 1)
		defendPosition = Position(mapBWEB.getMainChoke()->Center());
	else if (mapBWEB.getNaturalChoke() && Strategy().defendChoke() && BuildOrder().buildCount(baseType) >= 2)
		defendPosition = Position(mapBWEB.getNaturalChoke()->Center());
}

bool TerrainManager::findNaturalWall(vector<UnitType>& types, const vector<UnitType>& defenses)
{
	if (wall)
		return true;
	UnitType wallTight = Broodwar->enemy()->getRace() == Races::Protoss ? UnitTypes::Protoss_Zealot : UnitTypes::Zerg_Zergling;

	if (Broodwar->self()->getRace() == Races::Protoss) {
		mapBWEB.createWall(types, mapBWEB.getNaturalArea(), mapBWEB.getNaturalChoke(), UnitTypes::None, defenses, true);
		wall = mapBWEB.getWall(mapBWEB.getNaturalArea());
	}

	else if (Broodwar->self()->getRace() == Races::Terran) {
		mapBWEB.createWall(types, mapBWEB.getNaturalArea(), mapBWEB.getNaturalChoke(), wallTight, defenses);
		wall = mapBWEB.getWall(mapBWEB.getNaturalArea());
	}

	else if (Broodwar->self()->getRace() == Races::Zerg) {
		mapBWEB.createWall(types, mapBWEB.getNaturalArea(), mapBWEB.getNaturalChoke(), wallTight, defenses, true);
		wall = mapBWEB.getWall(mapBWEB.getNaturalArea());
	}

	if (wall)
		return true;

	return false;
}

bool TerrainManager::findMainWall(vector<UnitType>& types, const vector<UnitType>& defenses)
{
	if (mapBWEB.getWall(mapBWEB.getMainArea(), mapBWEB.getMainChoke()) || mapBWEB.getWall(mapBWEB.getNaturalArea(), mapBWEB.getMainChoke()))
		return true;
	UnitType wallTight = Broodwar->enemy()->getRace() == Races::Protoss ? UnitTypes::Protoss_Zealot : UnitTypes::Zerg_Zergling;

	mapBWEB.createWall(types, mapBWEB.getMainArea(), mapBWEB.getMainChoke(), wallTight, defenses, false, true);
	Wall * wallB = mapBWEB.getWall(mapBWEB.getMainArea(), mapBWEB.getMainChoke());
	if (!wallB)
		return false;
	else {
		wall = wallB;
		return true;
	}

	mapBWEB.createWall(types, mapBWEB.getNaturalArea(), mapBWEB.getMainChoke(), wallTight, defenses, false, true);
	Wall * wallA = mapBWEB.getWall(mapBWEB.getNaturalArea(), mapBWEB.getMainChoke());
	if (!wallA)
		return false;
	else {
		wall = wallA;
		return true;
	}
}

bool TerrainManager::overlapsBases(TilePosition here)
{
	for (auto base : allBases)
	{
		TilePosition tile = base->Location();
		if (here.x >= tile.x && here.x < tile.x + 4 && here.y >= tile.y && here.y < tile.y + 3) return true;
	}
	return false;
}

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
		if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(playerStartingTilePosition))) {
			islandMap = true;
			Broodwar << "Island map? Really?" << endl;
			break;
		}
	}

	// Store non island bases	
	for (auto &area : mapBWEM.Areas()) {
		if (!islandMap && area.AccessibleNeighbours().size() == 0)
			continue;
		for (auto &base : area.Bases()) {
			allBases.insert(&base);
		}
	}
	return;
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

bool TerrainManager::overlapsNeutrals(TilePosition here)
{
	for (auto &m : Resources().getMyMinerals()) {
		ResourceInfo &mineral = m.second;
		if (here.x >= mineral.getTilePosition().x && here.x < mineral.getTilePosition().x + 2 && here.y >= mineral.getTilePosition().y && here.y < mineral.getTilePosition().y + 1)
			return true;
	}

	for (auto &g : Resources().getMyGas()) {
		ResourceInfo &gas = g.second;
		if (here.x >= gas.getTilePosition().x && here.x < gas.getTilePosition().x + 4 && here.y >= gas.getTilePosition().y && here.y < gas.getTilePosition().y + 2)
			return true;
	}
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