#include "McRave.h"

namespace McRave
{
	void BuildingManager::onFrame()
	{
		Display().startClock();
		updateBuildings();
		queueBuildings();
		Display().performanceTest(__FUNCTION__);
	}

	void BuildingManager::updateBuildings()
	{
		// Reset some counters
		poweredSmall = 0; poweredMedium = 0; poweredLarge = 0;
		nukesAvailable = 0;

		// Add up how many powered available spots we have		
		for (auto &block : mapBWEB.Blocks()) {
			for (auto &tile : block.SmallTiles()) {
				if (Pylons().hasPower(tile, UnitTypes::Protoss_Photon_Cannon) && isBuildable(UnitTypes::Protoss_Photon_Cannon, tile) && isQueueable(UnitTypes::Protoss_Photon_Cannon, tile))
					poweredSmall++;
			}
			for (auto &tile : block.MediumTiles()) {
				if (Pylons().hasPower(tile, UnitTypes::Protoss_Forge) && isBuildable(UnitTypes::Protoss_Forge, tile) && isQueueable(UnitTypes::Protoss_Forge, tile))
					poweredMedium++;
			}
			for (auto &tile : block.LargeTiles()) {
				if (Pylons().hasPower(tile, UnitTypes::Protoss_Gateway) && isBuildable(UnitTypes::Protoss_Gateway, tile) && isQueueable(UnitTypes::Protoss_Gateway, tile))
					poweredLarge++;
			}
		}
		
		for (auto &b : myBuildings) {
			BuildingInfo &building = b.second;
			storeBuilding(b.second.unit());
			updateCommands(building);
		}
	}

	void BuildingManager::updateCommands(BuildingInfo& building)
	{
		// Lair morphing
		if (building.getType() == UnitTypes::Zerg_Hatchery && BuildOrder().buildCount(UnitTypes::Zerg_Lair) > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive))
			building.unit()->morph(UnitTypes::Zerg_Lair);		

		// Hive morphing
		else if (building.getType() == UnitTypes::Zerg_Lair && BuildOrder().buildCount(UnitTypes::Zerg_Hive) > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive))
			building.unit()->morph(UnitTypes::Zerg_Hive);	

		// Greater Spire morphing
		else if (building.getType() == UnitTypes::Zerg_Spire && BuildOrder().buildCount(UnitTypes::Zerg_Greater_Spire) > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Greater_Spire))
			building.unit()->morph(UnitTypes::Zerg_Greater_Spire);

		// Sunken morphing
		else if (building.getType() == UnitTypes::Zerg_Creep_Colony)
			building.unit()->morph(UnitTypes::Zerg_Sunken_Colony);

		// Terran building needs new scv
		else if (building.getType().getRace() == Races::Terran && !building.unit()->isCompleted() && !building.unit()->getBuildUnit()) {
			Unit builder = Workers().getClosestWorker(building.getPosition(), true);
			if (builder)
				builder->rightClick(building.unit());
		}
		
		// Nuke counting
		else if (building.getType() == UnitTypes::Terran_Nuclear_Silo && building.unit()->hasNuke())
			nukesAvailable++;

		// Barracks
		if (building.getType() == UnitTypes::Terran_Barracks) {

			// Wall lift
			if (building.getPosition().getDistance(Terrain().getDefendPosition()) < 128 && !building.unit()->isFlying()) {
				if (Units().getSupply() > 120 && BuildOrder().firstReady()) {
					building.unit()->lift();
					mapBWEB.onUnitDestroy(building.unit());
				}
			}

			// Find landing location as production building
			else if (BuildOrder().firstReady() && building.unit()->isFlying()) {
				TilePosition here = findProdLocation(building.getType(), mapBWEB.getMainPosition());
				Position center = (Position)here + Position(building.getType().tileWidth() * 16, building.getType().tileHeight() * 16);

				if (building.unit()->getLastCommand().getType() != UnitCommandTypes::Land || building.unit()->getLastCommand().getTargetTilePosition() != here)
					building.unit()->land(here);
			}

			// Add used tiles back to grid
			else if (!building.unit()->isFlying() && building.unit()->getLastCommand().getType() == UnitCommandTypes::Land)
				mapBWEB.onUnitDiscover(building.unit());
		}
		
		// Comsat scans
		if (building.getType() == UnitTypes::Terran_Comsat_Station)	{
			if (building.getEnergy() >= TechTypes::Scanner_Sweep.energyCost()) {
				UnitInfo* enemy = Units().getClosestInvisEnemy(building);
				if (enemy && enemy->unit() && enemy->unit()->exists()) {
					building.unit()->useTech(TechTypes::Scanner_Sweep, enemy->getPosition());
					Commands().addCommand(enemy->getPosition(), UnitTypes::Spell_Scanner_Sweep);
				}
			}
		}
	}

	void BuildingManager::queueBuildings()
	{
		queuedMineral = 0;
		queuedGas = 0;
		buildingsQueued.clear();

		// Add up how many buildings we have assigned to workers
		for (auto &worker : Workers().getMyWorkers()) {
			WorkerInfo &w = worker.second;
			if (w.getBuildingType().isValid() && w.getBuildPosition().isValid() && (w.getBuildingType().isRefinery() || mapBWEB.getUsedTiles().find(w.getBuildPosition()) == mapBWEB.getUsedTiles().end())) {
				buildingsQueued[w.getBuildPosition()] = w.getBuildingType();
			}
		}

		// Add up how many more buildings of each type we need
		for (auto &item : BuildOrder().getItemQueue()) {
			UnitType building = item.first;
			Item i = item.second;
			int bQueued = 0;
			int bDesired = i.getActualCount();

			// If the building morphed from another building type, add the visible amount of child type to the parent type
			// i.e. When we have a queued Hatchery, we need to check how many Lairs and Hives we have.
			int offset = 0;
			if (building == UnitTypes::Zerg_Creep_Colony)
				offset = Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Sunken_Colony);
			if (building == UnitTypes::Zerg_Hatchery)
				offset = Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive);

			if (building == UnitTypes::Zerg_Lair)
				continue;

			// Reserve building if our reserve count is higher than our visible count
			for (auto &queued : buildingsQueued) {
				if (queued.second == item.first) {
					bQueued++;
					if (i.getReserveCount() > Broodwar->self()->visibleUnitCount(building) + offset) {
						queuedMineral += building.mineralPrice();
						queuedGas += building.gasPrice();
					}
				}
			}

			// Queue building if our actual count is higher than our visible count
			if (!building.isAddon() && bDesired > (bQueued + Broodwar->self()->visibleUnitCount(building) + offset)) {
				TilePosition here = getBuildLocation(building);
				Unit builder = Workers().getClosestWorker(Position(here), true);

				if (here.isValid() && builder) {
					Workers().getMyWorkers()[builder].setBuildingType(building);
					Workers().getMyWorkers()[builder].setBuildPosition(here);
					buildingsQueued[here] = building;
				}
			}
		}
	}

	void BuildingManager::storeBuilding(Unit building)
	{
		BuildingInfo &b = myBuildings[building];
		b.setUnit(building);
		b.setType(building->getType());
		b.setEnergy(building->getEnergy());
		b.setPosition(building->getPosition());
		b.setWalkPosition(Util().getWalkPosition(building));
		b.setTilePosition(building->getTilePosition());
	}

	void BuildingManager::removeBuilding(Unit building)
	{
		myBuildings.erase(building);
	}

	TilePosition BuildingManager::getBuildLocation(UnitType building)
	{
		TilePosition here = TilePositions::Invalid;

		// If this is a Resource Depot, Zerg can place it as a production building or expansion, Protoss and Terran only expand
		if (building.isResourceDepot()) {
			if (Broodwar->self()->getRace() == Races::Zerg && mapBWEB.getNaturalChoke() && BuildOrder().isOpener() && BuildOrder().isWallNat() && Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery) >= 2) {
				here = findWallLocation(building, Position(mapBWEB.getNaturalChoke()->Center()));

				if (here.isValid() && isBuildable(building, here) && isQueueable(building, here))
					return here;
			}
			if (Broodwar->self()->getRace() == Races::Zerg) {
				int test = (int)floor(1.5 * (double)Stations().getMyStations().size());

				if (test > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery))
					here = findProdLocation(building, mapBWEB.getMainPosition());
				else
					here = findExpoLocation();
			}
			else
				here = findExpoLocation();

			return here;
		}

		// If this is a Pylon, check if any Nexus needs a Pylon
		if (building == UnitTypes::Protoss_Pylon && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Pylon) >= 4 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge) >= 1) {
			for (auto &station : Stations().getMyStations()) {
				Station s = station.second;
				here = findDefLocation(building, Position(s.ResourceCentroid()));
				if (here.isValid() && isBuildable(building, here) && isQueueable(building, here))
					return here;
			}
		}

		// Third pylon place near our main choke
		if (building == UnitTypes::Protoss_Pylon && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) == 2 && !Terrain().isIslandMap() && !Strategy().enemyRush()) {
			here = findProdLocation(building, (Position)mapBWEB.getMainChoke()->Center());
			if (here.isValid() && isBuildable(building, here) && isQueueable(building, here))
				return here;
		}

		// If this is a defensive building
		if (building == UnitTypes::Protoss_Photon_Cannon || building == UnitTypes::Zerg_Creep_Colony || building == UnitTypes::Terran_Missile_Turret) {

			if (building == UnitTypes::Protoss_Photon_Cannon && Strategy().getEnemyBuild() == "Z2HatchMuta" && Grids().getDefense(mapBWEB.getMainTile()) == 0)
				here = findDefLocation(building, mapBWEB.getMainPosition());

			else if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge) >= 1 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) >= 3 + (Players().getNumberTerran() > 0 || Players().getNumberProtoss() > 0)) {
				for (auto &station : Stations().getMyStations()) {
					Station s = station.second;					

					if (!Pylons().hasPower((TilePosition)s.ResourceCentroid(), UnitTypes::Protoss_Photon_Cannon))
						continue;

					if ((s.BWEMBase()->Location() == mapBWEB.getNaturalTile() || s.BWEMBase()->Location() == mapBWEB.getMainTile()) && Grids().getDefense(TilePosition(s.ResourceCentroid())) <= 0)
						here = findDefLocation(building, Position(s.ResourceCentroid()));
					else if (Grids().getDefense(TilePosition(s.ResourceCentroid())) <= 1)
						here = findDefLocation(building, Position(s.ResourceCentroid()));
					else if ((Players().getPlayers().size() > 1 || Broodwar->enemy()->getRace() == Races::Zerg) && s.BWEMBase()->Location() != mapBWEB.getMainTile() && s.BWEMBase()->Location() != mapBWEB.getNaturalTile() && Grids().getDefense(TilePosition(s.ResourceCentroid())) <= 3)
						here = findDefLocation(building, Position(s.ResourceCentroid()));

					if (here.isValid())
						break;
				}
			}

			else if (!here.isValid() && Terrain().getWall()) // Assumes we want to put it at the wall, not optimal, some crashes here
				here = findDefLocation(building, Terrain().getWall()->getCentroid());

			else if (building == UnitTypes::Zerg_Creep_Colony)
				here = findDefLocation(building, mapBWEB.getNaturalPosition());

			return here;
		}

		// If we are fast expanding
		if (mapBWEB.getNaturalChoke() && (BuildOrder().isOpener() || Broodwar->self()->getRace() == Races::Zerg) && (BuildOrder().isWallNat() || BuildOrder().isWallMain())) {
			here = findWallLocation(building, Position(mapBWEB.getNaturalChoke()->Center()));
			if (here.isValid() && isBuildable(building, here))
				return here;
		}

		// If we are proxying
		if (BuildOrder().isOpener() && BuildOrder().isProxy()) {
			here = findProdLocation(building, mapBWEM.Center());
			if (here.isValid())
				return here;
		}

		// Battery placing near chokes
		if (building == UnitTypes::Protoss_Shield_Battery) {
			if (BuildOrder().isWallNat())
				here = findProdLocation(building, (Position)mapBWEB.getNaturalChoke()->Center());
			else
				here = findProdLocation(building, (Position)mapBWEB.getMainChoke()->Center());

			if (here.isValid())
				return here;
		}

		here = findProdLocation(building, mapBWEB.getMainPosition());
		if (here.isValid())
			return here;
		return TilePositions::Invalid;
	}

	bool BuildingManager::isQueueable(UnitType building, TilePosition buildTilePosition)
	{
		// Check if there's a building queued there already
		for (auto &queued : buildingsQueued) {
			if (queued.first == buildTilePosition)
				return false;
		}
		return true;
	}

	bool BuildingManager::isBuildable(UnitType building, TilePosition here)
	{
		UnitType geyserType = Broodwar->self()->getRace().getRefinery();

		// Refinery only on Geysers
		if (building.isRefinery()) {
			for (auto &g : Resources().getMyGas()) {
				ResourceInfo &gas = g.second;
				if (here == gas.getTilePosition() && gas.getState() >= 2 && gas.getType() != geyserType)
					return true;				
			}
			return false;
		}

		// Used tile check
		for (int x = here.x; x < here.x + building.tileWidth(); x++) {
			for (int y = here.y; y < here.y + building.tileHeight(); y++) {
				TilePosition t(x, y);
				if (!t.isValid())
					return false;

				if (mapBWEB.getUsedTiles().find(TilePosition(x, y)) != mapBWEB.getUsedTiles().end())
					return false;				
			}
		}

		// Addon room check
		if (building.canBuildAddon()) {
			if (mapBWEB.getUsedTiles().find(TilePosition(here) + TilePosition(4, 1)) != mapBWEB.getUsedTiles().end())
				return false;
		}

		// Psi/Creep check
		if (building.requiresPsi() && !Pylons().hasPower(here, building))
			return false;
		if (building.requiresCreep() && !Broodwar->hasCreep(here))
			return false;
		return true;
	}

	bool BuildingManager::overlapsQueuedBuilding(UnitType type, TilePosition here)
	{
		// Check if there's a building queued there already
		for (auto &queued : buildingsQueued) {
			TilePosition tile = queued.first;
			UnitType building = queued.second;

			if (here.x >= tile.x && here.x < tile.x + building.tileWidth() && here.y >= tile.y && here.y < tile.y + building.tileWidth())
				return true;
		}
		return false;
	}

	TilePosition BuildingManager::findDefLocation(UnitType building, Position here)
	{
		set<TilePosition> placements;
		TilePosition tileBest = TilePositions::Invalid;
		double distBest = DBL_MAX;
		const Station* station = mapBWEB.getClosestStation(TilePosition(here));

		if (!station)
			return TilePositions::Invalid;

		// Check all Stations to see if one of their defense locations is best		
		for (auto &defense : station->DefenseLocations()) {
			// Pylon is separated because we want one unique best buildable position to check, rather than next best buildable position
			if (building == UnitTypes::Protoss_Pylon) {
				double dist = Position(defense).getDistance(Position(here));
				if (dist < distBest)
					distBest = dist, tileBest = defense;
			}
			else {
				double dist = Position(defense).getDistance(here);
				if (dist < distBest && isQueueable(building, defense) && isBuildable(building, defense))
					distBest = dist, tileBest = defense;
			}
		}

		// If this is a Cannon, also check the Walls defense locations
		if (building == UnitTypes::Protoss_Photon_Cannon || building == UnitTypes::Zerg_Creep_Colony) {
			for (auto &wall : mapBWEB.getWalls()) {
				for (auto &tile : wall.getDefenses()) {
					Position defenseCenter = Position(tile) + Position(32, 32);
					double dist = defenseCenter.getDistance(here);
					if (dist < distBest && isQueueable(building, tile) && isBuildable(building, tile))
						distBest = dist, tileBest = tile;
				}
			}
		}
		
		return tileBest;
	}

	TilePosition BuildingManager::findProdLocation(UnitType building, Position here)
	{
		TilePosition tileBest = TilePositions::Invalid;
		double distBest = DBL_MAX;
/*
		if (building.getRace() == Races::Zerg) {
			TilePosition tile = Broodwar->getBuildLocation(building, (TilePosition)here);
			if (isBuildable(building, tile) && isQueueable(building, tile))
				return tile;
		}*/

		// Refineries are only built on my own gas resources
		if (building.isRefinery()) {
			for (auto &g : Resources().getMyGas()) {
				ResourceInfo &gas = g.second;
				double dist = gas.getPosition().getDistance(here);
				if (isQueueable(UnitTypes::Protoss_Assimilator, gas.getTilePosition()) && isBuildable(UnitTypes::Protoss_Assimilator, gas.getTilePosition()) && gas.getState() >= 2 && dist < distBest) {
					distBest = dist, tileBest = gas.getTilePosition();
				}
			}
			return tileBest;
		}

		set<TilePosition> placements;
		for (auto &block : mapBWEB.Blocks()) {
			Position blockCenter = Position(block.Location()) + Position(block.width() * 16, block.height() * 16);

			bool d = true;
			if (Broodwar->self()->getRace() == Races::Protoss && building == UnitTypes::Protoss_Pylon) {
				for (auto &small : block.SmallTiles()) {
					if (mapBWEB.getUsedTiles().find(small) != mapBWEB.getUsedTiles().end()
						|| !Terrain().isInAllyTerritory(small))
						d = false;
				}

				if ((block.LargeTiles().empty() && poweredLarge == 0)
					|| (block.MediumTiles().empty() && poweredMedium == 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) <= 0 + BuildOrder().isFastExpand()))
					d = false;

				if (!d)
					continue;
			}

			// Setup placements
			if (building.tileWidth() == 4 || (building == UnitTypes::Protoss_Robotics_Facility && Terrain().isIslandMap()))
				placements = block.LargeTiles();
			else if (building.tileWidth() == 3)
				placements = block.MediumTiles();
			else
				placements = block.SmallTiles();

			double dist = blockCenter.getDistance(here);

			if (dist < distBest) {
				for (auto tile : placements) {
					if (isQueueable(building, tile) && isBuildable(building, tile))
						distBest = dist, tileBest = tile;
				}
			}
		}
		return tileBest;
	}

	TilePosition BuildingManager::findWallLocation(UnitType building, Position here)
	{
		TilePosition tileBest = TilePositions::Invalid;
		double distBest = DBL_MAX;
		const Wall* wall;
		set<TilePosition> placements;
		const Station* station = mapBWEB.getClosestStation(TilePosition(here));

		if (BuildOrder().isWallMain())
			wall = mapBWEB.getWall(mapBWEB.getMainArea());
		else if (BuildOrder().isWallNat())
			wall = mapBWEB.getWall(mapBWEB.getNaturalArea());

		if (!wall)
			return tileBest;

		if (building.tileWidth() == 4)
			placements = wall->largeTiles();
		else if (building.tileWidth() == 3)
			placements = wall->mediumTiles();
		else if (building.tileWidth() == 2)
			placements = wall->smallTiles();

		for (auto tile : placements) {
			double dist = Position(tile).getDistance(here);
			if (dist < distBest && isBuildable(building, tile) && isQueueable(building, tile))
				tileBest = tile, distBest = dist;
		}
		if (station && building == UnitTypes::Protoss_Pylon && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) <= 0 && !tileBest.isValid()) {
			for (auto tile : station->DefenseLocations()) {
				double dist = Position(tile).getDistance(here);
				if (dist < distBest && isBuildable(building, tile) && isQueueable(building, tile))
					tileBest = tile, distBest = dist;
			}
		}
		return tileBest;
	}

	TilePosition BuildingManager::findExpoLocation()
	{
		// If we are expanding, it must be on an expansion area
		UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
		double best = 0.0;
		TilePosition tileBest = TilePositions::Invalid;

		// Fast expands must be as close to home and have a gas geyser
		if (Stations().getMyStations().size() == 1)
			tileBest = mapBWEB.getNaturalTile();

		// Other expansions must be as close to home but as far away from the opponent
		else {
			for (auto &area : mapBWEM.Areas()) {
				for (auto &base : area.Bases()) {
					UnitType shuttle = Broodwar->self()->getRace().getTransport();

					// Shuttle check for island bases, check enemy owned bases
					if ((!area.AccessibleFrom(mapBWEB.getMainArea()) && Broodwar->self()->visibleUnitCount(shuttle) <= 0) || Terrain().isInEnemyTerritory(base.Location()))
						continue;

					// Get value of the expansion
					double value = 0.0;
					for (auto &mineral : base.Minerals())
						value += double(mineral->Amount());
					for (auto &gas : base.Geysers())
						value += double(gas->Amount());
					if (base.Geysers().size() == 0)
						value = value / 10.0;

					// Get distance of the expansion
					double distance;
					if (!area.AccessibleFrom(mapBWEB.getMainArea()))
						distance = log(base.Center().getDistance(mapBWEB.getMainPosition()));
					else if (Players().getPlayers().size() > 1 || !Terrain().getEnemyStartingPosition().isValid())
						distance = mapBWEB.getGroundDistance(mapBWEB.getMainPosition(), base.Center());
					else
						distance = mapBWEB.getGroundDistance(mapBWEB.getMainPosition(), base.Center()) / (mapBWEB.getGroundDistance(Terrain().getEnemyStartingPosition(), base.Center()));

					if (isBuildable(baseType, base.Location()) && isQueueable(baseType, base.Location()) && value / distance > best) {
						best = value / distance;
						tileBest = base.Location();
					}
				}
			}
		}
		currentExpansion = tileBest;
		return tileBest;
	}
}