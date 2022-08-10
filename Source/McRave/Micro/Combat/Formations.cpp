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
            if (dist < distBest && !unit->getFormation().isValid() && unit->getLocalState() != LocalState::Attack) {
                distBest = dist;
                closestUnit = &*unit;
            }
        }
        if (closestUnit) {
            closestUnit->setFormation(p);
            if (Terrain::inTerritory(PlayerState::Self, closestUnit->getPosition()))
                Zones::addZone(closestUnit->getFormation(), ZoneType::Engage, 160, 320);
        }
        assignmentsRemaining--;
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
        auto startPosition = Util::getClosestPointToRadiusGround(retreat->getBase()->Center(), center, radius).second;
        Visuals::drawLine(center, startPosition, cluster.color);
        auto angle = BWEB::Map::getAngle(make_pair(concave.center, startPosition));
        auto radsPositive = angle;
        auto radsNegative = angle;
        auto lastPosPosition = Positions::Invalid;
        auto lastNegPosition = Positions::Invalid;

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
                    || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 128.0))
                    return false;
                concave.positions.push_back(p);
                Broodwar->drawCircleMap(p, 2, Colors::Blue);
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
            if (radsPositive > angle + 1.0472)
                stopPositive = true;

            // Negative position
            auto rn = Position(-int(radius * cos(radsNegative)), int(radius * sin(radsNegative)));
            auto negPosition = concave.center - rn;
            if (!stopNegative && validPosition(negPosition, lastNegPosition)) {
                radsNegative -= radsPerUnit;
                lastNegPosition = negPosition;
            }
            else
                radsNegative -= 3.14 / 180.0;
            if (radsNegative < angle - 1.0472)
                stopNegative = true;

            // Wrap if needed
            if (stopPositive && stopNegative) {
                stopPositive = false;
                stopNegative = false;
                radius += unitTangentSize;
                radsPositive = angle;
                radsNegative = angle;
                wrap++;
                radsPerUnit = (arcRads / radius);

                if (!commander->getFormation().isValid() && !concave.positions.empty()) {
                    commander->setFormation(*concave.positions.begin());
                    concave.positions.erase(concave.positions.begin());
                }
                if (cluster.mobileCluster)
                    reverse(concave.positions.begin(), concave.positions.end());
                for (auto &position : concave.positions)
                    assignPosition(cluster, concave, position, assignmentsRemaining);

                if (assignmentsRemaining == 0 || wrap > 3)
                    break;
            }
        }
    }

    void staticConcave(Formation &concave, Cluster& cluster)
    {
        for (auto &[type, count] : cluster.typeCounts) {
            auto commander = cluster.commander.lock();

            const auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0));
            const auto center = cluster.sharedDestination;
            auto radius = (count * unitTangentSize / arcRads);
            const auto radsPerUnit = arcRads / radius;
            concave.center = center;

            // If we are setting up a static formation, align concave with buildings close by
            auto closestBuilding = Util::getClosestUnit(cluster.sharedDestination, PlayerState::Self, [&](auto &u) {
                return (u->getType().isBuilding() && u->getFormation().getDistance(cluster.sharedDestination) < 64.0);
            });
            if (closestBuilding)
                radius = closestBuilding->getPosition().getDistance(cluster.sharedDestination);
            generateConcavePositions(concave, cluster, type, center, radius, radsPerUnit, unitTangentSize);
        }
    }

    void forwardConcave(Formation &concave, Cluster& cluster)
    {
        return;
        auto commander = cluster.commander.lock();

        // TODO: Use all types
        // For now, get biggest type within formation distance of the commander
        auto fattestDimension = 0;
        auto type = UnitTypes::None;
        for (auto &unit : cluster.units) {
            auto maxDimType = max(unit->getType().width(), unit->getType().height());
            if (maxDimType > fattestDimension && unit->getPosition().getDistance(commander->getPosition()) < cluster.sharedRadius) {
                fattestDimension = maxDimType;
                type = unit->getType();
            }
        }

        auto count = int(cluster.units.size());

        // Create radius based on how many units we have and want to fit into the first row of an arc
        const auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0));
        auto radius = (count * unitTangentSize / arcRads);
        const auto radsPerUnit = arcRads / radius;

        // Offset the center by a distance of the radius towards the navigation point
        const auto dist = cluster.sharedNavigation.getDistance(commander->getPosition());
        const auto dirx = double(cluster.sharedNavigation.x - commander->getPosition().x) / dist;
        const auto diry = double(cluster.sharedNavigation.y - commander->getPosition().y) / dist;
        const auto center = cluster.sharedNavigation + Position(int(dirx*radius), int(diry*radius));
        concave.center = center;
        Visuals::drawLine(commander->getPosition(), center, cluster.color);
        generateConcavePositions(concave, cluster, type, center, radius, radsPerUnit, unitTangentSize);
    }

    void backwardConcave(Cluster& cluster)
    {

    }

    void createConcave(Cluster& cluster)
    {
        auto commander = cluster.commander.lock();

        // Create a concave
        Formation concave;

        if (cluster.mobileCluster)
            forwardConcave(concave, cluster);
        else
            staticConcave(concave, cluster);

        concave.cluster = &cluster;
        concave.center = cluster.sharedNavigation;
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