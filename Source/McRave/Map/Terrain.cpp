#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Terrain {

    namespace {

        // Territory
        map<const Area*, PlayerState> territoryArea;
        map<WalkPosition, PlayerState> territoryChokeGeometry;

        // Enemy
        const BWEB::Station * enemyNatural = nullptr;
        const BWEB::Station * enemyMain = nullptr;
        Position enemyStartingPosition = Positions::Invalid;
        TilePosition enemyStartingTilePosition = TilePositions::Invalid;
        TilePosition enemyExpand = TilePositions::Invalid;
        set<TilePosition> bannedStart;

        // Map
        map<WalkPosition, pair<const BWEM::Area *, const BWEM::Area *>> areaChokeGeometry;
        map<const BWEM::Area *, vector<const BWEM::Area *>> sharedArea;
        set<const Base *> allBases;
        vector<Position> mapEdges, mapCorners;
        bool islandMap = false;
        bool reverseRamp = false;
        bool flatRamp = false;
        bool narrowNatural = false;
        bool pocketNatural = false;

        // Ramps
        Ramp mainRamp;
        Ramp natRamp;
        // vector<Ramp> ramps;

        // Cleanup
        vector<Position> groundCleanupPositions;
        vector<Position> airCleanupPositions;

        // Overlord scouting
        map<const BWEB::Station * const, Position> safeSpots;

        // Chokepoints
        map<const BWEM::ChokePoint * const, double> chokeAngles;
        map<const BWEM::ChokePoint * const, Position> chokeCenters;

        string nodeName = "[Terrain]: ";

        void findEnemy()
        {
            // If we think we found the enemy but we were wrong
            if (enemyStartingPosition.isValid()) {
                if (Broodwar->isExplored(enemyStartingTilePosition) && Util::getTime() < Time(5, 00) && BWEB::Map::isUsed(enemyStartingTilePosition) == UnitTypes::None) {
                    bannedStart.insert(enemyStartingTilePosition);
                    enemyStartingPosition = Positions::Invalid;
                    enemyStartingTilePosition = TilePositions::Invalid;
                    enemyNatural = nullptr;
                    enemyMain = nullptr;
                    Stations::removeStation(enemyStartingPosition, PlayerState::Enemy);
                }
                else
                    return;
            }

            // Find closest enemy building
            if (Util::getTime() < Time(5, 00)) {
                for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                    UnitInfo &unit = *u;
                    if (!unit.getType().isBuilding()
                        || !unit.getTilePosition().isValid()
                        || unit.isFlying()
                        || unit.isProxy())
                        continue;

                    const auto closestMain = BWEB::Stations::getClosestMainStation(unit.getTilePosition());
                    const auto closestNatural = BWEB::Stations::getClosestNaturalStation(unit.getTilePosition());

                    if (find(bannedStart.begin(), bannedStart.end(), unit.getTilePosition()) != bannedStart.end())
                        continue;

                    // Set start if valid
                    if (closestMain && closestMain != getMyMain() && unit.getPosition().getDistance(closestMain->getBase()->Center()) < 960.0) {
                        enemyStartingTilePosition = closestMain->getBase()->Location();
                        enemyStartingPosition = Position(enemyStartingTilePosition) + Position(64, 48);
                    }

                    // Natural we only track inside the area itself
                    if (closestNatural && closestNatural != getMyNatural() && Terrain::inArea(closestNatural->getBase()->GetArea(), unit.getPosition())) {
                        enemyStartingTilePosition = closestMain->getBase()->Location();
                        enemyStartingPosition = Position(enemyStartingTilePosition) + Position(64, 48);
                    }
                }
            }

            // Infer based on enemy Overlord
            static bool inferComplete = false;
            if (Players::vZ() && Util::getTime() < Time(3, 15) && !inferComplete) {
                auto inferedStart = TilePositions::Invalid;
                auto inferedCount = 0;
                for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                    UnitInfo &unit = *u;

                    if (unit.getType() != Zerg_Overlord)
                        continue;

                    for (auto &station : BWEB::Stations::getStations()) {
                        if (Stations::isBaseExplored(&station) || !station.isMain())
                            continue;

                        auto startCenter = station.getBase()->Center();
                        auto overlordStart = Positions::Invalid;
                        auto left = (startCenter.x < 32 * Broodwar->mapWidth() / 2);
                        auto up = (startCenter.y < 32 * Broodwar->mapHeight() / 2);
                        overlordStart.x = startCenter.x + (left ? 99 : -99);
                        overlordStart.y = startCenter.y + (up ? 65 : -65);

                        auto maxTravelDist = Broodwar->getFrameCount() * unit.getSpeed();
                        auto curTravelDist = unit.getPosition().getDistance(overlordStart);

                        if (find(bannedStart.begin(), bannedStart.end(), station.getBase()->Location()) != bannedStart.end())
                            continue;

                        if (maxTravelDist > curTravelDist) {
                            inferedStart = station.getBase()->Location();
                            inferedCount++;
                        }
                    }

                    // Set start if valid
                    if (inferedCount == 1 && inferedStart.isValid()) {
                        enemyStartingPosition = Position(inferedStart) + Position(64, 48);
                        enemyStartingTilePosition = inferedStart;
                        inferComplete = true;
                        Util::debug(string("[Terrain]: Inferred enemy start: ") + to_string(enemyStartingPosition.x) + "," + to_string(enemyStartingPosition.y));
                    }
                }
            }

            // Assume based on only 1 remaining location
            vector<TilePosition> unexploredStarts;
            if (!enemyStartingPosition.isValid()) {
                for (auto &topLeft : mapBWEM.StartingLocations()) {
                    auto botRight = topLeft + TilePosition(3, 2);
                    if (!Broodwar->isExplored(botRight) || !Broodwar->isExplored(topLeft))
                        unexploredStarts.push_back(topLeft);
                }

                if (int(unexploredStarts.size()) == 1) {
                    enemyStartingPosition = Position(unexploredStarts.front()) + Position(64, 48);
                    enemyStartingTilePosition = unexploredStarts.front();
                }
            }

            // Locate Stations
            if (enemyStartingPosition.isValid()) {
                enemyMain = BWEB::Stations::getClosestMainStation(enemyStartingTilePosition);
                enemyNatural = BWEB::Stations::getClosestNaturalStation(enemyStartingTilePosition);

                if (enemyMain) {
                    addTerritory(PlayerState::Enemy, enemyMain);
                    Stations::storeStation(enemyStartingPosition, PlayerState::Enemy);
                }
            }
        }

        void findEnemyNextExpand()
        {
            if (!enemyStartingPosition.isValid())
                return;

            double best = 0.0;
            for (auto &station : BWEB::Stations::getStations()) {

                // If station is used
                if (BWEB::Map::isUsed(station.getBase()->Location()) != None
                    || enemyStartingTilePosition == station.getBase()->Location()
                    || !station.getBase()->GetArea()->AccessibleFrom(getMainArea())
                    || &station == enemyNatural)
                    continue;

                // Get value of the expansion
                double value = 0.0;
                for (auto &mineral : station.getBase()->Minerals())
                    value += double(mineral->Amount());
                for (auto &gas : station.getBase()->Geysers())
                    value += double(gas->Amount());
                if (station.getBase()->Geysers().size() == 0)
                    value = value / 10.0;

                // Get distance of the expansion
                double distance;
                if (!station.getBase()->GetArea()->AccessibleFrom(getMainArea()))
                    distance = log(station.getBase()->Center().getDistance(enemyStartingPosition));
                else
                    distance = BWEB::Map::getGroundDistance(enemyStartingPosition, station.getBase()->Center()) / (BWEB::Map::getGroundDistance(getMainPosition(), station.getBase()->Center()));

                double score = value / distance;

                if (score > best) {
                    best = score;
                    enemyExpand = (TilePosition)station.getBase()->Center();
                }
            }
        }

        void findCleanupPositions()
        {
            groundCleanupPositions.clear();
            airCleanupPositions.clear();

            // ZvZ early game set furthest main as cleanup
            if (Players::ZvZ() && Util::getTime() < Time(5, 00)) {
                auto furthestStation = getMyMain();
                auto distBest = 0.0;
                for (auto &station : BWEB::Stations::getStations()) {
                    auto dist = BWEB::Map::getGroundDistance(station.getBase()->Center(), getMyMain()->getBase()->Center());
                    if (station.isMain() && dist > distBest) {
                        distBest = dist;
                        furthestStation = &station;
                    }
                }
                groundCleanupPositions.push_back(furthestStation->getBase()->Center());
                airCleanupPositions.push_back(furthestStation->getBase()->Center());
                return;
            }

            // Early on it's just the starting locations
            if (Util::getTime() < Time(5, 00)) {
                for (auto &station : BWEB::Stations::getStations()) {
                    if (station.isMain() && !Stations::isBaseExplored(&station)) {
                        groundCleanupPositions.push_back(station.getBase()->Center());
                        airCleanupPositions.push_back(station.getBase()->Center());
                    }
                }
                return;
            }

            // If any base hasn't been visited in past 5 minutes
            for (auto &base : allBases) {
                auto frameDiff = (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(base->Center()));
                if (frameDiff > 7200 || !Broodwar->isExplored(TilePosition(base->Center()))) {
                    groundCleanupPositions.push_back(base->Center());
                    airCleanupPositions.push_back(base->Center());
                }
            }

            // If any reachable tile hasn't been visited in past 5 minutes
            if (groundCleanupPositions.empty()) {
                auto best = 0.0;
                for (int x = 0; x < Broodwar->mapWidth(); x++) {
                    for (int y = 0; y < Broodwar->mapHeight(); y++) {
                        auto t = TilePosition(x, y);
                        auto p = Position(t) + Position(16, 16);

                        if (!Broodwar->isBuildable(t) || !BWEB::Map::isWalkable(t, Protoss_Dragoon))
                            continue;

                        auto frameDiff = (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t));
                        if (frameDiff > 7200 || !Broodwar->isExplored(t)) {
                            groundCleanupPositions.push_back(p);
                            airCleanupPositions.push_back(p);
                        }
                    }
                }
            }
        }

        void updateRamps()
        {

        }

        void updateChokepoints()
        {
            // Notes:
            // BWEM angle is pretty good for flat ramps
            // For angled ramps we shift up to the high ground

            //for (int x = 0; x < Broodwar->mapWidth() * 4; x++) {
            //    for (int y = 0; y < Broodwar->mapHeight() * 4; y++) {
            //        auto w = WalkPosition(x, y);
            //        if (w.isValid() && !mapBWEM.GetMiniTile(w).Walkable())
            //            Visuals::drawBox(w, w + WalkPosition(1, 1), Colors::White);
            //    }
            //}

            auto const findCorrectAngle = [&](auto station) {
                auto chokeAngle = BWEB::Map::getAngle(make_pair(Position(station.getChokepoint()->Pos(station.getChokepoint()->end1)), Position(station.getChokepoint()->Pos(station.getChokepoint()->end2))));
                auto chokeCenter = Position(station.getChokepoint()->Center()) + Position(4, 4);
                auto p1 = chokeCenter + Position(cos(chokeAngle + M_PI_D2) * 96.0, sin(chokeAngle + M_PI_D2) * 96.0);
                auto p2 = chokeCenter - Position(cos(chokeAngle + M_PI_D2) * 96.0, sin(chokeAngle + M_PI_D2) * 96.0);

                if (Terrain::inArea(station.getBase()->GetArea(), p1)) {
                    chokeCenters[station.getChokepoint()] = chokeCenter;
                    chokeAngles[station.getChokepoint()] = BWEB::Map::getAngle(p2, p1);
                }
                else {
                    chokeCenters[Terrain::getMainChoke()] = chokeCenter;
                    chokeAngles[station.getChokepoint()] = BWEB::Map::getAngle(p1, p2);
                }
            };

            for (auto &station : BWEB::Stations::getStations()) {
                if (station.getChokepoint())
                    findCorrectAngle(station);
            }

            if (flatRamp) {
                return;
            }

            vector<WalkPosition> directions ={
                WalkPosition(-1, -1),
                WalkPosition(0, -1),
                WalkPosition(1, -1),
                WalkPosition(1, 0),
                WalkPosition(1, 1),
                WalkPosition(0, 1),
                WalkPosition(-1, 1),
                WalkPosition(-1, 0)
            };
            map<WalkPosition, int> scores;

            // 1. Get some sample tiles of the chokepoint (geometry?)
            //vector<WalkPosition> geometry ={ getMainChoke()->Center(), getMainChoke()->Pos(getMainChoke()->end1), getMainChoke()->Pos(getMainChoke()->end2) };
            auto geometry = getMainChoke()->Geometry();

            // 2. In directions, check walkability
            for (auto &walk : geometry) {
                //Visuals::drawBox(walk, walk + WalkPosition(1, 1), Colors::Yellow);
                for (int x = 0; x < 20; x++) {
                    for (auto &dir : directions) {
                        auto pos = walk + dir * x;
                        if (pos.isValid() && mapBWEM.GetMiniTile(pos).Walkable()) {
                            //Visuals::drawBox(pos, pos + WalkPosition(1, 1), Colors::Green);
                            scores[dir]++;
                            scores[dir*-1]++;
                        }
                    }
                }
            }

            // 3. Show the scores, choose best
            auto scoreBest = 0;
            WalkPosition dirBest;
            WalkPosition walkbest;
            for (auto &[walk, score] : scores) {
                auto position = (walk * 3) + getMainChoke()->Center();
                //Broodwar->drawTextMap(Position(position), "%d", score);

                if (score > scoreBest) {
                    walkbest = walk;
                    scoreBest = score;
                    dirBest = walk;
                }
            }

            // Broodwar terrain angles for ramps are 30deg - TODO this
            auto angle = BWEB::Map::getAngle(dirBest, getMainChoke()->Center());
            chokeAngles[getMainChoke()] = angle;

            // 4. Walk the direction towards main area to get the center
            auto tester1 = getMainChoke()->Center() + dirBest;
            auto tester2 = getMainChoke()->Center() - dirBest;
            auto chokeAltitude = mapBWEM.GetMiniTile(getMainChoke()->Center()).Altitude();
            vector<WalkPosition> similarAltitude1, similarAltitude2;
            vector<WalkPosition> geometry1, geometry2;
            for (auto &w : geometry) {
                geometry1.push_back(w);
                geometry2.push_back(w);
                if (mapBWEM.GetMiniTile(w).Altitude() >= chokeAltitude - 8) {
                    similarAltitude1.push_back(w);
                    similarAltitude2.push_back(w);
                }
            }

            auto limit = 8;
            auto breakout1 = false;
            while (!breakout1) {
                tester1 += dirBest;
                for (auto &w : geometry1) {
                    w += dirBest;
                    if (mapBWEM.GetMiniTile(w).Altitude() >= chokeAltitude + limit) {
                        breakout1 = true;
                    }
                }
            }

            auto breakout2 = false;
            while (!breakout2) {
                tester2 -= dirBest;
                for (auto &w : geometry2) {
                    w -= dirBest;
                    if (mapBWEM.GetMiniTile(w).Altitude() >= chokeAltitude + limit) {
                        breakout2 = true;
                    }
                }
            }

            // 5. Shift center to highest altitude of the vector to fix angle
            // Use geometry of original choke plus the offset 
            auto highest1 = chokeAltitude;
            for (auto &w : geometry1) {
                auto altitude = mapBWEM.GetMiniTile(w).Altitude();
                if (mapBWEM.GetMiniTile(w).Altitude() > highest1) {
                    tester1 = w;
                    highest1 = altitude;
                }
            }
            auto highest2 = chokeAltitude;
            for (auto &w : geometry2) {
                auto altitude = mapBWEM.GetMiniTile(w).Altitude();
                if (mapBWEM.GetMiniTile(w).Altitude() > highest2) {
                    tester2 = w;
                    highest2 = altitude;
                }
            }
            Visuals::drawBox(tester1, tester1 + WalkPosition(1, 1), Colors::Green);
            Visuals::drawBox(tester2, tester2 + WalkPosition(1, 1), Colors::Red);

            // 6. Store true center and angle
            auto c1 = Position(tester1) + Position(4, 4);
            auto c2 = Position(tester2) + Position(4, 4);
            if (Terrain::inArea(Terrain::getMainArea(), c1)) {
                chokeCenters[Terrain::getMainChoke()] = c1;
                chokeAngles[getMainChoke()] = BWEB::Map::getAngle(c2, c1);
                mainRamp.entrance = c1;
                mainRamp.exit = c2;
                mainRamp.angle = BWEB::Map::getAngle(c2, c1);
            }
            else {
                chokeCenters[Terrain::getMainChoke()] = c2;
                chokeAngles[getMainChoke()] = BWEB::Map::getAngle(c1, c2);
                mainRamp.entrance = c2;
                mainRamp.exit = c1;
                mainRamp.angle = BWEB::Map::getAngle(c1, c2);
            }
            mainRamp.center = (mainRamp.entrance + mainRamp.exit) / 2;
            Broodwar->drawTextMap(mainRamp.entrance, "%.2f", mainRamp.angle);

            // 7. Fix angle based on tileset (all angles from horizontal) - ROUGH estimates:
            // Space (benzene) tileset have 40deg ramps normally, 30deg if they are flipped with SCMDraft
            // Jungle (destination) tileset have 50deg ramps normally, 40deg flipped
            // Twilight (neo moon) tileset, 45 normal, 45 deg flipped
            // Desert (la mancha) tilset, 35 normal, 35 flipped

            // HACK: This is nasty, we're setting the ramp angle to 60deg so formations are 30deg
            // We can't get the tileset info from BWAPI, just set them to 60deg so perpendicular is 30deg
            if (mainRamp.angle > 0.0 && mainRamp.angle <= M_PI / 2.0)
                mainRamp.angle = M_PI / 3.0;
            else if (mainRamp.angle > M_PI / 2.0 && mainRamp.angle <= M_PI)
                mainRamp.angle = 2.0 * M_PI / 3.0;
            else if (mainRamp.angle > M_PI && mainRamp.angle <= 3.0 * M_PI / 2.0)
                mainRamp.angle = 4.0 * M_PI / 3.0;
            else
                mainRamp.angle = 5.0 * M_PI / 3.0;
            Broodwar->drawTextMap(mainRamp.entrance + Position(0, 16), "%.2f", mainRamp.angle);
        }

        void updateAreas()
        {
            // Squish areas
            if (getNaturalArea()) {

                // Add to territory if chokes are shared
                if (getMainChoke() == getNaturalChoke() || islandMap)
                    addTerritory(PlayerState::Self, getMyNatural());

                // Add natural if we plan to take it
                if (BuildOrder::isOpener() && BuildOrder::takeNatural())
                    addTerritory(PlayerState::Self, getMyNatural());
            }

            narrowNatural = getNaturalChoke() && getNaturalChoke()->Width() <= 64;
        }

        void drawTerritory()
        {
            if (false) {
                for (int x = 0; x < Broodwar->mapWidth() * 4; x++) {
                    for (int y = 0; y < Broodwar->mapHeight() * 4; y++) {
                        auto w = WalkPosition(x, y);
                        if (w.isValid() && !mapBWEM.GetArea(w))
                            Visuals::drawBox(w, w + WalkPosition(1, 1), Colors::Red, true);
                    }
                }
            }

            // Draw a circle on areas colored by owner
            for (auto &[area, player] : territoryArea) {
                auto color = Colors::White;
                if (player == PlayerState::Self)
                    color = Colors::Green;
                if (player == PlayerState::Enemy)
                    color = Colors::Red;
                Visuals::drawCircle(area->Top(), 12, color);
            }

            // Draw a box around chokepoint geoemtry colored by owner
            for (auto &[walk, player] : territoryChokeGeometry) {
                auto color = Colors::White;
                if (player == PlayerState::Self)
                    color = Colors::Green;
                if (player == PlayerState::Enemy)
                    color = Colors::Red;
                Visuals::drawBox(walk, walk + WalkPosition(1, 1), color);
            }
        }
    }

    Position getClosestMapCorner(Position here)
    {
        auto closestCorner = Positions::Invalid;
        auto closestDist = DBL_MAX;
        for (auto &corner : mapCorners) {
            auto dist = corner.getDistance(here);
            if (dist < closestDist) {
                closestDist = dist;
                closestCorner = corner;
            }
        }
        return closestCorner;
    }

    Position getClosestMapEdge(Position here)
    {
        auto closestCorner = Positions::Invalid;
        auto closestDist = DBL_MAX;
        for (auto &e : mapEdges) {
            auto edge = (e.x == -1) ? Position(here.x, e.y) : Position(e.x, here.y);
            auto dist = edge.getDistance(here);
            if (dist < closestDist) {
                closestDist = dist;
                closestCorner = edge;
            }
        }
        return closestCorner;
    }

    Position getOldestPosition(const Area * area)
    {
        auto oldest = DBL_MAX;
        auto oldestTile = TilePositions::Invalid;
        auto start = getMainArea()->TopLeft();
        auto end = getMainArea()->BottomRight();
        auto closestStation = Stations::getClosestStationGround(Position(area->Top()), PlayerState::Self);

        for (int x = start.x; x < end.x; x++) {
            for (int y = start.y; y < end.y; y++) {
                auto t = TilePosition(x, y);
                auto p = Position(t) + Position(16, 16);
                if (!t.isValid()
                    || mapBWEM.GetArea(t) != area
                    || Broodwar->isVisible(t)
                    || (Broodwar->getFrameCount() - Grids::getLastVisibleFrame(t) < 480)
                    || !Broodwar->isBuildable(t + TilePosition(-1, 0))
                    || !Broodwar->isBuildable(t + TilePosition(1, 0))
                    || !Broodwar->isBuildable(t + TilePosition(0, -1))
                    || !Broodwar->isBuildable(t + TilePosition(0, 1)))
                    continue;

                auto visible = closestStation ? Grids::getLastVisibleFrame(t) / p.getDistance(closestStation->getBase()->Center()) : Grids::getLastVisibleFrame(t);
                if (visible < oldest) {
                    oldest = visible;
                    oldestTile = t;
                }
            }
        }
        return Position(oldestTile) + Position(16, 16);
    }

    Position getSafeSpot(const BWEB::Station * station) {
        if (station && safeSpots.find(station) != safeSpots.end())
            return safeSpots[station];
        return Positions::Invalid;
    }

    bool inArea(const BWEM::Area * area, Position here)
    {
        if (!here.isValid())
            return false;
        auto areaToCheck = mapBWEM.GetArea(TilePosition(here));
        if (areaToCheck == area)
            return true;
        auto geo = areaChokeGeometry.find(WalkPosition(here));
        if (geo != areaChokeGeometry.end()) {
            const auto &areas = geo->second;
            return areas.first == area || areas.second == area;
        }

        auto shared = sharedArea.find(area);
        if (shared != sharedArea.end()) {
            return find(shared->second.begin(), shared->second.end(), areaToCheck) != shared->second.end();
        }
        return false;
    }

    bool inArea(Position there, Position here)
    {
        if (!here.isValid() || !there.isValid())
            return false;
        auto area = mapBWEM.GetArea(TilePosition(there));
        return inArea(area, here);
    }

    bool inTerritory(PlayerState playerState, Position here)
    {
        if (!here.isValid())
            return false;

        auto area = mapBWEM.GetArea(TilePosition(here));
        return (area && territoryArea[area] == playerState) || (!area && territoryChokeGeometry[WalkPosition(here)] == playerState);
    }

    bool inTerritory(PlayerState playerState, const BWEM::Area * area)
    {
        return (area && territoryArea[area] == playerState);
    }

    void addTerritory(PlayerState playerState, const BWEB::Station * station)
    {
        const auto addChokeGeo = [&](auto area) {
            for (auto &choke : area->ChokePoints()) {
                for (auto &geo : choke->Geometry()) {
                    auto tile = TilePosition(geo);
                    for (int x = 0; x < 4; x++) {
                        for (int y = 0; y < 4; y++) {
                            auto w = WalkPosition(tile) + WalkPosition(x, y);
                            if (territoryChokeGeometry[w] == PlayerState::None)
                                territoryChokeGeometry[w] = playerState;
                        }
                    }
                }
            }
        };

        // Add the current station area to territory
        if (territoryArea[station->getBase()->GetArea()] == PlayerState::None) {
            territoryArea[station->getBase()->GetArea()] = playerState;

            // Add individual choke tiles to territory
            if (station->getChokepoint()) {
                for (auto &geo : station->getChokepoint()->Geometry())
                    addChokeGeo(station->getBase()->GetArea());
            }

            // Add empty areas between main partner to territory
            if (station->isMain()) {
                auto closestPartner = station->isMain() ? BWEB::Stations::getClosestNaturalStation(station->getBase()->Location())
                    : BWEB::Stations::getClosestMainStation(station->getBase()->Location());

                if (closestPartner) {
                    for (auto &area : station->getBase()->GetArea()->AccessibleNeighbours()) {

                        // Count how many chokes arent blocked
                        auto openChokes = 0;
                        for (auto &choke : area->ChokePoints()) {
                            if (!choke->Blocked())
                                openChokes++;
                        }

                        if (openChokes > 2
                            || area == closestPartner->getBase()->GetArea())
                            continue;

                        // Found an empty area, add the territory
                        for (auto &choke : area->ChokePoints()) {
                            if (choke == closestPartner->getChokepoint() || choke == station->getChokepoint()) {
                                territoryArea[area] = playerState;
                                sharedArea[station->getBase()->GetArea()].push_back(area);
                                addChokeGeo(area);
                            }
                        }

                        // Found a dead end area
                        if (openChokes <= 1) {
                            territoryArea[area] = playerState;
                            sharedArea[station->getBase()->GetArea()].push_back(area);
                            addChokeGeo(area);
                        }
                    }
                }
            }
        }
    }

    void removeTerritory(PlayerState playerState, const BWEB::Station * const station)
    {
        // Remove the current station area from territory
        if (territoryArea[station->getBase()->GetArea()] == playerState) {
            territoryArea[station->getBase()->GetArea()] = PlayerState::None;

            // Remove individual choke tiles from territory
            if (station->getChokepoint()) {
                for (auto &geo : station->getChokepoint()->Geometry()) {
                    if (territoryChokeGeometry[geo] == playerState)
                        territoryChokeGeometry[geo] = PlayerState::None;
                }
            }

            // Remove empty areas between main partner from territory
            if (station->isMain()) {
                auto closestPartner = station->isMain() ? BWEB::Stations::getClosestNaturalStation(station->getBase()->Location())
                    : BWEB::Stations::getClosestMainStation(station->getBase()->Location());

                if (closestPartner) {
                    for (auto &area : station->getBase()->GetArea()->AccessibleNeighbours()) {

                        if (area->ChokePoints().size() > 2
                            || area == closestPartner->getBase()->GetArea())
                            continue;

                        for (auto &choke : area->ChokePoints()) {
                            if (choke == closestPartner->getChokepoint())
                                territoryArea[area] = PlayerState::None;
                        }
                    }
                }
            }
        }
    }

    void onStart()
    {
        // Initialize BWEM and BWEB
        mapBWEM.Initialize();
        mapBWEM.EnableAutomaticPathAnalysis();
        mapBWEM.FindBasesForStartingLocations();
        BWEB::Map::onStart();
        BWEB::Stations::findStations();

        // Check if the map is an island map
        for (auto &start : mapBWEM.StartingLocations()) {
            if (!mapBWEM.GetArea(start)->AccessibleFrom(mapBWEM.GetArea(getMainTile())))
                islandMap = true;
        }

        // HACK: Play Plasma as an island map
        if (Broodwar->mapFileName().find("Plasma") != string::npos)
            islandMap = true;

        // Store non island bases
        for (auto &area : mapBWEM.Areas()) {
            if (!islandMap && area.AccessibleNeighbours().size() == 0)
                continue;
            for (auto &base : area.Bases())
                allBases.insert(&base);
        }

        // For every area, store their choke geometry as belonging to both areas to prevent nullptr areas
        for (auto &area : mapBWEM.Areas()) {
            for (auto &choke : area.ChokePoints()) {
                for (auto &walk : choke->Geometry())
                    areaChokeGeometry[walk] = choke->GetAreas();
            }
        }

        // Check if we have a pocket natural
        if (Terrain::getMyNatural()) {
            if (Terrain::getMyNatural()->getBase()->GetArea()->ChokePoints().size() <= 1)
                pocketNatural = true;
            if (pocketNatural)
                Util::debug(nodeName + "Pocket natural detected.");
        }

        mapEdges ={ {-1, 0}, {-1, Broodwar->mapHeight() * 32}, {0, -1}, {Broodwar->mapWidth() * 32, -1} };
        mapCorners ={ {0, 0}, {0, Broodwar->mapHeight() * 32 }, {Broodwar->mapWidth() * 32, 0}, {Broodwar->mapWidth() * 32, Broodwar->mapHeight() * 32} };

        reverseRamp = Broodwar->getGroundHeight(getMainTile()) < Broodwar->getGroundHeight(getNaturalTile());
        flatRamp = Broodwar->getGroundHeight(getMainTile()) == Broodwar->getGroundHeight(getNaturalTile());

        updateChokepoints();
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        findEnemy();
        findEnemyNextExpand();
        findCleanupPositions();
        updateAreas();
        //drawTerritory();
        Visuals::endPerfTest("Terrain");
    }

    set<const Base*>& getAllBases() { return allBases; }

    Position getEnemyStartingPosition() { return enemyStartingPosition; }
    const BWEB::Station * getEnemyMain() { return enemyMain; }
    const BWEB::Station * getEnemyNatural() { return enemyNatural; }
    TilePosition getEnemyExpand() { return enemyExpand; }
    TilePosition getEnemyStartingTilePosition() { return enemyStartingTilePosition; }

    bool isIslandMap() { return islandMap; }
    bool isReverseRamp() { return reverseRamp; }
    bool isFlatRamp() { return flatRamp; }
    bool isNarrowNatural() { return narrowNatural; }
    bool isPocketNatural() { return pocketNatural; }
    bool foundEnemy() { return enemyStartingPosition.isValid() && Broodwar->isExplored(TilePosition(enemyStartingPosition)); }
    vector<Position>& getGroundCleanupPositions() { return groundCleanupPositions; }
    vector<Position>& getAirCleanupPositions() { return airCleanupPositions; }

    // Main information
    const BWEB::Station * const getMyMain() { return  BWEB::Stations::getStartingMain(); }
    BWAPI::Position getMainPosition() { return BWEB::Stations::getStartingMain()->getBase()->Center(); }
    BWAPI::TilePosition getMainTile() { return BWEB::Stations::getStartingMain()->getBase()->Location(); }
    const BWEM::Area * getMainArea() { return BWEB::Stations::getStartingMain()->getBase()->GetArea(); }
    const BWEM::ChokePoint * getMainChoke() { return BWEB::Stations::getStartingMain()->getChokepoint(); }

    // Natural information
    const BWEB::Station * const getMyNatural() { return BWEB::Stations::getStartingNatural(); }
    BWAPI::Position getNaturalPosition() { return BWEB::Stations::getStartingNatural()->getBase()->Center(); }
    BWAPI::TilePosition getNaturalTile() { return BWEB::Stations::getStartingNatural()->getBase()->Location(); }
    const BWEM::Area * getNaturalArea() { return BWEB::Stations::getStartingNatural()->getBase()->GetArea(); }
    const BWEM::ChokePoint * getNaturalChoke() { return BWEB::Stations::getStartingNatural()->getChokepoint(); }

    // Chokepoint information
    Position getChokepointCenter(const BWEM::ChokePoint * chokepoint)
    {
        auto itr = chokeCenters.find(chokepoint);
        if (itr != chokeCenters.end())
            return (*itr).second;
        return Positions::Invalid;
    }

    double getChokepointAngle(const BWEM::ChokePoint * chokepoint)
    {
        auto itr = chokeAngles.find(chokepoint);
        if (itr != chokeAngles.end())
            return (*itr).second;
        return 0.0;
    }

    Ramp& getMainRamp() {
        return mainRamp;
    };
}