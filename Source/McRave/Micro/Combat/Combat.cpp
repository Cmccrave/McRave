#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Combat {

    namespace {

        // Defending
        bool holdChoke = false;
        bool defendNatural = false;
        const ChokePoint * defendChoke = nullptr;
        const Area * defendArea = nullptr;
        Position defendPosition = Positions::Invalid;

        // Harassing
        int timeHarassingHere = 1500;
        Position harassPosition = Positions::Invalid;

        // Attacking
        Position attackPosition = Positions::Invalid;

        void findAttackPosition()
        {
            // Choose a default attack position, in FFA we choose closest enemy station to us that isn't ours
            auto distBest = Players::vFFA() ? DBL_MAX : 0.0;
            auto posBest = Positions::Invalid;
            if (Players::vFFA()) {
                for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                    auto dist = station->getBase()->Center().getDistance(Terrain::getMainPosition());
                    if (dist < distBest) {
                        distBest = dist;
                        posBest = station->getBase()->Center();
                    }
                }
            }
            else if (Terrain::getEnemyStartingPosition().isValid() && Util::getTime() < Time(6, 00)) {
                attackPosition = Terrain::getEnemyStartingPosition();
                return;
            }

            // Attack furthest enemy station from enemy main, closest enemy station to us in FFA
            attackPosition = Terrain::getEnemyStartingPosition();
            distBest = Players::vFFA() ? DBL_MAX : 0.0;
            posBest = Positions::Invalid;

            for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                auto dist = Terrain::getEnemyStartingPosition().getDistance(station->getBase()->Center());

                if ((!Players::vFFA() && dist < distBest)
                    || (Players::vFFA() && dist > distBest)
                    || BWEB::Map::isUsed(station->getBase()->Location()) == UnitTypes::None)
                    continue;

                const auto stationWalkable = [&](const TilePosition &t) {
                    return Util::rectangleIntersect(Position(station->getBase()->Location()), Position(station->getBase()->Location()) + Position(128, 96), Position(t));
                };

                auto pathFrom = Terrain::getNaturalChoke() ? Position(Terrain::getNaturalChoke()->Center()) : Terrain::getMainPosition();
                BWEB::Path path(pathFrom, station->getBase()->Center(), Zerg_Ultralisk, true, true);
                path.generateJPS([&](auto &t) { return path.unitWalkable(t) || stationWalkable(t); });

                if (path.isReachable()) {
                    distBest = dist;
                    posBest = station->getResourceCentroid();
                }
            }
            attackPosition = posBest;
        }

        void findDefendPosition()
        {
            const auto baseType = Broodwar->self()->getRace().getResourceDepot();

            // When we want to defend our natural
            defendNatural = false;
            if (Terrain::getMyNatural() && Terrain::getNaturalChoke() && !Stations::isBlocked(Terrain::getMyNatural())) {
                defendNatural = (BuildOrder::takeNatural() && !Players::ZvZ())
                    || (Broodwar->self()->getRace() != Races::Zerg && BuildOrder::buildCount(baseType) > (1 + !BuildOrder::takeNatural()))
                    || Stations::getStations(PlayerState::Self).size() >= 2
                    || (!Players::PvZ() && Players::getSupply(PlayerState::Self, Races::Protoss) > 140)
                    || (Broodwar->self()->getRace() != Races::Zerg && Terrain::isReverseRamp());
            }

            // When we don't want to defend our natural
            if (Players::ZvT() && (Spy::enemyRush() || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0))
                defendNatural = false;
            if (Players::ZvP() && Spy::enemyProxy() && Spy::getEnemyBuild() == "2Gate" && total(Zerg_Sunken_Colony) == 0)
                defendNatural = false;
            if (Players::ZvZ() && BuildOrder::getCurrentOpener() == "12Pool" && Util::getTime() < Time(3, 45))
                defendNatural = false;

            // Natural defending position
            if (defendNatural) {
                defendChoke = Terrain::getNaturalChoke();
                defendArea = Terrain::getNaturalArea();
                defendPosition = Stations::getDefendPosition(Terrain::getMyNatural());
            }

            // Main defending position
            else if (Terrain::getMainChoke()) {
                defendChoke = Terrain::getMainChoke();
                defendArea = Terrain::getMainArea();
                defendPosition =  Stations::getDefendPosition(Terrain::getMyMain());
            }
        }

        void findHarassPosition()
        {
            auto oldHarass = harassPosition;
            if (!Terrain::getEnemyMain())
                return;
            harassPosition = Positions::Invalid;
            vector<const BWEB::Station *> stations = Stations::getStations(PlayerState::Enemy);

            const auto commanderInRange = [&](Position here) {
                auto commander = Util::getClosestUnit(here, PlayerState::Self, [&](auto &u) {
                    return u->getType() == Zerg_Mutalisk;
                });
                return commander && commander->getPosition().getDistance(here) < 160.0;
            };
            const auto stationNotVisitedRecently = [&](auto &station) {
                return Broodwar->getFrameCount() - Grids::getLastVisibleFrame(station->getResourceCentroid()) > 2880 && !commanderInRange(station->getResourceCentroid());
            };

            // Adds the next closest ground station
            const auto nextStation = [&]() {
                auto station = Stations::getClosestStationGround(Terrain::getEnemyNatural()->getBase()->Center(), PlayerState::None, [&](auto &s) {
                    return find(stations.begin(), stations.end(), s) == stations.end();
                });
                if (station)
                    stations.push_back(station);
            };

            // In FFA just hit closest base to us
            if (Players::vFFA() && attackPosition.isValid()) {
                harassPosition = attackPosition;
                return;
            }

            // Check if enemy lost all bases
            auto lostAll = Stations::getStations(PlayerState::Enemy).empty();
            for (auto &station : Stations::getStations(PlayerState::Enemy)) {
                if (!Stations::isBaseExplored(station) || BWEB::Map::isUsed(station->getBase()->Location()) != UnitTypes::None)
                    lostAll = false;
            }
            if (lostAll) {
                harassPosition = Positions::Invalid;
                return;
            }

            // Some hardcoded ZvT stuff
            if (Players::ZvT() && Util::getTime() < Time(8, 00) && Terrain::getEnemyMain()) {
                harassPosition = Terrain::getEnemyMain()->getResourceCentroid();
                return;
            }

            // Create a list of valid positions to harass/check
            if (Util::getTime() < Time(10, 00)) {
                stations.push_back(Terrain::getEnemyMain());
                stations.push_back(Terrain::getEnemyNatural());
            }

            // At a certain point we need to ensure they aren't mass expanding - check closest 2 if not visible in last 2 minutes
            auto extrasToCheck = (Stations::getStations(PlayerState::Enemy).size() <= 2 && Util::getTime() > Time(10, 00))
                + (Stations::getStations(PlayerState::Enemy).size() <= 3 && Util::getTime() > Time(15, 00))
                + (Terrain::isIslandMap() && Stations::getStations(PlayerState::Enemy).size() < 2 && Util::getTime() > Time(10, 00));

            for (int x = 0; x < extrasToCheck; x++)
                nextStation();

            // Harass all stations by last visited
            auto best = -1.0;
            for (auto &station : stations) {
                auto defCount = double(Stations::getAirDefenseCount(station));
                auto score = double(Broodwar->getFrameCount() - Grids::getLastVisibleFrame(TilePosition(station->getResourceCentroid()))) / exp(1.0 + defCount);
                if (score > best) {
                    best = score;
                    harassPosition = station->getResourceCentroid();
                }
            }
        }

        void checkHoldChoke()
        {
            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss && Players::getSupply(PlayerState::Self, Races::None) > 40) {
                holdChoke = BuildOrder::takeNatural()
                    || vis(Protoss_Dragoon) > 0
                    || com(Protoss_Shield_Battery) > 0
                    || BuildOrder::isWallNat()
                    || (BuildOrder::isHideTech() && !Spy::enemyRush())
                    || Players::getSupply(PlayerState::Self, Races::None) > 60
                    || Players::vT();
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                holdChoke = !defendNatural && Players::getSupply(PlayerState::Self, Races::None) > 50;

                if (getDefendStation()) {
                    const auto defCount = Stations::getGroundDefenseCount(getDefendStation());
                    if (defCount > 0)
                        holdChoke = false;
                }
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::ZvZ()) {
                    holdChoke = !defendNatural;
                    if (!defendNatural && Stations::getGroundDefenseCount(Terrain::getMyMain()) > 2)
                        holdChoke = false;
                }
                if (Players::ZvP()) {
                    holdChoke = !defendNatural;
                    if (Spy::getEnemyBuild() == "CannonRush")
                        holdChoke = true;
                    if (Players::getTotalCount(PlayerState::Enemy, Protoss_Dragoon) > 0)
                        holdChoke = false;
                }
                if (Players::ZvT()) {
                    holdChoke = !defendNatural;
                    if (!defendNatural)
                        holdChoke = true;
                }
            }
        }
    }

    void onFrame() {
        Visuals::startPerfTest();
        findAttackPosition();
        findDefendPosition();
        findHarassPosition();
        checkHoldChoke();
        Simulation::onFrame();
        State::onFrame();
        Bearings::onFrame();
        Clusters::onFrame();
        Formations::onFrame();
        Navigation::onFrame();
        Decision::onFrame();
        Visuals::endPerfTest("Combat");
    }

    void onStart() {

    }

    Position getAttackPosition() { return attackPosition; }
    Position getDefendPosition() { return defendPosition; }
    Position getHarassPosition() { return harassPosition; }

    const ChokePoint * getDefendChoke() { return defendChoke; }
    const Area * getDefendArea() { return defendArea; }
    const BWEB::Station * getDefendStation() { return defendNatural ? Terrain::getMyNatural() : Terrain::getMyMain(); }
    bool isDefendNatural() { return defendNatural; }

    bool holdAtChoke() { return holdChoke; }
}