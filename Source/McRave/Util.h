#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	struct Line {
		double yInt;
		double slope;
		double y(int x) { return (slope * double(x)) + yInt; }
		Line(double y, double s) {
			yInt = y, slope = s;
		}
	};

	class UtilManager
	{
	public:

		UnitInfo * getClosestUnit(Position, Player, UnitType t = UnitTypes::None);
		UnitInfo * getClosestUnit(UnitInfo&, Player, UnitType t = UnitTypes::None);
		UnitInfo * getClosestThreat(UnitInfo&);
		UnitInfo * getClosestBuilder(Position);

		int chokeWidth(const BWEM::ChokePoint *);
		const BWEM::ChokePoint * getClosestChokepoint(Position);

		double getHighestThreat(WalkPosition, UnitInfo&);

		bool unitInRange(UnitInfo& unit);
		bool reactivePullWorker(UnitInfo& unit);
		bool proactivePullWorker(UnitInfo& unit);
		bool pullRepairWorker(UnitInfo& unit);
		bool accurateThreatOnPath(UnitInfo&, Path&);
		bool rectangleIntersect(Position, Position, Position);

		// Walkability checks
		template<class T>
		bool isWalkable(T here)
		{
			auto start = WalkPosition(here);
			for (int x = start.x; x < start.x + 4; x++) {
				for (int y = start.y; y < start.y + 4; y++) {
					if (Grids().getMobility(WalkPosition(x, y)) == -1)
						return false;
				}
			}
			return true;
		}
		bool isWalkable(WalkPosition start, WalkPosition finish, UnitType);

		// Create a line of best fit for a chokepoint
		Line lineOfBestFit(const BWEM::ChokePoint *);
		Line parallelLine(Line, double);

		Position getConcavePosition(UnitInfo&, BWEM::Area const * area = nullptr, Position here = Positions::Invalid);
	};
}

typedef Singleton<McRave::UtilManager> UtilSingleton;
