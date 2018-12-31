#pragma once
#include <BWAPI.h>
#include "Singleton.h"

namespace McRave
{
    class StationManager
    {
        std::map <BWAPI::Unit, const BWEB::Stations::Station *> myStations, enemyStations;
        void updateStations();
        std::map<const BWEB::Stations::Station *, std::map<const BWEB::Stations::Station *, BWEB::PathFinding::Path>> stationNetwork;
    public:
        std::map <BWAPI::Unit, const BWEB::Stations::Station *>& getMyStations() { return myStations; };
        std::map <BWAPI::Unit, const BWEB::Stations::Station *>& getEnemyStations() { return enemyStations; }
        BWAPI::Position getClosestEnemyStation(BWAPI::Position);
        void onFrame();
        void onStart();
        void storeStation(BWAPI::Unit);
        void removeStation(BWAPI::Unit);
        bool needDefenses(const BWEB::Stations::Station&);
        bool stationNetworkExists(const BWEB::Stations::Station *, const BWEB::Stations::Station *);
        BWEB::PathFinding::Path* pathStationToStation(const BWEB::Stations::Station *, const BWEB::Stations::Station *);
    };
}

typedef Singleton<McRave::StationManager> StationSingleton;
