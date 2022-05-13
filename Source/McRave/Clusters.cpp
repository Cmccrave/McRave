#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Clusters {

    vector<Cluster> clusters;

    void createClusters()
    {
        // Clear units out of the cluster
        for (auto& cluster : clusters) {
            cluster.units.clear();
            cluster.typeCounts.clear();
            cluster.sharedRadius = 160.0;
            if (cluster.commander.lock())
                cluster.typeCounts[cluster.commander.lock()->getType()]++;
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

            // Check if any existing formations match this units type and common objective
            bool foundCluster = false;
            for (auto &cluster : clusters) {
                auto flyingCluster = any_of(cluster.units.begin(), cluster.units.end(), [&](auto &u) {
                    return u.isFlying();
                });
                if ((flyingCluster && !unit.isFlying()) || (!flyingCluster && unit.isFlying()))
                    continue;

                auto positionInCommon = unit.getPosition().getDistance(cluster.sharedPosition) < cluster.sharedRadius;
                auto destinationInCommon = unit.getDestination().getDistance(cluster.sharedDestination) < cluster.sharedRadius;

                if (destinationInCommon) {
                    cluster.sharedRadius += unit.isLightAir() ? 0.0 : double(unit.getType().width() * unit.getType().height()) / cluster.sharedRadius;
                    cluster.units.push_back(unit);
                    cluster.typeCounts[unit.getType()]++;
                    foundCluster = true;
                    break;
                }
            }

            // Didn't find existing formation, create a new one
            if (!foundCluster) {
                Cluster newCluster(unit.getPosition(), unit.getDestination(), unit.getType());
                newCluster.units.push_back(unit);
                newCluster.typeCounts[unit.getType()]++;
                clusters.push_back(newCluster);
            }
        }

        // For each cluster
        for (auto &cluster : clusters) {

            // If commander satisifed for a static cluster, don't try to find a new one
            auto commander = cluster.commander.lock();
            if (commander && !commander->globalRetreat() && !commander->localRetreat() && !cluster.mobileCluster)
                continue;

            // Find a centroid
            auto avgPosition = Position(0, 0);
            for (auto &unit : cluster.units) {
                avgPosition += unit.getPosition();
            }
            avgPosition /= cluster.units.size();

            // Get closest unit to centroid
            auto closestToCentroid = Util::getClosestUnit(avgPosition, PlayerState::Self, [&](auto &u) {
                return !u->globalRetreat() && !u->localRetreat() && find(cluster.units.begin(), cluster.units.end(), u) != cluster.units.end();
            });
            if (!closestToCentroid) {
                closestToCentroid = Util::getClosestUnit(avgPosition, PlayerState::Self, [&](auto &u) {
                    return find(cluster.units.begin(), cluster.units.end(), u) != cluster.units.end();
                });
            }
            if (closestToCentroid) {
                cluster.mobileCluster = closestToCentroid->getGlobalState() != GlobalState::Retreat;
                cluster.sharedPosition = avgPosition;
                cluster.sharedDestination = closestToCentroid->getDestination();
                cluster.commander = closestToCentroid->weak_from_this();
                cluster.commandShare = cluster.commander.lock()->isLightAir() ? CommandShare::Exact : CommandShare::Parallel;
                cluster.shape = cluster.commander.lock()->isLightAir() ? Shape::None : Shape::Concave;

                // Assign commander to each unit
                for (auto &unit : cluster.units)
                    unit.setCommander(&*closestToCentroid);
            }
        }

        // Delete empty clusters - solo or no commander
        clusters.erase(remove_if(clusters.begin(), clusters.end(), [&](auto& cluster) {
            return cluster.units.empty() || cluster.commander.expired();
        }), clusters.end());
    }

    void onFrame()
    {
        createClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}