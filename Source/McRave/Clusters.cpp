#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Clusters {

    vector<Cluster> clusters;

    void createClusters()
    {
        // Delete empty clusters - solo or no commander
        clusters.erase(remove_if(clusters.begin(), clusters.end(), [&](auto& cluster) {
            return (int(cluster.units.size()) <= 1 || cluster.commander.expired());
        }), clusters.end());

        // Clear units out of the cluster
        for (auto& cluster : clusters) {
            cluster.units.clear();
            cluster.typeCounts.clear();
            cluster.sharedRadius = 160.0;
            if (auto commander = cluster.commander.lock()) {
                cluster.typeCounts[commander->getType()]++;
                cluster.units.push_back(cluster.commander);
                cluster.sharedDestination = commander->getDestination();
                cluster.sharedPosition = commander->getPosition();
            }
        }

        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;

            if (unit.getRole() != Role::Combat
                || unit.getType() == Protoss_High_Templar
                || unit.getType() == Protoss_Dark_Archon
                || unit.getType() == Protoss_Reaver
                || unit.getType() == Protoss_Interceptor
                || unit.getType() == Zerg_Defiler)
                continue;

            unit.setFormation(Positions::Invalid);

            // Check if any existing formations match this units type and common objective, get closest
            Cluster * closestCluster = nullptr;
            auto distBest = DBL_MAX;

            for (auto &cluster : clusters) {
                auto flyingCluster = any_of(cluster.units.begin(), cluster.units.end(), [&](auto &u) {
                    return u.lock()->isFlying();
                }) || (cluster.commander.lock() && cluster.commander.lock()->isFlying());
                if ((flyingCluster && !unit.isFlying()) || (!flyingCluster && unit.isFlying()))
                    continue;

                auto strategyInCommon = !cluster.commander.expired() && cluster.commander.lock()->isLightAir() && unit.isLightAir() && !unit.getGoal().isValid(); // TODO: Can we just set units destination to the commander instead?
                auto positionInCommon = unit.getPosition().getDistance(cluster.sharedPosition) < cluster.sharedRadius || unit.getPosition().getDistance(cluster.sharedDestination) < cluster.sharedRadius;
                auto destinationInCommon = unit.getDestination().getDistance(cluster.sharedDestination) < cluster.sharedRadius || unit.getDestination().getDistance(cluster.sharedPosition) < cluster.sharedRadius;
                auto dist = min({ unit.getPosition().getDistance(cluster.sharedPosition), unit.getPosition().getDistance(cluster.sharedDestination),
                                unit.getDestination().getDistance(cluster.sharedDestination), unit.getDestination().getDistance(cluster.sharedPosition) })
                        * (1.0 + cluster.sharedPosition.getDistance(cluster.sharedDestination));

                if (dist < distBest && (destinationInCommon || positionInCommon || strategyInCommon)) {
                    distBest = dist;
                    closestCluster = &cluster;
                }
            }

            if (closestCluster) {
                closestCluster->sharedRadius += unit.isLightAir() ? 160.0 : double(unit.getType().width() * unit.getType().height()) / closestCluster->sharedRadius;
                closestCluster->units.push_back(unit.weak_from_this());
                closestCluster->typeCounts[unit.getType()]++;
            }

            // Didn't find existing formation, create a new one
            else {
                Cluster newCluster(unit.getPosition(), unit.getDestination(), unit.getType());
                newCluster.units.push_back(unit.weak_from_this());
                newCluster.typeCounts[unit.getType()]++;
                clusters.push_back(newCluster);
            }
        }

        // For each cluster
        for (auto &cluster : clusters) {

            // Find a centroid
            auto avgPosition = Position(0, 0);
            auto cnt = 0;
            for (auto &u : cluster.units) {
                auto unit = u.lock();
                if (!unit->globalRetreat() && !unit->localRetreat()) {
                    avgPosition += unit->getPosition();
                    cnt++;
                }
            }
            if (cnt > 0)
                cluster.sharedPosition = avgPosition / cnt;
            else
                cluster.sharedPosition = cluster.units.begin()->lock()->getPosition();

            // If commander satisifed for a static cluster, don't try to find a new one
            auto commander = cluster.commander.lock();
            if (commander && !commander->globalRetreat() && !commander->localRetreat() && !cluster.mobileCluster && commander->getPosition().getDistance(cluster.sharedPosition) < cluster.sharedRadius)
                continue;

            // Get closest unit to centroid
            auto closestToCentroid = Util::getClosestUnit(avgPosition, PlayerState::Self, [&](auto &u) {
                return !u->isTargetedBySplash() && !u->isTargetedBySuicide() && !u->globalRetreat() && !u->localRetreat() && find(cluster.units.begin(), cluster.units.end(), u) != cluster.units.end();
            });
            if (!closestToCentroid) {
                closestToCentroid = Util::getClosestUnit(avgPosition, PlayerState::Self, [&](auto &u) {
                    return find(cluster.units.begin(), cluster.units.end(), u) != cluster.units.end();
                });
            }
            if (closestToCentroid) {
                cluster.mobileCluster = closestToCentroid->getGlobalState() != GlobalState::Retreat;                
                cluster.sharedDestination = closestToCentroid->getDestination();
                cluster.commander = closestToCentroid->weak_from_this();
                cluster.commandShare = closestToCentroid->isLightAir() ? CommandShare::Exact : CommandShare::Parallel;
                cluster.shape = closestToCentroid->isLightAir() ? Shape::None : Shape::Concave;

                // Assign commander to each unit
                for (auto &u : cluster.units) {
                    auto unit = u.lock();
                    unit->setCommander(&*closestToCentroid);
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