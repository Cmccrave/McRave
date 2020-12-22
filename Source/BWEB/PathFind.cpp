#include "PathFind.h"
#include "BWEB.h"
#include "JPS.h"

using namespace std;
using namespace BWAPI;

namespace BWEB
{
    namespace {

        struct PathCache {
            map<pair<TilePosition, TilePosition>, list<Path>::iterator> iteratorList;
            list<Path> pathCache;
            map<TilePosition, int> notReachableThisFrame;
        };
        map<function <bool(const TilePosition&)>*, PathCache> pathCache;

        int maxCacheSize = 10000;
    }

    void Path::generateBFS(function <bool(const TilePosition&)> isWalkable, bool diagonal)
    {
        auto maxDist = source.getDistance(target);
        const auto width = Broodwar->mapWidth();
        const auto height = Broodwar->mapHeight();
        vector<TilePosition> direction{ { 0, 1 },{ 1, 0 },{ -1, 0 },{ 0, -1 } };

        if (source == target
            || source == TilePosition(0, 0)
            || target == TilePosition(0, 0))
            return;

        TilePosition parentGrid[256][256];

        // This function requires that parentGrid has been filled in for a path from source to target
        const auto createPath = [&]() {
            tiles.push_back(target);
            reachable = true;
            TilePosition check = parentGrid[target.x][target.y];
            dist += Position(target).getDistance(Position(check));

            do {
                tiles.push_back(check);
                TilePosition prev = check;
                check = parentGrid[check.x][check.y];
                dist += Position(prev).getDistance(Position(check));
            } while (check != source);

            // HACK: Try to make it more accurate to positions instead of tiles
            auto correctionSource = Position(*(tiles.end() - 1));
            auto correctionTarget = Position(*(tiles.begin() + 1));
            dist += Position(source).getDistance(correctionSource);
            dist += Position(target).getDistance(correctionTarget);
            dist -= 64.0;
        };

        queue<TilePosition> nodeQueue;
        nodeQueue.emplace(source);
        parentGrid[source.x][source.y] = source;

        // While not empty, pop off top the closest TilePosition to target
        while (!nodeQueue.empty()) {
            auto const tile = nodeQueue.front();
            nodeQueue.pop();

            for (auto const &d : direction) {
                auto const next = tile + d;

                if (next.isValid()) {
                    // If next has parent or is a collision, continue
                    if (parentGrid[next.x][next.y] != TilePosition(0, 0) || !isWalkable(next))
                        continue;

                    // Check diagonal collisions where necessary
                    if ((d.x == 1 || d.x == -1) && (d.y == 1 || d.y == -1) && (!isWalkable(tile + TilePosition(d.x, 0)) || !isWalkable(tile + TilePosition(0, d.y))))
                        continue;

                    // Set parent here
                    parentGrid[next.x][next.y] = tile;

                    // If at target, return path
                    if (next == target) {
                        createPath();
                        return;
                    }

                    nodeQueue.emplace(next);
                }
            }
        }
        reachable = false;
        dist = DBL_MAX;
    }

    void Path::generateJPS(function <bool(const TilePosition&)> passedWalkable, bool diagonal)
    {
        // If this path does not exist in cache, remove last reference and erase reference
        auto &pathPoints = make_pair(source, target);
        auto &thisCached = pathCache[&passedWalkable];

        if (thisCached.iteratorList.find(pathPoints) == thisCached.iteratorList.end()) {
            if (thisCached.pathCache.size() == maxCacheSize) {
                auto last = thisCached.pathCache.back();
                thisCached.pathCache.pop_back();
                thisCached.iteratorList.erase(make_pair(last.getSource(), last.getTarget()));
            }
        }

        // If it does exist, set this path as cached version, update reference and push cached path to the front
        else {
            auto &oldPath = thisCached.iteratorList[pathPoints];
            dist = oldPath->getDistance();
            tiles = oldPath->getTiles();
            reachable = oldPath->isReachable();

            thisCached.pathCache.erase(thisCached.iteratorList[pathPoints]);
            thisCached.pathCache.push_front(*this);
            thisCached.iteratorList[pathPoints] = thisCached.pathCache.begin();
            return;
        }

        vector<TilePosition> newJPSPath;
        const auto width = Broodwar->mapWidth();
        const auto height = Broodwar->mapHeight();

        const auto isWalkable = [&](const int x, const int y) {
            const TilePosition tile(x, y);
            if (x > width || y > height || x < 0 || y < 0)
                return false;
            if (tile == source || tile == target)
                return true;
            if (passedWalkable(tile))
                return true;
            return false;
        };

        // If not reachable based on previous paths to this area
        if (target.isValid() && isWalkable(source.x, source.y)) {
            auto checkReachable = thisCached.notReachableThisFrame[target];
            if (checkReachable >= Broodwar->getFrameCount() && Broodwar->getFrameCount() > 0) {
                reachable = false;
                dist = DBL_MAX;
                return;
            }
        }

        // If we found a path, store what was found
        if (JPS::findPath(newJPSPath, isWalkable, source.x, source.y, target.x, target.y)) {
            Position current = Position(source);
            for (auto &t : newJPSPath) {
                dist += Position(t).getDistance(current);
                current = Position(t);
                tiles.push_back(t);
            }
            reachable = true;

            // Update cache 
            thisCached.pathCache.push_front(*this);
            thisCached.iteratorList[pathPoints] = thisCached.pathCache.begin();
        }

        // If not found, set destination area as unreachable for this frame
        else if (target.isValid()) {
            dist = DBL_MAX;
            thisCached.notReachableThisFrame[target] = Broodwar->getFrameCount();
            reachable = false;
        }
    }

    bool Path::terrainWalkable(const TilePosition &tile)
    {
        if (Map::isWalkable(tile, type))
            return true;
        return false;
    }

    bool Path::unitWalkable(const TilePosition &tile)
    {
        if (Map::isWalkable(tile, type) && Map::isUsed(tile) == UnitTypes::None)
            return true;
        return false;

    }

    namespace Pathfinding {
        void clearCacheFully()
        {
            pathCache.clear();
        }

        void clearCache(function <bool(const TilePosition&)> passedWalkable) {
            pathCache[&passedWalkable].iteratorList.clear();
            pathCache[&passedWalkable].pathCache.clear();
        }
    }
}