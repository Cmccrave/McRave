#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> concaves;
    constexpr double arcRads = 2.0944;

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
            assignmentsRemaining--;
            if (Terrain::inTerritory(PlayerState::Self, closestUnit->getPosition()))
                Zones::addZone(closestUnit->getFormation(), ZoneType::Engage, 160, 320);
        }
    }

    void generateConcavePositions(Formation& concave, Cluster& cluster, UnitType type, Position center, double radius, double radsPerUnit, double unitTangentSize)
    {
        // Get a retreat point
        auto commander = cluster.commander.lock();
        auto retreat = (Stations::getStations(PlayerState::Self).size() <= 2) ? Terrain::getMyMain() : Stations::getClosestRetreatStation(*commander);
        if (!retreat)
            return;

        // Prevent blocking our own buildings
        auto closestBuilder = Util::getClosestUnit(center, PlayerState::Self, [&](auto &u) {
            return u->getBuildPosition().isValid();
        });

        // Start creating positions starting at the start position
        //auto startPosition = Util::getClosestPointToRadiusGround(cluster.sharedNavigation, center, radius).second;
        auto angle = BWEB::Map::getAngle(make_pair(concave.center, cluster.sharedNavigation)) + (commander->getLocalState() == LocalState::Attack ? 3.14 : 0.0);
        auto radsPositive = angle;
        auto radsNegative = angle;
        auto lastPosPosition = Positions::Invalid;
        auto lastNegPosition = Positions::Invalid;

        Visuals::drawLine(center, cluster.sharedNavigation, Colors::Grey);

        //Broodwar->drawTextMap(concave.center, "Start");
        //Visuals::drawPath(cluster.path);

        bool stopPositive = false;
        bool stopNegative = false;
        auto wrap = 0;
        auto assignmentsRemaining = int(cluster.units.size());

        while (assignmentsRemaining > 0) {
            auto validPosition = [&](Position &p, Position &last) {
                if (!p.isValid()
                    || Grids::getMobility(p) <= 4
                    || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
                    || Util::boxDistance(type, p, type, last) <= 2
                    || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 128.0)) {
                    Broodwar->drawCircleMap(p, 1, Colors::Red);
                    return false;
                }
                Broodwar->drawCircleMap(p, 3, Colors::Blue);
                concave.positions.push_back(p);
                return true;
            };

            // Positive position
            auto rp = Position(-int(radius * cos(radsPositive)), int(radius * sin(radsPositive)));
            auto posPosition = concave.center - rp;
            if (!stopPositive && validPosition(posPosition, lastPosPosition)) {
                radsPositive += radsPerUnit;
                lastPosPosition = posPosition;
            }
            else
                radsPositive += 3.14 / 180.0;
            if (radsPositive > angle + 1.0472 || !posPosition.isValid())
                stopPositive = true;

            if (cluster.units.size() == concave.positions.size()) {
                stopPositive = true;
                stopNegative = true;
                wrap = 10;
            }

            // Negative position
            auto rn = Position(-int(radius * cos(radsNegative)), int(radius * sin(radsNegative)));
            auto negPosition = concave.center - rn;
            if (!stopNegative && validPosition(negPosition, lastNegPosition)) {
                radsNegative -= radsPerUnit;
                lastNegPosition = negPosition;
            }
            else
                radsNegative -= 3.14 / 180.0;
            if (radsNegative < angle - 1.0472 || !negPosition.isValid())
                stopNegative = true;

            if (cluster.units.size() == concave.positions.size()) {
                stopPositive = true;
                stopNegative = true;
                wrap = 10;
            }

            // Wrap if needed
            if (stopPositive && stopNegative) {
                stopPositive = false;
                stopNegative = false;
                radius += unitTangentSize;
                radsPositive = angle;
                radsNegative = angle;
                wrap++;
                radsPerUnit = (arcRads / radius);

                if (cluster.mobileCluster)
                    reverse(concave.positions.begin(), concave.positions.end());
                for (auto &position : concave.positions)
                    assignPosition(cluster, concave, position, assignmentsRemaining);

                if (assignmentsRemaining == 0 || wrap > 3)
                    break;
            }
        }
    }

    void createConcave(Cluster& cluster)
    {
        auto commander = cluster.commander.lock();

        // Create a concave
        Formation concave;

        // TODO: Use all types
        // For now, get biggest type within formation distance of the commander
        auto fattestDimension = 0;
        auto type = UnitTypes::None;
        for (auto &unit : cluster.units) {
            auto maxDimType = max(unit->getType().width(), unit->getType().height());
            if (maxDimType > fattestDimension) {
                fattestDimension = maxDimType;
                type = unit->getType();
            }
        }

        auto count = int(cluster.units.size());

        // Create radius based on how many units we have and want to fit into the first row of an arc
        auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0)) + (cluster.mobileCluster ? 16.0 : 0.0);
        auto radsPerUnit = arcRads / cluster.radius;
        auto center = cluster.sharedDestination;

        // Offset the center by a distance of the radius towards the navigation point
        if (cluster.mobileCluster) {
            const auto dist = cluster.sharedNavigation.getDistance(commander->getPosition());
            const auto dirx = double(cluster.sharedNavigation.x - commander->getPosition().x) / dist;
            const auto diry = double(cluster.sharedNavigation.y - commander->getPosition().y) / dist;
            //if (commander->getLocalState() == LocalState::Hold)
            //    center = commander->getPosition() + Position(int(dirx*cluster.radius), int(diry*cluster.radius));
            //else if (commander->getLocalState() == LocalState::Retreat)
            //    center = cluster.sharedNavigation - Position(int(dirx*cluster.radius), int(diry*cluster.radius));
            //else
            //    center = cluster.sharedNavigation + Position(int(dirx*cluster.radius), int(diry*cluster.radius));
            if (commander->getLocalState() == LocalState::Retreat)
                center = cluster.sharedNavigation - Position(int(dirx*cluster.radius), int(diry*cluster.radius));
            else
                center = cluster.sharedNavigation + Position(int(dirx*cluster.radius), int(diry*cluster.radius));
            Broodwar->drawCircleMap(center, 4, cluster.color, true);
        }
        concave.center = center;
        if (!cluster.mobileCluster) {
            generateConcavePositions(concave, cluster, type, center, cluster.radius, radsPerUnit, unitTangentSize);
        }

        concave.cluster = &cluster;
        concaves.push_back(concave);
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
        // TODO: For now we only make a concave of 1 size and ignore types, eventually want to size the concaves based on unittype counts
        for (auto &cluster : Clusters::getClusters()) {
            auto commander = cluster.commander.lock();
            if (!commander)
                continue;

            for_each(cluster.units.begin(), cluster.units.end(), [&](auto &u) {
                u->setFormation(Positions::Invalid);
            });

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