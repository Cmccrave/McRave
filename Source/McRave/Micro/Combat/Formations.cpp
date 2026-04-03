#include "Combat.h"
#include "Map/Grids.h"
#include "Map/Terrain.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Formations {

    vector<Formation> formations;

    void holdFormation(Formation &formation, Cluster &cluster)
    {
        auto commander = cluster.commander.lock();

        formation.start  = commander->getPosition();
        formation.angle  = BWEB::Map::getAngle(cluster.marchPosition, cluster.retreatPosition);
        formation.lState = LocalState::Hold;

        auto closestChoke    = Util::getClosestChokepoint(cluster.marchPosition);
        auto closestBuilding = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
            return u->getType().isBuilding() && u->getFormation().isValid() &&
                   (u->getFormation().getDistance(cluster.marchPosition) < 160.0 ||
                    (Terrain::inArea(u->getPosition(), cluster.avgPosition) && Terrain::inArea(u->getPosition(), cluster.retreatPosition)));
        });
        auto closestDefender = Util::getClosestUnit(cluster.marchPosition, PlayerState::Self, [&](auto &u) {
            return u->getType().isBuilding() && u->getRole() == Role::Defender && u->getFormation().isValid() &&
                   (u->getFormation().getDistance(cluster.marchPosition) < 160.0 ||
                    (Terrain::inArea(u->getPosition(), cluster.avgPosition) && Terrain::inArea(u->getPosition(), cluster.retreatPosition)));
        });

        auto formationWithBuilding  = !Combat::holdAtChoke() && closestBuilding;
        auto formationWithChoke     = Combat::holdAtChoke() && closestChoke && Position(closestChoke->Center()).getDistance(cluster.marchPosition) < 32.0;
        auto formationWithCommander = cluster.shape == Shape::Concave;

        // Hold with defender
        if (!Combat::holdAtChoke() && closestDefender && Players::ZvZ()) {
            formation.center = cluster.marchPosition;
            formation.start  = closestDefender->getPosition();
            formation.radius = 96.0;
            formation.angle  = (round(formation.angle / M_PI_D4)) * M_PI_D4;
        }

        // Hold with building
        else if (!Combat::holdAtChoke() && closestBuilding) {
            closestBuilding->circle(Colors::Yellow);
            formation.center = cluster.marchPosition;
            formation.start  = closestBuilding->getPosition();
            formation.radius = closestBuilding->getPosition().getDistance(cluster.marchPosition);
            formation.angle  = (round(formation.angle / M_PI_D4)) * M_PI_D4;
        }

        // Hold at retreat
        else if (!Combat::holdAtChoke()) {
            formation.center = cluster.marchPosition;
            formation.start  = cluster.retreatPosition;
            formation.radius = 96.0;
            formation.angle  = (round(formation.angle / M_PI_D4)) * M_PI_D4;
        }

        // Hold with choke, use ramp angles if provided
        else if (Combat::holdAtChoke() && closestChoke) {

            // If any unit is in the wrong side of the chokepoint, shift the center back. Assumes only main area is correct for now
            auto badArea = false;
            for (auto &unit : cluster.units) {
                if (!Terrain::inArea(Terrain::getMainArea(), unit->getPosition()) && !Combat::isDefendNatural())
                    badArea = true;
            }

            auto maxRange = max(commander->getGroundRange(), double(Players::getStrength(PlayerState::Enemy).maxGroundRange));
            auto maxSize  = max(double(closestChoke->Width()), double(cluster.units.size() * commander->getType().width() * 0.5));

            if (closestChoke == Terrain::getMainChoke()) {
                formation.angle  = BWEB::Map::getAngle(Terrain::getMainRamp().center, Terrain::getMainRamp().entrance);
                formation.radius = max(maxRange, maxSize);
                formation.center = Terrain::getMainRamp().center;
                formation.start  = Terrain::getMainRamp().entrance;
                Visuals::drawCircle(Terrain::getMainRamp().entrance, 6, Colors::Purple, true);
            }
            else {
                formation.angle  = Terrain::getChokepointAngle(closestChoke);
                formation.radius = max(commander->getGroundRange(), double(closestChoke->Width()));
                formation.center = Position(closestChoke->Center());
                formation.start  = Position(closestChoke->Center()); // ?? 
            }

            if (badArea) {
                formation.radius += 64.0;
                formation.start = Util::shiftTowards(formation.center, formation.start, 64.0);
            }
        }

        // Hold with commander
        else if (formationWithCommander) {
            formation.start = commander->getPosition();
            formation.angle = BWEB::Map::getAngle(cluster.marchNavigation, cluster.retreatNavigation);
        }
    }

    void marchFormation(Formation &formation, Cluster &cluster)
    {
        auto shift = max(160.0, formation.radius) + 32.0;
        shift += max(0, 5 - Grids::getMobility(cluster.marchNavigation)) * 16.0;

        formation.radius = clamp((cluster.units.size() * cluster.spacing / 1.3), 16.0, 640.0);
        formation.start  = Util::shiftTowards(cluster.avgPosition, cluster.marchNavigation, shift);
        formation.center = cluster.marchNavigation;
        formation.angle  = BWEB::Map::getAngle(cluster.marchNavigation, cluster.avgPosition);
        formation.lState = LocalState::Attack;
    }

    void retreatFormation(Formation &formation, Cluster &cluster)
    {
        auto shift = max(160.0, formation.radius) + 32.0;

        formation.radius = clamp((cluster.units.size() * cluster.spacing / 1.3), 16.0, 640.0);
        formation.start  = Util::shiftTowards(cluster.avgPosition, cluster.retreatNavigation, shift);
        formation.center = cluster.retreatNavigation;
        formation.angle  = BWEB::Map::getAngle(cluster.avgPosition, cluster.retreatNavigation);
        formation.lState = LocalState::Retreat;
    }

    void formationSetup(Formation &formation, Cluster &cluster)
    {
        if (cluster.state == LocalState::Hold)
            holdFormation(formation, cluster);
        else if (cluster.state == LocalState::Retreat)
            retreatFormation(formation, cluster);
        else
            marchFormation(formation, cluster);

        // Lines are perpendicular arrangements
        if (cluster.shape == Shape::Line)
            formation.angle += 1.57;
    }

    void assignPosition(Cluster &cluster, Formation &concave, Position p)
    {
        // Find the closest unit to this position without formation
        UnitInfo *closestUnit = nullptr;
        auto distBest         = DBL_MAX;
        for (auto &unit : cluster.units) {
            auto dist = min(unit->getPosition().getDistance(p), unit->getCommandPosition().getDistance(p));
            if (dist < distBest && !unit->getFormation().isValid()) {
                distBest    = dist;
                closestUnit = &*unit;
            }
        }
        if (closestUnit) {
            // Util::findWalkable(*closestUnit, p);
            closestUnit->setFormation(p);
            // Visuals::drawLine(closestUnit->getPosition(), p, Colors::Green);
        }
    }

    void generatePositions(Formation &formation, Cluster &cluster, function<void(Position &, Position &)> stepFunction, function<bool(bool)> wrapFunction)
    {
        auto commander                         = cluster.commander.lock();
        auto assignmentsRemaining              = max(3, int(cluster.units.size()));
        pair<Position, Position> lastPositions = {Positions::Invalid, Positions::Invalid};

        // Prevent blocking our own buildings
        auto closestBuilder = Util::getClosestUnit(formation.center, PlayerState::Self, [&](auto &u) { return u->getBuildPosition().isValid(); });
        bool first = true;

        // Checks if a position is okay, stores if it is
        auto checkPosition = [&](Position &p, Position &last, bool &skip) {
            // Can't be the same as the last positions
            if (!p.isValid() || p == lastPositions.first || p == lastPositions.second)
                return;
            last = p;

            // Must be walkable for this type (TODO: Check size being assigned)
            if (p.isValid() && !Util::findTerrainWalkable(p, commander->getType())) {
                skip = true;
                return;
            }

            //
            if (skip || assignmentsRemaining <= 0 || BWEB::Map::isUsed(TilePosition(p)) != UnitTypes::None || (closestBuilder && p.getDistance(closestBuilder->getPosition()) < 128.0)) {
                return;
            }

            auto box   = Util::typeBoundingBox(p, commander->getType());
            auto color = first ? Colors::Blue : Colors::Green;
            first      = false;
            // Visuals::drawBox(box.first, box.second, color);
            assignmentsRemaining--;
            formation.positions.push_back(p);
        };

        auto skipPos = false;
        auto skipNeg = false;
        auto wrap    = 0;

        auto posPosition = formation.center;
        auto negPosition = formation.center;

        while (assignmentsRemaining > 0) {

            stepFunction(posPosition, negPosition);
            checkPosition(posPosition, lastPositions.first, skipPos);
            checkPosition(negPosition, lastPositions.second, skipNeg);

            if (wrapFunction(skipPos && skipNeg)) {
                skipPos = false;
                skipNeg = false;
                wrap++;
            }

            if (assignmentsRemaining <= 0 || wrap >= 5)
                break;
        }

        if (cluster.state == LocalState::Attack)
            reverse(formation.positions.begin(), formation.positions.end());
        for (auto &position : formation.positions)
            assignPosition(cluster, formation, position);
    }

    void generateLinePositions(Formation &formation, Cluster &cluster)
    {
        auto commander       = cluster.commander.lock();
        auto pixelsPerUnit   = max(commander->getType().width(), commander->getType().height()) + 2 + (formation.lState != LocalState::None && formation.lState != LocalState::Hold) * 8;
        auto startCenterDist = formation.center.getDistance(formation.start);
        formation.radius     = max(formation.radius, 160.0);

        // Steps
        double xStepPer = cos(formation.angle) * double(pixelsPerUnit);
        double yStepPer = sin(formation.angle) * double(pixelsPerUnit);
        if (abs(cos(formation.angle)) > abs(sin(formation.angle)))
            xStepPer = (xStepPer >= 0 ? 1.0 : -1.0) * max(double(commander->getType().width()), abs(xStepPer));
        else
            yStepPer = (yStepPer >= 0 ? 1.0 : -1.0) * max(double(commander->getType().height()), abs(yStepPer));

        // Step function
        auto count          = 0;
        const auto stepFunc = [&](auto &np, auto &pp) {
            pp = formation.start + Position(int(-xStepPer * count), int(yStepPer * count));
            np = formation.start - Position(int(-xStepPer * count), int(yStepPer * count));
            count++;
        };

        // Wrap function
        const auto wrapFunc = [&](auto skipAll) {
            if (skipAll || count * pixelsPerUnit > formation.radius) {
                count           = 0;
                formation.start = Util::shiftTowards(formation.center, formation.start, startCenterDist + pixelsPerUnit);
                return true;
            }
            return false;
        };

        // Generate positions
        generatePositions(formation, cluster, stepFunc, wrapFunc);
    }

    void generateConcavePositions(Formation &formation, Cluster &cluster, double size)
    {
        auto commander     = cluster.commander.lock();
        auto pixelsPerUnit = max(commander->getType().width(), commander->getType().height()) + 2;
        auto radsPerUnit   = (double(max(commander->getType().width(), commander->getType().height()) + 2) / formation.radius);
        // formation.center = Util::shiftTowards(cluster.avgPosition, formation.start, formation.radius);
        // formation.radius = max(formation.radius, 160.0);

        auto pRads = formation.angle;
        auto nRads = formation.angle;

        // Step function
        auto count          = 0;
        const auto stepFunc = [&](auto &np, auto &pp) {
            pp = formation.center - Position(-int(formation.radius * cos(pRads)), int(formation.radius * sin(pRads)));
            np = formation.center - Position(-int(formation.radius * cos(nRads)), int(formation.radius * sin(nRads)));
            pRads += radsPerUnit;
            nRads -= radsPerUnit;
        };

        // Wrap function
        const auto wrapFunc = [&](auto skipAll) {
            if (skipAll || pRads > (formation.angle + size) || nRads < (formation.angle - size)) {
                formation.radius += pixelsPerUnit;
                radsPerUnit = (pixelsPerUnit / formation.radius);
                nRads = pRads = formation.angle;
                return true;
            }
            return false;
        };

        // Generate positions
        generatePositions(formation, cluster, stepFunc, wrapFunc);
    }

    void createConcave(Formation &formation, Cluster &cluster)
    {
        formationSetup(formation, cluster);
        generateConcavePositions(formation, cluster, 1.3);
    }

    void createLine(Formation &formation, Cluster &cluster)
    {
        formationSetup(formation, cluster);
        generateLinePositions(formation, cluster);
    }

    void createBox(Formation &concave, Cluster &cluster) {}

    void createFormations()
    {
        // Create formations of each cluster
        // Each cluster has a UnitType and count, make a formation that considers each
        // TODO: For now we only make a concave of 1 size and ignore types, eventually want to size the concaves based on unittype counts
        for (auto &cluster : Clusters::getClusters()) {
            auto commander = cluster.commander.lock();
            if (!commander || commander->getGoalType() == GoalType::Block)
                continue;

            for_each(cluster.units.begin(), cluster.units.end(), [&](auto &u) { u->setFormation(Positions::Invalid); });

            // Create a concave
            Formation formation;
            formation.leash  = 640.0;
            formation.radius = clamp((cluster.units.size() * cluster.spacing / 1.3), 16.0, 640.0);
            // formation.spacing = max(commander->getType().width(), commander->getType().height()) + 6; this breaks things for some reason???
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

            // Draw march navigation
            Visuals::drawLine(formation.center, formation.cluster->marchNavigation, Colors::Green);
            Visuals::drawCircle(formation.cluster->marchNavigation, 4, Colors::Green);
            Broodwar->drawTextMap(formation.cluster->marchNavigation, "%cNav", Text::Green);

            // Draw march position
            Visuals::drawLine(formation.cluster->marchPosition, formation.cluster->marchNavigation, Colors::Green);
            Visuals::drawCircle(formation.cluster->marchPosition, 4, Colors::Green, true);

            // Draw retreat navigation
            Visuals::drawLine(formation.center, formation.cluster->retreatNavigation, Colors::Red);
            Visuals::drawCircle(formation.cluster->retreatNavigation, 4, Colors::Red);
            Broodwar->drawTextMap(formation.cluster->retreatNavigation, "%cNav", Text::Red);

            // Draw retreat position
            Visuals::drawLine(formation.cluster->retreatPosition, formation.cluster->retreatNavigation, Colors::Red);
            Visuals::drawCircle(formation.cluster->retreatPosition, 4, Colors::Red, true);

            Visuals::drawCircle(formation.center, 3, Colors::White, true);
            Visuals::drawCircle(formation.cluster->avgPosition, 5, Colors::Black, false);
        }
    }

    void onFrame()
    {
        formations.clear();
        createFormations();
    }

    vector<Formation> &getFormations() { return formations; }
} // namespace McRave::Combat::Formations