#include "McRave.h"

namespace McRave
{
	void GridManager::onFrame()
	{		
		reset();
		updateAlly();
		updateEnemy();
		updateNeutral();
		draw();
		return;
	}

	void GridManager::onStart()
	{
		updateMobility();
		updateDistance();
	}

	void GridManager::reset()
	{
		Display().startClock();
		armyCenter = Positions::None;
		double best = 0.0;
		for (auto &w : resetVector) {
			int x = w.x;
			int y = w.y;

			double current = (aAirCluster[x][y] + aGroundCluster[x][y]);
			if (current > best) {
				best = current;
				armyCenter = Position(w);
			}

			aGroundCluster[x][y] = 0;
			aAirCluster[x][y] = 0;

			eGroundThreat[x][y] = 0.0;
			eAirThreat[x][y] = 0.0;
			eGroundCluster[x][y] = 0;
			eAirCluster[x][y] = 0;
			eSplash[x][y] = 0;

			collision[x][y] = 0;
			resetGrid[x][y] = 0;
		}
		resetVector.clear();

		Display().performanceTest(__FUNCTION__);
		return;
	}

	void GridManager::draw()
	{
		return;// Remove this to draw stuff

		// Temp debugging for tile positions
		for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
			for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
				WalkPosition w(x, y);
				/*if (timeGrid[x][y] != Broodwar->getFrameCount())
					continue;*/

					//if (mobility[x][y] <= 0)
					//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Black);
					//if (mobility[x][y] > 0 && mobility[x][y] <= 3)
					//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Purple);
					//if (mobility[x][y] > 3 && mobility[x][y] <= 6)
					//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
					//if (mobility[x][y] > 6 && mobility[x][y] < 10)
					//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Green);
					//if (mobility[x][y] >= 10)
					//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Yellow);

					//if (eGroundThreat[x][y] > 0)
					//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
					//if (distanceHome[x][y] <= 0)
					//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
					//if (collision[x][y] > 0)
						//Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
				//if (eGroundCluster[x][y] + eAirCluster[x][y] > HIGH_TEMPLAR_LIMIT)
				//	Broodwar->drawCircleMap(Position(w) + Position(4, 4), 2, Colors::Red);

				if (eGroundThreat[x][y] > 0)
					Broodwar->drawCircleMap(Position(w) + Position(4, 4), 2, Colors::Red);
			}
		}
	}

	void GridManager::updateAlly()
	{
		Display().startClock();
		for (auto &u : Units().getMyUnits()) {

			UnitInfo &unit = u.second;
			if (!unit.unit() || unit.getType() == UnitTypes::Protoss_Arbiter || unit.getType() == UnitTypes::Protoss_Observer || unit.getType() == UnitTypes::Protoss_Shuttle || unit.getType() == UnitTypes::Terran_Science_Vessel)
				continue;

			// Spider mines are added to the enemy splash grid so ally units avoid allied mines
			if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine) {
				if (!unit.isBurrowed() && unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists())
					addSplash(unit);
			}

			else if (!unit.unit()->isLoaded()) {
				addCluster(unit);
				addCollision(unit);
			}
		}

		// Building grid update
		for (auto &b : Buildings().getMyBuildings())
			addCollision(b.second);

		// Worker grid update
		for (auto &w : Workers().getMyWorkers()) {
			WorkerInfo& worker = w.second;
			addCollision(worker);
		}

		Display().performanceTest(__FUNCTION__);
		return;
	}

	void GridManager::updateEnemy()
	{
		Display().startClock();
		for (auto &u : Units().getEnemyUnits()) {

			UnitInfo &unit = u.second;
			if (unit.unit()->exists() && (unit.unit()->isStasised() || unit.unit()->isMaelstrommed()))
				continue;

			UnitInfo* c = Util().getClosestAllyUnit(unit);
			WorkerInfo* w = Util().getClosestAllyWorker(unit);

			if ((!w || w->getPosition().getDistance(unit.getPosition()) > 720) && (!c || c->getPosition().getDistance(unit.getPosition()) > 720))
				continue;

			WalkPosition start = unit.getWalkPosition();
			if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Protoss_Scarab) {
				if (unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists()) {
					addSplash(unit);
					//Broodwar->drawCircleMap(unit.getPosition(), 4, Colors::Green);
				}
			}
			else {
				addCollision(unit);
				addCluster(unit);
				addThreat(unit);
			}
		}
		Display().performanceTest(__FUNCTION__);
		return;
	}

	void GridManager::updateDefense(UnitInfo& unit)
	{
		// Defense Grid
		TilePosition tile = unit.getTilePosition();
		if (!unit.unit() || !tile.isValid())
			return;

		for (int x = tile.x - 6; x < tile.x + unit.getType().tileWidth() + 4; x++) {
			for (int y = tile.y - 6; y < tile.y + unit.getType().tileHeight() + 4; y++) {
				TilePosition t(x, y);
				if (!t.isValid())
					continue;

				Position center = Position(t) + Position(16, 16);
				if (unit.getPosition().getDistance(center) < 192)
					unit.unit()->exists() ? defense[x][y] += 1 : defense[x][y] -= 1;
			}
		}
	}

	void GridManager::updateNeutral()
	{
		// Collision Grid - TODO
		for (auto &u : Broodwar->neutral()->getUnits()) {

			WalkPosition start = Util().getWalkPosition(u);
			int width = u->getType().tileWidth() * 4;
			int height = u->getType().tileHeight() * 4;
			if (u->getType().isFlyer())
				continue;			

			for (int x = start.x; x < start.x + width; x++) {
				for (int y = start.y; y < start.y + height; y++) {

					WalkPosition w(x, y);
					if (!WalkPosition(x, y).isValid())
						continue;

					collision[x][y] = 1;
					saveReset(w);
					/*timeGrid[x][y] = Broodwar->getFrameCount();*/
				}
			}

			if (u->getType() == UnitTypes::Spell_Disruption_Web)
				Commands().addCommand(u->getPosition(), TechTypes::Disruption_Web);
		}
		return;
	}

	void GridManager::updateMobility()
	{
		for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
			for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {

				WalkPosition w(x, y);
				if (!w.isValid()
					|| !mapBWEM.GetMiniTile(w).Walkable())
					continue;

				for (int i = -12; i <= 12; i++) {
					for (int j = -12; j <= 12; j++) {

						WalkPosition w2(x + i, y + j);
						if (w2.isValid() && mapBWEM.GetMiniTile(w2).Walkable())
							mobility[x][y] += 1;
					}
				}

				mobility[x][y] = int(floor(mobility[x][y] / 56));

				if (mapBWEM.GetArea(WalkPosition(x, y))) {
					for (auto &choke : mapBWEM.GetArea(w)->ChokePoints())
						if (w.getDistance(choke->Center()) < 16)
							mobility[x][y] = 10;
				}

				mobility[x][y] = min(10, mobility[x][y]);


				// Island
				if (mapBWEM.GetArea(WalkPosition(x, y)) && mapBWEM.GetArea(WalkPosition(x, y))->AccessibleNeighbours().size() == 0)
					mobility[x][y] = -1;

				// Setup what is possible to check ground distances on
				if (mobility[x][y] <= 0)
					distanceHome[x][y] = -1;
				else if (mobility[x][y] > 0)
					distanceHome[x][y] = 0;
			}
		}
	}

	void GridManager::updateAllyMovement(Unit unit, WalkPosition start)
	{
		int walkWidth = (int)ceil(unit->getType().width() / 8.0);
		int walkHeight = (int)ceil(unit->getType().height() / 8.0);
		int halfW = walkWidth / 2;
		int halfH = walkHeight / 2;

		for (int x = start.x - halfW; x <= start.x + halfW; x++) {
			for (int y = start.y - halfH; y <= start.y + halfH; y++) {
				WalkPosition w(x, y);
				if (!w.isValid())
					continue;

				collision[x][y] = 1;
				saveReset(w);
			}
		}
	}

	void GridManager::updateDistance()
	{
		for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
			for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
				WalkPosition w(x, y);
				if (distanceHome[x][y] < 0)
					continue;

				distanceHome[x][y] = mapBWEB.getGroundDistance(Position(w), mapBWEB.getMainPosition());
			}
		}
	}

	void GridManager::saveReset(WalkPosition here)
	{
		int x = here.x, y = here.y;
		if (!resetGrid[x][y]) {
			resetVector.push_back(here);
			resetGrid[x][y] = 1;
		}
	}

	void GridManager::addSplash(UnitInfo& unit)
	{
		int walkWidth = (int)ceil(unit.getType().width() / 8.0);
		int walkHeight = (int)ceil(unit.getType().height() / 8.0);
		//int frame = Broodwar->getFrameCount();

		WalkPosition start(unit.getTarget().getWalkPosition());
		Position target = unit.getTarget().getPosition();

		for (int x = start.x - 12; x <= start.x + 12 + walkWidth; x++) {
			for (int y = start.y - 12; y <= start.y + 12 + walkHeight; y++) {

				WalkPosition w(x, y);
				Position p = Position(w) + Position(4,4);
				if (!w.isValid())
					continue;

				/*if (timeGrid[x][y] == frame)
					eSplash[x][y] += (unit.getTarget().getPosition().getDistance(Position(WalkPosition(x, y))) > 96);
				else
					eSplash[x][y] = (unit.getTarget().getPosition().getDistance(Position(WalkPosition(x, y))) > 96);
				timeGrid[x][y] = frame;*/

				saveReset(w);
				eSplash[x][y] += (target.getDistance(p) <= 96);
			}
		}
	}

	void GridManager::addCluster(UnitInfo& unit) {

		// Setup parameters
		int radius = unit.getType().isFlyer() ? 12 : 6;
		int walkWidth = (int)ceil(unit.getType().width() / 8.0);
		int walkHeight = (int)ceil(unit.getType().height() / 8.0);
		//int frame = Broodwar->getFrameCount();

		// Choose the grid
		auto grid = unit.getPlayer() == Broodwar->self() ?
			(unit.getType().isFlyer() ? aAirCluster : aGroundCluster) :
			(unit.getType().isFlyer() ? eAirCluster : eGroundCluster);

		WalkPosition start(unit.getWalkPosition());

		// Iterate tiles and add to grid
		for (int x = start.x - radius; x < start.x + walkWidth + radius; x++) {
			for (int y = start.y - radius; y < start.y + walkHeight + radius; y++) {

				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);

				if (!w.isValid())
					continue;

				/*if (timeGrid[x][y] == frame)
					grid[x][y] += (unit.getPriority() * (p.getDistance(unit.getPosition()) < radius*8.0));
				else
					grid[x][y] = (unit.getPriority() * (p.getDistance(unit.getPosition()) < radius*8.0));
				timeGrid[x][y] = frame;*/

				saveReset(w);
				grid[x][y] += (unit.getPriority() * (p.getDistance(unit.getPosition()) < radius*8.0));
			}
		}
	}

	void GridManager::addThreat(UnitInfo& unit)
	{
		if (unit.getType().isWorker() && (!unit.unit()->exists() || (Terrain().isInAllyTerritory(unit.getTilePosition()) && (Broodwar->getFrameCount() - unit.getLastAttackFrame() > 500))))
			return;

		if (unit.getVisibleGroundStrength() <= 0.0 && unit.getVisibleAirStrength() <= 0.0)
			return;

		if (unit.getType() == UnitTypes::Protoss_Interceptor)
			return;

		// Speed multipled by 4.0 because we want WalkPositions per second
		// unit.getSpeed() returns pixels per frame
		// Walks/Second = speed * 24 / 8

		// Setup parameters
		int maxRange = int(max({ unit.getGroundRange(), unit.getAirRange(), 64.0 }) / 8.0);
		int speed = int(unit.getSpeed());
		int radius = maxRange + (speed * 3);
		int walkWidth = (int)ceil(unit.getType().width() / 8.0);
		int walkHeight = (int)ceil(unit.getType().height() / 8.0);
		int grdReach = max(unit.getGroundRange(), 64.0) + (speed * 24.0) + (unit.getType().width() / 2);
		int airReach = max(unit.getAirRange(), 64.0) + (speed * 24.0) + (unit.getType().width() / 2);
		//int frame = Broodwar->getFrameCount();		


		// Choose the grid - NULL self grid for now
		auto grdGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eGroundThreat;
		auto airGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eAirThreat;

		WalkPosition start(unit.getWalkPosition());

		// Safety
		if (!grdGrid || !airGrid)
			return;

		// Iterate tiles and add to grid
		for (int x = start.x - radius; x < start.x + walkWidth + radius; x++) {
			for (int y = start.y - radius; y < start.y + walkHeight + radius; y++) {

				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);

				if (!w.isValid() || !p.isValid())
					continue;

				/*if (timeGrid[x][y] == frame) {
					grdGrid[x][y] += (unit.getVisibleGroundStrength() * (p.getDistance(unit.getPosition()) < grdReach));
					airGrid[x][y] += (unit.getVisibleAirStrength() * (p.getDistance(unit.getPosition()) < airReach));
				}
				else {
					grdGrid[x][y] = (unit.getVisibleGroundStrength() * (p.getDistance(unit.getPosition()) < grdReach));
					airGrid[x][y] = (unit.getVisibleAirStrength() * (p.getDistance(unit.getPosition()) < airReach));
				}
				timeGrid[x][y] = frame;*/

				grdGrid[x][y] += (unit.getVisibleGroundStrength() * (p.getDistance(unit.getPosition()) < grdReach));
				airGrid[x][y] += (unit.getVisibleAirStrength() * (p.getDistance(unit.getPosition()) < airReach));
				saveReset(w);
			}
		}
	}
}