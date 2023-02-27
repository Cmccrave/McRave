#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> formations;
    double arcRads = 2.0944;

    void assignPosition(Cluster& cluster, Formation& concave, Position p, int& assignmentsRemaining)
    {
        if (!cluster.commander.lock()->getFormation().isValid()) {
            cluster.commander.lock()->setFormation(p);
            assignmentsRemaining--;
            return;
        }

        // Find the closest unit to this position without formation
        UnitInfo * closestUnit = nullptr;
        auto distBest = DBL_MAX;
        for (auto &unit : cluster.units) {
            auto dist = unit->getPosition().getDistance(p);
            if (dist < distBest && !unit->getFormation().isValid()) {
                distBest = dist;
                closestUnit = &*unit;
            }
        }
        if (closestUnit) {
            closestUnit->setFormation(p);
            if (Terrain::inTerritory(PlayerState::Self, closestUnit->getPosition()))
                Zones::addZone(closestUnit->getFormation(), ZoneType::Engage, 160, 320);
        }
    }

    void generatePositions(Formation& concave, Cluster& cluster, double angle, double size)
    {
        // Prevent blocking our own buildings
        auto closestBuilder = Util::getClosestUnit(cluster.retreatPosition, PlayerState::Self, [&](auto &u) {
            return u->getBuildPosition().isValid();
        });

        auto type = cluster.commander.lock()->getType();
        auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0)) + (cluster.mobileCluster ? 16.0 : 0.0);
        auto radsPerUnit = arcRads / cluster.radius;

        auto first = true;
        auto radsPositive = angle;
        auto radsNegative = angle;
        auto lastPosPosition = Positions::Invalid;
        auto lastNegPosition = Positions::Invalid;

        bool stopPositive = false;
        bool stopNegative = false;
        auto wrap = 0;
        auto assignmentsRemaining = int(cluster.units.size()) + 1;

        while (assignmentsRemaining > 0) {
            auto validPosition = [&](Position &p, Position &last) {
                if (!p.isValid()
                    || Grids::getMobility(p) <= 4
                    || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
                    || Util::boxDistance(type, p, type, last) <= 2
                    || (cluster.shape == Shape::Choke && !Terrain::inTerritory(PlayerState::Self, p))
                    || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 96.0)) {
                    Broodwar->drawCircleMap(p, 1, Colors::Red);
                    return false;
                }
                assignmentsRemaining--;
                Broodwar->drawCircleMap(p, 3, Colors::Blue);
                concave.positions.push_back(p);
                return true;
            };

            // Positive position
            auto rp = Position(-int(cluster.radius * cos(radsPositive)), int(cluster.radius * sin(radsPositive)));
            auto posPosition = concave.center - rp;
            if (!stopPositive && validPosition(posPosition, lastPosPosition)) {
                radsPositive += radsPerUnit;
                lastPosPosition = posPosition;
            }
            else
                radsPositive += 3.14 / 180.0;
            if (radsPositive > angle + size || !posPosition.isValid())
                stopPositive = true;

            if ((cluster.units.size() + 4) == concave.positions.size()) {
                stopPositive = true;
                stopNegative = true;
                wrap = 10;
            }

            if (first) {
                first = false;
                Broodwar->drawCircleMap(posPosition, 5, Colors::Yellow, true);
            }

            // Negative position
            auto rn = Position(-int(cluster.radius * cos(radsNegative)), int(cluster.radius * sin(radsNegative)));
            auto negPosition = concave.center - rn;
            if (radsPositive == radsNegative) {
                radsNegative -= radsPerUnit;
                lastNegPosition = negPosition;
            }
            else if (!stopNegative && validPosition(negPosition, lastNegPosition)) {
                radsNegative -= radsPerUnit;
                lastNegPosition = negPosition;
            }
            else
                radsNegative -= 3.14 / 180.0;
            if (radsNegative < angle - size || !negPosition.isValid())
                stopNegative = true;

            if ((cluster.units.size() + 4) == concave.positions.size()) {
                stopPositive = true;
                stopNegative = true;
                wrap = 10;
            }

            // Wrap if needed
            if (stopPositive && stopNegative) {
                stopPositive = false;
                stopNegative = false;
                cluster.radius += unitTangentSize;
                radsPositive = angle;
                radsNegative = angle;
                wrap++;
                radsPerUnit = (arcRads / cluster.radius);

                if (assignmentsRemaining <= 0 || wrap >= 10)
                    break;
            }
        }

        reverse(concave.positions.begin(), concave.positions.end());
        for (auto &position : concave.positions)
            assignPosition(cluster, concave, position, assignmentsRemaining);
    }

    void createChoke(Formation& concave, Cluster& cluster)
    {
        auto angle = 0.0;
        const auto choke = Util::getClosestChokepoint(cluster.marchPosition);
        const auto chokeAngle = BWEB::Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))));
        auto perpLine = BWEB::Map::perpendicularLine(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))), 96.0);
        if (perpLine.first.getDistance(cluster.retreatNavigation) < perpLine.second.getDistance(cluster.retreatNavigation)) {
            angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, perpLine.first));
        }
        else {
            angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, perpLine.second));
        }

        // HACK: Set the center 1/4 radius away to make a slight oval during formation creation
        concave.center = cluster.marchPosition + Position(-int(0.25 * cluster.radius * cos(angle)), int(0.25 * cluster.radius * sin(angle)));
        auto arcSize = 1.517;
        generatePositions(concave, cluster, angle, arcSize);
    }

    void createConcave(Formation& concave, Cluster& cluster)
    {
        auto commander = cluster.commander.lock();

        // Create radius based on how many units we have and want to fit into the first row of an arc
        auto dir = commander->getLocalState() == LocalState::Retreat ? cluster.retreatNavigation : cluster.marchNavigation;
        auto otherDir = commander->getLocalState() == LocalState::Retreat ? cluster.marchNavigation : cluster.retreatNavigation;

        // Offset the center by a distance of the radius towards the navigation point
        concave.center = cluster.marchPosition;
        if (cluster.mobileCluster) {
            const auto dist = dir.getDistance(otherDir);
            const auto dirx = double(dir.x - otherDir.x) / dist;
            const auto diry = double(dir.y - otherDir.y) / dist;
            if (commander->getLocalState() == LocalState::Retreat)
                concave.center = dir - Position(int(dirx*cluster.radius), int(diry*cluster.radius));
            else
                concave.center = dir + Position(int(dirx*cluster.radius), int(diry*cluster.radius));
        }

        // Start creating positions starting at the start position
        auto angle = cluster.mobileCluster ? BWEB::Map::getAngle(make_pair(cluster.marchNavigation, cluster.retreatNavigation)) : BWEB::Map::getAngle(make_pair(cluster.marchPosition, cluster.retreatPosition));
        auto arcSize = arcRads / 2.0;
        generatePositions(concave, cluster, angle, arcSize);
    }

    void createLine(Formation& concave, Cluster& cluster)
    {

    }

    void createBox(Formation& concave, Cluster& cluster)
    {

    }

    void createFormations()
    {
        // Create formations of each cluster
        // Each cluster has a UnitType and count, make a formation that considers each
        // TODO: For now we only make a concave of 1 size and ignore types, eventually want to size the concaves based on unittype counts
        for (auto &cluster : Clusters::getClusters()) {
            auto commander = cluster.commander.lock();
            if (!commander)
                continue;

            for_each(cluster.units.begin(), cluster.units.end(), [&](auto &u) {
                u->setFormation(Positions::Invalid);
            });

            // Create a concave
            Formation formation;

            if (cluster.shape == Shape::Concave)
                createConcave(formation, cluster);
            else if (cluster.shape == Shape::Choke)
                createChoke(formation, cluster);
            else if (cluster.shape == Shape::Line)
                createLine(formation, cluster);
            else if (cluster.shape == Shape::Box)
                createBox(formation, cluster);

            formation.cluster = &cluster;
            formations.push_back(formation);
        }
    }

    void onFrame()
    {
        formations.clear();
        createFormations();
    }

    vector<Formation>& getFormations() { return formations; }
}