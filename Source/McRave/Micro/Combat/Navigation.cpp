#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::Navigation {

    namespace {
        map<UnitInfo*, vector<Position>> lastSimPositions;

        bool lightUnitNeedsRegroup(UnitInfo& unit)
        {
            if (!unit.isLightAir())
                return false;
            return unit.hasCommander() && unit.getPosition().getDistance(unit.getCommander().lock()->getPosition()) > 64.0;
        }

        Position getPathPoint(UnitInfo& unit, Position start)
        {
            // Create a pathpoint
            auto pathPoint = start;
            auto usedTile = BWEB::Map::isUsed(TilePosition(start));
            if (!BWEB::Map::isWalkable(TilePosition(start), unit.getType()) || usedTile != UnitTypes::None) {
                auto dimensions = usedTile != UnitTypes::None ? usedTile.tileSize() : TilePosition(1, 1);
                auto closest = DBL_MAX;
                for (int x = TilePosition(start).x - dimensions.x; x < TilePosition(start).x + dimensions.x + 1; x++) {
                    for (int y = TilePosition(start).y - dimensions.y; y < TilePosition(start).y + dimensions.y + 1; y++) {
                        auto center = Position(TilePosition(x, y)) + Position(16, 16);
                        auto dist = center.getDistance(unit.getPosition());
                        if (dist < closest && BWEB::Map::isWalkable(TilePosition(x, y), unit.getType()) && BWEB::Map::isUsed(TilePosition(x, y)) == UnitTypes::None) {
                            closest = dist;
                            pathPoint = center;
                        }
                    }
                }
            }
            return pathPoint;
        }
    }

    void updateHarassPath(UnitInfo& unit)
    {
        auto regrouping = (unit.attemptingRegroup() && unit.getDestination() == unit.getCommander().lock()->getPosition());
        auto harassing = (unit.getDestination() == Terrain::getHarassPosition() && unit.attemptingHarass());

        // For pathing purposes, we store light air commander sim positions
        auto &simPositions = lastSimPositions[&unit];
        if (unit.hasSimTarget()) {
            auto simTarget = unit.getSimTarget().lock();

            if (find(simPositions.begin(), simPositions.end(), simTarget->getPosition()) == simPositions.end()) {
                if (simPositions.size() >= 5)
                    simPositions.pop_back();
                simPositions.insert(simPositions.begin(), simTarget->getPosition());
            }
        }

        // Generate a flying path for retreating or regrouping
        if (!unit.getGoal().isValid() && (unit.globalRetreat() || unit.localRetreat() || unit.getLocalState() == LocalState::Retreat || regrouping)) {

            const auto flyerRegroup = [&](const TilePosition &t) {
                return Grids::getEAirThreat(Position(t) + Position(16, 16));
            };

            BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
            newPath.generateAS(flyerRegroup);
            unit.setDestinationPath(newPath);
            //unit.circle(Colors::Purple);
        }

        // Generate a flying path for harassing
        else if (!unit.getGoal().isValid() && harassing) {

            auto simDistCurrent = unit.hasSimTarget() ? unit.getPosition().getApproxDistance(unit.getSimTarget().lock()->getPosition()) : unit.getPosition().getApproxDistance(unit.getDestination());
            auto simPosition = unit.hasSimTarget() ? unit.getSimTarget().lock()->getPosition() : unit.getDestination();

            // Generate a flying path for harassing that obeys exploration and staying out of range of threats if possible
            const auto flyerAttack = [&](const TilePosition &t) {
                const auto center = Position(t) + Position(16, 16);

                auto d = center.getApproxDistance(simPosition);
                for (auto &pos : simPositions)
                    d = min(d, center.getApproxDistance(pos));

                auto dist = unit.getSimState() == SimState::Win ? 1.0 : max(0.01, double(d) - min(simDistCurrent, int(unit.getRetreatRadius() + 64.0)));
                auto vis = unit.getSimState() == SimState::Win ? 1.0 : clamp(double(Broodwar->getFrameCount() - Grids::lastVisibleFrame(t)) / 960.0, 0.5, 3.0);
                return 1.00 / (vis * dist);
            };

            BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
            newPath.generateAS(flyerAttack);
            unit.setDestinationPath(newPath);
            //unit.circle(Colors::Red);
        }

        // Generate a generic JPS path
        else {
            BWEB::Path newPath(unit.getPosition(), unit.getDestination(), unit.getType());
            newPath.generateJPS([&](const TilePosition &t) { return true; });
            unit.setDestinationPath(newPath);
            //unit.circle(Colors::Orange);
        }
    }

    void updateDestinationPath(UnitInfo& unit)
    {
        // Generate a new path that obeys collision of terrain and buildings
        if (!unit.isFlying() && unit.getDestination().isValid() && unit.getDestinationPath().getTiles().empty() && unit.getDestinationPath().getTarget() != TilePosition(unit.getDestination())) {
            auto pathPoint = getPathPoint(unit, unit.getDestination());
            if (!mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)))) {
                BWEB::Path newPath(unit.getPosition(), pathPoint, unit.getType());
                newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
                unit.setDestinationPath(newPath);
            }
        }
    }

    void updateNavigation(UnitInfo& unit)
    {
        unit.setNavigation(unit.getDestination());
        if (unit.getFormation().isValid() && (unit.getLocalState() == LocalState::Retreat || unit.getGlobalState() == GlobalState::Retreat)) {
            unit.setNavigation(unit.getFormation());
            return;
        }            

        // If path is reachable, find a point n pixels away to set as new destination
        if (unit.getDestinationPath().isReachable() && unit.getPosition().getDistance(unit.getDestination()) > 96.0) {
            auto newDestination = Util::findPointOnPath(unit.getDestinationPath(), [&](Position p) {
                return p.getDistance(unit.getPosition()) >= 160.0;
            });

            if (newDestination.isValid())
                unit.setNavigation(newDestination);
        }
    }

    void update(UnitInfo& unit)
    {
        if (unit.isLightAir())
            updateHarassPath(unit);
        else
            updateDestinationPath(unit);
        updateNavigation(unit);
    }
}