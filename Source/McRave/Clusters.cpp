#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Clusters {

    vector<Cluster> clusters;

    void createClusters()
    {
        // Group together units that are
        clusters.clear();
        for (auto &[_, unit] : Combat::getCombatUnitsByDistance()) {

            // Check if any existing formations match this units type and commanalities
            bool foundCluster = false;
            for (auto &cluster : clusters) {
                if (cluster.typeCounts.find(unit.getType()) != cluster.typeCounts.end()) {
                    auto positionInCommon = unit.getPosition().getDistance(cluster.sharedPosition) < cluster.sharedRadius;
                    auto destInCommon = unit.getDestination() == cluster.sharedPosition;

                    if (positionInCommon) {
                        cluster.sharedRadius += double(unit.getType().width() * unit.getType().height()) * 3.14 / cluster.sharedRadius;
                        cluster.typeCounts[unit.getType()]++;
                        cluster.units.push_back(unit.weak_from_this());
                        //Broodwar->drawLineMap(unit.getPosition(), cluster.sharedPosition, Colors::Blue);
                        foundCluster = true;
                    }
                }
            }

            // Didn't find existing formation, create a new one
            if (!foundCluster) {
                auto target = unit.getDestination();
                Cluster newCluster(unit.getPosition(), target, unit.getType());
                clusters.push_back(newCluster);
            }
        }
    }

    void onFrame()
    {
        createClusters();
    }

    vector<Cluster>& getClusters() { return clusters; }
}