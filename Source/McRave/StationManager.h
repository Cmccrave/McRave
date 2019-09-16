#pragma once
#include <BWAPI.h>

namespace McRave::Stations
{
    std::map <BWAPI::Unit, BWEB::Station *>& getMyStations();
    std::map <BWAPI::Unit, BWEB::Station *>& getEnemyStations();
    BWAPI::Position getClosestStation(PlayerState, BWAPI::Position);

    void onFrame();
    void onStart();
    void storeStation(BWAPI::Unit);
    void removeStation(BWAPI::Unit);
    int needDefenses(BWEB::Station&);
    bool needPower(BWEB::Station&);

    PlayerState ownedBy(BWEB::Station *);
};
