#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {

    namespace {
        const Station * startMainStation = nullptr;
        const Station * startNatStation = nullptr;
        vector<Station> stations;
        vector<const BWEM::Base *> mainBases;
        vector<const BWEM::Base *> natBases;
        vector<const BWEM::ChokePoint*> mainChokes;
        vector<const BWEM::ChokePoint*> natChokes;
        set<const BWEM::ChokePoint*> nonsenseChokes;
        map<const BWEM::Mineral * const, vector<TilePosition>> testTiles;
        vector<BWEB::Path> testPaths;
        UnitType defenseType;
    }

    void Station::addResourceReserves()
    {
        const auto biggerReserve = [&](Unit resource, TilePosition start, TilePosition end) {
            vector<TilePosition> directions{ {1,0}, {-1,0}, {0, 1}, {0,-1} };
            TilePosition next = start;
            Position pEnd = Position(end);

            while (next != end) {
                auto distBest = DBL_MAX;
                start = next;
                for (auto &t : directions) {
                    auto tile = start + t;
                    auto pos = Position(tile);

                    if (!tile.isValid())
                        continue;

                    auto dist = pos.getDistance(pEnd);
                    if (dist <= distBest) {
                        next = tile;
                        distBest = dist;
                    }
                }
                Map::addReserve(next, 1, 1);
            }
        };

        // Add reserved tiles
        for (auto &m : base->Minerals()) {
            Map::addReserve(m->TopLeft(), 2, 1);
            //biggerReserve(m->Unit(), m->Unit()->getTilePosition(), base->Location());
            //biggerReserve(m->Unit(), m->Unit()->getTilePosition() + TilePosition(1, 0), base->Location() + TilePosition(3, 2));
        }
        for (auto &g : base->Geysers()) {
            Map::addReserve(g->TopLeft(), 4, 2);
            //biggerReserve(g->Unit(), g->Unit()->getTilePosition(), base->Location());
            //biggerReserve(g->Unit(), g->Unit()->getTilePosition() + TilePosition(3, 1), base->Location() + TilePosition(3, 2));
        }
    }

    void Station::initialize()
    {
        auto cnt = 0;

        // Resource and defense centroids
        for (auto &mineral : base->Minerals()) {
            resourceCentroid += mineral->Pos();
            cnt++;
        }

        if (cnt > 0)
            defenseCentroid = resourceCentroid / cnt;

        for (auto &gas : base->Geysers()) {
            defenseCentroid = (defenseCentroid + gas->Pos()) / 2;
            resourceCentroid += gas->Pos();
            cnt++;
        }

        if (cnt > 0)
            resourceCentroid = resourceCentroid / cnt;
        Map::addUsed(base->Location(), Broodwar->self()->getRace().getResourceDepot());

        if (Broodwar->self()->getRace() == Races::Protoss)
            defenseType = UnitTypes::Protoss_Photon_Cannon;
        if (Broodwar->self()->getRace() == Races::Terran)
            defenseType = UnitTypes::Terran_Missile_Turret;
        if (Broodwar->self()->getRace() == Races::Zerg)
            defenseType = UnitTypes::Zerg_Creep_Colony;
    }

    void Station::findChoke()
    {
        // Only find a Chokepoint for mains or naturals
        if (!main && !natural && base->GetArea()->ChokePoints().size() > 1)
            return;

        // Get closest partner base
        auto distBest = DBL_MAX;
        for (auto &potentialPartner : (main ? natBases : mainBases)) {
            auto dist = Map::getGroundDistance(potentialPartner->Center(), base->Center());
            if (dist < distBest) {
                partnerBase = potentialPartner;
                distBest = dist;
            }
        }
        if (!partnerBase)
            return;

        // Get partner choke
        const BWEM::ChokePoint * partnerChoke = nullptr;
        if (main && !Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).empty()) {
            partnerChoke = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).front();

            // Partner only has one chokepoint means we have a shared choke with this path
            if (partnerBase->GetArea()->ChokePoints().size() == 1) {
                partnerChoke = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).back();
                if (base->GetArea()->ChokePoints().size() <= 2) {
                    for (auto &c : base->GetArea()->ChokePoints()) {
                        if (c != partnerChoke) {
                            choke = c;
                            return;
                        }
                    }
                }
            }
            else {
                choke = partnerChoke;
                return;
            }
        }

        // Only one chokepoint in this area, otherwise find an ideal one
        if (base->GetArea()->ChokePoints().size() == 1) {
            choke = base->GetArea()->ChokePoints().front();
            return;
        }

        // Find chokes between partner base and this base
        set<BWEM::ChokePoint const *> nonChokes;
        for (auto &c : Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()))
            nonChokes.insert(c);
        distBest = DBL_MAX;
        const BWEM::Area* second = nullptr;

        // Iterate each neighboring area to get closest to this natural area
        for (auto &area : base->GetArea()->AccessibleNeighbours()) {
            auto center = area->Top();
            const auto dist = Position(center).getDistance(Map::mapBWEM.Center());

            // Label areas that connect with partner
            bool wrongArea = false;
            for (auto &c : area->ChokePoints()) {
                if (find(nonsenseChokes.begin(), nonsenseChokes.end(), c) == nonsenseChokes.end()) {
                    if ((!c->Blocked() && c->Pos(c->end1).getDistance(c->Pos(c->end2)) <= 2) || nonChokes.find(c) != nonChokes.end())
                        wrongArea = true;
                }
            }
            if (wrongArea)
                continue;

            if (/*center.isValid() &&*/ dist < distBest) {
                second = area;
                distBest = dist;
            }
        }

        distBest = DBL_MAX;
        for (auto &c : base->GetArea()->ChokePoints()) {
            if (c->Blocked()
                || find(mainChokes.begin(), mainChokes.end(), c) != mainChokes.end()
                || c->Geometry().size() <= 3
                || (c->GetAreas().first != second && c->GetAreas().second != second)
                )
                continue;

            const auto dist = Position(c->Center()).getDistance(Position(partnerBase->Center()));
            if (dist < distBest) {
                choke = c;
                distBest = dist;
            }
        }
    }

    void Station::findAngles()
    {
        if (choke)
            main ? mainChokes.push_back(choke) : natChokes.push_back(choke);

        // Create angles that dictate a lot about how the station is formed
        if (choke) {
            anglePosition = Position(choke->Center()) + Position(4, 4);
            auto dist = getBase()->Center().getDistance(anglePosition);
            baseAngle = Map::getAngle(make_pair(getBase()->Center(), anglePosition));
            chokeAngle = Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))));

            baseAngle = (round(baseAngle / M_PI_D4)) * M_PI_D4;
            chokeAngle = (round(chokeAngle / M_PI_D4)) * M_PI_D4;

            defenseAngle = baseAngle + M_PI_D2;

            // Narrow chokes don't dictate our angles
            if (choke->Width() < 96.0)
                return;

            // If the chokepoint is against the map edge, we shouldn't treat it as an angular chokepoint
            // Matchpoint fix
            if (TilePosition(choke->Center()).x < 10 || TilePosition(choke->Center()).x > Broodwar->mapWidth() - 10) {
                chokeAngle = Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))));
                chokeAngle = round(chokeAngle / M_PI_D2) * M_PI_D2;
                defenseAngle = chokeAngle;
                return;
            }

            if (base->GetArea()->ChokePoints().size() >= 3) {
                const BWEM::ChokePoint * validSecondChoke = nullptr;
                for (auto &otherChoke : base->GetArea()->ChokePoints()) {
                    if (choke == otherChoke)
                        continue;

                    if ((choke->GetAreas().first == otherChoke->GetAreas().first && choke->GetAreas().second == otherChoke->GetAreas().second)
                        || (choke->GetAreas().first == otherChoke->GetAreas().second && choke->GetAreas().second == otherChoke->GetAreas().first))
                        validSecondChoke = otherChoke;
                }

                if (validSecondChoke)
                    defenseAngle = (Map::getAngle(make_pair(Position(choke->Center()), Position(validSecondChoke->Center()))) + Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))))) / 2.0;
            }

            else if ((dist >= 320.0 && !main && !natural) || (dist >= 368 && natural)) {
                auto baseMod = fmod(baseAngle + M_PI_D2, M_PI);
                auto chokeMod = fmod(chokeAngle, M_PI);
                defenseAngle = fmod((baseMod + chokeMod) / 2.0, M_PI);
            }
        }
    }

    void Station::findSecondaryLocations()
    {
        // Add scanner addon for Terran
        if (Broodwar->self()->getRace() == Races::Terran) {
            auto scannerTile = base->Location() + TilePosition(4, 1);
            smallPosition = scannerTile;
            Map::addUsed(scannerTile, defenseType);
        }

        if (Broodwar->self()->getRace() != Races::Zerg)
            return;

        vector<tuple<TilePosition, TilePosition, TilePosition>> tryOrder;

        // Determine some standard positions to place simcity pool/spire/sunken
        if (main) {
            auto mineralsLeft = resourceCentroid.x < base->Center().x;
            for (auto &geyser : base->Geysers()) {

                // Geyser west of hatchery
                if (geyser->TopLeft() == base->Location() + TilePosition(-7, 1)) {

                    // North
                    if (baseAngle < 2.355 && baseAngle >= 0.785) {
                        tryOrder ={
                        { TilePosition(-3, -2), TilePosition(-5, -1), TilePosition(0,0) }
                        };
                    }

                    // West
                    else if (baseAngle < 3.925 && baseAngle >= 2.355) {
                        if (mineralsLeft) {

                        }
                        else {
                            tryOrder ={
                            { TilePosition(-3, -4), TilePosition(-5, -2), TilePosition(-3,-2) }
                            };
                        }
                    }
                }

                // Geyser north of hatchery
                if (geyser->TopLeft() == base->Location() + TilePosition(0, -5)) {

                    // North
                    if (baseAngle < 2.355 && baseAngle >= 0.785) {
                        if (mineralsLeft) {
                            tryOrder ={
                            { TilePosition(-3, -4), TilePosition(0, -3), TilePosition(-2,-2) },
                            { TilePosition(0, -3), TilePosition(-2, -4), TilePosition(-2,-2) }
                            };
                        }
                        else {
                            tryOrder ={
                            { TilePosition(4, -4), TilePosition(0, -3), TilePosition(4,-2) },
                            { TilePosition(-1, -3), TilePosition(4, -4), TilePosition(4,-2) }
                            };
                        }
                    }

                    // West
                    else if (baseAngle < 3.925 && baseAngle >= 2.355) {
                        if (mineralsLeft) {

                        }
                        else {
                            tryOrder ={
                            { TilePosition(-3, -4), TilePosition(-2, -2), TilePosition(0,-3) }
                            };
                        }
                    }

                    // South
                    else if (baseAngle < 5.495 && baseAngle >= 3.925) {
                        if (mineralsLeft) {
                            tryOrder ={
                            { TilePosition(-3, 5), TilePosition(1, 3), TilePosition(-1,3) },
                            { TilePosition(-2, 5), TilePosition(1, 3), TilePosition(-1,3) }
                            };
                        }
                        else {
                            tryOrder ={
                            { TilePosition(4, 5), TilePosition(1, 3), TilePosition(3,3) },
                            { TilePosition(3, 5), TilePosition(1, 3), TilePosition(3,3) },
                            { TilePosition(2, 5), TilePosition(1, 3), TilePosition(3,3) },
                            { TilePosition(1, 5), TilePosition(1, 3), TilePosition(3,3) }
                            };
                        }
                    }

                    // East
                    else {
                        if (mineralsLeft) {
                            tryOrder ={
                            { TilePosition(4, -4), TilePosition(4, -2), TilePosition(0,-3) }
                            };
                        }
                    }
                }

                // For each pair, we try to place the best positions first
                for (auto &[medium, small, defense] : tryOrder) {
                    if (Map::isPlaceable(UnitTypes::Zerg_Spawning_Pool, base->Location() + medium) && Map::isPlaceable(UnitTypes::Zerg_Spire, base->Location() + small) && Map::isPlaceable(UnitTypes::Zerg_Sunken_Colony, base->Location() + defense)) {
                        mediumPosition = base->Location() + medium;
                        smallPosition = base->Location() + small;
                        Map::addUsed(smallPosition, UnitTypes::Zerg_Spire);
                        Map::addUsed(mediumPosition, UnitTypes::Zerg_Spawning_Pool);
                        break;
                    }
                }
                tryOrder.clear();
            }
        }

        const auto distCalc = [&](const auto& position) {
            if (natural && partnerBase) {
                const auto closestMain = Stations::getClosestMainStation(base->Location());
                if (closestMain && closestMain->getChokepoint())
                    return Position(closestMain->getChokepoint()->Center()).getDistance(position);
            }

            else if (!base->Minerals().empty()) {
                auto distMineralBest = DBL_MAX;
                for (auto &mineral : base->Minerals()) {
                    auto dist = mineral->Pos().getDistance(position);
                    if (dist < distMineralBest)
                        distMineralBest = dist;
                }
                return distMineralBest;
            }
            return 0.0;
        };

        // Non natural positions want an extra position for a hatchery
        auto cnt = int(!natural);
        for (int i = 0; i < cnt; i++) {
            auto distBest = DBL_MAX;
            auto tileBest = TilePositions::Invalid;
            for (auto x = base->Location().x - 4; x <= base->Location().x + 4; x++) {
                for (auto y = base->Location().y - 3; y <= base->Location().y + 3; y++) {
                    auto tile = TilePosition(x, y);
                    auto center = Position(tile) + Position(64, 48);
                    auto dist = distCalc(center);
                    if (natural && partnerBase && dist < 160.0)
                        continue;

                    if (dist < distBest && Map::isPlaceable(Broodwar->self()->getRace().getResourceDepot(), tile)) {
                        distBest = dist;
                        tileBest = tile;
                    }
                }
            }

            if (tileBest.isValid()) {
                secondaryLocations.insert(tileBest);
                Map::addUsed(tileBest, Broodwar->self()->getRace().getResourceDepot());
            }
        }
    }

    void Station::findNestedDefenses()
    {
        // Unused atm
        vector<TilePosition> placements;
        for (auto &mineral : base->Minerals()) {
            vector<TilePosition>& tiles = testTiles[mineral];

            for (int x = -2; x < 3; x++) {
                for (int y = -2; y < 2; y++) {
                    auto tile = mineral->TopLeft() + TilePosition(x, y);
                    auto pos = Position(tile) + Position(16, 16);
                    if (tile.isValid() && find(placements.begin(), placements.end(), tile) == placements.end() && pos.getDistance(base->Center()) < mineral->Pos().getDistance(base->Center()))
                        placements.push_back(tile);
                }
            }

            for (int x = -1; x < 3; x++) {
                for (int y = -1; y < 2; y++) {
                    auto tile = mineral->TopLeft() + TilePosition(x, y);
                    if (tile.isValid() && find(tiles.begin(), tiles.end(), tile) == tiles.end())
                        tiles.push_back(tile);
                }
            }

            // Sort tiles by closest to base
            sort(tiles.begin(), tiles.end(), [&](auto &l, auto &r) {
                auto pl = Position(l) + Position(16, 16);
                auto pr = Position(r) + Position(16, 16);
                return pl.getDistance(base->Center()) < pr.getDistance(base->Center());
            });

            // Resize to 3
            tiles.resize(3);

            // Narrow down to only walkable
            tiles.erase(remove_if(tiles.begin(), tiles.end(), [&](auto &t) {
                return BWEB::Map::isUsed(t).isMineralField();
            }), tiles.end());
        }

        if (choke) {
            auto chokeCenter = Position(choke->Center());
            sort(placements.begin(), placements.end(), [&](auto &l, auto &r) {
                auto pl = Position(l) + Position(16, 16);
                auto pr = Position(r) + Position(16, 16);
                return pl.getDistance(chokeCenter) < pr.getDistance(chokeCenter);
            });
        }

        // Iterate all placements and verify that each mineral has a guaranteed reasonable path to the hatchery
        for (auto &placement : placements) {
            if (BWEB::Map::isPlaceable(defenseType, placement)) {
                auto buildable = true;
                auto pathable = true;
                Map::addUsed(placement, defenseType);
                for (auto &mineral : base->Minerals()) {
                    vector<TilePosition>& tiles = testTiles[mineral];
                    auto count = count_if(tiles.begin(), tiles.end(), [&](auto &t) {
                        return t.x >= placement.x && t.x < placement.x + 2 && t.y >= placement.y && t.y < placement.y + 2;
                    });
                    if (count >= int(tiles.size())) {
                        buildable = false;
                    }

                    count = 0;
                    for (auto &tile : tiles) {
                        BWEB::Path newPath(base->Center(), Position(tile) + Position(16, 16), UnitTypes::Zerg_Drone, false, false);
                        newPath.generateBFS([&](auto &t) { return newPath.unitWalkable(t) || BWEB::Map::isUsed(t).isResourceDepot(); });
                        if (newPath.isReachable() && newPath.getDistance() < 320.0) {
                            count++;
                            break;
                        }
                    }
                    BWEB::Pathfinding::clearCacheFully();
                    if (count == 0)
                        pathable = false;
                }

                Map::removeUsed(placement, 2, 2);
                if (buildable && pathable) {
                    defenses.insert(placement);
                    Map::addUsed(placement, defenseType);
                }
            }
        }
    }

    void Station::findDefenses()
    {
        vector<TilePosition> basePlacements ={ {-2, -2}, {-2, 1}, {2, -2} };
        auto here = base->Location();

        // Generate defenses
        defenseArrangement = int(round(defenseAngle / 0.785)) % 4;

        // Flip them vertically / horizontally as needed
        if (base->Center().y < defenseCentroid.y) {
            for (auto &placement : basePlacements)
                placement.y = -(placement.y - 1);
        }
        if (base->Center().x < defenseCentroid.x) {
            for (auto &placement : basePlacements)
                placement.x = -(placement.x - 2);
        }

        // If geyser is above, as it should be, add in a defense to left/right
        if (base->Geysers().size() == 1) {
            auto geyser = base->Geysers().front();
            auto tile = geyser->TopLeft();
            if ((tile.y < base->Location().y - 3) || tile.y > base->Location().y + 5) {
                auto placement = tile - base->Location();
                basePlacements.push_back(placement + TilePosition(-2, 1));
                basePlacements.push_back(placement + TilePosition(4, 1));

                // If a placement is where miners would path, shift it over
                for (auto &spot : basePlacements) {
                    if (spot == TilePosition(2, -2))
                        spot = TilePosition(0, -2);
                }
            }
        }

        // Add a defense near each base placement if possible
        for (auto &placement : basePlacements) {
            auto tile = base->Location() + placement;
            if (Map::isPlaceable(defenseType, tile)) {
                defenses.insert(tile);
                Map::addUsed(tile, defenseType);
            }
        }
    }

    const void Station::draw() const
    {
        int color = Broodwar->self()->getColor();
        int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

        // Draw boxes around each feature
        for (auto &tile : defenses) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
            Broodwar->drawTextMap(Position(tile) + Position(4, 52), "%cS", textColor);
        }

        // Draw corresponding choke
        if (choke && base && (main || natural)) {

            if (base->GetArea()->ChokePoints().size() >= 3) {
                const BWEM::ChokePoint * validSecondChoke = nullptr;
                for (auto &otherChoke : base->GetArea()->ChokePoints()) {
                    if (choke == otherChoke)
                        continue;

                    if ((choke->GetAreas().first == otherChoke->GetAreas().first && choke->GetAreas().second == otherChoke->GetAreas().second)
                        || (choke->GetAreas().first == otherChoke->GetAreas().second && choke->GetAreas().second == otherChoke->GetAreas().first))
                        validSecondChoke = choke;
                }

                if (validSecondChoke) {
                    Broodwar->drawLineMap(Position(validSecondChoke->Pos(validSecondChoke->end1)), Position(validSecondChoke->Pos(validSecondChoke->end2)), Colors::Grey);
                    Broodwar->drawLineMap(base->Center(), Position(validSecondChoke->Center()), Colors::Grey);
                    Broodwar->drawLineMap(Position(choke->Center()), Position(validSecondChoke->Center()), Colors::Grey);
                }
            }

            Broodwar->drawLineMap(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2)), Colors::Grey);
            Broodwar->drawLineMap(base->Center(), anglePosition, Colors::Grey);
        }

        Broodwar->drawCircleMap(resourceCentroid, 3, color, true);

        // Label angle
        Broodwar->drawTextMap(Position(base->Location()) + Position(2, 0), "BA: %c%.2f", Text::White, baseAngle);
        Broodwar->drawTextMap(Position(base->Location()) + Position(2, 10), "CA: %c%.2f", Text::White, chokeAngle);
        Broodwar->drawTextMap(Position(base->Location()) + Position(2, 20), "DA: %c%.2f", Text::White, defenseAngle);

        Broodwar->drawBoxMap(Position(base->Location()), Position(base->Location()) + Position(129, 97), color);
        Broodwar->drawTextMap(Position(base->Location()) + Position(4, 84), "%cS", textColor);

        // Draw secondary locations
        for (auto &location : secondaryLocations) {
            Broodwar->drawBoxMap(Position(location), Position(location) + Position(129, 97), color);
            Broodwar->drawTextMap(Position(location) + Position(4, 84), "%cS", textColor);
        }
        Broodwar->drawBoxMap(Position(mediumPosition), Position(mediumPosition) + Position(97, 65), color);
        Broodwar->drawTextMap(Position(mediumPosition) + Position(4, 52), "%cS", textColor);
        Broodwar->drawBoxMap(Position(smallPosition), Position(smallPosition) + Position(65, 65), color);
        Broodwar->drawTextMap(Position(smallPosition) + Position(4, 52), "%cS", textColor);
        Broodwar->drawBoxMap(Position(pocketDefense), Position(pocketDefense) + Position(65, 65), color);
        Broodwar->drawTextMap(Position(pocketDefense) + Position(4, 52), "%cS", textColor);
    }

    void Station::cleanup()
    {
        // Remove used on defenses
        for (auto &tile : defenses) {
            Map::removeUsed(tile, 2, 2);
            Map::addReserve(tile, 2, 2);
        }

        // Remove used on secondary locations
        for (auto &tile : secondaryLocations) {
            Map::removeUsed(tile, 4, 3);
            Map::addReserve(tile, 4, 3);
        }

        // Remove used on base location
        Map::removeUsed(getBase()->Location(), 4, 3);
        Map::addReserve(getBase()->Location(), 4, 3);
        Map::removeUsed(mediumPosition, 3, 2);
        Map::addReserve(mediumPosition, 3, 2);
        Map::removeUsed(smallPosition, 2, 2);
        Map::addReserve(smallPosition, 2, 2);
        Map::removeUsed(pocketDefense, 2, 2);
        Map::addReserve(pocketDefense, 2, 2);
    }
}

namespace BWEB::Stations {

    void findStations()
    {
        // Some chokepoints are purely nonsense (Vermeer has an area with ~3 chokepoints on the same tile), let's ignore those
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &choke : area.ChokePoints()) {
                if (choke->Width() <= 32) {
                    for (auto &areaTwo : Map::mapBWEM.Areas()) {
                        for (auto &chokeTwo : area.ChokePoints()) {
                            if (chokeTwo != choke) {
                                for (auto &geo : chokeTwo->Geometry()) {
                                    if (geo.getDistance(choke->Center()) <= 4)
                                        nonsenseChokes.insert(choke);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Find all main bases
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                if (base.Starting())
                    mainBases.push_back(&base);
            }
        }

        // Find all natural bases
        for (auto &main : mainBases) {

            const BWEM::Base * baseBest = nullptr;
            auto distBest = DBL_MAX;

            for (auto &area : Map::mapBWEM.Areas()) {
                for (auto &base : area.Bases()) {

                    // Must have gas, be accesible and at least 5 mineral patches
                    if (base.Starting()
                        || base.Geysers().empty()
                        || base.GetArea()->AccessibleNeighbours().empty()
                        || base.Minerals().size() < 5)
                        continue;

                    const auto dist = Map::getGroundDistance(base.Center(), main->Center());

                    // Sometimes we have a pocket natural, it contains 1 chokepoint
                    if (base.GetArea()->ChokePoints().size() == 1 && dist < 1600.0) {
                        distBest = 0.0;
                        baseBest = &base;
                    }

                    // Otherwise the closest base with a shared choke is the natural
                    if (dist < distBest && dist < 1600.0) {
                        distBest = dist;
                        baseBest = &base;
                    }
                }
            }

            // Store any natural we found
            if (baseBest)
                natBases.push_back(baseBest);

            // Refuse to not have a natural for each main, try again but less strict
            else {
                for (auto &area : Map::mapBWEM.Areas()) {
                    for (auto &base : area.Bases()) {
                        if (base.Starting())
                            continue;

                        const auto dist = Map::getGroundDistance(main->Center(), base.Center());
                        if (dist < distBest) {
                            distBest = dist;
                            baseBest = &base;
                        }
                    }
                }

                // Store any natural we found
                if (baseBest)
                    natBases.push_back(baseBest);
            }
        }

        // Create main stations
        for (auto &main : mainBases) {
            Station newStation(main, true, false);
            stations.push_back(newStation);
        }

        // Create natural stations
        for (auto &nat : natBases) {
            Station newStation(nat, false, true);
            stations.push_back(newStation);
        }

        // Create remaining stations
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                if (find(natBases.begin(), natBases.end(), &base) != natBases.end() || find(mainBases.begin(), mainBases.end(), &base) != mainBases.end())
                    continue;

                Station newStation(&base, false, false);
                stations.push_back(newStation);
            }
        }

        startMainStation = Stations::getClosestMainStation(Broodwar->self()->getStartLocation());
        if (startMainStation)
            startNatStation = Stations::getClosestNaturalStation(startMainStation->getChokepoint() ? TilePosition(startMainStation->getChokepoint()->Center()) : TilePosition(startMainStation->getBase()->Center()));
    }

    void draw()
    {
        for (auto &station : Stations::getStations())
            station.draw();
    }

    const Station * getStartingMain() { return startMainStation; }

    const Station * getStartingNatural() { return startNatStation; }

    const Station * getClosestStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        const Station * bestStation = nullptr;
        for (auto &station : stations) {
            const auto dist = here.getDistance(station.getBase()->Location());

            if (dist < distBest) {
                distBest = dist;
                bestStation = &station;
            }
        }
        return bestStation;
    }

    const Station * getClosestMainStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        const Station * bestStation = nullptr;
        for (auto &station : stations) {
            if (!station.isMain())
                continue;
            const auto dist = here.getDistance(station.getBase()->Location());

            if (dist < distBest) {
                distBest = dist;
                bestStation = &station;
            }
        }
        return bestStation;
    }

    const Station * getClosestNaturalStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        const Station * bestStation = nullptr;
        for (auto &station : stations) {
            if (!station.isNatural())
                continue;
            const auto dist = here.getDistance(station.getBase()->Location());

            if (dist < distBest) {
                distBest = dist;
                bestStation = &station;
            }
        }
        return bestStation;
    }

    const vector<Station>& getStations() {
        return stations;
    }
}