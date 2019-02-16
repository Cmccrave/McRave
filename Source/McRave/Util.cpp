#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Util {

    bool isWalkable(UnitInfo& unit, WalkPosition here)
    {
        // Pixel rectangle
        auto walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : (int)ceil(unit.getType().width() / 8.0);
        auto walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : (int)ceil(unit.getType().height() / 8.0);
        auto topLeft = Position(unit.getWalkPosition());
        auto botRight = topLeft + Position(walkWidth * 8, walkHeight * 8);

        // Round up
        int halfW = (walkWidth + 1) / 2;
        int halfH = (walkHeight + 1) / 2;

        if (unit.getType().isFlyer())
            return true;

        auto collision = Grids::getCollision(here);
        if (collision > 0 && collision != unit.unit()->getID())
            return false;

        for (int x = here.x - halfW; x < here.x + halfW; x++) {
            for (int y = here.y - halfH; y < here.y + halfH; y++) {
                WalkPosition w(x, y);
                Position p = Position(w) + Position(4, 4);
                if (!w.isValid())
                    return false;

                if (!unit.getType().isFlyer()) {
                    if (rectangleIntersect(topLeft, botRight, p))
                        continue;
                    else if (!Broodwar->isWalkable(w) || Grids::getCollision(w) > 0)
                        return false;
                }
            }
        }
        return true;
    }

    bool unitInRange(UnitInfo& unit)
    {
        if (!unit.hasTarget())
            return false;

        double widths = unit.getTarget().getType().width() + unit.getType().width();
        double allyRange = (widths / 2) + (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());

        // HACK: Reavers have a weird ground distance range, try to reduce the amount of times Reavers try to engage
        // TODO: Add a Reaver ground dist check
        if (unit.getType() == UnitTypes::Protoss_Reaver)
            allyRange -= 96.0;

        if (unit.getPosition().getDistance(unit.getTarget().getPosition()) <= allyRange)
            return true;
        return false;
    }


    bool proactivePullWorker(UnitInfo& unit)
    {
        auto myStrength = Players::getStrength(PlayerState::Self);

        if (Broodwar->self()->getRace() == Races::Protoss) {
            int completedDefenders = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot);
            int visibleDefenders = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot);

            if (unit.getType() == UnitTypes::Protoss_Probe && (unit.getShields() < 20 || (unit.hasResource() && unit.getResource().getType().isRefinery())))
                return false;

            if (BuildOrder::isHideTech() && Units::getMyRoleCount(Role::Combat) == 1 + int(unit.getRole() == Role::Combat))
                return true;

            if (BuildOrder::getCurrentBuild() == "FFE") {
                if (Strategy::enemyRush() && myStrength.groundToGround < 4.00 && completedDefenders < 2 && visibleDefenders >= 1)
                    return true;
                if (Strategy::enemyRush() && myStrength.groundToGround < 1.00 && completedDefenders < 2 && visibleDefenders >= 2)
                    return true;
                if (Strategy::enemyPressure() && myStrength.groundToGround < 4.00 && completedDefenders < 5 && visibleDefenders >= 2)
                    return true;
                if (!Terrain::getEnemyStartingPosition().isValid() && Strategy::getEnemyBuild() == "Unknown" && myStrength.groundToGround < 2.00 && completedDefenders < 1 && visibleDefenders > 0)
                    return true;
            }
            else if (BuildOrder::getCurrentBuild() == "2Gate" && BuildOrder::getCurrentOpener() == "Natural") {
                if (Strategy::getEnemyBuild() == "4Pool" && myStrength.groundToGround < 4.00 && completedDefenders < 2)
                    return true;
                if (Strategy::getEnemyBuild() == "9Pool" && myStrength.groundToGround < 4.00 && completedDefenders < 3)
                    return true;
            }
        }
        else if (Broodwar->self()->getRace() == Races::Terran && BuildOrder::isWallNat()) {
            if (Strategy::enemyRush() && BuildOrder::isFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) < 1)
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Zerg)
            return false;

        if (Strategy::enemyProxy() && Strategy::getEnemyBuild() != "2Gate" && Units::getImmThreat() > myStrength.groundToGround + myStrength.groundDefense)
            return true;

        return false;
    }

    bool reactivePullWorker(UnitInfo& unit)
    {
        auto myStrength = Players::getStrength(PlayerState::Self);
        auto closestStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());

        if (Units::getEnemyCount(UnitTypes::Terran_Vulture) > 2)
            return false;

        if (unit.getType() == UnitTypes::Protoss_Probe) {
            if (unit.getShields() < 8)
                return false;
        }
        if (unit.getType() == UnitTypes::Terran_SCV) {
            if (unit.getHealth() < 10)
                return false;
        }

        if (unit.hasTarget()) {
            if (unit.getTarget().hasAttackedRecently() && unit.getTarget().getPosition().getDistance(closestStation) < unit.getTarget().getGroundReach() && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0 && Broodwar->getFrameCount() < 10000)
                return true;
        }

        // If we have no combat units and there is a threat
        if (Units::getImmThreat() > (myStrength.groundToGround + myStrength.groundDefense) + unit.getVisibleGroundStrength() && Broodwar->getFrameCount() < 10000) {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                return true;
            }
            else if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Zergling) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Mutalisk) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Hydralisk) == 0)
                    return true;
            }
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture) == 0)
                    return true;
            }
        }
        return false;
    }

    bool pullRepairWorker(UnitInfo& unit)
    {
        return false; // Disabled
        if (Broodwar->self()->getRace() == Races::Terran) {
            int mechUnits = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture)
                + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)
                + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)
                + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Goliath)
                + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Wraith)
                + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Battlecruiser)
                + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Valkyrie);

            //if ((mechUnits > 0 && Units::getRepairWorkers() < Units::getSupply() / 30)
            //	|| (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) > 0 && BuildOrder::isFastExpand() && Units::getRepairWorkers() < 2))
            //	return true;
        }
        return false;
    }


    double getHighestThreat(WalkPosition here, UnitInfo& unit)
    {
        auto threat = max(MIN_THREAT, unit.getType().isFlyer() ? Grids::getEAirThreat(here) : Grids::getEGroundThreat(here));
        return threat;

        // Disable for now

        //// Determine highest threat possible here
        //auto t = unit.getType();
        //auto highest = MIN_THREAT;
        //auto dx = int(ceil(t.width() / 16.0));		// Half walk resolution width
        //auto dy = int(ceil(t.height() / 16.0));		// Half walk resolution height

        //WalkPosition center = here + WalkPosition(dx, dy);
        //// Testing a performance increase for ground units instead
        //if (!unit.getType().isFlyer()) {
        //    auto grid = Grids::getEGroundThreat(center);
        //    return max(grid, MIN_THREAT);
        //}

        //for (int x = here.x - dx; x < here.x + dx; x++) {
        //    for (int y = here.y - dy; y < here.y + dy; y++) {
        //        WalkPosition w(x, y);
        //        if (!w.isValid())
        //            continue;

        //        auto current = unit.getType().isFlyer() ? Grids::getEAirThreat(w) : Grids::getEGroundThreat(w);
        //        highest = (current > highest) ? current : highest;
        //    }
        //}
        //return highest;
    }

    bool accurateThreatOnPath(UnitInfo& unit, BWEB::PathFinding::Path& path)
    {
        if (path.getTiles().empty())
            return false;

        for (auto &tile : path.getTiles()) {
            auto w = WalkPosition(tile);
            auto threat = unit.getType().isFlyer() ? Grids::getEAirThreat(w) > 0.0 : Grids::getEGroundThreat(w) > 0.0;
            if (threat)
                return true;
        }
        return false;
    }

    bool rectangleIntersect(Position topLeft, Position botRight, Position target)
    {
        if (target.x >= topLeft.x
            && target.x < botRight.x
            && target.y >= topLeft.y
            && target.y < botRight.y)
            return true;
        return false;
    }

    bool rectangleIntersect(Position topLeft, Position botRight, int x, int y)
    {
        if (x >= topLeft.x
            && x < botRight.x
            && y >= topLeft.y
            && y < botRight.y)
            return true;
        return false;
    }

    const BWEM::ChokePoint * getClosestChokepoint(Position here)
    {
        double distBest = DBL_MAX;
        const BWEM::ChokePoint * closest = nullptr;
        for (auto &area : mapBWEM.Areas()) {
            for (auto &choke : area.ChokePoints()) {
                double dist = Position(choke->Center()).getDistance(here);
                if (dist < distBest) {
                    distBest = dist;
                    closest = choke;
                }
            }
        }
        return closest;
    }

    int chokeWidth(const BWEM::ChokePoint * choke)
    {
        if (!choke)
            return 0;
        return int(choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2))) * 8;
    }

    Line lineOfBestFit(const BWEM::ChokePoint * choke)
    {
        int minX= INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
        double sumX = 0, sumY = 0;
        double sumXY = 0, sumX2 = 0, sumY2 = 0;
        for (auto geo : choke->Geometry()) {
            if (geo.x < minX) minX = geo.x;
            if (geo.y < minY) minY = geo.y;
            if (geo.x > maxX) maxX = geo.x;
            if (geo.y > maxY) maxY = geo.y;

            BWAPI::Position p = BWAPI::Position(geo) + BWAPI::Position(4, 4);
            sumX += p.x;
            sumY += p.y;
            sumXY += p.x * p.y;
            sumX2 += p.x * p.x;
            sumY2 += p.y * p.y;
            //BWAPI::Broodwar->drawBoxMap(BWAPI::Position(geo), BWAPI::Position(geo) + BWAPI::Position(9, 9), BWAPI::Colors::Black);
        }
        double xMean = sumX / choke->Geometry().size();
        double yMean = sumY / choke->Geometry().size();
        double denominator, slope, yInt;
        if ((maxY - minY) > (maxX - minX))
        {
            denominator = (sumXY - sumY * xMean);
            // handle vertical line error
            if (std::fabs(denominator) < 1.0) {
                slope = 0;
                yInt = xMean;
            }
            else {
                slope = (sumY2 - sumY * yMean) / denominator;
                yInt = yMean - slope * xMean;
            }
        }
        else {
            denominator = sumX2 - sumX * xMean;
            // handle vertical line error
            if (std::fabs(denominator) < 1.0) {
                slope = DBL_MAX;
                yInt = 0;
            }
            else {
                slope = (sumXY - sumX * yMean) / denominator;
                yInt = yMean - slope * xMean;
            }
        }
        return Line(yInt, slope);
    }

    Line parallelLine(Line line1, int x0, double distance)
    {
        double inverseSlope = (-1.0 / line1.slope);
        double y0 = line1.y(int(x0));
        double sq = sqrt(1.0 / (1.0 + pow(inverseSlope, 2.0)));

        int x = x0 + int(distance * sq);
        int y = int(y0 + distance * sq * inverseSlope);
        double yInt2 = y - (line1.slope * x);

        Line newLine(yInt2, line1.slope);
        return newLine;
    }

    Position getConcavePosition(UnitInfo& unit, double radius, BWEM::Area const * area, Position here)
    {
        auto center = WalkPositions::None;
        auto distBest = DBL_MAX;
        auto posBest = Positions::None;

        // Finds which position we are forming the concave at
        if (here.isValid())
            center = (WalkPosition)here;
        else if (area == BWEB::Map::getNaturalArea() && BWEB::Map::getNaturalChoke())
            center = BWEB::Map::getNaturalChoke()->Center();
        else if (area == BWEB::Map::getMainArea() && BWEB::Map::getMainChoke())
            center = BWEB::Map::getMainChoke()->Center();
        else if (area) {
            auto distEnemyBest = DBL_MAX;
            for (auto &c : area->ChokePoints()) {
                double dist = BWEB::Map::getGroundDistance(Position(c->Center()), Terrain::getEnemyStartingPosition());
                if (dist < distEnemyBest) {
                    distEnemyBest = dist;
                    center = c->Center();
                }
            }
        }

        const auto checkbest = [&](WalkPosition w) {
            auto t = TilePosition(w);
            auto p = Position(w) + Position(4, 4);
            auto dist = p.getDistance(Position(center));

            if (!w.isValid()
                || (!Terrain::isInAllyTerritory(t) && area && mapBWEM.GetArea(w) != area)
                || dist < radius
                || dist > distBest
                || Command::overlapsActions(unit.unit(), unit.getType(), p, 8)
                || Command::isInDanger(unit, p)
                || !isWalkable(unit, w)
                || Buildings::overlapsQueue(unit.getType(), TilePosition(w)))
                return;

            posBest = p;
            distBest = dist;
        };

        // Find a position around the center that is suitable
        for (int x = center.x - 40; x <= center.x + 40; x++) {
            for (int y = center.y - 40; y <= center.y + 40; y++) {
                WalkPosition w(x, y);
                checkbest(w);
            }
        }
        return posBest;
    }

    Position clipPosition(Position source, Position target)
    {
        if (target.isValid())
            return target;

        auto sqDist = source.getApproxDistance(target);
        auto clip = clipToMap(target);
        auto dx = clip.x - target.x;
        auto dy = clip.y - target.y;

        if (abs(dx) < abs(dy)) {
            int y = (int)sqrt(sqDist - dx * dx);
            target.x = clip.x;
            if (source.y - y < 0) {
                target.y = source.y + y;
            }
            else if (source.y + y >= Broodwar->mapHeight() * 32) {
                target.y = source.y - y;
            }
            else {
                target.y = (target.y >= source.y) ? source.y + y : source.y - y;
            }
        }
        else {
            int x = (int)sqrt(sqDist - dy * dy);
            target.y = clip.y;

            if (source.x - x < 0)
                target.x = source.x + x;
            else if (source.x + x >= Broodwar->mapWidth() * 32)
                target.x = source.x - x;
            else
                target.x = (target.x >= source.x) ? source.x + x : source.x - x;
        }
        return target;
    }

    Position clipToMap(Position source)
    {
        source.x = clamp(source.x, 0, Broodwar->mapWidth() * 32);
        source.y = clamp(source.y, 0, Broodwar->mapHeight() * 32);
        return source;
    }
}