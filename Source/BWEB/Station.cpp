#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {

    namespace {
        Station * startMainStation = nullptr;
        Station * startNatStation = nullptr;
        vector<Station> stations;
        vector<const BWEM::Base *> mainBases;
        vector<const BWEM::Base *> natBases;
        vector<const BWEM::ChokePoint*> mainChokes;
        vector<const BWEM::ChokePoint*> natChokes;
        map<const BWEM::Mineral *, vector<TilePosition>> testTiles;
        vector<BWEB::Path> testPaths;
        UnitType defenseType;
    }

    void Station::addResourceReserves()
    {
        const auto addReserve = [&](Unit resource, TilePosition start) {
            vector<TilePosition> directions{ {1,0}, {-1,0}, {0, 1}, {0,-1} };
            auto diff = (base->Center() - resourceCentroid);
            auto end = resource->getType().isMineralField() ? base->Center() + (diff / 4) : base->Center() - (diff / 4);

            // Get the starting tile for a geyser
            if (resource->getType().isRefinery()) {
                auto distClosest = resource->getType().isMineralField() ? 0.0 : DBL_MAX;
                for (int x = resource->getTilePosition().x; x < resource->getTilePosition().x + resource->getType().tileWidth(); x++) {
                    for (int y = resource->getTilePosition().y; y < resource->getTilePosition().y + resource->getType().tileHeight(); y++) {
                        auto tile = TilePosition(x, y);
                        auto center = Position(tile) + Position(16, 16);
                        auto dist = center.getDistance(resourceCentroid);
                        if (resource->getType().isMineralField() ? dist > distClosest : dist < distClosest) {
                            start = tile;
                            distClosest = dist;
                        }
                    }
                }
            }

            TilePosition next = start;
            while (next != TilePosition(end)) {
                auto distBest = DBL_MAX;
                start = next;
                for (auto &t : directions) {
                    auto tile = start + t;
                    auto pos = Position(tile) + Position(16, 16);

                    if (!tile.isValid())
                        continue;

                    auto dist = pos.getDistance(end);
                    if (dist <= distBest) {
                        next = tile;
                        distBest = dist;
                    }
                }

                if (next.isValid()) {
                    Map::addReserve(next, 1, 1);
                }
            }
        };

        // Add reserved tiles
        for (auto &m : base->Minerals()) {
            Map::addReserve(m->TopLeft(), 2, 1);
            addReserve(m->Unit(), m->Unit()->getTilePosition());
            addReserve(m->Unit(), m->Unit()->getTilePosition() + TilePosition(1, 0));
        }
        for (auto &g : base->Geysers()) {
            Map::addReserve(g->TopLeft(), 4, 2);
            addReserve(g->Unit(), g->Unit()->getTilePosition());
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
        if (!Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).empty()) {
            partnerChoke = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).front();

            // Partner only has one chokepoint means we have a shared choke with this path
            if (partnerBase->GetArea()->ChokePoints().size() == 1)
                partnerChoke = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).back();
        }

        if (main && partnerChoke)
            choke = partnerChoke;

        else {

            // Only one chokepoint in this area
            if (base->GetArea()->ChokePoints().size() == 1) {
                choke = base->GetArea()->ChokePoints().front();
                return;
            }

            set<BWEM::ChokePoint const *> nonChokes;
            for (auto &choke : Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()))
                nonChokes.insert(choke);

            auto distBest = DBL_MAX;
            const BWEM::Area* second = nullptr;

            // Iterate each neighboring area to get closest to this natural area
            for (auto &area : base->GetArea()->AccessibleNeighbours()) {
                auto center = area->Top();
                const auto dist = Position(center).getDistance(Map::mapBWEM.Center());

                bool wrongArea = false;
                for (auto &choke : area->ChokePoints()) {
                    if ((!choke->Blocked() && choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) <= 2) || nonChokes.find(choke) != nonChokes.end()) {
                        wrongArea = true;
                    }
                }
                if (wrongArea)
                    continue;

                if (center.isValid() && dist < distBest) {
                    second = area;
                    distBest = dist;
                }
            }

            distBest = DBL_MAX;
            for (auto &c : base->GetArea()->ChokePoints()) {
                if (c->Blocked()
                    || find(mainChokes.begin(), mainChokes.end(), c) != mainChokes.end()
                    || c->Geometry().size() <= 3
                    || (c->GetAreas().first != second && c->GetAreas().second != second))
                    continue;

                const auto dist = Position(c->Center()).getDistance(Position(partnerBase->Center()));
                if (dist < distBest) {
                    choke = c;
                    distBest = dist;
                }
            }
        }

        if (choke && !main)
            defenseCentroid = Position(choke->Center());
        if (choke)
            main ? mainChokes.push_back(choke) : natChokes.push_back(choke);

        // Create angles that dictate a lot about how the station is formed
        if (choke) {
            anglePosition = Position(choke->Center()) + Position(4, 4);
            auto dist = min(640.0, getBase()->Center().getDistance(anglePosition));

            baseAngle = Map::getAngle(make_pair(getBase()->Center(), anglePosition));
            chokeAngle = Map::getAngle(make_pair(Position(choke->Pos(choke->end1)), Position(choke->Pos(choke->end2))));

            baseAngle = (round(baseAngle / 0.785)) * 0.785;
            chokeAngle = (round(chokeAngle / 0.785)) * 0.785;

            auto baseMod = fmod(baseAngle + 1.57, 3.14);
            auto chokeMod = fmod(chokeAngle, 3.14);

            defenseAngle = fmod((baseMod + chokeMod) / 2.0, 3.14);
        }
    }

    void Station::findSecondaryLocations()
    {
        if (Broodwar->self()->getRace() != Races::Zerg || !main)
            return;

        vector<tuple<TilePosition, TilePosition, TilePosition>> tryOrder;

        // Determine some standard positions
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
                        { TilePosition(0, -3), TilePosition(4, -4), TilePosition(4,-2) }
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
                    pocketDefense = base->Location() + defense;
                    Map::addUsed(smallPosition, UnitTypes::Zerg_Spire);
                    Map::addUsed(mediumPosition, UnitTypes::Zerg_Spawning_Pool);
                    Map::addUsed(mediumPosition, UnitTypes::Zerg_Sunken_Colony);
                    break;
                }
            }
            tryOrder.clear();
        }

        auto cnt = 0;
        if (main)
            cnt = 1;
        if (!main && !natural)
            cnt = 2;

        for (int i = 0; i < cnt; i++) {
            auto distBest = DBL_MAX;
            auto tileBest = TilePositions::Invalid;
            for (auto x = base->Location().x - 4; x <= base->Location().x + 4; x++) {
                for (auto y = base->Location().y - 3; y <= base->Location().y + 3; y++) {
                    auto tile = TilePosition(x, y);
                    auto center = Position(tile) + Position(64, 48);
                    auto dist = center.getDistance(resourceCentroid);
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
        vector<TilePosition> basePlacements;
        vector<TilePosition> geyserPlacements ={ {-2, -2}, {-2, 0}, {-2, 2}, {0, -2}, {0, 2}, {2, -2}, {2, 2}, {4, -2}, {4, 0}, {4, 2} };
        auto here = base->Location();

        // Get angle of chokepoint
        if (choke && base && (main || natural)) {
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
        }
        else {
            defenseCentroid = BWEB::Map::mapBWEM.Center();
            defenseAngle = fmod(Map::getAngle(make_pair(Position(getBase()->Center()), defenseCentroid)), 3.14) + 1.57;
        }

        // Generate defenses
        defenseArrangement = int(round(defenseAngle / 0.785)) % 4;
        if (main)
            basePlacements ={ {-2, -2}, {-2, 1}, {1, -2} };
        else if (defenseArrangement == 0)
            basePlacements ={ {-2, 2}, {-2, 0}, {-2, -2}, {0, 3}, {0, -2}, {2, -2}, {4, -2}, {4, 0}, {4, 2} };   // 0/8
        else if (defenseArrangement == 1 || defenseArrangement == 3)
            basePlacements ={ {-2, 2}, {-2, 0}, {0, 3}, {0, -2}, {2, -2}, {4, -2}, {4, 0} };                     // pi/4
        else if (defenseArrangement == 2)
            basePlacements ={ {-2, 2}, {-2, 0}, {-2, -2}, {0, 3}, {0, -2}, {2, 3}, {2, -2}, {4, 3}, {4, -2} };   // pi/2        

        // Flip them vertically / horizontally as needed
        if (base->Center().y < defenseCentroid.y) {
            for (auto &placement : basePlacements)
                placement.y = -(placement.y - 1);
        }
        if (base->Center().x < defenseCentroid.x) {
            for (auto &placement : basePlacements)
                placement.x = -(placement.x - 2);
        }

        // Add scanner addon for Terran
        if (Broodwar->self()->getRace() == Races::Terran) {
            auto scannerTile = here + TilePosition(4, 1);
            defenses.insert(scannerTile);
            Map::addUsed(scannerTile, defenseType);
        }

        // Add a defense near each base placement if possible
        for (auto &placement : basePlacements) {
            auto tile = base->Location() + placement;
            if (Map::isPlaceable(defenseType, tile)) {
                defenses.insert(tile);
                Map::addUsed(tile, defenseType);
            }
        }

        // Try to fit more defenses with secondary positions
        for (auto secondary : secondaryLocations) {
            for (auto placement : basePlacements) {
                auto tile = secondary + placement;
                if (Map::isPlaceable(defenseType, tile)) {
                    defenses.insert(tile);
                    Map::addUsed(tile, defenseType);
                }
            }
        }
    }

    void Station::draw()
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

        // Label angle
        Broodwar->drawTextMap(base->Center() - Position(0, 16), "BA: %c%.2f", Text::White, baseAngle);
        Broodwar->drawTextMap(base->Center(), "CA: %c%.2f", Text::White, chokeAngle);
        Broodwar->drawTextMap(base->Center() + Position(0, 16), "DA: %c%.2f", Text::White, defenseAngle);

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
    }
}

namespace BWEB::Stations {

    void findStations()
    {
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
        startNatStation = Stations::getClosestNaturalStation(startMainStation->getChokepoint() ? TilePosition(startMainStation->getChokepoint()->Center()) : TilePosition(startMainStation->getBase()->Center()));
    }

    void draw()
    {
        for (auto &station : Stations::getStations())
            station.draw();
    }

    Station * getStartingMain() { return startMainStation; }

    Station * getStartingNatural() { return startNatStation; }

    Station * getClosestStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Station* bestStation = nullptr;
        for (auto &station : stations) {
            const auto dist = here.getDistance(station.getBase()->Location());

            if (dist < distBest) {
                distBest = dist;
                bestStation = &station;
            }
        }
        return bestStation;
    }

    Station * getClosestMainStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Station * bestStation = nullptr;
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

    Station * getClosestNaturalStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Station* bestStation = nullptr;
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

    vector<Station>& getStations() {
        return stations;
    }
}