#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

#include <queue>

namespace McRave::Combat::Clusters {

    namespace {

        vector<Cluster> clusters;
        vector<ClusterNode> clusterNodes;
        vector<weak_ptr<UnitInfo>> previousCommanders;
        constexpr double arcRads = 2.0944;

        vector<Color> bwColors ={ Colors::Red, Colors::Orange, Colors::Yellow, Colors::Green, Colors::Blue, Colors::Purple, Colors::Black, Colors::Grey, Colors::White };

        void createNodes()
        {
            clusterNodes.clear();

            // Store previous commanders
            previousCommanders.clear();
            for (auto &cluster : clusters) {
                if (!cluster.commander.expired())
                    previousCommanders.push_back(cluster.commander);
            }
            clusters.clear();

            // Every unit creates its own cluster node
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() != Role::Combat
                    || unit.unit()->isLoaded()
                    || unit.getType() == Protoss_Interceptor)
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
            auto matching = [&](auto &parent, auto &child) {
                auto matchedType = (parent.unit->isFlying() && child.unit->isFlying()) || (!parent.unit->isFlying() && !child.unit->isFlying());
                auto matchedStrat = parent.unit->getLocalState() == child.unit->getLocalState()
                    || parent.unit->getLocalState() == LocalState::None
                    || child.unit->getLocalState() == LocalState::None
                    || (parent.unit->getGlobalState() == GlobalState::Retreat && parent.unit->getRetreat() == child.unit->getRetreat())
                    || (parent.unit->getGlobalState() == GlobalState::Attack && parent.unit->isLightAir() && child.unit->isLightAir());
                auto matchedDistance = child.position.getDistance(parent.position) < 160.0
                    || (Terrain::inTerritory(PlayerState::Self, parent.unit->getPosition()) && Terrain::inTerritory(PlayerState::Self, child.unit->getPosition()))
                    || (parent.unit->isLightAir() && child.unit->isLightAir());
                return matchedType && matchedStrat && matchedDistance;
            };

            auto getNeighbors = [&](auto &currentNode, auto &queue) {
                for (auto &child : clusterNodes) {
                    if (child.id == 0 && matching(parent, child))
                        queue.push(&child);
                }
            };

            std::queue<ClusterNode*> nodeQueue;
            getNeighbors(parent, nodeQueue);
            if (int(nodeQueue.size()) < minsize)
                return false;

            // Create cluster
            Cluster newCluster;
            parent.id = id;
            newCluster.color = bwColors.at(id % 8);
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

                    if (int(neighborQueue.size()) >= minsize) {
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
            // Check if a commander previously existed within a similar cluster
            auto closestPreviousCommander = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self, [&](auto &u) {
                return !u->isTargetedBySplash() && !u->getType().isBuilding() && !u->getType().isWorker()
                    && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end()
                    && find(previousCommanders.begin(), previousCommanders.end(), u) != previousCommanders.end()
                    && !u->isTargetedBySuicide() && !u->globalRetreat() && !u->localRetreat();
            });
            if (closestPreviousCommander) {
                cluster.commander = closestPreviousCommander->weak_from_this();
                closestPreviousCommander->setCommander(nullptr);
                return;
            }

            // Get closest unit to centroid
            auto closestToCentroid = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self, [&](auto &u) {
                return !u->isTargetedBySplash() && !u->getType().isBuilding() && !u->getType().isWorker() && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end() && !u->isTargetedBySuicide() && !u->globalRetreat() && !u->localRetreat();
            });
            if (!closestToCentroid) {
                closestToCentroid = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self, [&](auto &u) {
                    return find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end();
                });
            }
            if (closestToCentroid) {
                cluster.commander = closestToCentroid->weak_from_this();
                closestToCentroid->setCommander(nullptr);
            }
        }

        void pathCluster(Cluster& cluster, double dist)
        {
            auto commander = cluster.commander.lock();
            auto marchPathPoint = getPathPoint(*commander, cluster.marchPosition);
            BWEB::Path newMarchPath(commander->getPosition(), marchPathPoint, commander->getType());
            newMarchPath.generateJPS([&](const TilePosition &t) { return newMarchPath.unitWalkable(t);  });
            cluster.marchPath = newMarchPath;
            //Visuals::drawPath(cluster.marchPath);

            // If path is reachable, find a point n pixels away to set as new destination;
            cluster.marchNavigation = cluster.marchPosition;
            const auto march = Util::findPointOnPath(cluster.marchPath, [&](Position p) {
                return p.getDistance(commander->getPosition()) >= max(160.0, dist - 32.0);
            });
            if (march.isValid())
                cluster.marchNavigation = march;

            auto retreatPathPoint = getPathPoint(*commander, cluster.retreatPosition);
            BWEB::Path newRetreatPath(commander->getPosition(), retreatPathPoint, commander->getType());
            newRetreatPath.generateJPS([&](const TilePosition &t) { return newRetreatPath.unitWalkable(t);  });
            cluster.retreatPath = newRetreatPath;
            //Visuals::drawPath(cluster.retreatPath);

            // If path is reachable, find a point n pixels away to set as new destination;
            cluster.retreatNavigation = cluster.retreatPosition;
            const auto retreat = Util::findPointOnPath(cluster.retreatPath, [&](Position p) {
                return p.getDistance(commander->getPosition()) >= max(160.0, dist - 32.0);
            });
            if (retreat.isValid())
                cluster.retreatNavigation = retreat;
        }

        void mergeClusters(Cluster& cluster1, Cluster& cluster2)
        {
            auto commander1 = cluster1.commander.lock();
            auto commander2 = cluster2.commander.lock();
            auto commander = (commander1->getPosition().getDistance(cluster1.avgPosition) < commander2->getPosition().getDistance(cluster1.avgPosition)) ? commander1 : commander2;

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

                    // Calculate rough spacing of the clustered units for formation and find path points
                    cluster.spacing = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0)) + (cluster.mobileCluster ? 16.0 : 0.0);
                    auto dist = min(commander->getRetreatRadius(), (count * cluster.spacing / arcRads));
                    pathCluster(cluster, dist);
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
                    cluster.avgPosition = avgPosition / cnt;

                // If old commander no longer satisfactory
                auto oldCommander = cluster.commander.lock();
                if (!oldCommander || oldCommander->globalRetreat() || oldCommander->localRetreat())
                    getCommander(cluster);

                if (auto commander = cluster.commander.lock()) {
                    auto retreatStation = Stations::getClosestRetreatStation(*commander);
                    cluster.mobileCluster = commander->getGlobalState() != GlobalState::Retreat || !Terrain::inTerritory(PlayerState::Self, commander->getPosition());
                    cluster.marchPosition = (commander->getGlobalState() != GlobalState::Retreat && commander->hasTarget()) ? commander->getTarget().lock()->getPosition() : commander->getDestination();
                    cluster.retreatPosition = commander->getRetreat();
                    cluster.commandShare = commander->isLightAir() ? CommandShare::Exact : CommandShare::Parallel;
                    cluster.shape = commander->isLightAir() ? Shape::None : Shape::Concave;

                    // Determine the shape we want
                    if (!commander->isLightAir()) {
                        if (!cluster.mobileCluster && Combat::holdAtChoke() && commander->getGlobalState() == GlobalState::Retreat)
                            cluster.shape = Shape::Choke;
                        else
                            cluster.shape = Shape::Concave;
                    }

                    // Assign commander to each unit
                    for (auto &unit : cluster.units) {
                        if (unit != &*commander)
                            unit->setCommander(&*commander);
                        cluster.typeCounts[unit->getType()]++;
                    }
                }
            }
        }

        void drawClusters()
        {
            for (auto &cluster : clusters) {
                for (auto &unit : cluster.units) {
                    Visuals::drawLine(unit->getPosition(), cluster.commander.lock()->getPosition(), cluster.color);
                    unit->circle(cluster.color);
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
        //drawClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}