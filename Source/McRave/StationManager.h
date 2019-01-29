#pragma once
#include <BWAPI.h>

namespace McRave::Stations
{
    std::map <BWAPI::Unit, const BWEB::Stations::Station *>& getMyStations();
    std::map <BWAPI::Unit, const BWEB::Stations::Station *>& getEnemyStations();
    BWAPI::Position getClosestStation(PlayerState, BWAPI::Position);

    void onFrame();
    void onStart();
    void storeStation(BWAPI::Unit);
    void removeStation(BWAPI::Unit);
    bool needDefenses(const BWEB::Stations::Station&);
    bool stationNetworkExists(const BWEB::Stations::Station *, const BWEB::Stations::Station *);
    BWEB::PathFinding::Path* pathStationToStation(const BWEB::Stations::Station *, const BWEB::Stations::Station *);
};
