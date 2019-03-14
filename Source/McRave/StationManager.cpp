#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Stations {

    namespace {
        map <Unit, const BWEB::Stations::Station *> myStations, enemyStations;
        map<const BWEB::Stations::Station *, std::map<const BWEB::Stations::Station *, BWEB::PathFinding::Path>> stationNetwork;

        void updateStations()
        {
            return; // Turn this off to draw station network
            for (auto &s1 : stationNetwork) {
                auto connectedPair = s1.second;
                for (auto &path : connectedPair) {
                    Visuals::displayPath(path.second.getTiles());
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateStations();
        Visuals::endPerfTest("Stations");
    }

    void onStart()
    {
        // Add paths to our station network
        for (auto &s1 : BWEB::Stations::getStations()) {
            for (auto &s2 : BWEB::Stations::getStations()) {
                auto ptrs1 = &s1;
                auto ptrs2 = &s2;
                if (stationNetworkExists(ptrs1, ptrs2) || ptrs1 == ptrs2)
                    continue;

                BWEB::PathFinding::Path newPath;
                newPath.createUnitPath(ptrs1->getResourceCentroid(), ptrs2->getResourceCentroid());
                stationNetwork[ptrs1][ptrs2] = newPath;
            }
        }
    }

    Position getClosestStation(PlayerState pState, Position here)
    {
        auto &list = pState == PlayerState::Self ? myStations : enemyStations;
        double distBest = DBL_MAX;
        Position best;
        for (auto &station : list) {
            auto s = *station.second;
            double dist = here.getDistance(s.getBWEMBase()->Center());
            if (dist < distBest)
                best = s.getBWEMBase()->Center(), distBest = dist;
        }
        return best;
    }

    void storeStation(Unit unit)
    {
        auto newStation = BWEB::Stations::getClosestStation(unit->getTilePosition());
        if (!newStation
            || !unit->getType().isResourceDepot()
            || unit->getTilePosition() != newStation->getBWEMBase()->Location())
            return;

        // 1) Change the resource states and store station
        unit->getPlayer() == Broodwar->self() ? myStations.emplace(unit, newStation) : enemyStations.emplace(unit, newStation);
        ResourceState state = unit->isCompleted() ? ResourceState::Mineable : ResourceState::Assignable;
        if (unit->getPlayer() == Broodwar->self()) {
            for (auto &mineral : newStation->getBWEMBase()->Minerals()) {

                if (Broodwar->getFrameCount() == 0)
                    Resources::storeResource(mineral->Unit());

                const auto &resource = Resources::getResource(mineral->Unit());

                if (resource) {
                    resource->setResourceState(state);

                    // HACK: Added this to fix some weird gas steal stuff
                    if (state == ResourceState::Mineable)
                        resource->setStation(newStation);
                }
            }
            for (auto &gas : newStation->getBWEMBase()->Geysers()) {

                if (Broodwar->getFrameCount() == 0)
                    Resources::storeResource(gas->Unit());

                const auto &resource = Resources::getResource(gas->Unit());

                if (resource) {
                    resource->setResourceState(state);

                    // HACK: Added this to fix some weird gas steal stuff
                    if (state == ResourceState::Mineable)
                        resource->setStation(newStation);
                }
            }
        }

        // 2) Add to territory
        if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition())) {
            if (unit->getPlayer() == Broodwar->self())
                Terrain::getAllyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition()));
            else
                Terrain::getEnemyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition()));
        }
    }

    void removeStation(Unit unit)
    {
        auto newStation = BWEB::Stations::getClosestStation(unit->getTilePosition());
        if (!newStation)
            return;

        auto &list = unit->getPlayer() == Broodwar->self() ? myStations : enemyStations;
        if (list.find(unit) == list.end())
            return;

        auto &station = list[unit];
        auto state = ResourceState::None;

        // 1) Change resource state of resources connected to not mineable
        for (auto &mineral : station->getBWEMBase()->Minerals()) {
            const auto &resource = Resources::getResource(mineral->Unit());
            if (resource)
                resource->setResourceState(state);            
        }
        for (auto &gas : station->getBWEMBase()->Geysers()) {
            const auto &resource = Resources::getResource(gas->Unit());
            if (resource)
                resource->setResourceState(state);
        }
        list.erase(unit);

        // 2) Remove any territory it was in
        if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition())) {
            if (unit->getPlayer() == Broodwar->self())
                Terrain::getAllyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
            else
                Terrain::getEnemyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
        }
    }

    bool needDefenses(const BWEB::Stations::Station& station)
    {
        auto center = TilePosition(station.getBWEMBase()->Center());
        auto defenseCount = station.getDefenseCount();
        auto main = station.getBWEMBase()->Location() == BWEB::Map::getMainTile();
        auto nat = station.getBWEMBase()->Location() == BWEB::Map::getNaturalTile();

        if (!Pylons::hasPower(center, UnitTypes::Protoss_Photon_Cannon))
            return false;

        if ((nat || main) && !Terrain::isIslandMap() && defenseCount <= 0)
            return true;
        else if (defenseCount <= 0)
            return true;
        else if ((Players::getPlayers().size() > 3 || Players::vZ()) && !main && !nat && defenseCount < int(station.getDefenseLocations().size()))
            return true;
        else if (station.getDefenseCount() < 2 && (Strategy::getEnemyBuild() == "2HatchMuta" || Strategy::getEnemyBuild() == "3HatchMuta"))
            return true;
        return false;
    }

    bool stationNetworkExists(const BWEB::Stations::Station * start, const BWEB::Stations::Station * finish)
    {
        for (auto &s : stationNetwork) {
            auto s1 = s.first;

            // For each connected station, check if it exists
            auto connectedStations = s.second;
            for (auto &pair : connectedStations) {
                auto s2 = pair.first;
                if ((s1 == start && s2 == finish) || (s1 == finish && s2 == start))
                    return true;
            }
        }
        return false;
    }

    BWEB::PathFinding::Path* pathStationToStation(const BWEB::Stations::Station * start, const BWEB::Stations::Station * finish)
    {
        for (auto &s : stationNetwork) {
            auto s1 = s.first;

            // For each connected station, check if it exists
            auto connectedStations = s.second;
            for (auto &pair : connectedStations) {
                auto s2 = pair.first;
                auto &path = pair.second;
                if ((s1 == start && s2 == finish) || (s1 == finish && s2 == start))
                    return &path;
            }
        }
        return nullptr;
    }

    map <BWAPI::Unit, const BWEB::Stations::Station *>& getMyStations() { return myStations; };
    map <BWAPI::Unit, const BWEB::Stations::Station *>& getEnemyStations() { return enemyStations; }
}