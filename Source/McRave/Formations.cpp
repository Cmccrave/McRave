#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> concaves;

    void assignPosition(Cluster& cluster, Formation& concave, Position p, int& assignmentsRemaining)
    {
        UnitInfo* closestUnit = nullptr;
        auto distBest = DBL_MAX;
        for (auto &unit : cluster.units) {
            auto dist = unit.getPosition().getDistance(p);
            if (dist < distBest && !unit.concaveFlag && unit.getLocalState() != LocalState::Attack) {
                distBest = dist;
                closestUnit = &unit;
            }
        }
        auto closestBuilder = Util::getClosestUnit(p, PlayerState::Self, [&](auto &u) {
            return u->getBuildPosition().isValid();
        });

        if (!closestUnit
            || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 128.0)
            || Planning::overlapsPlan(*closestUnit, p)
            || Actions::overlapsActions(closestUnit->unit(), p, TechTypes::Spider_Mines, PlayerState::Enemy, 96)
            || !Util::findWalkable(*closestUnit, p))
            return;

        // TODO: Check if we would be in range of an enemy before a defense (defending only)
        auto inRange = closestUnit->getPosition().getDistance(concave.center) - 64 < p.getDistance(concave.center)
            || closestUnit->getPosition().getDistance(p) < 160.0
            || (cluster.mobileCluster && closestUnit->getPosition().getDistance(p) < 256.0);

        if (inRange) {
            closestUnit->setFormation(p);
            closestUnit->setDestination(p);
            closestUnit->concaveFlag = true;
            Broodwar->drawLineMap(closestUnit->getPosition(), p, Colors::Orange);
        }
        assignmentsRemaining--;
    }

    void createConcave(Cluster& cluster)
    {
        for (auto &[type, count] : cluster.typeCounts) {
            auto commander = cluster.commander.lock();

            // Create a concave
            Formation concave;
            concave.cluster = &cluster;
            concave.center = commander->getDestination();

            // Set the radius of the concave
            const auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0));
            auto radius = count * unitTangentSize / 2.0;
            bool useDefense = false;

            if (commander->getLocalState() != LocalState::Retreat && commander->hasTarget())
                radius = max(0.0, double(Util::boxDistance(commander->getType(), commander->getPosition(), commander->getTarget().lock()->getType(), commander->getTarget().lock()->getPosition())) - 64.0);
            if (radius < 1.0)
                continue;

            // If we are setting up a static formation, align concave with buildings close by
            if (!cluster.mobileCluster) {
                auto closestDefense = Util::getClosestUnit(cluster.sharedDestination, PlayerState::Self, [&](auto &u) {
                    return u->getType().isBuilding() && u->isCompleted() && u->getFormation() == cluster.sharedDestination;
                });
                if (closestDefense) {
                    radius = closestDefense->getPosition().getDistance(cluster.sharedDestination);
                    useDefense = true;
                }
            }

            // Get a retreat point
            auto retreat = Stations::getClosestRetreatStation(*commander);
            if (!retreat)
                continue;

            // Start creating positions starting at the start position
            auto startPosition = Util::getClosestPointToRadiusGround(retreat->getBase()->Center(), cluster.sharedDestination, 32).second;
            auto angle = BWEB::Map::getAngle(make_pair(startPosition, concave.center));
            auto radsPerUnit = min(radius / (unitTangentSize * count * 3.14), unitTangentSize / (1.0 * radius));
            auto radsPositive = angle;
            auto radsNegative = angle;
            auto lastPosPosition = Positions::Invalid;
            auto lastNegPosition = Positions::Invalid;

            bool stopPositive = false;
            bool stopNegative = false;
            auto wrap = 0;

            Broodwar->drawLineMap(commander->getPosition(), concave.center, Colors::Purple);
            Broodwar->drawCircleMap(retreat->getBase()->Center(), 4, Colors::Red, true);
            Broodwar->drawCircleMap(concave.center, 6, Colors::White);

            auto assignmentsRemaining = int(cluster.units.size());
            while (assignmentsRemaining > 0) {
                auto rp = Position(int(radius * cos(radsPositive)), int(radius * sin(radsPositive)));
                auto rn = Position(int(radius * cos(radsNegative)), int(radius * sin(radsNegative)));
                auto posPosition = concave.center - rp;
                auto negPosition = concave.center - rn;

                auto validPosition = [&](Position &p, Position &last) {
                    if (!p.isValid()
                        || (!cluster.mobileCluster && !Terrain::inTerritory(PlayerState::Self, p))
                        || Grids::getMobility(p) <= 4
                        || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
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
                    assignPosition(cluster, concave, posPosition, assignmentsRemaining);
                    continue;
                }

                // Positive position
                if (!stopPositive && validPosition(posPosition, lastPosPosition)) {
                    radsPositive += radsPerUnit;
                    lastPosPosition = posPosition;
                    assignPosition(cluster, concave, posPosition, assignmentsRemaining);
                }
                else
                    radsPositive += 3.14 / 180.0;

                // Negative position
                if (!stopNegative && validPosition(negPosition, lastNegPosition)) {
                    radsNegative -= radsPerUnit;
                    lastNegPosition = negPosition;
                    assignPosition(cluster, concave, negPosition, assignmentsRemaining);
                }
                else
                    radsNegative -= 3.14 / 180.0;

                if (radsPositive > angle + 1.57)
                    stopPositive = true;
                if (radsNegative < angle - 1.57)
                    stopNegative = true;

                if (stopPositive && stopNegative) {
                    stopPositive = false;
                    stopNegative = false;
                    radius += unitTangentSize;
                    radsPositive = angle;
                    radsNegative = angle;
                    wrap++;
                    if (wrap > 5)
                        break;
                }
            }
            if (cluster.mobileCluster)
                reverse(concave.positions.begin(), concave.positions.end());
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
            auto commander = cluster.commander.lock();
            if (!commander)
                continue;

            if (cluster.shape == Shape::Concave)
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