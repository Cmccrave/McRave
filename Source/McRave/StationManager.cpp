#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

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

        // Remove workers from any resources on this station
        for (auto &mineral : Resources::getMyMinerals()) {
            if (mineral->getStation() == newStation)
                for (auto &worker : mineral->targetedByWhat()) {
                    if (!worker.expired()) {
                        worker.lock()->setResource(nullptr);
                        mineral->removeTargetedBy(worker);
                    }
                }
        }
        for (auto &gas : Resources::getMyGas()) {
            if (gas->getStation() == newStation)
                for (auto &worker : gas->targetedByWhat()) {
                    if (!worker.expired()) {
                        worker.lock()->setResource(nullptr);
                        gas->removeTargetedBy(worker);
                    }
                }
        }

        // Remove any territory it was in
        if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition())) {
            if (unit->getPlayer() == Broodwar->self())
                Terrain::getAllyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
            else
                Terrain::getEnemyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
        }
    }

    int needGroundDefenses(BWEB::Station& station)
    {
        // See how many workers are assigned
        auto workerCount = 0;
        for (auto &m : Resources::getMyMinerals()) {
            auto &mineral = *m;
            if (mineral.getStation() == &station && !mineral.targetedByWhat().empty())
                workerCount++;
        }
        for (auto &g : Resources::getMyMinerals()) {
            auto &gas = *g;
            if (gas.getStation() == &station && !gas.targetedByWhat().empty())
                workerCount++;
        }

        if (workerCount < 3)
            return 0;

        auto groundCount = station.getGroundDefenseCount();
        const auto main = station.getBWEMBase()->Location() == BWEB::Map::getMainTile();
        const auto nat = station.getBWEMBase()->Location() == BWEB::Map::getNaturalTile();

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

        // Main defenses
        if (main) {
            if (Players::PvP() && Strategy::needDetection())
                return 1 - groundCount;
            if (Players::PvZ() && (Strategy::getEnemyBuild() == "2HatchMuta" || Strategy::getEnemyBuild() == "3HatchMuta"))
                return 3 - groundCount; 
            if (Players::ZvZ() && Players::getCurrentCount(PlayerState::Self, Zerg_Zergling) <= Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) && Players::getCurrentCount(PlayerState::Self, Zerg_Zergling) >= 6)
                return 1 - groundCount;
        }

        // Natural defenses
        else if (nat) {
            if (Players::PvP() && Strategy::needDetection())
                return 1 - groundCount;
            if (Players::PvZ() && (Strategy::getEnemyBuild() == "2HatchMuta" || Strategy::getEnemyBuild() == "3HatchMuta"))
                return 2 - groundCount;
        }

        // Calculate percentage remaining and determine desired resources for this base
        else {
            const auto percentage = double(current) / double(total);
            const auto desired = (percentage >= 0.75) + (percentage >= 0.5) + (percentage >= 0.25) - (Stations::getMyStations().size() <= 3) - (Stations::getMyStations().size() <= 4) + (Util::getTime() > Time(15, 0)) + (Util::getTime() > Time(20, 0));
            return desired - groundCount;
        }
        return 0;
    }

    int needAirDefenses(BWEB::Station& station)
    {
        // See how many workers are assigned
        auto workerCount = 0;
        for (auto &m : Resources::getMyMinerals()) {
            auto &mineral = *m;
            if (!mineral.targetedByWhat().empty())
                workerCount++;
        }
        for (auto &g : Resources::getMyMinerals()) {
            auto &gas = *g;
            if (!gas.targetedByWhat().empty())
                workerCount++;
        }

        if (workerCount < 3)
            return 0;

        auto airCount = station.getAirDefenseCount();
        const auto enemyAir = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Starport) > 0
            || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0
            || Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0;

        if (!Players::ZvZ() || vis(Zerg_Spire) == 0) {
            if (enemyAir && Util::getTime() > Time(5, 0))
                return 1 - airCount;
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

    map <Unit, BWEB::Station *>& getMyStations() { return myStations; };
    map <Unit, BWEB::Station *>& getEnemyStations() { return enemyStations; }
}