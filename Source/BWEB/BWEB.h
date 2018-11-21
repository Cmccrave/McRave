#pragma once
#pragma warning(disable : 4351)
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB
{
	namespace Map
	{
		// Needed
		BWEM::Map& mapBWEM;	

		// Unneeded
		void findMain(), findMainChoke(), findNatural(), findNaturalChoke(), findNeutrals();

		// UNSORTED BELOW THIS LINE
		// -------------------------------------

		// Map		
		BWAPI::Position mainPosition, naturalPosition;
		BWAPI::TilePosition mainTile, naturalTile;
		const BWEM::Area * naturalArea{};
		const BWEM::Area * mainArea{};
		const BWEM::ChokePoint * naturalChoke{};
		const BWEM::ChokePoint * mainChoke{};
		std::set<BWAPI::TilePosition> usedTiles;
		void addOverlap(BWAPI::TilePosition, int, int);
		bool isPlaceable(BWAPI::UnitType, BWAPI::TilePosition);
		
		// General
		int testGrid[256][256];
		int reserveGrid[256][256] ={};

		void draw(), onStart(), onUnitDiscover(BWAPI::Unit), onUnitDestroy(BWAPI::Unit), onUnitMorph(BWAPI::Unit);
		int overlapGrid[256][256] = {};
		int usedGrid[256][256] ={};

		/// This is just put here so pathfinding can use it for now
		BWAPI::UnitType overlapsCurrentWall(BWAPI::TilePosition tile, int width = 1, int height = 1);

		bool overlapsAnything(BWAPI::TilePosition here, int width = 1, int height = 1, bool ignoreBlocks = false);
		static bool isWalkable(BWAPI::TilePosition);
		int tilesWithinArea(BWEM::Area const *, BWAPI::TilePosition here, int width = 1, int height = 1);

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
		const BWEM::Area * getNaturalArea() { return naturalArea; }

		/// <summary> Returns the BWEM::Area of the main </summary>
		const BWEM::Area * getMainArea() { return mainArea; }

		/// <summary> Returns the BWEM::Chokepoint of the natural </summary>
		const BWEM::ChokePoint * getNaturalChoke() { return naturalChoke; }

		/// <summary> Returns the BWEM::Chokepoint of the main </summary>
		const BWEM::ChokePoint * getMainChoke() { return mainChoke; }
		
		/// Returns the TilePosition of the natural expansion
		BWAPI::TilePosition getNaturalTile() { return naturalTile; }

		/// Returns the Position of the natural expansion
		BWAPI::Position getNaturalPosition() { return naturalPosition; }

		/// Returns the TilePosition of the main
		BWAPI::TilePosition getMainTile() { return mainTile; }

		/// Returns the Position of the main
		BWAPI::Position getMainPosition() { return mainPosition; }

		/// Returns the set of used TilePositions
		std::set<BWAPI::TilePosition>& getUsedTiles() { return usedTiles; }		

		static std::pair<BWAPI::Position, BWAPI::Position> lineOfBestFit(BWEM::ChokePoint const *);
	};


	// Create a pathfind.h and move this to it
	class Path {
		std::vector<BWAPI::TilePosition> tiles;
		double dist;		
	public:
		Path();
		std::vector<BWAPI::TilePosition>& getTiles() { return tiles; }
		double getDistance() { return dist; }
		void createUnitPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position);
		void createWallPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position, bool);

		void createPath(BWEM::Map&, const BWAPI::Position, const BWAPI::Position, std::function <bool(const BWAPI::TilePosition)>, std::vector<BWAPI::TilePosition>);
	};
}
