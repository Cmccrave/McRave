#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {

    namespace {
        vector<Station> stations;
        vector<const BWEM::Base *> mainBases;
        vector<const BWEM::Base *> natBases;
    }

    void Station::addResourceReserve(Position resourceCenter, Position startCenter, Position stationCenter)
    {
        TilePosition start(startCenter);
        TilePosition test = start;

        while (test != TilePosition(stationCenter)) {
            auto distBest = DBL_MAX;
            start = test;
            for (int x = start.x - 1; x <= start.x + 1; x++) {
                for (int y = start.y - 1; y <= start.y + 1; y++) {
                    TilePosition t(x, y);
                    Position p = Position(t) + Position(16, 16);

                    if (!t.isValid())
                        continue;

                    auto dist = Map::isReserved(t) ? p.getDistance(stationCenter) + 16 : p.getDistance(stationCenter);
                    if (dist <= distBest) {
                        test = t;
                        distBest = dist;
                    }
                }
            }

            if (test.isValid())
                Map::addReserve(test, 1, 1);
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

        // Add reserved tiles
        for (auto &m : base->Minerals()) {
            Map::addReserve(m->TopLeft(), 2, 1);
            addResourceReserve(resourceCentroid, m->Pos(), base->Center());
        }
        for (auto &g : base->Geysers()) {
            Map::addReserve(g->TopLeft(), 4, 2);
            addResourceReserve(resourceCentroid, g->Pos(), base->Center());
        }
        Map::addReserve(base->Location(), 4, 3);
    }

    void Station::findChoke()
    {
        // Only find a Chokepoint for mains or naturals
        if (!main && !natural)
            return;

        // Get closest partner base
        auto distBest = DBL_MAX;
        for (auto &potentialPartner : (main ? natBases : mainBases)) {
            auto dist = potentialPartner->Center().getDistance(base->Center());
            if (dist < distBest) {
                partnerBase = potentialPartner;
                distBest = dist;
            }
        }
        if (!partnerBase)
            return;


        if (main && !Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).empty()) {
            chokepoint = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).front();

            // Partner only has one chokepoint means we have a shared choke with this path
            if (partnerBase->GetArea()->ChokePoints().size() == 1)
                chokepoint = Map::mapBWEM.GetPath(partnerBase->Center(), base->Center()).back();
        }

        else {

            // Only one chokepoint in this area
            if (base->GetArea()->ChokePoints().size() == 1) {
                chokepoint = base->GetArea()->ChokePoints().front();
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
            for (auto &choke : base->GetArea()->ChokePoints()) {
                if (choke->Center() == BWEB::Map::getMainChoke()->Center()
                    || choke->Blocked()
                    || choke->Geometry().size() <= 3
                    || (choke->GetAreas().first != second && choke->GetAreas().second != second))
                    continue;

                const auto dist = Position(choke->Center()).getDistance(Position(partnerBase->Center()));
                if (dist < distBest) {
                    chokepoint = choke;
                    distBest = dist;
                }
            }
        }
    }

    void Station::findDefenses()
    {
        set<TilePosition> basePlacements;
        set<TilePosition> geyserPlacements;
        auto here = base->Location();
        auto placeRight = base->Center().x > defenseCentroid.x;
        auto placeBelow = base->Center().y > defenseCentroid.y;

        const auto &bottomRight = [&]() {
            if (!main)
                basePlacements ={ {-2, -2}, {2, -2}, {-2, 1},                                         // Close
                                  {4, -3}, {4, -1}, {4, 1}, {4, 3}, {2, 3}, {0, 3}, {-2, 3} };        // Far
            else
                basePlacements ={ {-2, -2}, {1, -2}, {-2, 1} };                                       // Close

            if (!natural) {
                geyserPlacements ={ {0, -2}, {2, -2}, // Above
                                    {-2, 0}, {4, 0},  // Sides
                                    //{2, 2}            // Below - was stopping blocks from forming properly
                };
            }
        };

        const auto &bottomLeft = [&]() {
            if (!main)
                basePlacements ={ {4, -2}, {0, -2}, {4, 1},                                         // Close
                                  {-2, -3}, {-2, -1}, {-2, 1}, {-2, 3}, {0, 3}, {2, 3}, {4, 3} };   // Far
            else
                basePlacements ={ {4, -2}, {1, -2}, {4, 1} };                                       // Close

            if (!natural) {
                geyserPlacements ={ {0, -2}, {2, -2}, // Above
                                    {-2, 0}, {4, 0},  // Sides
                                    //{0, 2}            // Below - was stopping blocks from forming properly
                };
            }
        };

        const auto &topRight = [&]() {
            if (!main)
                basePlacements ={ {-2, 3}, {-2, 0}, {2, 3},                                         // Close
                                  {-2, -2}, {0, -2}, {2, -2}, {4, -2}, {4, 0}, {4, 2}, {4, 4} };    // Far
            else
                basePlacements ={ {-2, 3}, {-2, 0}, {1, 3} };                                       // Close

            if (!natural) {
                geyserPlacements ={ //{2, -2},          // Above - was stopping blocks from forming properly
                                    {-2, 0}, {4, 0},  // Sides
                                    {0, 2}, {2, 2}    // Below
                };
            }
        };

        const auto &topLeft = [&]() {
            if (!main)
                basePlacements ={ {4, 0}, {0, 3}, {4, 3},                                             // Close
                                  {-2, 4}, {-2, 2}, {-2, 0}, {-2, -2}, {0, -2}, {2, -2}, {4, -2} };   // Far
            else
                basePlacements ={ {4, 0}, {1, 3}, {4, 3} };                                           // Close

            if (!natural) {
                geyserPlacements ={ //{0, -2},          // Above - was stopping blocks from forming properly
                                    {-2, 0}, {4, 0},  // Sides
                                    {0, 2}, {2, 2}    // Below
                };
            }
        };

        // Insert defenses
        placeBelow ? (placeRight ? bottomRight() : bottomLeft()) : (placeRight ? topRight() : topLeft());
        auto defenseType = UnitTypes::None;
        if (Broodwar->self()->getRace() == Races::Protoss)
            defenseType = UnitTypes::Protoss_Photon_Cannon;
        if (Broodwar->self()->getRace() == Races::Terran)
            defenseType = UnitTypes::Terran_Missile_Turret;
        if (Broodwar->self()->getRace() == Races::Zerg)
            defenseType = UnitTypes::Zerg_Creep_Colony;

        // Add scanner addon for Terran
        if (Broodwar->self()->getRace() == Races::Terran) {
            auto scannerTile = here + TilePosition(4, 1);
            defenses.insert(scannerTile);
            Map::addReserve(scannerTile, 2, 2);
            Map::addUsed(scannerTile, defenseType);
        }

        // Add a defense near each base placement if possible
        for (auto &placement : basePlacements) {
            auto tile = base->Location() + placement;
            if (Map::isPlaceable(defenseType, tile)) {
                defenses.insert(tile);
                Map::addReserve(tile, 2, 2);
                Map::addUsed(tile, defenseType);
            }
        }

        // Add a defense near the geysers of this base if possible
        for (auto &geyser : base->Geysers()) {
            for (auto &placement : geyserPlacements) {
                auto tile = geyser->TopLeft() + placement;
                if (Map::isPlaceable(defenseType, tile)) {
                    defenses.insert(tile);
                    Map::addReserve(tile, 2, 2);
                    Map::addUsed(tile, defenseType);
                }
            }
        }

        // Remove used
        for (auto &tile : defenses)
            Map::removeUsed(tile, 2, 2);
    }

    int Station::getGroundDefenseCount()
    {
        int count = 0;
        for (auto &defense : defenses) {
            auto type = Map::isUsed(defense);
            if (type == UnitTypes::Protoss_Photon_Cannon
                || type == UnitTypes::Zerg_Sunken_Colony
                || type == UnitTypes::Terran_Bunker)
                count++;
        }
        return count;
    }

    int Station::getAirDefenseCount()
    {
        int count = 0;
        for (auto &defense : defenses) {
            auto type = Map::isUsed(defense);
            if (type == UnitTypes::Protoss_Photon_Cannon
                || type == UnitTypes::Zerg_Spore_Colony
                || type == UnitTypes::Terran_Missile_Turret)
                count++;
        }
        return count;
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
        if (chokepoint && base && (main || natural)) {
            Broodwar->drawLineMap(Position(chokepoint->Pos(chokepoint->end1)), Position(chokepoint->Pos(chokepoint->end2)), Colors::Grey);
            Broodwar->drawLineMap(base->Center(), Position(chokepoint->Center()), Colors::Grey);
        }

        Broodwar->drawBoxMap(Position(base->Location()), Position(base->Location()) + Position(129, 97), color);
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

        // Create Stations
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {

                auto isMain = find(mainBases.begin(), mainBases.end(), &base) != mainBases.end();
                auto isNatural = find(natBases.begin(), natBases.end(), &base) != natBases.end();

                // Add to our station lists
                Station newStation(&base, isMain, isNatural);
                stations.push_back(newStation);
            }
        }
    }

    void draw()
    {
        for (auto &station : Stations::getStations())
            station.draw();
    }

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