#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> formations;
    double arcRads = 1.57;

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

    void generatePositions(Formation& concave, Cluster& cluster, double size)
    {
        // Prevent blocking our own buildings
        auto closestBuilder = Util::getClosestUnit(cluster.retreatPosition, PlayerState::Self, [&](auto &u) {
            return u->getBuildPosition().isValid();
        });

        auto type = cluster.commander.lock()->getType();
        auto unitTangentSize = sqrt(pow(type.width(), 2.0) + pow(type.height(), 2.0)) + (cluster.mobileCluster ? 16.0 : 0.0);
        auto radsPerUnit = (arcRads / concave.radius) + (cluster.mobileCluster ? 32.0 : 0.0);

        auto first = concave.center - Position(-int(concave.radius * cos(concave.angle)), int(concave.radius * sin(concave.angle)));

        auto radsPositive = concave.angle;
        auto radsNegative = concave.angle;
        auto lastPosPosition = Positions::Invalid;
        auto lastNegPosition = Positions::Invalid;

        bool stopPositive = false;
        bool stopNegative = false;
        auto wrap = 0;
        auto assignmentsRemaining = int(cluster.units.size()) + 2;

        while (assignmentsRemaining > 0) {
            auto validPosition = [&](Position &p, Position &last) {
                if (!p.isValid()
                    || Grids::getMobility(p) <= 4
                    || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
                    || Util::boxDistance(type, p, type, last) <= 2
                    || (cluster.shape == Shape::Choke && !Terrain::inTerritory(PlayerState::Self, p))
                    || (p.getDistance(first) > concave.leash)
                    || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 96.0)) {
                    //Broodwar->drawCircleMap(p, 1, Colors::Red);
                    return false;
                }
                assignmentsRemaining--;
                Broodwar->drawCircleMap(p, 3, Colors::Blue);
                concave.positions.push_back(p);
                return true;
            };

            // Positive position
            auto rp = Position(-int(concave.radius * cos(radsPositive)), int(concave.radius * sin(radsPositive)));
            auto posPosition = concave.center - rp;
            if (!stopPositive && validPosition(posPosition, lastPosPosition)) {
                radsPositive += radsPerUnit;
                lastPosPosition = posPosition;
            }
            else
                radsPositive += 3.14 / 180.0;
            if (radsPositive > concave.angle + size || !posPosition.isValid())
                stopPositive = true;

            if ((cluster.units.size() + 4) == concave.positions.size()) {
                stopPositive = true;
                stopNegative = true;
                wrap = 10;
            }

            // Negative position
            auto rn = Position(-int(concave.radius * cos(radsNegative)), int(concave.radius * sin(radsNegative)));
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
            if (radsNegative < concave.angle - size || !negPosition.isValid())
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
                concave.radius += unitTangentSize;
                radsPositive = concave.angle;
                radsNegative = concave.angle;
                wrap++;
                radsPerUnit = (arcRads / concave.radius);

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
        const auto choke = Util::getClosestChokepoint(cluster.marchPosition);
        const auto chokeAngle = BWEB::Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))));
        auto perpLine = BWEB::Map::perpendicularLine(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))), 96.0);
        concave.radius = max({ concave.radius, double(choke->Width()), cluster.commander.lock()->getGroundRange() });


        if (perpLine.first.getDistance(cluster.retreatNavigation) < perpLine.second.getDistance(cluster.retreatNavigation)) {
            concave.angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, perpLine.first));
        }
        else {
            concave.angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, perpLine.second));
        }

        // HACK: Set the center 1/4 radius away to make a slight oval during formation creation
        concave.center = cluster.marchPosition + Position(-int(0.25 * concave.radius * cos(concave.angle)), int(0.25 * concave.radius * sin(concave.angle)));
        auto arcSize = 1.517;
        generatePositions(concave, cluster, arcSize);
    }

    void createConcave(Formation& concave, Cluster& cluster)
    {
        auto commander = cluster.commander.lock();
        concave.center = cluster.marchPosition;
        const auto retreatCluster = commander->getLocalState() == LocalState::Retreat || commander->getLocalState() == LocalState::ForcedRetreat;
        auto dir = retreatCluster ? cluster.retreatNavigation : cluster.marchNavigation;
        //auto otherDir = retreatCluster ? cluster.marchNavigation : cluster.retreatNavigation;

        // Offset the center by a distance of the radius towards the navigation point
        if (cluster.mobileCluster) {

            // Clamp radius so that it moves forward/backwards as needed
            //const auto radiusOffset = retreatCluster ? -96 : 96;
            //if (retreatCluster) {
            //    concave.radius = max(concave.radius, commander->getPosition().getDistance(cluster.retreatNavigation) + 96.0);
            //}
            //else {
            //    concave.radius = min(concave.radius, commander->getPosition().getDistance(cluster.marchNavigation) - 96.0);
            //}

            // Radius has to be at least this units range
            concave.radius = max(concave.radius, commander->getGroundRange());

            // Offset the center by a distance of the radius towards the navigation point
            const auto dist = dir.getDistance(cluster.avgPosition);
            const auto dirx = double(dir.x - cluster.avgPosition.x) / dist;
            const auto diry = double(dir.y - cluster.avgPosition.y) / dist;
            const auto offset = retreatCluster ? -concave.radius : concave.radius; // Any more or less and it kinda causes problems
            concave.center = Util::clipPosition(dir + Position(int(dirx*offset), int(diry*offset)));
            concave.angle = BWEB::Map::getAngle(make_pair(concave.center, dir));

            // Based on how many units fit in this formation, how low does the altitude need to be
            auto desiredAltitude = int(cluster.units.size());

            // Now take the center and try to shift it perpendicular towards lower altitude
            // TODO Make the clusters do this for their navigation instead
            auto perpAngle = concave.angle;
            auto trials = 0;
            auto size = 16;
            while (mapBWEM.GetMiniTile(WalkPosition(concave.center)).Altitude() < desiredAltitude && trials < 50) {
                trials++;
                auto p1 = Util::clipPosition(concave.center - Position(size * cos(perpAngle), size * sin(perpAngle)));
                auto p2 = Util::clipPosition(concave.center + Position(size * cos(perpAngle), size * sin(perpAngle)));
                auto altitude1 = mapBWEM.GetMiniTile(WalkPosition(p1)).Altitude();
                auto altitude2 = mapBWEM.GetMiniTile(WalkPosition(p2)).Altitude();

                if (altitude1 < mapBWEM.GetMiniTile(WalkPosition(concave.center)).Altitude() && altitude2 < mapBWEM.GetMiniTile(WalkPosition(concave.center)).Altitude())
                    break;

                if (altitude1 > 0 && altitude1 > altitude2) {
                    concave.center = p1;
                    size=16;
                }
                else if (altitude2 > 0) {
                    concave.center = p2;
                    size=16;
                }
                else if (altitude1 == 0 && altitude2 == 0)
                    size+=16;
            }
        }
        else {
            // If we are setting up a static formation, align concave with buildings close by
            auto closestBuilding = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
                return (u->getType().isBuilding() && u->getFormation().getDistance(cluster.marchPosition) < 64.0);
            });
            auto closestDefender = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
                return (u->getType().isBuilding() && u->canAttackGround() && u->getRole() == Role::Defender && u->getFormation().getDistance(cluster.marchPosition) < 64.0);
            });

            const auto extra = !Players::ZvZ() ? 64.0 : 0.0;

            if (closestBuilding && !closestDefender)
                concave.radius = closestBuilding->getPosition().getDistance(cluster.marchPosition) + extra;
            if (closestDefender) {
                concave.leash = closestDefender->getGroundRange() + extra;
                concave.radius = closestBuilding->getPosition().getDistance(cluster.marchPosition) + extra;
            }
            concave.angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, cluster.retreatPosition));
        }

        // Start creating positions starting at the start position
        Broodwar->drawLineMap(concave.center, cluster.retreatNavigation, Colors::Red);
        Broodwar->drawLineMap(concave.center, cluster.marchNavigation, Colors::Green);
        Broodwar->drawLineMap(cluster.marchNavigation, cluster.retreatNavigation, Colors::Cyan);
        auto arcSize = arcRads / 2.0;
        generatePositions(concave, cluster, arcSize);
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
            formation.leash = 640.0;
            formation.radius = (cluster.units.size() * cluster.spacing / arcRads);

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