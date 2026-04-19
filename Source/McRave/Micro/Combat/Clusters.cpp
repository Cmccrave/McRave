#include "Combat.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Map/Grids.h"
#include "Map/Terrain.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

#include <queue>

namespace McRave::Combat::Clusters {

    namespace {

        vector<Cluster> clusters;
        vector<ClusterNode> clusterNodes;
        vector<weak_ptr<UnitInfo>> previousCommanders;

        vector<Color> bwColors = {Colors::Red, Colors::Orange, Colors::Yellow, Colors::Green, Colors::Blue, Colors::Purple, Colors::Black, Colors::Grey, Colors::White};

        void createNodes()
        {
            clusterNodes.clear();

            // Store previous commanders
            previousCommanders.clear();
            for (auto &cluster : clusters) {
                if (!cluster.commander.expired()) {
                    previousCommanders.push_back(cluster.commander);
                }
            }
            clusters.clear();

            // Every unit creates its own cluster node
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() != Role::Combat || unit.unit()->isLoaded() || unit.getType() == Protoss_Interceptor)
                    continue;

                unit.setCommander(nullptr);

                ClusterNode newNode;
                newNode.position = unit.getPosition();
                newNode.unit     = &*u;
                clusterNodes.push_back(newNode);
            }
        }

        bool generateCluster(ClusterNode &root, int id, int minsize, int maxsize)
        {
            // Calculate eps
            double clusterNearby = root.unit->isFlying() ? Grids::getAirDensity(root.position, PlayerState::Self) : Grids::getGroundDensity(root.position, PlayerState::Self);
            double baseEps       = 96.0 + clusterNearby * 16.0;
            double eps           = baseEps * 1.5;
            eps                  = std::min(eps, 200.0);

            auto matching = [&](auto &parent, auto &child) {
                if (child.unit->getType() == Zerg_Queen)
                    return false;

                auto matchedGoal  = parent.unit->getGoal() == child.unit->getGoal();
                auto matchedStrat = parent.unit->getGlobalState() == child.unit->getGlobalState();

                if (parent.unit->isLightAir() && child.unit->isLightAir())
                    return matchedStrat && matchedGoal;

                auto matchedType     = parent.unit->isFlying() == child.unit->isFlying();
                auto matchedDistance = abs(parent.unit->getEngDist() - child.unit->getEngDist()) < eps &&
                                       (child.position.getDistance(root.position) < eps || child.position.getDistance(parent.position) < eps);

                return matchedType && matchedStrat && matchedDistance && matchedGoal;
            };

            auto getNeighbors = [&](auto &parent, auto &queue) {
                for (auto &child : clusterNodes) {
                    if (parent.unit != child.unit && child.id == 0 && matching(parent, child))
                        queue.push(&child);
                }
            };

            std::queue<ClusterNode *> nodeQueue;
            getNeighbors(root, nodeQueue);

            // If we didn't find enough neighbors, no cluster generates
            if (int(nodeQueue.size()) < minsize)
                return false;

            // Create cluster
            Cluster newCluster;
            root.id          = id;
            newCluster.color = bwColors.at(id % 8);
            newCluster.units.push_back(root.unit);

            // Every other node looks at neighbor and tries to add it, if enough neighbors
            int x = 0;
            while (!nodeQueue.empty()) {
                auto &node = nodeQueue.front();
                nodeQueue.pop();

                if (int(newCluster.units.size()) >= maxsize && !node->unit->isFlying())
                    break;

                if (node->id == 0) {
                    node->id = id;
                    newCluster.units.push_back(node->unit);
                    getNeighbors(*node, nodeQueue);
                }
            }
            clusters.push_back(newCluster);
            return true;
        }

        void runDBSCAN()
        {
            auto id = 1;
            for (auto &node : clusterNodes) {
                if (node.id == 0 && generateCluster(node, id, 0, 500))
                    id++;
            }
        }

        void getCommander(Cluster &cluster)
        {
            // Check if a commander previously existed within a similar cluster for flying units
            auto nextCommander = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self, [&](auto &u) {
                return u->isFlying() && !u->getType().isBuilding() && !u->getType().isWorker() && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end() &&
                       find(previousCommanders.begin(), previousCommanders.end(), u) != previousCommanders.end();
            });

            // Get closest unit to centroid
            if (!nextCommander) {
                nextCommander = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self, [&](auto &u) {
                    return !u->getType().isBuilding() && !u->getType().isWorker() && !u->targetsFriendly() && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end();
                });
            }

            // Get any units for commander
            if (!nextCommander) {
                nextCommander = Util::getClosestUnit(cluster.avgPosition, PlayerState::Self,
                                                     [&](auto &u) { return !u->targetsFriendly() && find(cluster.units.begin(), cluster.units.end(), &*u) != cluster.units.end(); });
            }

            if (nextCommander) {
                cluster.commander = nextCommander->weak_from_this();
                nextCommander->setCommander(nullptr);
            }
        }

        void pathCluster(Cluster &cluster, double dist)
        {
            auto commander      = cluster.commander.lock();
            cluster.marchPath   = commander->getMarchPath();
            cluster.retreatPath = commander->getRetreatPath();

            const auto validPathPoint = [&](auto &p) {
                auto distExtra = (!Util::isAdjacentUsed(p, 3) * 32.0) + (!Util::isAdjacentUnwalkable(p, 3) * 32.0);
                return p.getDistance(commander->getPosition()) >= dist + distExtra;
            };

            // If path is reachable, find a point n pixels away to set as new destination;
            cluster.marchNavigation = cluster.marchPosition;
            const auto march        = Util::findPointOnPath(cluster.marchPath, validPathPoint);
            if (march.isValid())
                cluster.marchNavigation = march;

            // If path is reachable, find a point n pixels away to set as new destination;
            cluster.retreatNavigation = cluster.retreatPosition;
            const auto retreat        = Util::findPointOnPath(cluster.retreatPath, validPathPoint);
            if (retreat.isValid())
                cluster.retreatNavigation = retreat;

            // Remove the center to the tile from Util function above
            cluster.marchNavigation -= Position(16, 16);
            cluster.retreatNavigation -= Position(16, 16);
        }

        void fixNavigations() {}

        void finishClusters()
        {
            for (auto &cluster : clusters) {
                if (auto commander = cluster.commander.lock()) {
                    auto count  = int(cluster.units.size());
                    auto atHome = Terrain::inTerritory(PlayerState::Self, cluster.avgPosition);

                    // Calculate rough spacing of the clustered units for formation
                    auto fattestDimension = 0;
                    auto type             = UnitTypes::None;
                    for (auto &unit : cluster.units) {
                        auto maxDimType = max(unit->getType().width(), unit->getType().height());
                        if (maxDimType > fattestDimension) {
                            fattestDimension = maxDimType;
                            type             = unit->getType();
                        }
                    }
                    cluster.spacing = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0));
                    pathCluster(cluster, 96.0);

                    // Determine the state of the cluster
                    // Move to formation
                    if (commander->getLocalState() == LocalState::Hold)
                        cluster.state = LocalState::Hold;
                    else if (commander->getLocalState() == LocalState::Retreat || (!atHome && commander->getGlobalState() == GlobalState::Retreat))
                        cluster.state = LocalState::Retreat;
                    else
                        cluster.state = LocalState::Attack;

                    // Determine the shape we want
                    cluster.shape = Shape::None;
                    if (!commander->isLightAir() && !commander->isSuicidal() && !commander->getType().isWorker()) {
                        if (cluster.state == LocalState::Hold && atHome && Combat::holdAtChoke())
                            cluster.shape = Shape::Concave;
                        else
                            cluster.shape = Shape::Line;
                    }

                    // Testing
                    if (commander->isLightAir() && commander->attemptingAvoidance())
                        cluster.shape = Shape::Concave;
                }
            }
        }

        void createClusters()
        {
            // For each cluster
            for (auto &cluster : clusters) {

                // Find a centroid
                auto avgPosition = Position(0, 0);
                auto cnt         = 0;
                for (auto &unit : cluster.units) {
                    avgPosition += unit->getPosition();
                    cnt++;
                }
                if (cnt > 0)
                    cluster.avgPosition = avgPosition / cnt;

                // If old commander no longer satisfactory
                auto oldCommander = cluster.commander.lock();
                if (!oldCommander)
                    getCommander(cluster);
            }
        }

        void shapeClusters()
        {
            for (auto &cluster : clusters) {
                if (auto commander = cluster.commander.lock()) {

                    // Determine how commands are sent out
                    if (commander->isLightAir()) {

                        cluster.commandShare    = CommandShare::Exact;
                        cluster.marchPosition   = Combat::getHarassPosition();
                        cluster.retreatPosition = commander->retreatPos;
                    }
                    else {

                        cluster.commandShare    = CommandShare::Parallel;
                        cluster.marchPosition   = commander->marchPos;
                        cluster.retreatPosition = commander->retreatPos;
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
    } // namespace

    void onFrame()
    {
        createNodes();
        runDBSCAN();
        createClusters();
        shapeClusters();
        finishClusters();
        fixNavigations();
    }

    void drawClusters()
    {
        for (auto &cluster : clusters) {
            if (auto cmder = cluster.commander.lock()) {
                for (auto &unit : cluster.units) {
                    Visuals::drawLine(unit->getPosition(), cmder->getPosition(), cluster.color);
                    unit->circle(cluster.color);
                }
            }

            Visuals::drawPath(cluster.marchPath);
            Visuals::drawPath(cluster.retreatPath);

            // March
            Visuals::drawCircle(cluster.marchNavigation, 8, Colors::Green);
            Visuals::drawLine(cluster.marchNavigation, cluster.marchPosition, Colors::Green);

            // Retreat
            Visuals::drawCircle(cluster.retreatNavigation, 6, Colors::Red);
            Visuals::drawLine(cluster.retreatNavigation, cluster.retreatPosition, Colors::Red);

            // Broodwar->drawTextMap(cluster.marchNavigation, "%d", mapBWEM.GetMiniTile(WalkPosition(cluster.marchNavigation)).Altitude());
        }
    }

    vector<Cluster> &getClusters() { return clusters; }

    bool canDecimate(UnitInfo &unit, UnitInfo &target, bool logData)
    {
        if (target.isHidden() || target.movedFlag || target.isToken())
            return false;

        // Only allow certain types for cnt > 1
        auto cnt = 1;
        if (target.getType() == Terran_Goliath || target.getType() == Protoss_Dragoon || target.getType() == Terran_Science_Vessel || target.getType().isBuilding())
            cnt = 2;
        if (target.isSiegeTank())
            cnt = 3;

        // Check if this unit could load into a bunker
        if (Util::getTime() < Time(10, 00) && (target.getType() == Terran_Marine || target.getType() == Terran_SCV || target.getType() == Terran_Firebat)) {
            if (Players::getVisibleCount(PlayerState::Enemy, Terran_Bunker) > 0) {
                auto closestBunker = Util::getClosestUnit(target.getPosition(), PlayerState::Enemy, [&](auto &b) { return b->getType() == Terran_Bunker; });
                if (closestBunker && closestBunker->getPosition().getDistance(target.getPosition()) < 200.0) {
                    return false;
                }
            }
        }

        // TODO: Use cluster size instead with count of units in range
        auto multiplier  = unit.getGroundDamage() - target.getArmor();
        auto clusterSize = Grids::getAirDensity(unit.getPosition(), PlayerState::Self);
        if (clusterSize < 5)
            return false;

        // Estimate damage, padded by expected losses before we land an attack
        clusterSize -= (Util::getTime() > Time(10, 00)) + (Util::getTime() > Time(12, 00)) + (Util::getTime() > Time(14, 00));
        auto damageEstimate = clusterSize * multiplier;

        // One shotting units for free / two shotting important units
        auto easilyKilled = (target.unit()->exists() && damageEstimate * cnt >= (target.getHealth() + target.getShields())) ||
                            (!target.unit()->exists() && damageEstimate * cnt >= (target.getType().maxHitPoints() + target.getType().maxShields()));
        if (!easilyKilled)
            return false;

        // Calculate the risk
        auto unitRange  = target.isFlying() ? unit.getAirRange() : unit.getGroundRange();
        auto dpsInRange = 0.0;
        for (auto u : Units::getUnits(PlayerState::Enemy)) {
            auto &enemy = *u;
            Visuals::drawLine(unit.getPosition(), enemy.getPosition(), Colors::Red);
            if (enemy.canAttackAir() && (!enemy.isStale() || enemy.getType().isBuilding())) {

                // Have to check this estimate as engage position isn't set yet
                if (enemy.getPosition().getDistance(target.getPosition()) <= enemy.getAirReach()) {
                    dpsInRange += enemy.getDpsAgainst(unit);
                }
            }
        }

        if (logData) {
            LOG_FAST("DPS in range of target is ", dpsInRange);
            LOG_FAST("Target is ", target.getType().c_str(), " at ", target.getPosition());
        }

        if (unit.getLocalState() == LocalState::Attack)
            dpsInRange /= 2.0;

        if ((dpsInRange <= 0.0 && Players::ZvZ() && !target.getType().isWorker())                      //
            || (dpsInRange <= 2.0 && Util::getTime() < Time(8, 00))                                    //
            || (dpsInRange <= 3.0 && Util::getTime() > Time(8, 00) && Util::getTime() < Time(12, 00))  //
            || (dpsInRange <= 4.0 && Util::getTime() > Time(12, 00) && Util::getTime() < Time(16, 00)) //
            || (dpsInRange <= 5.0 && Util::getTime() > Time(16, 00)))                                  //
            return true;

        // If already in range and haven't attack recently, it's fine to swing once, this helps for recalculations done once in range
        if (unit.isWithinRange(target) && !unit.hasAttackedRecently())
            return true;
        return false;
    }
} // namespace McRave::Combat::Clusters