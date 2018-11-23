#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>
#include "Block.h"
#include "PathFind.h"
#include "Station.h"
#include "Wall.h"

namespace BWEB::Map
{
	inline BWEM::Map& mapBWEM = BWEM::Map::Instance();

	// Unneeded
	inline void findMain(), findMainChoke(), findNatural(), findNaturalChoke(), findNeutrals();
	void draw(), onStart(), onUnitDiscover(BWAPI::Unit), onUnitDestroy(BWAPI::Unit), onUnitMorph(BWAPI::Unit);

	/// This is just put here so pathfinding can use it for now
	//BWAPI::UnitType overlapsCurrentWall(BWAPI::TilePosition tile, int width = 1, int height = 1);

	void removeOverlap(BWAPI::TilePosition, int, int);

	void addOverlap(BWAPI::TilePosition, int, int);

	bool isOverlaping(BWAPI::TilePosition here, int width = 1, int height = 1, bool ignoreBlocks = false);

	void addReserve(BWAPI::TilePosition, int, int);

	bool isReserved(BWAPI::TilePosition here, int width = 1, int height = 1);

	bool isUsed(BWAPI::TilePosition here, int width = 1, int height = 1);

	bool isWalkable(BWAPI::TilePosition);
	int tilesWithinArea(BWEM::Area const *, BWAPI::TilePosition here, int width = 1, int height = 1);

	/// <summary> Returns true if the given BWAPI::UnitType is placeable at the given BWAPI::TilePosition </summary>
	bool isPlaceable(BWAPI::UnitType, BWAPI::TilePosition);

	/// <summary> Returns the closest buildable TilePosition for any type of structure </summary>
	/// <param name="type"> The UnitType of the structure you want to build.</param>
	/// <param name="tile"> The TilePosition you want to build closest to.</param>
	BWAPI::TilePosition getBuildPosition(BWAPI::UnitType type, BWAPI::TilePosition searchCenter = BWAPI::Broodwar->self()->getStartLocation());

	/// <summary> Returns the closest buildable TilePosition for a defensive structure </summary>
	/// <param name="type"> The UnitType of the structure you want to build.</param>
	/// <param name="tile"> The TilePosition you want to build closest to. </param>
	BWAPI::TilePosition getDefBuildPosition(BWAPI::UnitType type, BWAPI::TilePosition tile = BWAPI::Broodwar->self()->getStartLocation());

	template <class T>
	/// <summary> Returns the estimated ground distance from one Position type to another Position type.</summary>
	/// <param name="first"> The first Position. </param>
	/// <param name="second"> The second Position. </param>
	double getGroundDistance(T start, T end);

	/// <summary> Returns the BWEM::Area of the natural expansion </summary>
	const BWEM::Area * getNaturalArea();

	/// <summary> Returns the BWEM::Area of the main </summary>
	const BWEM::Area * getMainArea();

	/// <summary> Returns the BWEM::Chokepoint of the natural </summary>
	const BWEM::ChokePoint * getNaturalChoke();

	/// <summary> Returns the BWEM::Chokepoint of the main </summary>
	const BWEM::ChokePoint * getMainChoke();

	/// Returns the TilePosition of the natural expansion
	BWAPI::TilePosition getNaturalTile();

	/// Returns the Position of the natural expansion
	BWAPI::Position getNaturalPosition();

	/// Returns the TilePosition of the main
	BWAPI::TilePosition getMainTile();

	/// Returns the Position of the main
	BWAPI::Position getMainPosition();

	/// Returns the set of used TilePositions
	std::set<BWAPI::TilePosition>& getUsedTiles();

	/// Returns two positions representing a line of best fit for a given BWEM::Chokepoint
	std::pair<BWAPI::Position, BWAPI::Position> lineOfBestFit(BWEM::ChokePoint const *);
}
