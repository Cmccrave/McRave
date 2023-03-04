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
        int frame6MutasDone = 0;
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
            if (Terrain::getMyNatural() && Terrain::getNaturalChoke()) {
                defendNatural = BuildOrder::isWallNat()
                    || BuildOrder::takeNatural()
                    || BuildOrder::buildCount(baseType) > (1 + !BuildOrder::takeNatural())
                    || Stations::getStations(PlayerState::Self).size() >= 2
                    || (!Players::PvZ() && Players::getSupply(PlayerState::Self, Races::Protoss) > 140)
                    || (Broodwar->self()->getRace() != Races::Zerg && Terrain::isReverseRamp());
            }

            // When we don't want to defend our natural
            if (Players::ZvT() && Spy::enemyRush())
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

            Broodwar->drawTriangleMap(defendPosition + Position(0, -20), defendPosition + Position(-20, 10), defendPosition + Position(20, 10), Colors::Green);
        }

        void findHarassPosition()
        {
            auto oldHarass = harassPosition;
            if (!Terrain::getEnemyMain())
                return;
            harassPosition = Positions::Invalid;

            if (com(Zerg_Mutalisk) >= 6 && frame6MutasDone == 0)
                frame6MutasDone = Broodwar->getFrameCount();

            const auto commanderInRange = [&](Position here) {
                auto commander = Util::getClosestUnit(here, PlayerState::Self, [&](auto &u) {
                    return u->getType() == Zerg_Mutalisk;
                });
                return commander && commander->getPosition().getDistance(here) < 160.0;
            };
            const auto stationNotVisitedRecently = [&](auto &station) {
                return max(frame6MutasDone, Broodwar->getFrameCount() - Grids::lastVisibleFrame(TilePosition(station->getResourceCentroid()))) > 2880 && !commanderInRange(station->getResourceCentroid());
            };

            // In FFA just hit closest base to us
            if (Players::vFFA() && attackPosition.isValid()) {
                harassPosition = attackPosition;
                return;
            }

            // Check if enemy lost all bases
            auto lostAll = !Stations::getStations(PlayerState::Enemy).empty();
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
            vector<BWEB::Station*> stations = Stations::getStations(PlayerState::Enemy);
            if (Util::getTime() < Time(10, 00)) {
                stations.push_back(Terrain::getEnemyMain());
                stations.push_back(Terrain::getEnemyNatural());
            }

            // At a certain point we need to ensure they aren't mass expanding - check closest 2 if not visible in last 2 minutes
            auto checkNeeded = (Stations::getStations(PlayerState::Enemy).size() <= 2 && Util::getTime() > Time(10, 00)) || (Stations::getStations(PlayerState::Enemy).size() <= 3 && Util::getTime() > Time(15, 00));
            if (checkNeeded && Terrain::getEnemyNatural()) {
                auto station1 = Stations::getClosestStationGround(Terrain::getEnemyNatural()->getBase()->Center(), PlayerState::None, [&](auto &s) {
                    return s != Terrain::getEnemyMain() && s != Terrain::getEnemyNatural() && !s->getBase()->Geysers().empty();
                });
                if (station1) {
                    stations.push_back(station1);

                    auto station2 = Stations::getClosestStationGround(Terrain::getEnemyNatural()->getBase()->Center(), PlayerState::None, [&](auto &s) {
                        return s != station1 && s != Terrain::getEnemyMain() && s != Terrain::getEnemyNatural() && !s->getBase()->Geysers().empty();
                    });

                    if (station2)
                        stations.push_back(station2);
                }
            }

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
            // UMS Setting
            if (Broodwar->getGameType() == BWAPI::GameTypes::Use_Map_Settings) {
                holdChoke = true;
                return;
            }

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
            if (Broodwar->self()->getRace() == Races::Terran && Players::getSupply(PlayerState::Self, Races::None) > 40)
                holdChoke = true;

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                holdChoke = Players::getSupply(PlayerState::Self, Races::None) > 60;

                if (Players::ZvZ()) {
                    if (!defendNatural && com(Zerg_Zergling) >= 8)
                        holdChoke = true;
                }
                if (Players::ZvP()) {

                }
                if (Players::ZvT()) {
                    holdChoke = true;
                }

                const auto defCount = Stations::getGroundDefenseCount(getDefendStation()) + Stations::getColonyCount(getDefendStation());
                if (defCount > 0)
                    holdChoke = false;
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
        Destination::onFrame();
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
    BWEB::Station * getDefendStation() { return defendNatural ? Terrain::getMyNatural() : Terrain::getMyMain(); }
    bool isDefendNatural() { return defendNatural; }

    bool holdAtChoke() { return holdChoke; }
}