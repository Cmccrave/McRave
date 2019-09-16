#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Stations {

    namespace {
        map <Unit, BWEB::Station *> myStations, enemyStations;

        void updateStations()
        {

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

    }

    Position getClosestStation(PlayerState pState, Position here)
    {
        auto &list = pState == PlayerState::Self ? myStations : enemyStations;
        auto distBest = DBL_MAX;
        Position best = Positions::Invalid;
        for (auto &station : list) {
            auto s = *station.second;
            double dist = here.getDistance(s.getBWEMBase()->Center());
            if (dist < distBest) {
                best = s.getBWEMBase()->Center();
                distBest = dist;
            }
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

        // Store station and set resource states if we own this station
        unit->getPlayer() == Broodwar->self() ? myStations.emplace(unit, newStation) : enemyStations.emplace(unit, newStation);
        if (unit->getPlayer() == Broodwar->self()) {
            for (auto &mineral : newStation->getBWEMBase()->Minerals()) {

                // If mineral no longer exists
                if (!mineral || !mineral->Unit())
                    continue;

                if (Broodwar->getFrameCount() == 0)
                    Resources::storeResource(mineral->Unit());

                auto resource = Resources::getResourceInfo(mineral->Unit());
                if (resource)
                    resource->setStation(myStations.at(unit));
            }

            for (auto &geyser : newStation->getBWEMBase()->Geysers()) {

                // If geyser no longer exists
                if (!geyser || !geyser->Unit())
                    continue;

                if (Broodwar->getFrameCount() == 0)
                    Resources::storeResource(geyser->Unit());

                auto resource = Resources::getResourceInfo(geyser->Unit());
                if (resource)
                    resource->setStation(myStations.at(unit));
            }
        }

        // Add stations area to territory tracking
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
        list.erase(unit);

        // Remove any territory it was in
        if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition())) {
            if (unit->getPlayer() == Broodwar->self())
                Terrain::getAllyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
            else
                Terrain::getEnemyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
        }
    }

    int needDefenses(BWEB::Station& station)
    {
        const auto groundCount = station.getGroundDefenseCount();
        const auto airCount = station.getAirDefenseCount();
        const auto mainOrNat = station.getBWEMBase()->Location() == BWEB::Map::getMainTile() || station.getBWEMBase()->Location() == BWEB::Map::getNaturalTile();

        // Grab total and current counts of minerals remaining for this base
        auto total = 0;
        auto current = 0;
        for (auto &mineral : station.getBWEMBase()->Minerals()) {
            if (mineral && mineral->Unit()->exists()) {
                total   += mineral->InitialAmount();
                current += mineral->Amount();
            }
        }
        for (auto &gas : station.getBWEMBase()->Geysers()) {
            if (gas && gas->Unit()->exists()) {
                total   += gas->InitialAmount();
                current += gas->Amount();
            }
        }

        // Situations where main or natural want cannons
        if (mainOrNat) {
            if (BuildOrder::isTechUnit(UnitTypes::Protoss_Forge) || Strategy::getEnemyBuild() == "2HatchMuta" || Strategy::getEnemyBuild() == "3HatchMuta")
                return (2 + int(station.getBWEMBase()->Location() == BWEB::Map::getMainTile())) - groundCount;
        }
        else {

            // Calculate percentage remaining and determine desired resources for this non main and non natural base
            const auto percentage = double(current) / double(total);
            const auto desired = (percentage >= 0.75) + (percentage >= 0.5) + (percentage >= 0.25) + (2 * Players::vZ()) - (Stations::getMyStations().size() <= 3) - (Stations::getMyStations().size() <= 4);
            return desired - groundCount;
        }
        return 0;
    }

    bool needPower(BWEB::Station& station)
    {
        for (auto &defense : station.getDefenseLocations()) {
            if (!Pylons::hasPower(defense, UnitTypes::Protoss_Photon_Cannon))
                return true;
        }
        return false;
    }

    PlayerState ownedBy(BWEB::Station * thisStation)
    {
        if (!thisStation || BWEB::Map::isUsed(thisStation->getBWEMBase()->Location(), 4, 3) == UnitTypes::None)
            return PlayerState::None;

        for (auto &[_, station] : myStations) {
            if (station == thisStation)
                return PlayerState::Self;
        }
        for (auto &[_, station] : enemyStations) {
            if (station == thisStation)
                return PlayerState::Enemy;
        }
        return PlayerState::None;
    }

    map <BWAPI::Unit, BWEB::Station *>& getMyStations() { return myStations; };
    map <BWAPI::Unit, BWEB::Station *>& getEnemyStations() { return enemyStations; }
}