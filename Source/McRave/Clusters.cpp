#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Clusters {

    vector<Cluster> clusters;

    void createClusters()
    {
        clusters.clear();
        for (auto &[_, unit] : Combat::getCombatUnitsByDistance()) {

            // Check if any existing formations match this units type and common objective
            bool foundCluster = false;
            for (auto &cluster : clusters) {
                if (cluster.typeCounts.find(unit.getType()) != cluster.typeCounts.end()) {
                    auto destinationInCommon = unit.getObjective().getDistance(cluster.sharedTarget) < cluster.sharedRadius || unit.getObjective() == cluster.sharedTarget;

                    if (destinationInCommon) {
                        cluster.sharedRadius += double(unit.getType().width() * unit.getType().height()) * 3.14 / cluster.sharedRadius;
                        cluster.typeCounts[unit.getType()]++;
                        cluster.units.push_back(unit.weak_from_this());
                        foundCluster = true;
                    }
                }
            }

            // Didn't find existing formation, create a new one
            if (!foundCluster) {
                Cluster newCluster(unit.getPosition(), unit.getObjective(), unit.getType());
                clusters.push_back(newCluster);
            }
        }

        for (auto &cluster : clusters)
        {
            Broodwar->drawCircleMap(cluster.sharedTarget, 3, Colors::Red);
        }
    }

    void onFrame()
    {
        createClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}