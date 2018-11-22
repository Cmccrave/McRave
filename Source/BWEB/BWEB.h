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

	// UNSORTED BELOW THIS LINE
	// -------------------------------------

	// Map		
	inline BWAPI::Position mainPosition, naturalPosition;
	inline BWAPI::TilePosition mainTile, naturalTile;
	inline const BWEM::Area * naturalArea{};
	inline const BWEM::Area * mainArea{};
	inline const BWEM::ChokePoint * naturalChoke{};
	inline const BWEM::ChokePoint * mainChoke{};
	inline std::set<BWAPI::TilePosition> usedTiles;
	inline void addOverlap(BWAPI::TilePosition, int, int);
	inline bool isPlaceable(BWAPI::UnitType, BWAPI::TilePosition);

	// General
	inline int testGrid[256][256];
	inline int reserveGrid[256][256] ={};
	inline int overlapGrid[256][256] ={};
	inline int usedGrid[256][256] ={};


	inline void draw(), onStart(), onUnitDiscover(BWAPI::Unit), onUnitDestroy(BWAPI::Unit), onUnitMorph(BWAPI::Unit);

	/// This is just put here so pathfinding can use it for now
	inline BWAPI::UnitType overlapsCurrentWall(BWAPI::TilePosition tile, int width = 1, int height = 1);

	inline bool overlapsAnything(BWAPI::TilePosition here, int width = 1, int height = 1, bool ignoreBlocks = false);
	inline bool isWalkable(BWAPI::TilePosition);
	inline int tilesWithinArea(BWEM::Area const *, BWAPI::TilePosition here, int width = 1, int height = 1);

	/// <summary> Returns the closest buildable TilePosition for any type of structure </summary>
	/// <param name="type"> The UnitType of the structure you want to build.</param>
	/// <param name="tile"> The TilePosition you want to build closest to.</param>
	inline BWAPI::TilePosition getBuildPosition(BWAPI::UnitType type, BWAPI::TilePosition searchCenter = BWAPI::Broodwar->self()->getStartLocation());

	/// <summary> Returns the closest buildable TilePosition for a defensive structure </summary>
	/// <param name="type"> The UnitType of the structure you want to build.</param>
	/// <param name="tile"> The TilePosition you want to build closest to. </param>
	inline BWAPI::TilePosition getDefBuildPosition(BWAPI::UnitType type, BWAPI::TilePosition tile = BWAPI::Broodwar->self()->getStartLocation());

	template <class T>
	/// <summary> Returns the estimated ground distance from one Position type to another Position type.</summary>
	/// <param name="first"> The first Position. </param>
	/// <param name="second"> The second Position. </param>
	inline double getGroundDistance(T start, T end);

	/// <summary> Returns the BWEM::Area of the natural expansion </summary>
	inline const BWEM::Area * getNaturalArea() { return naturalArea; }

	/// <summary> Returns the BWEM::Area of the main </summary>
	inline const BWEM::Area * getMainArea() { return mainArea; }

	/// <summary> Returns the BWEM::Chokepoint of the natural </summary>
	inline const BWEM::ChokePoint * getNaturalChoke() { return naturalChoke; }

	/// <summary> Returns the BWEM::Chokepoint of the main </summary>
	inline const BWEM::ChokePoint * getMainChoke() { return mainChoke; }

	/// Returns the TilePosition of the natural expansion
	inline BWAPI::TilePosition getNaturalTile() { return naturalTile; }

	/// Returns the Position of the natural expansion
	inline BWAPI::Position getNaturalPosition() { return naturalPosition; }

	/// Returns the TilePosition of the main
	inline BWAPI::TilePosition getMainTile() { return mainTile; }

	/// Returns the Position of the main
	inline BWAPI::Position getMainPosition() { return mainPosition; }

	/// Returns the set of used TilePositions
	inline std::set<BWAPI::TilePosition>& getUsedTiles() { return usedTiles; }

	/// Returns two positions representing a line of best fit for a given BWEM::Chokepoint
	inline std::pair<BWAPI::Position, BWAPI::Position> lineOfBestFit(BWEM::ChokePoint const *);
}
