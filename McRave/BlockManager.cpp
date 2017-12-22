#include "McRave.h"

void BlockTrackerClass::update()
{
	updateBlocks();
}

bool BlockTrackerClass::canAddBlock(TilePosition here, int width, int height)
{
	// Check if a block of specified size would overlap any bases, resources or other blocks
	for (int x = here.x; x < here.x + width; x++)
	{
		for (int y = here.y; y < here.y + height; y++)
		{
			if (!TilePosition(x, y).isValid()) return false;
			if (!theMap.GetTile(TilePosition(x, y)).Buildable()) return false;
			if (Grids().getResourceGrid(x, y) > 0) return false;
			if (Blocks().overlapsBlocks(TilePosition(x, y))) return false;
			if (Terrain().overlapsBases(TilePosition(x, y))) return false;
		}
	}
	return true;
}

bool BlockTrackerClass::overlapsBlocks(TilePosition here)
{
	for (auto &b : blocks)
	{
		Block& block = b.second;
		if (here.x >= block.tile().x && here.x < block.tile().x + block.width() && here.y >= block.tile().y && here.y < block.tile().y + block.height()) return true;
	}
	return false;
}

void BlockTrackerClass::insertSmallBlock(TilePosition here, bool mirror)
{
	Block(4, 5, here);

	if (mirror)
	{
		smallPosition.insert(here);
		smallPosition.insert(here + TilePosition(2, 0));
		largePosition.insert(here + TilePosition(0, 2));
	}
	else
	{
		smallPosition.insert(here + TilePosition(0, 2));
		smallPosition.insert(here + TilePosition(2, 2));
		largePosition.insert(here);
	}
}

void BlockTrackerClass::insertMediumBlock(TilePosition here, bool mirror)
{
	// https://imgur.com/a/nE7dL for reference

	Block(6, 8, here);

	mediumPosition.insert(here + TilePosition(0, 6));
	mediumPosition.insert(here + TilePosition(3, 6));

	if (mirror)
	{
		smallPosition.insert(here);
		smallPosition.insert(here + TilePosition(0, 2));
		smallPosition.insert(here + TilePosition(0, 4));
		largePosition.insert(here + TilePosition(2, 0));
		largePosition.insert(here + TilePosition(2, 3));
	}
	else
	{
		smallPosition.insert(here + TilePosition(4, 0));
		smallPosition.insert(here + TilePosition(4, 2));
		smallPosition.insert(here + TilePosition(4, 4));
		largePosition.insert(here);
		largePosition.insert(here + TilePosition(0, 3));
	}
}

void BlockTrackerClass::insertLargeBlock(TilePosition here, bool mirror)
{

}


void BlockTrackerClass::updateBlocks()
{
	// Currently: Small is 4x5, medium is 6x8
	Area const * mainArea = theMap.GetArea(Terrain().getPlayerStartingTilePosition());
	set<TilePosition> mainTiles;

	for (int x = 0; x <= Broodwar->mapWidth(); x++)
	{
		for (int y = 0; y <= Broodwar->mapHeight(); y++)
		{
			if (!TilePosition(x, y).isValid()) continue;
			if (!theMap.GetArea(TilePosition(x, y))) continue;
			if (theMap.GetArea(TilePosition(x, y)) == mainArea) mainTiles.insert(TilePosition(x, y));
		}
	}

	// Mirror? (Gate on right)
	bool mirror = false;
	if (Position(Terrain().getFirstChoke()).x > Terrain().getPlayerStartingPosition().x) mirror = true;

	// Find first chunk position
	if (Broodwar->getFrameCount() == 500)
	{
		double distA = DBL_MAX;
		TilePosition tile = Terrain().getPlayerStartingTilePosition();
		for (int x = tile.x - 8; x < tile.x + 12; x++)
		{
			for (int y = tile.y - 10; y < tile.y + 11; y++)
			{
				if (!canAddBlock(TilePosition(x, y), 6, 8)) continue;

				Position blockCenter = Position(TilePosition(x + 3, y + 4));

				double distB = blockCenter.getDistance(Terrain().getPlayerStartingPosition()) * blockCenter.getDistance(Position(Terrain().getFirstChoke()));
				if (distB < distA)
				{
					distA = distB;
					best = TilePosition(x, y);
				}
			}
		}
		insertMediumBlock(best, mirror);

		for (auto tile : mainTiles)
			if (canAddBlock(tile, 6, 8)) insertMediumBlock(tile, mirror);
		for (auto tile : mainTiles)
			if (canAddBlock(tile, 4, 5)) insertSmallBlock(tile, mirror);
	}

	for (auto tile : smallPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(2, 2)), Broodwar->self()->getColor());
	for (auto tile : mediumPosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(3, 2)), Broodwar->self()->getColor());
	for (auto tile : largePosition)
		Broodwar->drawBoxMap(Position(tile), Position(tile + TilePosition(4, 3)), Broodwar->self()->getColor());
}