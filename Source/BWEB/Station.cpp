#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {

    namespace {
        vector<Station> stations;
    }

    int Station::getGroundDefenseCount() {
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

    int Station::getAirDefenseCount() {
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
}

namespace BWEB::Stations {

    set<TilePosition> stationDefenses(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
    {
        set<TilePosition> defenses;

        const auto &offset = [&](int x, int y) {
            return here + TilePosition(x, y);
        };

        const auto &topLeft = [&]() {
            defenses.insert({
                offset(-2, -2),
                offset(0, -2),
                offset(2, -2),
                offset(-2, 1) });
            if (here != Map::getMainTile() && here != Map::getNaturalTile())
                defenses.insert({
                offset(4, -1),
                offset(4, 1),
                offset(4, 3),
                offset(0,3),
                offset(2, 3) });
        };

        const auto &topRight = [&]() {
            if (Broodwar->self()->getRace() == Races::Terran)
                defenses.insert({
                offset(4, -2),
                offset(0, -2),
                offset(2, -2) });
            else {
                defenses.insert({
                offset(4, -2),
                offset(0, -2),
                offset(2, -2),
                offset(4, 1) });

                if (here != Map::getMainTile() && here != Map::getNaturalTile())
                    defenses.insert({
                    offset(-2, -1),
                    offset(-2, 1),
                    offset(-2, 3),
                    offset(0, 3),
                    offset(2, 3) });
            }
        };

        const auto &bottomLeft = [&]() {
            defenses.insert({
                offset(-2, 3),
                offset(0, 3),
                offset(-2, 0),
                offset(2, 3) });

            if (here != Map::getMainTile() && here != Map::getNaturalTile())
                defenses.insert({
                offset(0, -2),
                offset(2, -2),
                offset(4, -2),
                offset(4, 0),
                offset(4, 2) });
        };

        const auto &bottomRight = [&]() {
            if (Broodwar->self()->getRace() == Races::Terran)
                defenses.insert({
                offset(0, 3),
                offset(2, 3),
                offset(4, 3) });
            else {
                defenses.insert({
                offset(4, 0),
                offset(0, 3),
                offset(2, 3),
                offset(4, 3) });

                if (here != Map::getMainTile() && here != Map::getNaturalTile())
                    defenses.insert({
                    offset(-2, 2),
                    offset(-2, 0),
                    offset(-2,-2),
                    offset(0, -2),
                    offset(2, -2) });
            }
        };

        if (mirrorVertical) {
            if (mirrorHorizontal)
                bottomRight();
            else
                bottomLeft();
        }
        else {
            if (mirrorHorizontal)
                topRight();
            else
                topLeft();
        }

        // Check if it's buildable (some maps don't allow for these tiles, like Destination)
        set<TilePosition>::iterator itr = defenses.begin();
        while (itr != defenses.end()) {
            auto tile = *itr;
            if (!Map::isPlaceable(UnitTypes::Protoss_Photon_Cannon, tile))
                itr = defenses.erase(itr);
            else
                ++itr;
        }

        // Temporary fix for CC Addons
        if (Broodwar->self()->getRace() == Races::Terran)
            defenses.insert(here + TilePosition(4, 1));

        // Add overlap
        for (auto &tile : defenses)
            Map::addReserve(tile, 2, 2);

        return defenses;
    }

    void findStations()
    {
        const auto addResourceOverlap = [&](Position resourceCenter, Position startCenter, Position stationCenter) {
            TilePosition start(startCenter);

            for (int x = start.x - 2; x < start.x + 2; x++) {
                for (int y = start.y - 2; y < start.y + 2; y++) {
                    TilePosition t(x, y);
                    Position p = Position(t) + Position(16, 16);

                    if (!t.isValid())
                        continue;
                    if (p.getDistance(stationCenter) <= resourceCenter.getDistance(stationCenter))
                        Map::addReserve(t, 1, 1);
                }
            }
        };

        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                auto h = false, v = false;
                Position genCenter, sCenter;
                auto cnt = 0;

                for (auto &mineral : base.Minerals()) {
                    genCenter += mineral->Pos();
                    cnt++;
                }

                if (cnt > 0)
                    sCenter = genCenter / cnt;

                for (auto &gas : base.Geysers()) {
                    sCenter = (sCenter + gas->Pos()) / 2;
                    genCenter += gas->Pos();
                    cnt++;
                }

                if (cnt > 0)
                    genCenter = genCenter / cnt;

                h = base.Center().x < sCenter.x;
                v = base.Center().y < sCenter.y;

                for (auto &m : base.Minerals()) {
                    Map::addReserve(m->TopLeft(), 2, 1);
                    addResourceOverlap(genCenter, (m->Pos() + base.Center()) / 2, base.Center());
                }


                for (auto &g : base.Geysers()) {
                    Map::addReserve(g->TopLeft(), 4, 2);
                    addResourceOverlap(genCenter, (g->Pos() + base.Center()) / 2, base.Center());
                }


                Station newStation(genCenter, stationDefenses(base.Location(), h, v), &base);
                stations.push_back(newStation);

                if (Broodwar->self()->getRace() == Races::Zerg)
                    Map::addReserve(base.Location(), 4, 4);
                else
                    Map::addReserve(base.Location(), 4, 3);
            }
        }
    }

    void draw()
    {
        for (auto &station : Stations::getStations()) {
            for (auto &tile : station.getDefenseLocations())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
            Broodwar->drawBoxMap(Position(station.getBWEMBase()->Location()), Position(station.getBWEMBase()->Location()) + Position(129, 97), Broodwar->self()->getColor());
        }
    }

    Station * getClosestStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Station* bestStation = nullptr;
        for (auto &station : stations) {
            const auto dist = here.getDistance(station.getBWEMBase()->Location());

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