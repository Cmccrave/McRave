#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Util {

    bool isWalkable(WalkPosition start, WalkPosition end, UnitType unitType)
    {
        // Pixel rectangle
        auto walkWidth = unitType.isBuilding() ? unitType.tileWidth() * 4 : (int)ceil(unitType.width() / 8.0);
        auto walkHeight = unitType.isBuilding() ? unitType.tileHeight() * 4 : (int)ceil(unitType.height() / 8.0);
        auto topLeft = Position(start);
        auto botRight = topLeft + Position(walkWidth * 8, walkHeight * 8);

        // Round up
        int halfW = (walkWidth + 1) / 2;
        int halfH = (walkHeight + 1) / 2;

        if (unitType.isFlyer()) {
            halfW += 2;
            halfH += 2;
        }

        for (int x = end.x - halfW; x < end.x + halfW; x++) {
            for (int y = end.y - halfH; y < end.y + halfH; y++) {
                WalkPosition w(x, y);
                Position p = Position(w) + Position(4, 4);
                if (!w.isValid())
                    return false;

                if (!unitType.isFlyer()) {
                    if (Broodwar->isWalkable(w) && rectangleIntersect(topLeft, botRight, p))
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
        double allyRange = (widths / 2) + (unit.getTarget().getType().isFlyer() ? unit.getAirReach() : unit.getGroundReach());

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
        if (Broodwar->self()->getRace() == Races::Protoss) {
            int completedDefenders = Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot);
            int visibleDefenders = Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) + Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot);

            if (unit.getType() == UnitTypes::Protoss_Probe && (unit.getShields() < 20 || (unit.hasResource() && unit.getResource().getType().isRefinery())))
                return false;

            if (BuildOrder::isHideTech() && completedDefenders == 1 && Units::getMyUnits().size() == 1)
                return true;

            if (BuildOrder::getCurrentBuild() == "PFFE") {
                if (Units::getEnemyCount(UnitTypes::Zerg_Zergling) >= 5) {
                    if (Strategy::getEnemyBuild() == "Z5Pool" && Units::getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 2 && visibleDefenders >= 1)
                        return true;
                    if (Strategy::getEnemyBuild() == "Z9Pool" && Units::getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 5 && visibleDefenders >= 2)
                        return true;
                    if (!Terrain::getEnemyStartingPosition().isValid() && Strategy::getEnemyBuild() == "Unknown" && Units::getGlobalAllyGroundStrength() < 2.00 && completedDefenders < 1 && visibleDefenders > 0)
                        return true;
                    if (Units::getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 2 && visibleDefenders >= 1)
                        return true;
                }
                else {
                    if (Strategy::getEnemyBuild() == "Z5Pool" && Units::getGlobalAllyGroundStrength() < 1.00 && completedDefenders < 2 && visibleDefenders >= 2)
                        return true;
                    if (!Terrain::getEnemyStartingPosition().isValid() && Strategy::getEnemyBuild() == "Unknown" && Units::getGlobalAllyGroundStrength() < 2.00 && completedDefenders < 1 && visibleDefenders > 0)
                        return true;
                }
            }
            else if (BuildOrder::getCurrentBuild() == "P2GateExpand") {
                if (Strategy::getEnemyBuild() == "Z5Pool" && Units::getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 2)
                    return true;
                if (Strategy::getEnemyBuild() == "Z9Pool" && Units::getGlobalAllyGroundStrength() < 4.00 && completedDefenders < 3)
                    return true;
            }
        }
        else if (Broodwar->self()->getRace() == Races::Terran && BuildOrder::isWallNat()) {
            if (Strategy::enemyRush() && BuildOrder::isFastExpand() && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) < 1)
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Zerg)
            return false;

        if (Strategy::enemyProxy() && Strategy::getEnemyBuild() != "P2Gate" && Units::getImmThreat() > Units::getGlobalAllyGroundStrength() + Units::getAllyDefense())
            return true;

        return false;
    }

    bool reactivePullWorker(UnitInfo& unit)
    {
        if (Units::getEnemyCount(UnitTypes::Terran_Vulture) > 2)
            return false;

        if (unit.getType() == UnitTypes::Protoss_Probe) {
            if (unit.getShields() < 8)
                return false;
        }

        if (unit.hasResource()) {
            if (unit.getPosition().getDistance(unit.getResource().getPosition()) < 64.0 && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0 && Broodwar->getFrameCount() < 10000)
                return true;
        }
        else {
            auto station = BWEB::Stations::getClosestStation(unit.getTilePosition());
            if (station && station->ResourceCentroid().getDistance(unit.getPosition()) < 160.0) {
                if (Terrain::isInAllyTerritory(unit.getTilePosition()) && Grids::getEGroundThreat(unit.getWalkPosition()) > 0.0 && Broodwar->getFrameCount() < 10000)
                    return true;
            }
        }

        // If we have no combat units and there is a threat
        if (Units::getImmThreat() > Units::getGlobalAllyGroundStrength() + Units::getAllyDefense() && Broodwar->getFrameCount() < 10000) {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Dragoon) == 0 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Zealot) == 0)
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
        // Determine highest threat possible here
        auto t = unit.getType();
        auto highest = MIN_THREAT;
        auto dx = int(ceil(t.width() / 16.0));		// Half walk resolution width
        auto dy = int(ceil(t.height() / 16.0));		// Half walk resolution height

        WalkPosition center = here + WalkPosition(dx, dy);
        // Testing a performance increase for ground units instead
        if (!unit.getType().isFlyer()) {
            auto grid = Grids::getEGroundThreat(center);
            return max(grid, MIN_THREAT);
        }

        for (int x = here.x - dx; x < here.x + dx; x++) {
            for (int y = here.y - dy; y < here.y + dy; y++) {
                WalkPosition w(x, y);
                if (!w.isValid())
                    continue;

                auto current = unit.getType().isFlyer() ? Grids::getEAirThreat(w) : Grids::getEGroundThreat(w);
                highest = (current > highest) ? current : highest;
            }
        }
        return highest;
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

    UnitInfo * getClosestUnit(Position here, Player p, UnitType t) {
        double distBest = DBL_MAX;
        UnitInfo* best = nullptr;
        auto &units = (p == Broodwar->self()) ? Units::getMyUnits() : Units::getEnemyUnits();

        for (auto &u : units) {
            UnitInfo &unit = u.second;

            if (!unit.unit() || (t != UnitTypes::None && unit.getType() != t))
                continue;

            double dist = here.getDistance(unit.getPosition());
            if (dist < distBest) {
                best = &unit;
                distBest = dist;
            }
        }
        return best;
    }

    UnitInfo * getClosestUnit(UnitInfo& source, Player p, UnitType t) {
        double distBest = DBL_MAX;
        UnitInfo* best = nullptr;
        auto &units = (p == Broodwar->self()) ? Units::getMyUnits() : Units::getEnemyUnits();

        for (auto &u : units) {
            UnitInfo &unit = u.second;

            if (!unit.unit() || source.unit() == unit.unit() || (t != UnitTypes::None && unit.getType() != t))
                continue;

            double dist = source.getPosition().getDistance(unit.getPosition());
            if (dist < distBest) {
                best = &unit;
                distBest = dist;
            }
        }
        return best;
    }

    UnitInfo * getClosestThreat(UnitInfo& unit)
    {
        double distBest = DBL_MAX;
        UnitInfo* best = nullptr;
        auto &units = (unit.getPlayer() == Broodwar->self()) ? Units::getEnemyUnits() : Units::getMyUnits();

        for (auto &t : units) {
            UnitInfo &threat = t.second;
            auto canAttack = unit.getType().isFlyer() ? threat.getAirDamage() > 0.0 : threat.getGroundDamage() > 0.0;

            if (!threat.unit() || !canAttack)
                continue;

            double dist = threat.getPosition().getDistance(unit.getPosition());
            if (dist < distBest) {
                best = &threat;
                distBest = dist;
            }
        }
        return best;
    }

    UnitInfo * getClosestBuilder(Position here)
    {
        double distBest = DBL_MAX;
        UnitInfo* best = nullptr;
        auto &units = Units::getMyUnits();

        for (auto &u : units) {
            UnitInfo &unit = u.second;

            if (!unit.unit() || unit.getRole() != Role::Working || unit.getBuildPosition().isValid() || (unit.hasResource() && !unit.getResource().getType().isMineralField()))
                continue;

            double dist = here.getDistance(unit.getPosition());
            if (dist < distBest) {
                best = &unit;
                distBest = dist;
            }
        }
        return best;
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
        int y = y0 + int(distance * sq * inverseSlope);
        double yInt2 = y - (line1.slope * x);

        Line newLine(yInt2, line1.slope);
        return newLine;
    }

    Position getConcavePosition(UnitInfo& unit, BWEM::Area const * area, Position here)
    {
        // Setup parameters	
        auto min = unit.getGroundRange();
        auto distBest = DBL_MAX;
        auto center = WalkPositions::None;
        auto bestPosition = Positions::None;

        // Finds which position we are forming the concave at
        const auto getConcaveCenter = [&]() {
            if (here.isValid())
                center = (WalkPosition)here;
            else if (area == BWEB::Map::getNaturalArea() && BWEB::Map::getNaturalChoke())
                center = BWEB::Map::getNaturalChoke()->Center();
            else if (area == BWEB::Map::getMainArea() && BWEB::Map::getMainChoke())
                center = BWEB::Map::getMainChoke()->Center();

            else if (area) {
                for (auto &c : area->ChokePoints()) {
                    double dist = BWEB::Map::getGroundDistance(Position(c->Center()), Terrain::getEnemyStartingPosition());
                    if (dist < distBest) {
                        distBest = dist;
                        center = c->Center();
                    }
                }
            }
        };

        const auto checkbest = [&](WalkPosition w) {
            TilePosition t(w);
            Position p = Position(w) + Position(4, 4);
            double dist = p.getDistance((Position)center);

            if (!w.isValid()
                || (here != Terrain::getDefendPosition() && area && mapBWEM.GetArea(t) != area)
                || (here != Terrain::getDefendPosition() && dist < min)
                || unit.getType() == UnitTypes::Protoss_Reaver && Terrain::isDefendNatural() && mapBWEM.GetArea(w) != BWEB::Map::getNaturalArea()
                || dist > distBest
                || Command::overlapsCommands(unit.unit(), UnitTypes::None, p, 8)
                || Command::isInDanger(unit, p)
                || !isWalkable(unit.getWalkPosition(), w, unit.getType())
                || Buildings::overlapsQueue(unit.getType(), t))
                return false;

            bestPosition = p;
            distBest = dist;
            return true;
        };

        // Find the center
        getConcaveCenter();
        distBest = DBL_MAX;

        // If this is the defending position, grab from a vector we already made
        auto &positions = (unit.getGroundRange() < 64.0 && (!Terrain::isDefendNatural() || Players::vZ())) ? Terrain::getMeleeChokePositions() : Terrain::getRangedChokePositions();
        if (here == Terrain::getDefendPosition() && !positions.empty()) {
            for (auto &position : positions) {
                checkbest(WalkPosition(position));
            }
        }

        // Find a position around the center that is suitable
        else {
            for (int x = center.x - 40; x <= center.x + 40; x++) {
                for (int y = center.y - 40; y <= center.y + 40; y++) {
                    WalkPosition w(x, y);
                    checkbest(w);
                }
            }
        }
        return bestPosition;
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
        source.x = clamp(source.x, 0, Broodwar->mapWidth());
        source.y = clamp(source.y, 0, Broodwar->mapHeight());
        return source;
    }
}