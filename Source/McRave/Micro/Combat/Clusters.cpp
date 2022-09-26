#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

#include <queue>

namespace McRave::Combat::Clusters {

    vector<Cluster> clusters;
    vector<ClusterNode> clusterNodes;
    constexpr double arcRads = 2.0944;

    vector<Color> bwColors ={ Colors::Red, Colors::Orange, Colors::Yellow, Colors::Green, Colors::Blue, Colors::Purple, Colors::Black, Colors::Grey, Colors::White };

    void createNodes()
    {
        clusterNodes.clear();

        // Delete solo clusters
        clusters.erase(remove_if(clusters.begin(), clusters.end(), [&](auto& cluster) {
            if (auto commander = cluster.commander.lock()) {
                if (commander->getType().isWorker() || commander->getType().isBuilding())
                    return true;
            }
            return (int(cluster.units.size()) <= 1);
        }), clusters.end());

        // Clear units out of clusters except commanders
        for (auto &cluster : clusters) {
            cluster.units.clear();

            if (auto commander = cluster.commander.lock()) {
                ClusterNode newNode;
                newNode.position = commander->getPosition();
                newNode.unit = &*commander;
                clusterNodes.push_back(newNode);
            }
        }

        // Every unit creates its own cluster node
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;

            if (unit.getRole() != Role::Combat
                || unit.getType() == Protoss_High_Templar
                || unit.getType() == Protoss_Dark_Archon
                || unit.getType() == Protoss_Reaver
                || unit.getType() == Protoss_Interceptor
                || unit.getType() == Zerg_Defiler)
                continue;

            unit.setCommander(nullptr);

            ClusterNode newNode;
            newNode.position = unit.getPosition();
            newNode.unit = &*u;
            clusterNodes.push_back(newNode);
        }
    }

    bool generateCluster(ClusterNode &parent, int id, int minsize)
    {
        auto getNeighbors = [&](auto &currentNode, auto &queue) {
            for (auto &node : clusterNodes) {
                auto matchedType = (parent.unit->isFlying() && node.unit->isFlying()) || (!parent.unit->isFlying() && !node.unit->isFlying());
                if (node.id == 0 && matchedType && node.unit != currentNode.unit && node.position.getDistance(currentNode.position) < 640.0)
                    queue.push(&node);
            }
        };

        std::queue<ClusterNode*> nodeQueue;
        getNeighbors(parent, nodeQueue);
        if (nodeQueue.size() < minsize)
            return false;

        // Create cluster
        Cluster newCluster;
        parent.id = id;
        newCluster.color = bwColors.at((id - 1) % bwColors.size());
        newCluster.units.push_back(parent.unit);

        // Every other node looks at neighbor and tries to add it, if enough neighbors
        while (!nodeQueue.empty()) {
            auto &node = nodeQueue.front();
            nodeQueue.pop();

            if (node->id == 0) {
                node->id = id;
                newCluster.units.push_back(node->unit);

                std::queue<ClusterNode*> neighborQueue;
                getNeighbors(*node, neighborQueue);

                if (neighborQueue.size() >= minsize) {
                    while (!neighborQueue.empty()) {
                        auto &neighbor = neighborQueue.front();
                        neighborQueue.pop();
                        if (neighbor->id == 0) {
                            neighbor->id = id;
                            nodeQueue.push(neighbor);
                            newCluster.units.push_back(neighbor->unit);
                        }
                    }
                }
            }
        }
        clusters.push_back(newCluster);
        return true;
    }

    void runDBSCAN()
    {
        auto id = 1;
        for (auto &node : clusterNodes) {
            if (node.id == 0 && generateCluster(node, id, 0))
                id++;
        }
    }

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
            return !u->isTargetedBySplash() && !u->getType().isBuilding() && !u->getType().isWorker() && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end() && !u->isTargetedBySuicide() && !u->globalRetreat() && !u->localRetreat();
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
        newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t);  });
        cluster.path = newPath;
        Visuals::drawPath(newPath);

        // If path is reachable, find a point n pixels away to set as new destination;
        cluster.sharedNavigation = cluster.sharedDestination;
        auto newDestination = Util::findPointOnPath(cluster.path, [&](Position p) {
            return p.getDistance(commander->getPosition()) >= cluster.radius;
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

    void finishClusters()
    {
        for (auto &cluster : clusters) {
            if (auto commander = cluster.commander.lock()) {
                auto count = int(cluster.units.size());

                auto fattestDimension = 0;
                auto type = UnitTypes::None;
                for (auto &unit : cluster.units) {
                    auto maxDimType = max(unit->getType().width(), unit->getType().height());
                    if (maxDimType > fattestDimension) {
                        fattestDimension = maxDimType;
                        type = unit->getType();
                    }
                }

                // Create radius based on how many units we have and want to fit into the first row of an arc
                auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0)) + (cluster.mobileCluster ? 16.0 : 0.0);
                cluster.radius = min(commander->getRetreatRadius(), (count * unitTangentSize / arcRads));

                // Offset the center by a distance of the radius towards the navigation point
                if (cluster.mobileCluster) {

                    // Clamp radius so that it moves forward/backwards as needed
                    if (commander->getLocalState() == LocalState::Attack)
                        cluster.radius = min(cluster.radius, commander->getPosition().getDistance(cluster.sharedNavigation) - 64.0);
                    if (commander->getLocalState() == LocalState::Retreat)
                        cluster.radius = max(cluster.radius, commander->getPosition().getDistance(cluster.sharedNavigation) + 64.0);
                }
                else {
                    // If we are setting up a static formation, align concave with buildings close by
                    auto closestBuilding = Util::getClosestUnit(cluster.sharedDestination, PlayerState::Self, [&](auto &u) {
                        return (u->getType().isBuilding() && u->getFormation().getDistance(cluster.sharedDestination) < 64.0);
                    });
                    if (closestBuilding)
                        cluster.radius = closestBuilding->getPosition().getDistance(cluster.sharedDestination);
                }
                pathCluster(cluster);
            }
        }
    }

    void createClusters()
    {
        // For each cluster
        for (auto &cluster : clusters) {

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

            if (auto commander = cluster.commander.lock()) {
                cluster.mobileCluster = commander->getGlobalState() != GlobalState::Retreat;
                cluster.sharedDestination = commander->getDestination();
                cluster.commandShare = commander->isLightAir() ? CommandShare::Exact : CommandShare::Parallel;
                cluster.shape = commander->isLightAir() ? Shape::None : Shape::Concave;

                // Assign commander to each unit
                for (auto &unit : cluster.units) {
                    if (unit != &*commander)
                        unit->setCommander(&*commander);
                    cluster.typeCounts[unit->getType()]++;
                    Broodwar->drawLineMap(unit->getPosition(), commander->getPosition(), Colors::Black);
                }
            }
        }
    }

    void onFrame()
    {
        createNodes();
        runDBSCAN();
        createClusters();
        finishClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}