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

        //clusters.clear();
        for (auto &[_, unit] : Combat::getCombatUnitsByDistance()) {

            if (unit.isFlying())
                continue;

            // Check if any existing formations match this units type and common objective
            bool foundCluster = false;
            for (auto &cluster : clusters) {
                if (cluster.typeCounts.find(unit.getType()) != cluster.typeCounts.end()) {
                    auto positionInCommon = unit.getPosition().getDistance(cluster.sharedPosition) < cluster.sharedRadius;
                    auto retreatInCommon = unit.getRetreat().getDistance(cluster.sharedRetreat) < cluster.sharedRadius;
                    auto objectiveInCommon = unit.getObjective().getDistance(cluster.sharedObjective) < cluster.sharedRadius;

                    if (objectiveInCommon && retreatInCommon) {
                        cluster.sharedRadius += double(unit.getType().width() * unit.getType().height()) * 3.14 / cluster.sharedRadius;
                        cluster.typeCounts[unit.getType()]++;
                        cluster.units.push_back(unit.weak_from_this());
                        foundCluster = true;
                        break;
                    }
                }
            }

            // Didn't find existing formation, create a new one
            if (!foundCluster) {
                Cluster newCluster(unit.getPosition(), unit.getRetreat(), unit.getObjective(), unit.getType());
                newCluster.units.push_back(unit.weak_from_this());
                newCluster.typeCounts[unit.getType()]++;
                clusters.push_back(newCluster);
            }
        }

        // Delete empty clusters (1 or less units)
        clusters.erase(remove_if(clusters.begin(), clusters.end(), [&](auto& cluster) {
            return int(cluster.units.size()) <= 1;
        }), clusters.end());

        // For each cluster
        for (auto &cluster : clusters) {

            // If no commander, find one
            if (cluster.commander.lock()) {
                cluster.commander.lock()->circle(Colors::Orange);
                continue;
            }

            // Find a centroid
            auto avgPosition = Position(0, 0);
            for (auto &unit : cluster.units) {
                avgPosition += unit.lock()->getPosition();
            }
            avgPosition /= cluster.units.size();

            // Get closest unit to centroid
            const auto closestToCentroid = Util::getClosestUnit(avgPosition, PlayerState::Self, [&](auto &u) {
                return find(cluster.units.begin(), cluster.units.end(), u.weak_from_this()) != cluster.units.end();
            });
            if (closestToCentroid) {
                cluster.sharedPosition = closestToCentroid->getPosition();
                cluster.sharedRetreat = closestToCentroid->getRetreat();
                cluster.sharedObjective = closestToCentroid->getObjective();
                cluster.commander = closestToCentroid->weak_from_this();
                closestToCentroid->circle(Colors::Yellow);
            }
        }
    }

    void onFrame()
    {
        createClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}