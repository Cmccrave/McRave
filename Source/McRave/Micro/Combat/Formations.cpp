#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> formations;
    double arcRads = 1.57; // wtf is this

    void formationStuff(Formation& formation, Cluster& cluster)
    {
        auto commander = cluster.commander.lock();
        formation.center = cluster.marchPosition;
        const auto retreatCluster = commander->getLocalState() == LocalState::Retreat || commander->getLocalState() == LocalState::ForcedRetreat;
        const auto dir = retreatCluster ? cluster.retreatNavigation : cluster.marchNavigation;

        // Radius has to be at least this units range
        formation.radius = max(formation.radius, commander->getGroundRange());

        // Offset the center by a distance of the radius towards the navigation point
        if (cluster.mobileCluster) {
            const auto dist = dir.getDistance(cluster.avgPosition);
            const auto dirx = double(dir.x - cluster.avgPosition.x) / dist;
            const auto diry = double(dir.y - cluster.avgPosition.y) / dist;
            const auto offset = retreatCluster ? -formation.radius : formation.radius; // Any more or less and it kinda causes problems
            formation.center = Util::clipPosition(dir + Position(int(dirx*offset), int(diry*offset)));
            formation.angle = BWEB::Map::getAngle(make_pair(formation.center, dir));
        }

        // If we are setting up a static formation, align formation with buildings close by
        else {
            auto closestBuilding = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
                return (u->getType().isBuilding() && u->getFormation().getDistance(cluster.marchPosition) < 64.0);
            });
            auto closestDefender = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
                return (u->getType().isBuilding() && u->canAttackGround() && u->getRole() == Role::Defender && u->getFormation().getDistance(cluster.marchPosition) < 64.0);
            });

            const auto extra = !Players::ZvZ() ? 96.0 : 0.0;

            if (closestBuilding && !closestDefender)
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
        auto radsPerUnit = (arcRads * biggestDimension / concave.radius);
        auto first = concave.center - Position(-int(concave.radius * cos(concave.angle)), int(concave.radius * sin(concave.angle)));

        auto pRads = concave.angle;
        auto nRads = concave.angle;
        pair<Position, Position> lastPositions ={ Positions::Invalid, Positions::Invalid };

        auto wrap = 0;
        auto assignmentsRemaining = int(cluster.units.size()) + 2;

        // Checks if a position is okay, stores if it is
        auto checkPosition = [&](Position &p, Position &last) {
            if (!p.isValid()
                || Grids::getMobility(p) <= 4
                || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
                || Util::boxDistance(type, p, type, last) <= 2
                || (cluster.shape == Shape::Choke && !Terrain::inTerritory(PlayerState::Self, p))
                || (p.getDistance(first) > concave.leash)
                || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 96.0)) {
                return;
            }
            assignmentsRemaining--;
            concave.positions.push_back(p);
            last = p;
        };

        while (assignmentsRemaining > 0) {

            // Check positions
            auto rp = Position(-int(concave.radius * cos(pRads)), int(concave.radius * sin(pRads)));
            auto rn = Position(-int(concave.radius * cos(nRads)), int(concave.radius * sin(nRads)));
            auto posPosition = concave.center - rp;
            auto negPosition = concave.center - rn;
            checkPosition(posPosition, lastPositions.first);
            checkPosition(negPosition, lastPositions.second);
            pRads+=radsPerUnit;
            nRads-=radsPerUnit;

            if (pRads > (concave.angle + size) || nRads < (concave.angle - size)) {
                concave.radius += biggestDimension;
                radsPerUnit = (arcRads * biggestDimension / concave.radius);
                nRads = pRads = concave.angle;
                wrap++;
            }

            if (assignmentsRemaining <= 0 || wrap >= 10)
                break;
        }

        reverse(concave.positions.begin(), concave.positions.end());
        for (auto &position : concave.positions)
            assignPosition(cluster, concave, position);
    }

    void createChoke(Formation& formation, Cluster& cluster)
    {
        formationStuff(formation, cluster);
        const auto choke = Util::getClosestChokepoint(cluster.marchPosition);
        const auto perpLine = BWEB::Map::perpendicularLine(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))), 96.0);
        const auto anglePos = (perpLine.first.getDistance(cluster.retreatNavigation) < perpLine.second.getDistance(cluster.retreatNavigation)) ? perpLine.first : perpLine.second;
        formation.radius = max({ formation.radius, double(choke->Width()), cluster.commander.lock()->getGroundRange() });
        formation.angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, anglePos));

        auto arcSize = 1.3;
        generatePositions(formation, cluster, arcSize);
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
            if (!commander || cluster.units.size() <= 2)
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

    void drawFormations()
    {
        for (auto &formation : formations) {
            for (auto &p : formation.positions) {
                Visuals::drawCircle(p, 4, Colors::Blue);
                Visuals::drawLine(p, formation.center, Colors::Blue);
            }
        }
    }

    void onFrame()
    {
        formations.clear();
        createFormations();
    }

    vector<Formation>& getFormations() { return formations; }
}