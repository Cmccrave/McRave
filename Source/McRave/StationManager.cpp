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

    BWEB::Station * getClosestStation(PlayerState pState, Position here)
    {
        auto &list = pState == PlayerState::Self ? myStations : enemyStations;
        auto distBest = DBL_MAX;
        BWEB::Station * closestStation = nullptr;
        for (auto &station : list) {
            auto &s = *station.second;
            double dist = here.getDistance(s.getBase()->Center());
            if (dist < distBest) {
                closestStation = &s;
                distBest = dist;
            }
        }
        return closestStation;
    }

    void storeStation(Unit unit)
    {
        auto newStation = BWEB::Stations::getClosestStation(unit->getTilePosition());
        if (!newStation
            || !unit->getType().isResourceDepot()
            || unit->getTilePosition() != newStation->getBase()->Location())
            return;

        // Store station and set resource states if we own this station
        unit->getPlayer() == Broodwar->self() ? myStations.emplace(unit, newStation) : enemyStations.emplace(unit, newStation);
        if (unit->getPlayer() == Broodwar->self()) {
            for (auto &mineral : newStation->getBase()->Minerals()) {

                // If mineral no longer exists
                if (!mineral || !mineral->Unit())
                    continue;

                if (Broodwar->getFrameCount() == 0)
                    Resources::storeResource(mineral->Unit());

                auto resource = Resources::getResourceInfo(mineral->Unit());
                if (resource)
                    resource->setStation(myStations.at(unit));
            }

            for (auto &geyser : newStation->getBase()->Geysers()) {

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
        const auto groundCount = station.getGroundDefenseCount();

        // Grab total and current counts of minerals remaining for this base
        auto total = 0;
        auto current = 0;
        for (auto &mineral : station.getBase()->Minerals()) {
            if (mineral && mineral->Unit()->exists()) {
                total   += mineral->InitialAmount();
                current += mineral->Amount();
            }
        }
        for (auto &gas : station.getBase()->Geysers()) {
            if (gas && gas->Unit()->exists()) {
                total   += gas->InitialAmount();
                current += gas->Amount();
            }
        }

        // Main defenses
        if (station.isMain()) {
            if (Players::PvP() && Strategy::needDetection())
                return 1 - groundCount;
            if (Players::PvZ() && (Strategy::getEnemyTransition() == "2HatchMuta" || Strategy::getEnemyTransition() == "3HatchMuta"))
                return 3 - groundCount;
            if (Players::ZvZ()) {
                if (Strategy::getEnemyTransition() == "2HatchLing" && vis(Zerg_Spire) > 0)
                    return (Util::getTime() > Time(3, 45)) + (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(5, 30)) - groundCount;
                else if (Util::getTime() < Time(6, 00) && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 40)
                    return 6 - groundCount;
                else if (Util::getTime() < Time(8, 00) && (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hatchery) >= 3 || Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 26))
                    return 4 - groundCount;
                else if (Strategy::enemyPressure())
                    return (Util::getTime() > Time(4, 10)) + (vis(Zerg_Sunken_Colony) > 0) + (vis(Zerg_Drone) >= 8 && com(Zerg_Sunken_Colony) >= 2) - groundCount;
                else if (Strategy::enemyRush())
                    return 1 + (vis(Zerg_Sunken_Colony) > 0) + (vis(Zerg_Drone) >= 8 && com(Zerg_Sunken_Colony) >= 2) - groundCount;
                else if (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) > 12)
                    return 1 - groundCount;
            }
            return 0;
        }

        // Natural defenses
        else if (station.isNatural()) {
            if (Players::PvP() && Strategy::needDetection())
                return 1 - groundCount;
            if (Players::PvZ() && (Strategy::getEnemyTransition() == "2HatchMuta" || Strategy::getEnemyTransition() == "3HatchMuta"))
                return 2 - groundCount;
            return 0;
        }

        // Calculate percentage remaining and determine desired resources for this base
        else if (Util::getTime() > Time(6, 30)) {

            if (Broodwar->self()->getRace() == Races::Zerg) {
                auto chokeCount = max(2, int(station.getBase()->GetArea()->ChokePoints().size()));
                auto mineralCount = 0;
                auto droneCount = 0;
                for (auto &mineral : Resources::getMyMinerals()) {
                    if (mineral->getStation() == &station) {
                        droneCount += int(mineral->targetedByWhat().size());
                        mineralCount++;
                    }
                }
                auto saturationRatio = mineralCount > 0 ? double(2 * droneCount) / double(mineralCount) : 0.0;

                if (Players::vT() || Players::ZvP())
                    return int(round(saturationRatio * double(chokeCount))) - groundCount;
            }

            const auto percentage = double(current) / double(total);
            const auto desired = (percentage >= 0.75) + (percentage >= 0.5) + (percentage >= 0.25) - (Stations::getMyStations().size() <= 4) - (Stations::getMyStations().size() <= 5) + (Util::getTime() > Time(15, 0));
            return desired - groundCount;
        }
        return 0;
    }

    int needAirDefenses(BWEB::Station& station)
    {
        auto airCount = station.getAirDefenseCount();
        const auto enemyAir = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0
            || Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0
            || Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0
            || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0
            || Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0;

        if ((!BuildOrder::isTechUnit(Zerg_Mutalisk) && vis(Zerg_Lair) == 0) || (Players::ZvT() && enemyAir && Util::getTime() > Time(11, 0))) {
            if (enemyAir && !Strategy::enemyFastExpand() && Util::getTime() > Time(4, 0))
                return 1 - airCount;
            if (enemyAir && Util::getTime() > Time(10, 0))
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
        if (!thisStation || BWEB::Map::isUsed(thisStation->getBase()->Location(), 4, 3) == UnitTypes::None)
            return PlayerState::None;

        for (auto &[_, station] : myStations) {
            if (station == thisStation)
                return PlayerState::Self;
            if (station->getBase() == thisStation->getBase() && station != thisStation)
                Broodwar << "Bad ptr" << endl;
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