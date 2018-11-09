#include "BWEB.h"

namespace BWEB
{
	bool Map::overlapsAnything(const TilePosition here, const int width, const int height, bool ignoreBlocks)
	{
		for (auto x = here.x; x < here.x + width; x++) {
			for (auto y = here.y; y < here.y + height; y++) {
				TilePosition t(x, y);
				if (!t.isValid())
					continue;
				if (overlapGrid[x][y] > 0)
					return true;
			}
		}
		return false;
	}

	bool Map::isWalkable(const TilePosition here)
	{
		int cnt = 0;
		const auto start = WalkPosition(here);
		for (auto x = start.x; x < start.x + 4; x++) {
			for (auto y = start.y; y < start.y + 4; y++) {
				if (!WalkPosition(x, y).isValid())
					return false;
				if (!Broodwar->isWalkable(WalkPosition(x, y)))
					cnt++;
			}
		}
		return cnt <= 1;
	}

	int Map::tilesWithinArea(BWEM::Area const * area, const TilePosition here, const int width, const int height)
	{
		auto cnt = 0;
		for (auto x = here.x; x < here.x + width; x++) {
			for (auto y = here.y; y < here.y + height; y++) {
				TilePosition t(x, y);
				if (!t.isValid())
					return false;

				// Make an assumption that if it's on a chokepoint geometry, it belongs to the area provided
				if (mapBWEM.GetArea(t) == area /*|| !mapBWEM.GetArea(t)*/)
					cnt++;
			}
		}
		return cnt;
	}

	int Utils::tilesWithinArea(BWEM::Area const * area, const TilePosition here, const int width, const int height)
	{
		return BWEB::Map::Instance().tilesWithinArea(area, here, width, height);
	}

	pair<Position,Position> Map::lineOfBestFit(const BWEM::ChokePoint * choke)
	{
		auto sumX = 0.0, sumY = 0.0;
		auto sumXY = 0.0, sumX2 = 0.0;
		for (auto geo : choke->Geometry()) {
			Position p = Position(geo) + Position(4, 4);
			sumX += p.x;
			sumY += p.y;
			sumXY += p.x * p.y;
			sumX2 += p.x * p.x;
		}
		double xMean = sumX / choke->Geometry().size();
		double yMean = sumY / choke->Geometry().size();
		double denominator = sumX2 - sumX * xMean;

		double slope = (sumXY - sumX * yMean) / denominator;
		double yInt = yMean - slope * xMean;

		// y = mx + b
		// solve for x and y

		// end 1
		int x1 = Position(choke->Pos(choke->end1)).x;
		int y1 = int(ceil(x1 * slope)) + int(yInt);
		Position p1 = Position(x1, y1);

		// end 2
		int x2 = Position(choke->Pos(choke->end2)).x;
		int y2 = int(ceil(x2 * slope)) + int(yInt);
		Position p2 = Position(x2, y2);

		return make_pair(p1, p2);
	}
}