#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

// Check for formation is loose (static and there are units closer to the center than the radius by 16 pixels (type width/height)
// Formation loose = assign in order based on distance to center first
// Formation tight = assign in order based on distance to formation spot

namespace McRave::Combat::Formations {

    vector<Formation> formations;

    void formationWithChoke(Formation& formation, Cluster& cluster)
    {
        auto choke = Util::getClosestChokepoint(cluster.marchPosition);

        // Concave formations
        if (cluster.shape == Shape::Concave) {
            if (choke == Terrain::getMainChoke()) {
                formation.angle = Terrain::getMainRamp().angle;
                formation.radius = Terrain::getMainRamp().center.getDistance(Terrain::getMainRamp().entrance) + 48.0;
                formation.center = Terrain::getMainRamp().center;
            }
            else {
                formation.angle = Terrain::getChokepointAngle(choke);
                formation.radius = clamp(cluster.spacing * double(cluster.units.size()) / 2.0, 64.0, double(choke->Width()));
                formation.center = Terrain::getChokepointCenter(choke);
            }
        }
        
        // Line formations
        if (cluster.shape == Shape::Line) {
            if (choke == Terrain::getMainChoke()) {
                formation.angle = Terrain::getMainRamp().angle;
                formation.radius = Terrain::getMainRamp().center.getDistance(Terrain::getMainRamp().entrance) + 48.0;
                formation.center = Terrain::getMainRamp().entrance;
            }
            else {
                formation.angle = Terrain::getChokepointAngle(choke);
                formation.radius = clamp(cluster.spacing * double(cluster.units.size()) / 2.0, 64.0, double(choke->Width()));
                formation.center = Util::shiftTowards(Terrain::getChokepointCenter(choke), cluster.retreatPosition, formation.radius);
            }
        }
    }

    void formationWithBuilding(Formation& formation, Cluster& cluster)
    {
        formation.radius = clamp((cluster.units.size() * cluster.spacing / 1.3), 16.0, 640.0);
        formation.center = cluster.marchPosition;
        formation.angle = BWEB::Map::getAngle(make_pair(cluster.marchPosition, cluster.retreatPosition));

        auto enemyRange = 32.0; // TODO: Check what units they have, make radius at least this max range
        if (Players::getTotalCount(PlayerState::Enemy, Zerg_Hydralisk) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0)
            enemyRange = 96.0;

        auto closestBuilding = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
            return (u->getType().isBuilding() && u->getFormation().getDistance(cluster.marchPosition) < 64.0 && u->getType() != Zerg_Creep_Colony);
        });
        auto closestDefender = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
            return (u->getType().isBuilding() && u->canAttackGround() && u->getRole() == Role::Defender && u->getFormation().getDistance(cluster.marchPosition) < 64.0);
        });

        if (closestBuilding && !closestDefender && !Combat::holdAtChoke())
            formation.radius = closestBuilding->getPosition().getDistance(cluster.marchPosition) + enemyRange;
        if (closestDefender) {
            formation.leash = closestDefender->getGroundRange();
            formation.radius = closestDefender->getPosition().getDistance(cluster.marchPosition) + enemyRange;
        }
    }

    void formationWithNothing(Formation& formation, Cluster& cluster)
    {
        // Offset the center by a distance of the radius towards the navigation point if needed
        formation.radius = clamp((cluster.units.size() * cluster.spacing / 1.3), 16.0, 640.0);
        auto shift = max(160.0, formation.radius);
        if (cluster.state == LocalState::Retreat) {
            formation.radius += 64.0;
            formation.center = Util::shiftTowards(cluster.retreatNavigation, cluster.avgPosition, shift);
            formation.angle = BWEB::Map::getAngle(cluster.avgPosition, cluster.retreatNavigation);
        }
        else {
            formation.radius += -64.0;
            formation.center = Util::shiftTowards(cluster.avgPosition, cluster.marchNavigation, shift);
            formation.angle = BWEB::Map::getAngle(cluster.marchNavigation, cluster.avgPosition);
        }
    }

    void formationSetup(Formation& formation, Cluster& cluster)
    {
        auto staticCluster = (cluster.marchNavigation.getDistance(cluster.marchPosition) <= formation.radius * 2);

        if (cluster.state == LocalState::Hold && Combat::holdAtChoke()) {
            formationWithChoke(formation, cluster);
        }
        else if (cluster.state == LocalState::Hold && staticCluster) {
            formationWithBuilding(formation, cluster);
        }
        else {
            formationWithNothing(formation, cluster);
        }

        // Lines are perpendicular arrangements
        if (cluster.shape == Shape::Line)
            formation.angle += 1.57;
    }

    void assignPosition(Cluster& cluster, Formation& concave, Position p)
    {
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
            //Util::findWalkable(*closestUnit, p);
            closestUnit->setFormation(p);
            Visuals::drawLine(closestUnit->getPosition(), p, Colors::Yellow);
        }
    }

    void generateLinePositions(Formation& line, Cluster& cluster)
    {
        // Prevent blocking our own buildings
        auto closestBuilder = Util::getClosestUnit(line.center, PlayerState::Self, [&](auto &u) {
            return u->getBuildPosition().isValid();
        });

        const auto type = cluster.commander.lock()->getType();
        auto pixelsPerUnit = max(type.width(), type.height()) + (Combat::holdAtChoke() ? 0 : 8);;
        pair<Position, Position> lastPositions ={ Positions::Invalid, Positions::Invalid };

        auto first = line.center;
        auto wrap = 0;
        auto assignmentsRemaining = max(3, int(cluster.units.size()));

        // Checks if a position is okay, stores if it is
        vector<Position> row;
        auto checkPosition = [&](Position &p, Position &last, bool& skip, int& bump) {
            if (skip
                || !p.isValid()
                || (p == lastPositions.first)
                || (p == lastPositions.second)
                || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
                || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 128.0)) {
                return;
            }
            const auto mobility = Grids::getMobility(p);

            // We bumped into terrain
            if (mobility <= 3) {
                bump++;
                skip = (bump >= 2);
                return;
            }
            assignmentsRemaining--;
            row.push_back(p);
            line.positions.push_back(p);
            last = p;
        };

        auto bumpedPos = 0;
        auto bumpedNeg = 0;
        auto skipPos = false;
        auto skipNeg = false;
        auto count = 0;
        auto xStepPer = cos(line.angle) * pixelsPerUnit;
        auto yStepPer = sin(line.angle) * pixelsPerUnit;
        while (assignmentsRemaining > 0) {

            // Check positions
            auto posPosition = line.center + Position(-xStepPer * count, yStepPer * count);
            auto negPosition = line.center - Position(-xStepPer * count, yStepPer * count);

            checkPosition(posPosition, lastPositions.first, skipPos, bumpedPos);
            checkPosition(negPosition, lastPositions.second, skipNeg, bumpedNeg);
            count++;

            if (count * pixelsPerUnit > line.radius) {
                wrap++;
                line.center += Position(cos(-line.angle) * pixelsPerUnit, sin(-line.angle) * pixelsPerUnit);
                line.radius += 2 * pixelsPerUnit;
                count = 0;
                skipPos = false;
                skipNeg = false;
                bumpedPos = 0;
                bumpedNeg = 0;
                //reverse(row.begin(), row.end());
                //line.positions.insert(line.positions.end(), row.begin(), row.end());
                //row.clear();

                //for (auto &position : row)
                //    line.positions.push_back(position);
                //row.clear();
            }

            if (assignmentsRemaining <= 0 || wrap >= 50)
                break;
        }

        //reverse(line.positions.begin(), line.positions.end());
        for (auto &position : line.positions)
            assignPosition(cluster, line, position);
    }

    void generateConcavePositions(Formation& concave, Cluster& cluster, double size)
    {
        // Prevent blocking our own buildings
        auto closestBuilder = Util::getClosestUnit(concave.center, PlayerState::Self, [&](auto &u) {
            return u->getBuildPosition().isValid();
        });

        if (cluster.radius <= 0)
            return;

        const auto type = cluster.commander.lock()->getType();
        const auto biggestDimension = max(type.width(), type.height()) + 2 + (Combat::holdAtChoke() ? 0 : 8);
        const auto radiusIncrement = cluster.state == LocalState::Retreat ? biggestDimension : -biggestDimension;
        auto radsPerUnit = (biggestDimension / concave.radius);
        auto first = concave.center - Position(-int(concave.radius * cos(concave.angle)), int(concave.radius * sin(concave.angle)));
        Visuals::drawCircle(first, 5, Colors::Grey, true);


        // Check if the units are loose
        for (auto &u : concave.cluster->units) {
            if (u->getPosition().getDistance(concave.center) < concave.radius - biggestDimension) {
                concave.loose = true;
                break;
            }
        }

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
                || (!mapBWEM.GetMiniTile(WalkPosition(p)).Walkable())
                || (!BWEB::Map::isWalkable(TilePosition(p), type))
                //|| Planning::overlapsPlan(p) impl?
                || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None
                || (p.getDistance(first) > concave.leash)
                || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 128.0)) {
                return;
            }
            const auto mobility = Grids::getMobility(p);

            // We bumped into terrain
            if (mobility <= 3) {
                bump++;
                skip = (bump >= 2);
                return;
            }
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
                concave.radius += radiusIncrement;
                radsPerUnit = (biggestDimension / concave.radius);
                nRads = pRads = concave.angle;
                wrap++;
                skipPos = false;
                skipNeg = false;
                bumpedPos = 0;
                bumpedNeg = 0;
            }

            if (assignmentsRemaining <= 0 || wrap >= 50 || concave.radius <= 32)
                break;
        }

        //reverse(concave.positions.begin(), concave.positions.end());
        for (auto &position : concave.positions)
            assignPosition(cluster, concave, position);
    }

    void createConcave(Formation& formation, Cluster& cluster)
    {
        formationSetup(formation, cluster);
        generateConcavePositions(formation, cluster, 1.3);
    }

    void createLine(Formation& formation, Cluster& cluster)
    {
        formationSetup(formation, cluster);
        generateLinePositions(formation, cluster);
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
            formation.radius = clamp((cluster.units.size() * cluster.spacing / 1.3), 16.0, 640.0);
            //formation.spacing = max(commander->getType().width(), commander->getType().height()) + 6; this breaks things for some reason???
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

            auto color = Colors::White;
            if (formation.cluster->state == LocalState::Retreat)
                color = Colors::Red;
            else if (formation.cluster->state == LocalState::Attack)
                color = Colors::Green;
            else if (formation.cluster->state == LocalState::Hold)
                color = Colors::Yellow;


            for (auto &p : formation.positions) {
                Visuals::drawCircle(p, 4, color);
                Visuals::drawLine(p, formation.center, color);
            }
            Visuals::drawLine(formation.center, formation.cluster->marchPosition, Colors::Green);
            Visuals::drawLine(formation.center, formation.cluster->retreatPosition, Colors::Red);

            Visuals::drawCircle(formation.center, 3, Colors::White, true);
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