#include "Combat.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Navigation {

    namespace {
        map<UnitInfo *, vector<Position>> lastSimPositions;
        map<UnitInfo *, map<weak_ptr<UnitInfo>, int>> lastSimUnits;

        BWEB::Path flyerHarassPath;
        BWEB::Path groundHarassPath;
    } // namespace

    void getGroundPath(UnitInfo &unit)
    {
        auto pathPoint      = unit.getFormation().isValid() ? Util::getPathPoint(unit, unit.getFormation()) : Util::getPathPoint(unit, unit.getDestination());
        auto newPathAllowed = !mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) ||
                              mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)));

        if (unit.attemptingRunby()) {
            unit.setDestinationPath(groundHarassPath);
            return;
        }

        if (newPathAllowed && !unit.hasSamePath(unit.getPosition(), pathPoint)) {
            BWEB::Path newPath(pathPoint, unit.getPosition(), unit.getType());
            newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
            unit.setDestinationPath(newPath);
        }
    }

    void getFlyingPath(UnitInfo &unit)
    {
        BWEB::Path newPath(unit.getDestination(), unit.getPosition(), unit.getType());
        newPath.generateJPS([&](const TilePosition &t) { return true; });
        unit.setDestinationPath(newPath);
    }

    void getFlyingRetreatPath(UnitInfo &unit)
    {
        auto newPathAllowed = !mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(unit.retreatPos)) ||
                              mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(unit.retreatPos)));
        const auto retreat = [&](const TilePosition &t) {
            const auto threat = Grids::getAirThreat(t, PlayerState::Enemy) * 1000.0;
            return threat;
        };

        if (newPathAllowed && !unit.hasSamePath(unit.getPosition(), unit.retreatPos)) {
            BWEB::Path newPath(unit.getPosition(), unit.retreatPos, unit.getType());
            newPath.generateAS_h(retreat);
            unit.setDestinationPath(newPath);
        }
    }

    void getFlyingRegroupPath(UnitInfo &unit)
    {
        if (unit.hasCommander(); auto cmder = unit.getCommander().lock()) {
            auto newPathAllowed = !mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(cmder->getPosition())) ||
                                  mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(cmder->getPosition())));

            if (newPathAllowed && !unit.hasSamePath(unit.getPosition(), cmder->getPosition())) {
                const auto regroup = [&](const TilePosition &t) {
                    const auto threat = Grids::getAirThreat(t, PlayerState::Enemy) * 1000.0;
                    return threat;
                };
                BWEB::Path newPath(unit.getPosition(), cmder->getPosition(), unit.getType());
                newPath.generateAS_h(regroup);
                unit.setDestinationPath(newPath);
            }
        }
    }

    void updatePath(UnitInfo &unit)
    {
        // Check if we need a new path
        if (!unit.getDestination().isValid() || (!unit.getDestinationPath().getTiles().empty() && unit.getDestinationPath().getTarget() == TilePosition(unit.getDestination())) &&
                                                    unit.getDestinationPath().getSource() == TilePosition(unit.getPosition()))
            return;

        if (unit.isLightAir() && !unit.getGoal().isValid()) {
            auto regrouping = unit.attemptingRegroup();
            auto retreating = unit.getLocalState() == LocalState::Retreat;
            auto attacking  = unit.getLocalState() == LocalState::Attack;
            auto harassing  = unit.attemptingHarass();

            if (regrouping) {
                getFlyingRegroupPath(unit);
            }
            else if (retreating) {
                getFlyingRetreatPath(unit);
                Visuals::drawPath(unit.getDestinationPath());
            }
            else if (harassing) {
                // unit.circle(Colors::Green);
                unit.setDestinationPath(flyerHarassPath);
            }
            else {
                getFlyingPath(unit);
            }

            return;
        }

        // Generate a generic flying JPS path
        if (unit.isFlying())
            getFlyingPath(unit);

        // Generate a generic ground JPS path
        else
            getGroundPath(unit);
    }

    void updateNavigation(UnitInfo &unit)
    {
        unit.setNavigation(unit.getFormation().isValid() ? unit.getFormation() : unit.getDestination());

        if (unit.getFormation().isValid() && unit.getLocalState() != LocalState::Attack && !unit.attemptingRunby()) {
            unit.setNavigation(unit.getFormation());
            return;
        }

        // If path is reachable, find a point n pixels away to set as new destination
        auto dist = 160.0;
        if (unit.getDestinationPath().isReachable() && unit.getPosition().getDistance(unit.getDestination()) > dist) {
            auto closestPoint   = unit.getDestination();
            auto closestDist    = DBL_MAX;
            auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                const auto curDist = p.getDistance(unit.getPosition());
                if (curDist < closestDist) {
                    closestDist  = curDist;
                    closestPoint = p;
                }
                return p.getDistance(unit.getPosition()) < dist + 32.0 && p.getDistance(unit.getPosition()) >= dist - 32.0;
            });

            unit.setNavigation(closestPoint);
            if (newDestination.isValid())
                unit.setNavigation(newDestination);
        }
    }

    void updateSimPositions(UnitInfo &unit)
    {
        if (!unit.isLightAir())
            return;

        auto &simUnits = lastSimUnits[&unit];
        for (auto itr = simUnits.begin(); itr != simUnits.end();) {
            if (itr->first.expired() || Broodwar->getFrameCount() >= itr->second)
                itr = simUnits.erase(itr);
            else
                ++itr;
        }

        // Add any new sim units
        if (unit.hasSimTarget()) {
            auto simTarget = unit.getSimTarget().lock();
            if (simTarget->canAttackAir()) {
                auto newSimUnit = simUnits.find(unit.getSimTarget()) == simUnits.end();
                if (newSimUnit)
                    simUnits[unit.getSimTarget()] = Broodwar->getFrameCount() + 1500;
            }
        }

        // For pathing purposes, we store light air commander sim positions
        auto &simPositions = lastSimPositions[&unit];
        simPositions.clear();
        for (auto &[sim, _] : simUnits)
            simPositions.push_back(sim.lock()->getPosition());
    }

    void updateGlobalGroundPaths()
    {
        if (!Terrain::getEnemyStartingPosition().isValid())
            return;

        // Create one path from commander to retreat
        auto harassingCommander = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self,
                                                       [&](auto &u) { return !u->hasCommander() && u->attemptingRunby() && u->getGlobalState() == GlobalState::Attack; });

        if (harassingCommander) {
            auto &unit = *harassingCommander;

            auto start = Util::getPathPoint(unit, Terrain::getEnemyStartingPosition());
            auto end   = Util::getPathPoint(unit, harassingCommander->getPosition());

            if (int(Stations::getStations(PlayerState::Enemy).size()) >= 2 && Units::getEnemyArmyCenter().isValid() &&
                Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), Units::getEnemyArmyCenter()))
                start = Terrain::getEnemyNatural()->getBase()->Center();

            const auto threat = [&](const TilePosition &t) {
                if (Position(t).getDistance(unit.getPosition()) < 96.0)
                    return 1.0;
                return 1.0 + (Grids::getGroundThreat(Position(t) + Position(16, 16), PlayerState::Enemy) * 500.0);
            };

            const auto walkable = [&](const TilePosition &t) { return groundHarassPath.unitWalkable(t); };

            groundHarassPath = {start, end, unit.getType()};
            groundHarassPath.generateAS(threat, walkable);
        }
    }

    void updateGlobalFlyingPaths()
    {
        if (!Combat::getHarassPosition().isValid())
            return;

        // Create one path from commander to retreat
        auto harassingCommander = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self,
                                                       [&](auto &u) { return u->isLightAir() && !u->hasCommander() && u->getGlobalState() == GlobalState::Attack && !u->getGoal().isValid(); });

        if (harassingCommander) {
            auto &unit = *harassingCommander;

            if (flyerHarassPath.getSource() == TilePosition(unit.getDestination()) && flyerHarassPath.getTarget() == TilePosition(unit.getPosition()))
                return;
            flyerHarassPath = {};

            // Harass path
            auto simDistCurrent = unit.hasSimTarget() ? unit.getPosition().getApproxDistance(unit.getSimTarget().lock()->getPosition()) : unit.getPosition().getApproxDistance(unit.getDestination());
            auto simPosition    = unit.hasSimTarget() ? unit.getSimTarget().lock()->getPosition() : unit.getDestination();

            // Generate a flying path for harassing that obeys exploration and staying out of range of threats if possible
            auto &simPositions     = lastSimPositions[&unit];
            auto cachedDist        = min(simDistCurrent, int(unit.getRetreatRadius() + 64.0));
            const auto flyerAttack = [&](const TilePosition &t) {
                const auto center = Position(t) + Position(16, 16);

                auto d = center.getApproxDistance(simPosition);
                for (auto &pos : simPositions)
                    d = min(d, center.getApproxDistance(pos));

                auto dist = max(0.01, double(d) - cachedDist);
                auto vis  = clamp(double(Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t)) * 0.0002, 0.05, 0.5);
                return Util::fastReciprocal(dist * vis);
            };

            flyerHarassPath = {unit.getDestination(), unit.getPosition(), unit.getType()};
            flyerHarassPath.generateAS_h(flyerAttack);
        }
    }

    void onFrame()
    {
        updateGlobalFlyingPaths();
        updateGlobalGroundPaths();

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
                auto sharedDecision = cluster.commandShare == CommandShare::Exact && unit->canMirrorCommander(*commander);
                updateSimPositions(*unit);

                // If it's not a shared decision, indepdently update pathing and navigation waypoint
                if (!sharedDecision) {
                    updatePath(*unit);
                    updateNavigation(*unit);
                }
                else {
                    unit->setNavigation(commander->getNavigation());
                    unit->setDestinationPath(commander->getDestinationPath());
                }
            }
        }
    }
} // namespace McRave::Combat::Navigation