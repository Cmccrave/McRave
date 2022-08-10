#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Clusters {

    vector<Cluster> clusters;

    Position getPathPoint(UnitInfo& unit, Position start)
    {
        // Create a pathpoint
        auto pathPoint = start;
        auto usedTile = BWEB::Map::isUsed(TilePosition(start));
        if (!BWEB::Map::isWalkable(TilePosition(start), unit.getType()) || usedTile != UnitTypes::None) {
            auto dimensions = usedTile != UnitTypes::None ? usedTile.tileSize() : TilePosition(1, 1);
            auto closest = DBL_MAX;
            for (int x = TilePosition(start).x - dimensions.x; x < TilePosition(start).x + dimensions.x + 1; x++) {
                for (int y = TilePosition(start).y - dimensions.y; y < TilePosition(start).y + dimensions.y + 1; y++) {
                    auto center = Position(TilePosition(x, y)) + Position(16, 16);
                    auto dist = center.getDistance(unit.getPosition());
                    if (dist < closest && BWEB::Map::isWalkable(TilePosition(x, y), unit.getType()) && BWEB::Map::isUsed(TilePosition(x, y)) == UnitTypes::None) {
                        closest = dist;
                        pathPoint = center;
                    }
                }
            }
        }
        return pathPoint;
    }

    void getCommander(Cluster& cluster)
    {
        // Get closest unit to centroid
        auto closestToCentroid = Util::getClosestUnit(cluster.sharedPosition, PlayerState::Self, [&](auto &u) {
            return !u->isTargetedBySplash() && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end() && !u->isTargetedBySuicide() && !u->globalRetreat() && !u->localRetreat();
        });
        if (!closestToCentroid) {
            closestToCentroid = Util::getClosestUnit(cluster.sharedPosition, PlayerState::Self, [&](auto &u) {
                return find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end();
            });
        }
        if (closestToCentroid) {
            cluster.commander = closestToCentroid->weak_from_this();
            closestToCentroid->setCommander(nullptr);
        }
    }

    void pathCluster(Cluster& cluster)
    {
        auto commander = cluster.commander.lock();
        auto pathPoint = getPathPoint(*commander, commander->getDestination());
        BWEB::Path newPath(commander->getPosition(), pathPoint, commander->getType());
        newPath.generateAS([&](const TilePosition &t) { return newPath.unitWalkable(t) ? 10000.0 / (mapBWEM.GetTile(t).MinAltitude()) : DBL_MAX; });
        cluster.path = newPath;
        Visuals::drawPath(newPath);

        // If path is reachable, find a point n pixels away to set as new destination
        auto dist = 160.0;
        auto newDestination = Util::findPointOnPath(cluster.path, [&](Position p) {
            return p.getDistance(commander->getPosition()) >= dist;
        });

        if (newDestination.isValid())
            cluster.sharedNavigation = newDestination;
    }

    void mergeClusters(Cluster& cluster1, Cluster& cluster2)
    {
        auto commander1 = cluster1.commander.lock();
        auto commander2 = cluster2.commander.lock();
        auto commander = (commander1->getPosition().getDistance(cluster1.sharedDestination) < commander2->getPosition().getDistance(cluster1.sharedDestination)) ? commander1 : commander2;

        cluster1.units.insert(cluster1.units.end(), cluster2.units.begin(), cluster2.units.end());
        cluster1.commander = commander;
        cluster2.units.clear();
        cluster2.commander.reset();
    }

    void createClusters()
    {
        // Delete solo clusters
        clusters.erase(remove_if(clusters.begin(), clusters.end(), [&](auto& cluster) {
            return (int(cluster.units.size()) <= 1);
        }), clusters.end());

        // Clear units out of the cluster
        for (auto& cluster : clusters) {
            cluster.units.clear();
            cluster.typeCounts.clear();
            cluster.sharedRadius = 160.0;
            if (cluster.commander.expired())
                getCommander(cluster);

            if (auto commander = cluster.commander.lock()) {
                cluster.typeCounts[commander->getType()]++;
                cluster.units.push_back(&*commander);
                cluster.sharedDestination = commander->getDestination();
                cluster.sharedPosition = commander->getPosition();
            }
        }

        auto colors ={ Colors::Red, Colors::Orange, Colors::Yellow, Colors::Green, Colors::Teal, Colors::Blue, Colors::Purple, Colors::Black, Colors::Brown, Colors::Black, Colors::Grey };
        auto coloritr = colors.begin();

        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;

            if (unit.getRole() != Role::Combat
                || unit.getType() == Protoss_High_Templar
                || unit.getType() == Protoss_Dark_Archon
                || unit.getType() == Protoss_Reaver
                || unit.getType() == Protoss_Interceptor
                || unit.getType() == Zerg_Defiler)
                continue;

            // Check if any existing formations match this units type and common objective, get closest
            Cluster * closestCluster = nullptr;
            auto distBest = DBL_MAX;

            for (auto &cluster : clusters) {
                auto flyingCluster = any_of(cluster.units.begin(), cluster.units.end(), [&](auto &u) {
                    return u->isFlying();
                }) || (cluster.commander.lock() && cluster.commander.lock()->isFlying());
                if ((flyingCluster && !unit.isFlying()) || (!flyingCluster && unit.isFlying()))
                    continue;

                //auto positionInCommon = unit.getPosition().getDistance(cluster.sharedPosition) < cluster.sharedRadius
                //    || unit.getPosition().getDistance(cluster.sharedDestination) < cluster.sharedRadius;
                //auto destinationInCommon = unit.getDestination().getDistance(cluster.sharedDestination) < cluster.sharedRadius
                //    || unit.getDestination().getDistance(cluster.sharedPosition) < cluster.sharedRadius;
                auto dist = min({ unit.getPosition().getDistance(cluster.sharedPosition), unit.getPosition().getDistance(cluster.sharedDestination),
                                unit.getDestination().getDistance(cluster.sharedDestination), unit.getDestination().getDistance(cluster.sharedPosition) })
                    * (1.0 + cluster.sharedPosition.getDistance(unit.getDestination()));

                if (dist < distBest && /*(destinationInCommon || positionInCommon)*/ !unit.getGoal().isValid() && !unit.getType().isBuilding()) {
                    distBest = dist;
                    closestCluster = &cluster;
                }
            }

            if (closestCluster) {
                closestCluster->sharedRadius += unit.isLightAir() ? 64.0 : double(unit.getType().width() * unit.getType().height()) / closestCluster->sharedRadius;
                closestCluster->units.push_back(&unit);
            }

            // Didn't find existing formation, create a new one
            else {
                Cluster newCluster(unit.getPosition(), unit.getDestination(), unit.getType());
                newCluster.units.push_back(&unit);
                clusters.push_back(newCluster);
            }
        }

        // Merge any similar clusters
        for (auto &cluster : clusters) {
            if (cluster.commander.expired()
                || cluster.units.empty())
                continue;

            for (auto &otherCluster : clusters) {
                if (otherCluster.commander.expired()
                    || otherCluster.units.empty())
                    continue;
                if (cluster.commander.expired()
                    || cluster.units.empty())
                    break;
                if (cluster.commander.lock()->getType() != otherCluster.commander.lock()->getType())
                    continue;

                if (cluster.commander.lock() != otherCluster.commander.lock() && cluster.sharedDestination.getDistance(otherCluster.sharedDestination) < cluster.sharedRadius)
                    mergeClusters(cluster, otherCluster);
            }
        }

        // For each cluster
        for (auto &cluster : clusters) {

            cluster.color = coloritr != colors.end() ? *coloritr : Colors::White;
            coloritr++;

            // Find a centroid
            auto avgPosition = Position(0, 0);
            auto cnt = 0;
            for (auto &unit : cluster.units) {
                if (!unit->globalRetreat() && !unit->localRetreat()) {
                    avgPosition += unit->getPosition();
                    cnt++;
                }
            }
            if (cnt > 0)
                cluster.sharedPosition = avgPosition / cnt;

            // If old commander no longer satisfactory
            auto oldCommander = cluster.commander.lock();
            if (!oldCommander || oldCommander->globalRetreat() || oldCommander->localRetreat())
                getCommander(cluster);
            if (cluster.commander.expired())
                continue;

            if (auto commander = cluster.commander.lock()) {
                cluster.mobileCluster = commander->getGlobalState() != GlobalState::Retreat;
                cluster.sharedDestination = commander->getDestination();
                cluster.commandShare = commander->isLightAir() ? CommandShare::Exact : CommandShare::Parallel;
                cluster.shape = commander->isLightAir() ? Shape::None : Shape::Concave;
                pathCluster(cluster);

                // Assign commander to each unit
                for (auto &unit : cluster.units) {
                    if (unit != &*commander)
                        unit->setCommander(&*commander);
                    cluster.typeCounts[unit->getType()]++;
                }
            }
        }

        // Delete empty clusters - empty or no commander
        clusters.erase(remove_if(clusters.begin(), clusters.end(), [&](auto& cluster) {
            return int(cluster.units.size()) < 1 || cluster.commander.expired();
        }), clusters.end());
    }

    void onFrame()
    {
        createClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}