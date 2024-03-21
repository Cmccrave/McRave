#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> formations;

    void formationStuff(Formation& formation, Cluster& cluster)
    {
        auto commander = cluster.commander.lock();

        // Offset the center by a distance of the radius towards the navigation point if needed
        if (cluster.mobileCluster) {
            auto dist = max(160.0, ((cluster.units.size() * cluster.spacing / 1.3) * 1.2));
            formation.center = cluster.retreatCluster ? Util::shiftTowards(cluster.retreatNavigation, cluster.avgPosition, dist) : Util::shiftTowards(cluster.avgPosition, cluster.marchNavigation, dist);
            formation.radius = cluster.retreatCluster ? (dist + 64.0) : (dist - 64.0);
            formation.angle = cluster.retreatCluster ? BWEB::Map::getAngle(make_pair(cluster.avgPosition, cluster.retreatNavigation)) : BWEB::Map::getAngle(make_pair(cluster.marchNavigation, cluster.avgPosition));
        }

        // If we are setting up a static formation, align formation with buildings close by
        else {
            formation.center = cluster.marchPosition;
            auto closestBuilding = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
                return (u->getType().isBuilding() && u->getFormation().getDistance(cluster.marchPosition) < 64.0);
            });
            auto closestDefender = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
                return (u->getType().isBuilding() && u->canAttackGround() && u->getRole() == Role::Defender && u->getFormation().getDistance(cluster.marchPosition) < 64.0);
            });

            const auto extra = 96.0;

            if (closestBuilding && !closestDefender && !Combat::holdAtChoke())
                formation.radius = closestBuilding->getPosition().getDistance(cluster.marchPosition) + extra;
            if (closestDefender) {
                formation.leash = closestDefender->getGroundRange() + extra;
                formation.radius = closestBuilding->getPosition().getDistance(cluster.marchPosition) + extra;
            }
            formation.angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, cluster.retreatPosition));
        }
    }

    void assignPosition(Cluster& cluster, Formation& concave, Position p)
    {
        if (!cluster.commander.lock()->getFormation().isValid()) {
            cluster.commander.lock()->setFormation(p);
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

        const auto type = cluster.commander.lock()->getType();
        const auto biggestDimension = max(type.width(), type.height()) * 2;
        auto radsPerUnit = (biggestDimension / concave.radius);
        auto first = concave.center - Position(-int(concave.radius * cos(concave.angle)), int(concave.radius * sin(concave.angle)));

        auto pRads = concave.angle;
        auto nRads = concave.angle;
        pair<Position, Position> lastPositions ={ Positions::Invalid, Positions::Invalid };

        auto wrap = 0;
        auto assignmentsRemaining = max(3, int(cluster.units.size()));

        // Checks if a position is okay, stores if it is
        auto checkPosition = [&](Position &p, Position &last, bool& skip, int& bump) {            
            if (skip
                || !p.isValid()
                || (p == lastPositions.first)
                || (p == lastPositions.second)
                //|| Planning::overlapsPlan(p) impl?
                || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
                || (p.getDistance(first) > concave.leash)
                || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 128.0)) {
                return;
            }
            const auto mobility = Grids::getMobility(p);

            // We bumped into terrain
            if (mobility <= 4) {
                bump++;
                skip = (bump >= 2);
                return;
            }

            //// Slowly check if it's in a different area
            //if (bump && mobility > 4) {
            //    bump = false;
            //    auto dist = BWEB::Map::getGroundDistance(p, last);
            //    if (dist > biggestDimension * 2) {
            //        skip = true;
            //        return;
            //    }
            //}
            //bump = 0;
            assignmentsRemaining--;
            concave.positions.push_back(p);
            last = p;
        };

        auto bumpedPos = 0;
        auto bumpedNeg = 0;
        auto skipPos = false;
        auto skipNeg = false;
        while (assignmentsRemaining > 0) {

            // Check positions
            auto rp = Position(-int(concave.radius * cos(pRads)), int(concave.radius * sin(pRads)));
            auto rn = Position(-int(concave.radius * cos(nRads)), int(concave.radius * sin(nRads)));
            auto posPosition = concave.center - rp;
            auto negPosition = concave.center - rn;

            checkPosition(posPosition, lastPositions.first, skipPos, bumpedPos);
            checkPosition(negPosition, lastPositions.second, skipNeg, bumpedNeg);
            pRads+=radsPerUnit;
            nRads-=radsPerUnit;

            if ((skipPos && skipNeg) || pRads > (concave.angle + size) || nRads < (concave.angle - size)) {
                concave.radius += (cluster.mobileCluster ? -biggestDimension : biggestDimension);
                radsPerUnit = (biggestDimension / concave.radius);
                nRads = pRads = concave.angle;
                wrap++;
                skipPos = false;
                skipNeg = false;
                bumpedPos = 0;
                bumpedNeg = 0;
            }

            if (assignmentsRemaining <= 0 || wrap >= 10 || concave.radius <= 64)
                break;
        }

        reverse(concave.positions.begin(), concave.positions.end());
        for (auto &position : concave.positions)
            assignPosition(cluster, concave, position);
    }

    void createConcave(Formation& formation, Cluster& cluster)
    {
        formationStuff(formation, cluster);
        auto arcSize = 1.3;
        generatePositions(formation, cluster, arcSize);
    }

    void createLine(Formation& formation, Cluster& cluster)
    {
        formationStuff(formation, cluster);
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
            if (!commander || commander->getGoalType() == GoalType::Block)
                continue;

            for_each(cluster.units.begin(), cluster.units.end(), [&](auto &u) {
                u->setFormation(Positions::Invalid);
            });

            // Create a concave
            Formation formation;
            formation.leash = 640.0;
            formation.radius = (cluster.units.size() * cluster.spacing / 1.3);
            formation.cluster = &cluster;

            if (cluster.shape == Shape::Concave)
                createConcave(formation, cluster);
            else if (cluster.shape == Shape::Line)
                createLine(formation, cluster);
            else if (cluster.shape == Shape::Box)
                createBox(formation, cluster);

            formations.push_back(formation);
        }
    }

    void drawFormations()
    {
        for (auto &formation : formations) {
            for (auto &p : formation.positions) {
                Visuals::drawCircle(p, 4, Colors::Blue);
                Visuals::drawLine(p, formation.center, Colors::Blue);
            }
            Visuals::drawLine(formation.center, formation.cluster->marchNavigation, Colors::Green);
            Visuals::drawLine(formation.center, formation.cluster->retreatNavigation, Colors::Red);

            Visuals::drawCircle(formation.center, 3, Colors::Purple, true);
        }
    }

    void onFrame()
    {
        formations.clear();
        createFormations();
        drawFormations();
    }

    vector<Formation>& getFormations() { return formations; }
}