#include "McRave.h"

namespace McRave
{
	void GridManager::onFrame()
	{
		reset();
		updateAlly();
		updateEnemy();
		updateNeutral();
		updateVisibility();
		draw();
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
	}

	void GridManager::draw()
	{
		return;// Remove this to draw stuff

		// Temp debugging for tile positions
		for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
			for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
				WalkPosition w(x, y);

				/*if (distanceHome[x][y] <= 0)
					Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Black);
				if (distanceHome[x][y] > 0 && distanceHome[x][y] <= 600)
					Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Purple);
				if (distanceHome[x][y] > 600 && distanceHome[x][y] <= 1000)
					Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
				if (distanceHome[x][y] > 1000 && distanceHome[x][y] < 1500)
					Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Green);
				if (distanceHome[x][y] >= 1500 && distanceHome[x][y] != DBL_MAX)
					Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Yellow);*/

				if (!mapBWEM.GetMiniTile(w).Walkable())
					Broodwar->drawBoxMap(Position(w), Position(w) + Position(9, 9), Colors::Blue);

				//if (distanceHome[x][y] <= 0)
				//	Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
				//if (collision[x][y] > 0)
					//Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
			//if (eGroundCluster[x][y] + eAirCluster[x][y] > HIGH_TEMPLAR_LIMIT)
			//	Broodwar->drawCircleMap(Position(w) + Position(4, 4), 2, Colors::Red);


				if (eSplash[x][y] > 0)
					Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::Blue);
			}
		}
	}

	void GridManager::updateAlly()
	{
		Display().startClock();
		for (auto &u : Units().getMyUnits()) {

			UnitInfo &unit = u.second;

			// Add a visited grid for rough guideline of what we've seen by this unit recently
			auto start = unit.getWalkPosition();
			for (int x = start.x - 4; x < start.x + 8; x++) {
				for (int y = start.y - 4; y < start.y + 8; y++) {
					auto t = WalkPosition(x, y);
					if (t.isValid())
						visitedGrid[x][y] = Broodwar->getFrameCount();
				}
			}

			// Spider mines are added to the enemy splash grid so ally units avoid allied mines
			if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine) {
				if (!unit.isBurrowed() && unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists())
					addSplash(unit);
			}

			else if (!unit.unit()->isLoaded()) {
				addToGrids(unit);
			}
		}

		Display().performanceTest(__FUNCTION__);
	}

	void GridManager::updateEnemy()
	{
		Display().startClock();
		for (auto &u : Units().getEnemyUnits()) {

			UnitInfo &unit = u.second;
			if (unit.unit()->exists() && (unit.unit()->isStasised() || unit.unit()->isMaelstrommed()))
				continue;

			auto longRangeUnit = unit.getGroundRange() >= 224.0;

			if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Protoss_Scarab) {
				if (unit.hasTarget() && unit.getTarget().unit() && unit.getTarget().unit()->exists())
					addSplash(unit);
			}
			else if (unit.unit()->exists() || longRangeUnit) {
				addToGrids(unit);
			}
		}
		Display().performanceTest(__FUNCTION__);
	}

	void GridManager::updateNeutral()
	{
		// Collision Grid - TODO
		for (auto &u : Broodwar->neutral()->getUnits()) {

			WalkPosition start = WalkPosition(u->getTilePosition());
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
				}
			}

			if (u->getType() == UnitTypes::Spell_Disruption_Web)
				Commands().addCommand(u, u->getPosition(), TechTypes::Disruption_Web);
		}
	}

	void GridManager::updateVisibility()
	{
		for (int x = 0; x <= Broodwar->mapWidth(); x++) {
			for (int y = 0; y <= Broodwar->mapHeight(); y++) {
				TilePosition t(x, y);
				if (Broodwar->isVisible(t))
					visibleGrid[x][y] = Broodwar->getFrameCount();
			}
		}
	}

	void GridManager::updateMobility()
	{
		for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
			for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {

				WalkPosition w(x, y);
				if (!w.isValid()
					|| !mapBWEM.GetMiniTile(w).Walkable())
					continue;

				for (int i = -12; i < 12; i++) {
					for (int j = -12; j < 12; j++) {

						WalkPosition w2(x + i, y + j);
						if (w2.isValid() && mapBWEM.GetMiniTile(w2).Walkable())
							mobility[x][y] += 1;
					}
				}

				mobility[x][y] = min(10, int(floor(mobility[x][y] / 56)));


				// Island
				if ((mapBWEM.GetArea(w) && mapBWEM.GetArea(w)->AccessibleNeighbours().size() == 0) || !BWEB::Map::isWalkable((TilePosition)w))
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
		WalkPosition source(BWEB::Map::getMainPosition());
		vector<WalkPosition> direction{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 },{ -1,-1 },{ -1, 1 },{ 1, -1 },{ 1, 1 } };
		double root2 = sqrt(2.0);

		std::queue<BWAPI::WalkPosition> nodeQueue;
		nodeQueue.emplace(source);

		// While not empty, pop off top
		while (!nodeQueue.empty()) {
			auto const tile = nodeQueue.front();
			nodeQueue.pop();

			int i = 0;
			for (auto const &d : direction) {
				auto const next = tile + d;
				i++;

				if (next.isValid() && tile.isValid()) {
					// If next has a distance assigned or isn't walkable
					if (distanceHome[next.x][next.y] != 0.0 || !BWEB::Map::isWalkable((TilePosition)next))
						continue;

					// Add distance and add to queue
					distanceHome[next.x][next.y] = (i > 4 ? root2 : 1.0) + parentDistance[tile.x][tile.y];
					parentDistance[next.x][next.y] = distanceHome[next.x][next.y];
					nodeQueue.emplace(next);
				}
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

		WalkPosition start(unit.getTarget().getWalkPosition());
		Position target = unit.getTarget().getPosition();

		for (int x = start.x - 12; x <= start.x + 12 + walkWidth; x++) {
			for (int y = start.y - 12; y <= start.y + 12 + walkHeight; y++) {

				WalkPosition w(x, y);
				Position p = Position(w) + Position(4, 4);
				if (!w.isValid())
					continue;

				saveReset(w);
				eSplash[x][y] += (target.getDistance(p) <= 96);
			}
		}
	}

	void GridManager::addToGrids(UnitInfo& unit)
	{
		if ((unit.getType().isWorker() && unit.getPlayer() != Broodwar->self() && (!unit.unit()->exists() || Broodwar->getFrameCount() > 10000 || unit.unit()->isConstructing() || (Terrain().isInAllyTerritory(unit.getTilePosition()) && (Broodwar->getFrameCount() - unit.getLastAttackFrame() > 500))))
			|| unit.getType() == UnitTypes::Protoss_Interceptor)
			return;

		// Max range and speed
		auto maxRange = int(max({ unit.getGroundRange(), unit.getAirRange(), 32.0 }) / 8.0);
		auto speed = int(max(unit.getSpeed(), 1.0));

		// Pixel and walk sizes
		auto pixelSize = unit.getType().isBuilding() ? unit.getType().tileWidth() * 32 : max(unit.getType().width(), unit.getType().height());
		auto walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : (int)ceil(unit.getType().width() / 8.0);
		auto walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : (int)ceil(unit.getType().height() / 8.0);

		// Reach: range + size + speed for 1 second
		auto grdReach = int(unit.getGroundRange() + (speed * 32.0) + (pixelSize / 2));
		auto airReach = int(unit.getAirRange() + (speed * 32.0) + (pixelSize / 2));

		// HACK: Make workers range smaller
		if (unit.getType().isWorker()) {
			grdReach = int(grdReach / 1.5);
			airReach = int(airReach / 1.5);
		}

		// Choose threat grid
		auto grdGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eGroundThreat;
		auto airGrid = unit.getPlayer() == Broodwar->self() ? nullptr : eAirThreat;

		// Choose cluster grid
		auto clusterGrid = unit.getPlayer() == Broodwar->self() ?
			(unit.getType().isFlyer() ? aAirCluster : aGroundCluster) :
			(unit.getType().isFlyer() ? eAirCluster : eGroundCluster);


		// Limit checks so we don't have to check validity
		int radius = 1 + (max(grdReach, airReach)) / 8;
		auto left = max(0, unit.getWalkPosition().x - radius);
		auto right = min(1024, unit.getWalkPosition().x + walkWidth + radius);
		auto top = max(0, unit.getWalkPosition().y - radius);
		auto bottom = min(1024, unit.getWalkPosition().y + walkHeight + radius);

		// Pixel rectangle
		auto topLeft = Position(unit.getWalkPosition());
		auto botRight = topLeft + Position(8 * walkWidth, 8 * walkHeight);

		// Iterate tiles and add to grid
		for (int x = left; x < right; x++) {
			for (int y = top; y < bottom; y++) {

				WalkPosition w(x, y);
				auto p = Position(w) + Position(4, 4);
				auto dist = p.getDistance(unit.getPosition());

				// Collision
				if (!unit.getType().isFlyer() && Util().rectangleIntersect(topLeft, botRight, p)) {
					collision[x][y] += 1;
					saveReset(w);
				}

				// Threat
				if (grdGrid && dist <= grdReach) {
					grdGrid[x][y] += (unit.getVisibleGroundStrength());
					saveReset(w);
				}
				if (airGrid && dist <= airReach) {
					airGrid[x][y] += (unit.getVisibleAirStrength());
					saveReset(w);
				}

				// Cluster
				if (clusterGrid && dist < 96.0 && (unit.getPlayer() != Broodwar->self() || unit.getRole() == Role::Fighting)) {
					clusterGrid[x][y] += unit.getPriority();
					saveReset(w);
				}
			}
		}
	}
}