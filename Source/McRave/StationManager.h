#pragma once
#include <BWAPI.h>

namespace McRave::Stations
{
    std::map <BWAPI::Unit, const BWEB::Station *>& getMyStations();
    std::map <BWAPI::Unit, const BWEB::Station *>& getEnemyStations();
    BWAPI::Position getClosestStation(PlayerState, BWAPI::Position);

    void onFrame();
    void onStart();
    void storeStation(BWAPI::Unit);
    void removeStation(BWAPI::Unit);
    bool needDefenses(const BWEB::Station&);
    bool stationNetworkExists(const BWEB::Station *, const BWEB::Station *);
    BWEB::PathFinding::Path* pathStationToStation(const BWEB::Station *, const BWEB::Station *);
};
