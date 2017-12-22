#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

class Block
{
	int w, h;
	TilePosition t;
public:
	Block();
	Block(int width, int height, TilePosition tile) { w = width, h = height, t = tile; }
	int width() { return w; }
	int height() { return h; }
	TilePosition tile() { return t; }
};

class BlockTrackerClass
{
	map<TilePosition, Block> blocks;
	TilePosition best;
	set<TilePosition> smallPosition, mediumPosition, largePosition;
	void updateBlocks();
	bool overlapsBlocks(TilePosition);
	bool canAddBlock(TilePosition, int, int);

	void insertSmallBlock(TilePosition, bool);
	void insertMediumBlock(TilePosition, bool);
	void insertLargeBlock(TilePosition, bool);

public:
	void update();	
	map<TilePosition, Block>& getBlocks() { return blocks; }
	set<TilePosition> getSmallPosition() { return smallPosition; }
	set<TilePosition> getMediumPosition() { return mediumPosition; }
	set<TilePosition> getLargePosition() { return largePosition; }
};

typedef Singleton<BlockTrackerClass> BlockTracker;