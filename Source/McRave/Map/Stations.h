#pragma once
#include <BWAPI.h>

namespace McRave::Stations
{
    std::multimap<double, const BWEB::Station * const>& getStationsBySaturation();
    std::multimap<double, const BWEB::Station * const>& getStationsByProduction();

    void onFrame();
    void onStart();
    void storeStation(BWAPI::Unit);
    void removeStation(BWAPI::Unit);
    int getColonyCount(const BWEB::Station * const);
    int needGroundDefenses(const BWEB::Station * const);
    int needAirDefenses(const BWEB::Station * const);
    int getGroundDefenseCount(const BWEB::Station * const);
    int getAirDefenseCount(const BWEB::Station * const);
    bool needPower(const BWEB::Station * const);
    bool isIsland(const BWEB::Station * const);
    bool isBaseExplored(const BWEB::Station * const);
    bool isGeyserExplored(const BWEB::Station * const);
    bool isCompleted(const BWEB::Station * const);
    bool isBlocked(const BWEB::Station * const);
    int lastVisible(const BWEB::Station * const);
    double getSaturationRatio(const BWEB::Station * const);
    double getStationSaturation(const BWEB::Station * const);
    BWAPI::Position getDefendPosition(const BWEB::Station * const);
    const BWEB::Station * const getClosestRetreatStation(UnitInfo&);
    int getGasingStationsCount();
    int getMiningStationsCount();
    int getMineralsRemaining(const BWEB::Station * const);
    int getGasRemaining(const BWEB::Station * const);
    int getMineralsInitial(const BWEB::Station * const);
    int getGasInitial(const BWEB::Station * const);

    PlayerState ownedBy(const BWEB::Station * const);

    std::vector<const BWEB::Station *> getStations(PlayerState);
    template<typename F>
    std::vector<const BWEB::Station *> getStations(PlayerState, F &&pred);

    BWEB::Path getPathBetween(const BWEB::Station *, const BWEB::Station *);
    std::map<const BWEB::Station *, BWEB::Path> getStationNetwork(const BWEB::Station *);

    template<typename F>
    const BWEB::Station * getClosestStationAir(BWAPI::Position here, PlayerState player, F &&pred) {
        auto &list = getStations(player);
        auto distBest = DBL_MAX;
        const BWEB::Station * closestStation = nullptr;
        for (auto &station : list) {
            double dist = here.getDistance(station->getBase()->Center());
            if (dist < distBest && pred(station)) {
                closestStation = station;
                distBest = dist;
            }
        }
        return closestStation;
    }

    inline const BWEB::Station * const getClosestStationAir(BWAPI::Position here, PlayerState player) {
        return getClosestStationAir(here, player, [](auto) {
            return true;
        });
    }

    template<typename F>
    const BWEB::Station * getClosestStationGround(BWAPI::Position here, PlayerState player, F &&pred) {
        auto &list = getStations(player);
        auto distBest = DBL_MAX;
        const BWEB::Station * closestStation = nullptr;
        for (auto &station : list) {
            double dist = BWEB::Map::getGroundDistance(here, station->getBase()->Center());
            if (dist < distBest && pred(station)) {
                closestStation = station;
                distBest = dist;
            }
        }
        return closestStation;
    }

    inline const BWEB::Station * const getClosestStationGround(BWAPI::Position here, PlayerState player) {
        return getClosestStationGround(here, player, [](auto) {
            return true;
        });
    }    
};
