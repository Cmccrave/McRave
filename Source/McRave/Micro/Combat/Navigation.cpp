#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Navigation {

    namespace {
        map<UnitInfo*, vector<Position>> lastSimPositions;
        map<UnitInfo*, map<weak_ptr<UnitInfo>, int>> lastSimUnits;

        BWEB::Path flyerRegroupPath;
        BWEB::Path flyerHarassPath;
        BWEB::Path flyerRetreatPath;
    }

    void getGroundPath(UnitInfo& unit)
    {
        auto pathPoint = unit.getFormation().isValid() ? Util::getPathPoint(unit, unit.getFormation()) : Util::getPathPoint(unit, unit.getDestination());
        auto newPathNeeded = !mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)));

        if (unit.hasCommander()) {
            unit.setDestinationPath(unit.getCommander().lock()->getDestinationPath());
        }

        if (newPathNeeded) {
            BWEB::Path newPath(pathPoint, unit.getPosition(), unit.getType());
            newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
            unit.setDestinationPath(newPath);
        }
    }

    void getFlyingPath(UnitInfo& unit)
    {
        BWEB::Path newPath(unit.getDestination(), unit.getPosition(), unit.getType());
        newPath.generateJPS([&](const TilePosition &t) { return true; });
        unit.setDestinationPath(newPath);
    }

    void updatePath(UnitInfo& unit)
    {
        // Check if we need a new path
        if (!unit.getDestination().isValid() || (!unit.getDestinationPath().getTiles().empty() && unit.getDestinationPath().getTarget() == TilePosition(unit.getDestination())))
            return;

        if (unit.isLightAir() /*&& !unit.getGoal().isValid()*/) {
            auto regrouping = unit.attemptingRegroup();
            auto retreating = unit.getLocalState() == LocalState::Retreat;
            auto harassing = unit.getDestination() == Combat::getHarassPosition() && unit.attemptingHarass() && unit.getLocalState() == LocalState::None;

            if (regrouping)
                unit.setDestinationPath(flyerRegroupPath);
            else if (retreating)
                unit.setDestinationPath(flyerRetreatPath);
            else if (harassing)
                unit.setDestinationPath(flyerHarassPath);
            Visuals::drawPath(unit.getDestinationPath());
            return;
        }

        // Generate a generic flying JPS path
        if (unit.isFlying())
            getFlyingPath(unit);

        // Generate a generic ground JPS path
        else
            getGroundPath(unit);
    }

    void updateNavigation(UnitInfo& unit)
    {
        unit.setNavigation(unit.getFormation().isValid() ? unit.getFormation() : unit.getDestination());

        if (unit.getFormation().isValid() && unit.getLocalState() != LocalState::Attack) {
            unit.setNavigation(unit.getFormation());
            return;
        }

        // If path is reachable, find a point n pixels away to set as new destination
        auto dist = 160.0;
        if (unit.getDestinationPath().isReachable() && unit.getPosition().getDistance(unit.getDestination()) > dist) {
            auto closestPoint = unit.getDestination();
            auto closestDist = DBL_MAX;
            auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                const auto curDist = p.getDistance(unit.getPosition());
                if (curDist < closestDist) {
                    closestDist = curDist;
                    closestPoint = p;
                }
                return p.getDistance(unit.getPosition()) < dist;
            });

            unit.setNavigation(closestPoint);
            if (newDestination.isValid())
                unit.setNavigation(newDestination);
        }
    }

    void updateSimPositions(UnitInfo& unit)
    {
        if (!unit.isLightAir())
            return;

        auto &simUnits = lastSimUnits[&unit];
        for (auto itr = simUnits.begin(); itr != simUnits.end(); ) {
            if (itr->first.expired() || Broodwar->getFrameCount() >= itr->second)
                itr = simUnits.erase(itr);
            else
                ++itr;
        }

        // Add any new sim units
        if (unit.hasSimTarget()) {
            auto newSimUnit = simUnits.find(unit.getSimTarget()) == simUnits.end();
            if (newSimUnit)
                simUnits[unit.getSimTarget()] = Broodwar->getFrameCount() + (unit.getSimTarget().lock()->getType().isBuilding() ? 480 : 60);
        }

        // For pathing purposes, we store light air commander sim positions
        auto &simPositions = lastSimPositions[&unit];
        simPositions.clear();
        for (auto &[sim, _] : simUnits)
            simPositions.push_back(sim.lock()->getPosition());
    }

    void updateGlobalRoughPaths()
    {
        // Create one path from commander to retreat
        auto harassingCommander = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) {
            return u->isLightAir() && !u->hasCommander() && u->attemptingHarass();
        });

        if (harassingCommander) {
            auto &unit = *harassingCommander;
            const auto flyerRegroup = [&](const TilePosition &t) {
                return Grids::getAirThreat(t, PlayerState::Enemy) * 2500.0;
            };
            flyerRegroupPath ={ unit.getPosition(), unit.retreatPos, unit.getType() };
            flyerRegroupPath.generateAS_h(flyerRegroup);
            flyerRetreatPath ={ unit.retreatPos, unit.getPosition(), unit.getType() };
            flyerRetreatPath.generateAS_h(flyerRegroup);

            // Harass path
            auto simDistCurrent = unit.hasSimTarget() ? unit.getPosition().getApproxDistance(unit.getSimTarget().lock()->getPosition()) : unit.getPosition().getApproxDistance(unit.getDestination());
            auto simPosition = unit.hasSimTarget() ? unit.getSimTarget().lock()->getPosition() : unit.getDestination();

            // Generate a flying path for harassing that obeys exploration and staying out of range of threats if possible
            auto &simPositions = lastSimPositions[&unit];
            auto cachedDist = min(simDistCurrent, int(unit.getRetreatRadius() + 64.0));
            const auto flyerAttack = [&](const TilePosition &t) {
                const auto center = Position(t) + Position(16, 16);

                auto d = center.getApproxDistance(simPosition);
                for (auto &pos : simPositions)
                    d = min(d, center.getApproxDistance(pos));

                auto dist = max(0.01, double(d) - cachedDist);
                auto vis = clamp(double(Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t)) / 960.0, 0.1, 3.0);
                return 1.0 / (vis * dist);
            };

            flyerHarassPath ={ unit.getDestination(), unit.getPosition(),unit.getType() };
            flyerHarassPath.generateAS_h(flyerAttack);
        }
    }

    void onFrame()
    {
        updateGlobalRoughPaths();

        for (auto &cluster : Clusters::getClusters()) {

            // Update the commander first
            auto commander = cluster.commander.lock();
            if (commander) {
                updateSimPositions(*commander);
                updatePath(*commander);
                updateNavigation(*commander);
            }

            // Update remaining units
            for (auto &unit : cluster.units) {
                if (unit == &*commander)
                    continue;

                // Determine if this is a shared decision                
                auto sharedDecision = cluster.commandShare == CommandShare::Exact && !unit->isNearSuicide()
                    && !unit->attemptingRegroup() && (unit->getType() == commander->getType() || unit->getLocalState() != LocalState::Attack);

                // If it's not a shared decision, indepdently update pathing and navigation waypoint
                if (!sharedDecision) {
                    updateSimPositions(*unit);
                    updatePath(*unit);
                    updateNavigation(*unit);
                }
                else {
                    unit->setNavigation(commander->getNavigation());
                }
            }
        }
    }
}