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
                auto matchedGoal = (parent.unit->getGoal() == child.unit->getGoal());
                auto matchedType = (parent.unit->isFlying() && child.unit->isFlying()) || (!parent.unit->isFlying() && !child.unit->isFlying());
                auto matchedStrat = (parent.unit->getGlobalState() == GlobalState::Attack && parent.unit->getLocalState() == child.unit->getLocalState())
                    || (parent.unit->getGlobalState() == GlobalState::Retreat && parent.unit->getRetreat() == child.unit->getRetreat())
                    || (parent.unit->getGlobalState() == GlobalState::Attack && parent.unit->isLightAir() && child.unit->isLightAir())
                    || (parent.unit->getGlobalState() == child.unit->getGlobalState() && parent.unit->getLocalState() == child.unit->getLocalState());
                auto matchedDistance = child.position.getDistance(parent.position) < 256.0
                    || (Terrain::inTerritory(PlayerState::Self, parent.unit->getPosition()) && Terrain::inTerritory(PlayerState::Self, child.unit->getPosition()))
                    || (parent.unit->isLightAir() && child.unit->isLightAir());
                return matchedType && matchedStrat && matchedDistance && matchedGoal;
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

        void getCommander(Cluster& cluster)
        {
            // Check if a commander previously existed within a similar cluster
            auto closestPreviousCommander = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self, [&](auto &u) {
                return !u->isTargetedBySplash() && !u->getType().isBuilding() && !u->getType().isWorker()
                    && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end()
                    && find(previousCommanders.begin(), previousCommanders.end(), u) != previousCommanders.end()
                    && !u->isTargetedBySuicide();
            });
            if (closestPreviousCommander) {
                cluster.commander = closestPreviousCommander->weak_from_this();
                closestPreviousCommander->setCommander(nullptr);
                return;
            }

            // Get closest unit to centroid
            auto closestToCentroid = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self, [&](auto &u) {
                return !u->isTargetedBySplash() && !u->getType().isBuilding() && !u->getType().isWorker()
                    && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end() && !u->isTargetedBySuicide();
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
            auto marchPathPoint = Util::getPathPoint(*commander, cluster.marchPosition);
            BWEB::Path newMarchPath(cluster.avgPosition, marchPathPoint, commander->getType());
            newMarchPath.generateJPS([&](const TilePosition &t) { return newMarchPath.unitWalkable(t);  });
            cluster.marchPath = newMarchPath;

            // If path is reachable, find a point n pixels away to set as new destination;
            cluster.marchNavigation = cluster.marchPosition;
            const auto march = Util::findPointOnPath(cluster.marchPath, [&](Position p) {
                if (mapBWEM.GetMiniTile(WalkPosition(p)).Altitude() * 4 < cluster.units.size()) {
                    Visuals::drawBox(TilePosition(p), TilePosition(1, 1), Colors::Red);
                    return false;
                }
                return p.getDistance(cluster.avgPosition) >= dist;
            });
            if (march.isValid())
                cluster.marchNavigation = march;            

            auto retreatPathPoint = Util::getPathPoint(*commander, cluster.retreatPosition);
            BWEB::Path newRetreatPath(cluster.avgPosition, retreatPathPoint, commander->getType());
            newRetreatPath.generateJPS([&](const TilePosition &t) { return newRetreatPath.unitWalkable(t);  });
            cluster.retreatPath = newRetreatPath;

            // If path is reachable, find a point n pixels away to set as new destination;
            cluster.retreatNavigation = cluster.retreatPosition;
            const auto retreat = Util::findPointOnPath(cluster.retreatPath, [&](Position p) {
                if (mapBWEM.GetMiniTile(WalkPosition(p)).Altitude() * 4 < cluster.units.size()) {
                    Visuals::drawBox(TilePosition(p), TilePosition(1, 1), Colors::Red);
                    return false;
                }
                return p.getDistance(cluster.avgPosition) >= dist;
            });
            if (retreat.isValid())
                cluster.retreatNavigation = retreat;
        }

        void fixNavigations()
        {
            // Create new navigation points that are more centered to the terrain
            for (auto &cluster : clusters) {

                const auto desiredAltitude = int(cluster.units.size()) * 4 * 8;

                const auto newNavigations = [&](Position navigation) {
                    auto perpAngle = BWEB::Map::getAngle(make_pair(navigation, cluster.avgPosition)) + 1.57;
                    auto size = 32;
                    auto newNav = navigation;
                    auto currentAltitude = 0;

                    // Now take the center and try to shift it perpendicular towards lower altitude
                    while (mapBWEM.GetMiniTile(WalkPosition(newNav)).Altitude() < desiredAltitude) {
                        auto p1 = Util::clipPosition(newNav - Position(-size * cos(perpAngle), size * sin(perpAngle)));
                        auto p2 = Util::clipPosition(newNav + Position(-size * cos(perpAngle), size * sin(perpAngle)));
                        auto altitude1 = mapBWEM.GetMiniTile(WalkPosition(p1)).Altitude();
                        auto altitude2 = mapBWEM.GetMiniTile(WalkPosition(p2)).Altitude();

                        if (altitude1 > altitude2 && altitude1 > currentAltitude) {
                            newNav = p1;
                            size = 32;
                            currentAltitude = altitude1;
                        }
                        else if (altitude2 > currentAltitude) {
                            newNav = p2;
                            size = 32;
                            currentAltitude = altitude2;
                        }
                        else if (altitude1 == 0 && altitude2 == 0)
                            size+=32;
                        else
                            break;
                    }
                    return newNav;
                };

                cluster.marchNavigation = newNavigations(cluster.marchNavigation);
                cluster.retreatNavigation = newNavigations(cluster.retreatNavigation);
            }
        }

        void finishClusters()
        {
            for (auto &cluster : clusters) {
                if (auto commander = cluster.commander.lock()) {
                    auto count = int(cluster.units.size());

                    // Calculate rough spacing of the clustered units for formation
                    auto fattestDimension = 0;
                    auto type = UnitTypes::None;
                    for (auto &unit : cluster.units) {
                        auto maxDimType = max(unit->getType().width(), unit->getType().height());
                        if (maxDimType > fattestDimension) {
                            fattestDimension = maxDimType;
                            type = unit->getType();
                        }
                    }
                    cluster.spacing = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0)) + (cluster.mobileCluster ? 2.0 : 0.0);

                    
                    auto sizeSpacing = cluster.spacing * count / 2.0;
                    auto speedSpacing = commander->getSpeed() * 48;
                    
                    auto dist = min(sizeSpacing, speedSpacing);
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
                    avgPosition += unit->getPosition();
                    cnt++;
                }
                if (cnt > 0)
                    cluster.avgPosition = avgPosition / cnt;

                // If old commander no longer satisfactory
                auto oldCommander = cluster.commander.lock();
                if (!oldCommander || oldCommander->getGlobalState() == GlobalState::ForcedRetreat || oldCommander->getLocalState() == LocalState::ForcedRetreat)
                    getCommander(cluster);
            }
        }

        void shapeClusters()
        {
            for (auto &cluster : clusters) {
                if (auto commander = cluster.commander.lock()) {
                    const auto retreatCluster = commander->getGlobalState() == GlobalState::Retreat || commander->getGlobalState() == GlobalState::ForcedRetreat;

                    // Determine the directions of the cluster and whether it's moving
                    if (Terrain::inTerritory(PlayerState::Self, commander->getPosition()) && retreatCluster) {
                        cluster.mobileCluster = false;
                        cluster.marchPosition = commander->getDestination();
                        cluster.retreatPosition = commander->getRetreat();
                    }
                    else {
                        cluster.mobileCluster = true;
                        cluster.marchPosition = commander->getDestination();
                        cluster.retreatPosition = commander->getRetreat();
                    }

                    // Determine how commands are sent out
                    if (commander->isLightAir())
                        cluster.commandShare = CommandShare::Exact;                    
                    else
                        cluster.commandShare = CommandShare::Parallel;

                    // Determine the shape we want
                    if (!commander->isLightAir() && !commander->isSuicidal()) {
                        if (!cluster.mobileCluster && Combat::holdAtChoke() && retreatCluster)
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
                //for (auto &unit : cluster.units) {
                //    Visuals::drawLine(unit->getPosition(), cluster.commander.lock()->getPosition(), cluster.color);
                //    unit->circle(cluster.color);
                //}

                //Visuals::drawPath(cluster.marchPath);
                //Visuals::drawPath(cluster.retreatPath);

                Visuals::drawCircle(cluster.marchNavigation, 4, Colors::Green);
                Visuals::drawCircle(cluster.retreatNavigation, 4, Colors::Red);

                Broodwar->drawTextMap(cluster.marchNavigation, "%d", mapBWEM.GetMiniTile(WalkPosition(cluster.marchNavigation)).Altitude());
            }
        }
    }

    void onFrame()
    {
        createNodes();
        runDBSCAN();
        createClusters();
        shapeClusters();
        finishClusters();
        fixNavigations();
        drawClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}