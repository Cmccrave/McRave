#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> concaves;


    //// See if there's a defense, set radius to at least its distance
    //auto closestDefense = Util::getClosestUnit(formation.center, PlayerState::Self, [&](auto &u) {
    //    return u.getRole() == Role::Defender && u.isCompleted() && mapBWEM.GetArea(u.getTilePosition()) == mapBWEM.GetArea(TilePosition(formation.start));
    //});
    //if (closestSelfStation && closestSelfStation->isNatural() && mapBWEM.GetArea(TilePosition(formation.start)) == closestSelfStation->getBase()->GetArea())
    //    formation.radius = closestSelfStation->getBase()->Center().getDistance(formation.center) + 32.0;
    //if (closestDefense && !Players::ZvT())
    //    formation.radius = closestDefense->getPosition().getDistance(formation.center) + 64.0;

    //// Adjust radius if a small formation exists inside it
    //for (auto &[_, otherFormation] : groundFormations) {
    //    if (otherFormation.center == formation.center && otherFormation.range < formation.range)
    //        formation.radius = max(formation.radius, otherFormation.radius + formation.unitTangentSize * 2.0);
    //}

    void createConcave(Cluster& cluster, bool includeDefenders = false)
    {
        for (auto &[type, count] : cluster.typeCounts) {

            // Determine how large of a concave we are making
            const auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0)) + 2.0;
            auto radius = count * unitTangentSize / 2.0;

            // Create an initial directional vector to find the starting location of the concave
            const auto dVectorX = double(cluster.sharedTarget.x - cluster.sharedPosition.x) / cluster.sharedTarget.getDistance(cluster.sharedPosition);
            const auto dVectorY = double(cluster.sharedTarget.y - cluster.sharedPosition.y) / cluster.sharedTarget.getDistance(cluster.sharedPosition);
            const auto dVector = Position(int(dVectorX * radius), int(dVectorY * radius));
            auto startPosition = Util::clipPosition(cluster.sharedTarget + dVector);
            Broodwar->drawCircleMap(startPosition, 4, Colors::Green, true);

            // See if there's a defense, set radius to at least its distance
            /*if (includeDefenders) {
                auto closestDefense = Util::getClosestUnit(cluster.sharedTarget, PlayerState::Self, [&](auto &u) {
                    return u.getRole() == Role::Defender && u.isCompleted() && mapBWEM.GetArea(u.getTilePosition()) == mapBWEM.GetArea(TilePosition(startPosition));
                });
                if (closestDefense && !Players::ZvT())
                    radius = closestDefense->getPosition().getDistance(cluster.sharedTarget) + 64.0;
            }*/

            // Start creating positions starting at the start position
            auto angle = BWEB::Map::getAngle(make_pair(startPosition, cluster.sharedPosition));
            auto radsPerUnit = radius / (unitTangentSize * count * 3.14);
            auto radsPositive = angle;
            auto radsNegative = angle;
            auto lastPosPosition = Positions::Invalid;
            auto lastNegPosition = Positions::Invalid;

            bool stopPositive = false;
            bool stopNegative = false;

            // Create a concave
            Formation concave;
            concave.center = cluster.sharedTarget;
            concave.type = type;
            
            while (int(concave.positions.size()) < 5000) {
                auto posPosition = cluster.sharedTarget + Position(int(radius * cos(radsPositive)), int(radius * sin(radsPositive)));
                auto negPosition = cluster.sharedTarget + Position(int(radius * cos(radsNegative)), int(radius * sin(radsNegative)));

                auto validPosition = [&](Position &p, Position &last) {
                    if (!p.isValid()
                        || Grids::getMobility(p) <= 4
                        || Util::boxDistance(type, p, type, last) <= 2)
                        return false;
                    concave.positions.push_back(p);
                    Broodwar->drawCircleMap(p, 2, Colors::Orange);
                    return true;
                };

                // Neutral position
                if (radsPositive == radsNegative && validPosition(posPosition, lastPosPosition)) {
                    radsPositive += radsPerUnit;
                    radsNegative -= radsPerUnit;
                    lastPosPosition = posPosition;
                    lastNegPosition = negPosition;
                    continue;
                }

                // Positive position
                if (!stopPositive && validPosition(posPosition, lastPosPosition)) {
                    radsPositive += radsPerUnit;
                    lastPosPosition = posPosition;
                }
                else
                    radsPositive += 3.14 / 180.0;

                // Negative position
                if (!stopNegative && validPosition(negPosition, lastNegPosition)) {
                    radsNegative -= radsPerUnit;
                    lastNegPosition = negPosition;
                }
                else
                    radsNegative -= 3.14 / 180.0;

                if (radsPositive > angle + 1.57)
                    stopPositive = true;
                if (radsNegative < angle - 1.57)
                    stopNegative = true;

                if (stopPositive && stopNegative) {
                    //stopPositive = false;
                    //stopNegative = false;
                    //radius += unitTangentSize;
                    //radsPositive = angle;
                    //radsNegative = angle;
                    break;
                }
            }

            for (auto &unit : cluster.units) {
                unit.lock()->setFormation(concave.center);
            }
            for (auto &position : concave.positions) {
                //Broodwar->drawLineMap(position, concave.center, Colors::Blue);
                Broodwar->drawCircleMap(position, 4, Colors::Blue);
            }
            concaves.push_back(concave);
        }
    }

    void createLine(Cluster& cluster)
    {

    }

    void createBox(Cluster& cluster)
    {

    }

    void createFormations()
    {
        // Create formations of each cluster
        // Each cluster has a UnitType and count, make a formation that considers each
        for (auto &cluster : Clusters::getClusters()) {
            if (cluster.shape == Shape::Concave, true)
                createConcave(cluster);
            else if (cluster.shape == Shape::Line)
                createLine(cluster);
            else if (cluster.shape == Shape::Box)
                createBox(cluster);
        }
    }

    void onFrame()
    {
        concaves.clear();
        createFormations();
    }

    vector<Formation>& getConcaves() { return concaves; }
}