#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Navigation {

    namespace {
        map<UnitInfo*, vector<Position>> lastSimPositions;
        map<UnitInfo*, map<weak_ptr<UnitInfo>, int>> lastSimUnits;

        bool lightUnitNeedsRegroup(UnitInfo& unit)
        {
            if (!unit.isLightAir())
                return false;
            return unit.hasCommander() && unit.getPosition().getDistance(unit.getCommander().lock()->getPosition()) > 64.0;
        }


    }

    void getRegroupPath(UnitInfo& unit)
    {
        const auto flyerRegroup = [&](const TilePosition &t) {
            return Grids::getAirThreat(Position(t) + Position(16, 16), PlayerState::Enemy) * 250.0;
        };

        const auto closestFriend = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
            return u->getType() == unit.getType() && *u != unit && u->getPosition().getDistance(unit.getPosition()) < 64.0
                && u->getDestination() == unit.getDestination() && u->getDestinationPath().getTarget() == TilePosition(unit.getDestination());
        });
        if (closestFriend) {
            unit.setDestinationPath(closestFriend->getDestinationPath());
        }
        else {
            BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
            newPath.generateAS_h(flyerRegroup);
            unit.setDestinationPath(newPath);
        }
    }

    void getGroundPath(UnitInfo& unit)
    {
        auto pathPoint = unit.getFormation().isValid() ? Util::getPathPoint(unit, unit.getFormation()) : Util::getPathPoint(unit, unit.getDestination());
        auto newPathNeeded = !mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)));

        if (newPathNeeded) {
            BWEB::Path newPath(unit.getPosition(), pathPoint, unit.getType());
            newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
            unit.setDestinationPath(newPath);
        }
    }

    void getFlyingPath(UnitInfo& unit)
    {
        BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
        newPath.generateJPS([&](const TilePosition &t) { return true; });
        unit.setDestinationPath(newPath);
    }

    void getHarassPath(UnitInfo& unit)
    {
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
            auto vis = clamp(double(Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t)) / 960.0, 0.5, 3.0);
            return 1.0 / (vis * dist);
        };

        BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
        newPath.generateAS_h(flyerAttack);
        unit.setDestinationPath(newPath);
    }

    void updatePath(UnitInfo& unit)
    {
        // Check if we need a new path
        if (!unit.getDestination().isValid() || (!unit.getDestinationPath().getTiles().empty() && unit.getDestinationPath().getTarget() == TilePosition(unit.getDestination())))
            return;

        auto regrouping = unit.isLightAir() && !unit.getGoal().isValid()
            && Util::getTime() < Time(15, 00)
            && ((unit.attemptingRegroup() && unit.getDestination() == unit.getCommander().lock()->getPosition())
                || unit.getLocalState() == LocalState::Retreat
                || unit.getLocalState() == LocalState::ForcedRetreat
                || unit.getGlobalState() == GlobalState::ForcedRetreat);
        auto harassing = unit.isLightAir() && !unit.getGoal().isValid() && unit.getDestination() == Combat::getHarassPosition() && unit.attemptingHarass() && unit.getLocalState() == LocalState::None;

        // Generate a flying path for retreating or regrouping
        if (regrouping)
            getRegroupPath(unit);

        // Generate a flying path for harassing
        else if (harassing)
            getHarassPath(unit);

        // Generate a generic flying JPS path
        else if (unit.isFlying())
            getFlyingPath(unit);

        // Generate a generic ground JPS path
        else
            getGroundPath(unit);
    }

    void updateNavigation(UnitInfo& unit)
    {
        unit.setNavigation(unit.getFormation().isValid() ? unit.getFormation() : unit.getDestination());

        // If path is reachable, find a point n pixels away to set as new destination
        auto dist = unit.isFlying() ? 96.0 : 160.0;
        if (unit.getDestinationPath().isReachable() && unit.getPosition().getDistance(unit.getDestination()) > dist) {
            auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                return p.getDistance(unit.getPosition()) >= dist;
            });

            if (newDestination.isValid())
                unit.setNavigation(newDestination);
        }
    }

    void updateSimPositions(UnitInfo& unit)
    {
        if (!unit.isLightAir())
            return;

        auto &simUnits = lastSimUnits[&unit];

        //// Remove any expired sim units
        //simUnits.erase(remove_if(simUnits.begin(), simUnits.end(), [&](auto &u) {
        //    return u.first.expired() || Broodwar->getFrameCount() >= u.second;
        //}), simUnits.end());

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
                simUnits[unit.getSimTarget()] = Broodwar->getFrameCount() + (unit.getSimTarget().lock()->getType().isBuilding() ? 480 : 240);
        }

        // For pathing purposes, we store light air commander sim positions
        auto &simPositions = lastSimPositions[&unit];
        simPositions.clear();
        for (auto &[sim, _] : simUnits)
            simPositions.push_back(sim.lock()->getPosition());
    }

    void onFrame()
    {
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
                auto sharedDecision = cluster.commandShare == CommandShare::Exact && unit->getLocalState() != LocalState::ForcedRetreat && unit->getGlobalState() != GlobalState::ForcedRetreat && !unit->isNearSuicide()
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