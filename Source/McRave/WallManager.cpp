#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls {

    namespace {
        BWEB::Wall* mainWall = nullptr;
        BWEB::Wall* naturalWall = nullptr;
        vector<UnitType> buildings;
        vector<UnitType> defenses;
        bool tight;
        bool openWall;
        UnitType tightType = None;
    }

    void findNaturalWall()
    {
        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings || Broodwar->self()->getRace() == Races::Terran)
            return;

        auto choke = BWEB::Map::getNaturalChoke();
        auto area = BWEB::Map::getNaturalArea();
        openWall = true;

        naturalWall = BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
    }

    void findMainWall()
    {
        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings || Broodwar->self()->getRace() != Races::Terran)
            return;

        auto choke = BWEB::Map::getMainChoke();
        auto area = BWEB::Map::getMainArea();
        openWall = Broodwar->self()->getRace() != Races::Terran;

        mainWall = BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
    }

    void findOtherWalls()
    {
        const auto naturalChoke = [&](BWEB::Station * natural, BWEB::Station * closestMain) {
            set<BWEM::ChokePoint const *> nonChokes;
            for (auto &choke : mapBWEM.GetPath(closestMain->getBWEMBase()->Center(), natural->getBWEMBase()->Center()))
                nonChokes.insert(choke);

            auto distBest = DBL_MAX;
            const BWEM::Area* second = nullptr;
            const BWEM::ChokePoint * naturalChoke = nullptr;

            // Iterate each neighboring area to get closest to this natural area
            for (auto &area : natural->getBWEMBase()->GetArea()->AccessibleNeighbours()) {
                auto center = area->Top();
                const auto dist = Position(center).getDistance(mapBWEM.Center());

                bool wrongArea = false;
                for (auto &choke : area->ChokePoints()) {
                    if ((!choke->Blocked() && choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) <= 2) || nonChokes.find(choke) != nonChokes.end()) {
                        wrongArea = true;
                    }
                }
                if (wrongArea)
                    continue;

                if (center.isValid() && dist < distBest) {
                    second = area;
                    distBest = dist;
                }
            }

            distBest = DBL_MAX;
            for (auto &choke : natural->getBWEMBase()->GetArea()->ChokePoints()) {
                if (choke->Center() == BWEB::Map::getMainChoke()->Center()
                    || choke->Blocked()
                    || nonChokes.find(choke) != nonChokes.end()
                    || choke->Geometry().size() <= 3
                    || (choke->GetAreas().first != second && choke->GetAreas().second != second))
                    continue;

                const auto dist = Position(choke->Center()).getDistance(Position(closestMain->getBWEMBase()->Center()));
                if (dist < distBest) {
                    naturalChoke = choke;
                    distBest = dist;
                }
            }
            return naturalChoke;
        };

        // Create a Zerg/Protoss wall at every natural
        if (Broodwar->self()->getRace() != Races::Terran) {
            openWall = true;
            for (auto &natural : BWEB::Stations::getNaturalStations()) {
                if (natural.getBWEMBase()->Center() == BWEB::Map::getNaturalPosition())
                    continue;

                auto closestMain = BWEB::Stations::getClosestMainStation(natural.getBWEMBase()->Location());
                auto choke = naturalChoke(&natural, closestMain);
                BWEB::Walls::createWall(buildings, natural.getBWEMBase()->GetArea(), choke, tightType, defenses, openWall, tight);
            }
        }
        
        // Create a Terran wall at every main
        else {
            for (auto &main : BWEB::Stations::getMainStations()) {
                if (main.getBWEMBase()->Center() == BWEB::Map::getMainPosition())
                    continue;

                auto closestNatural = BWEB::Stations::getClosestNaturalStation(main.getBWEMBase()->Location());
                auto choke = mapBWEM.GetPath(main.getBWEMBase()->Center(), closestNatural->getBWEMBase()->Center()).front();
                BWEB::Walls::createWall(buildings, main.getBWEMBase()->GetArea(), choke, tightType, defenses, openWall, tight);
            }
        }
    }

    void initializeWallParameters()
    {
        // Figure out what we need to be tight against
        if (Players::TvP())
            tightType = Protoss_Zealot;
        else if (Players::TvZ())
            tightType = Zerg_Zergling;
        else
            tightType = None;

        // Protoss wall parameters
        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Players::vZ()) {
                tightType = None;
                tight = false;
                buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
                defenses.insert(defenses.end(), 10, Protoss_Photon_Cannon);
            }
            else {
                int count = 2;
                tight = false;
                buildings.insert(buildings.end(), count, Protoss_Pylon);
                defenses.insert(defenses.end(), 10, Protoss_Photon_Cannon);
            }
        }

        // Terran wall parameters
        if (Broodwar->self()->getRace() == Races::Terran) {
            tight = true;

            if (Broodwar->mapFileName().find("Heartbreak") != string::npos || Broodwar->mapFileName().find("Empire") != string::npos)
                buildings ={ Terran_Barracks, Terran_Supply_Depot };
            else
                buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };
        }

        // Zerg wall parameters
        if (Broodwar->self()->getRace() == Races::Zerg) {
            tight = false;            
            defenses.insert(defenses.end(), 10, Zerg_Creep_Colony);

            auto p1 = BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end1);
            auto p2 = BWEB::Map::getNaturalChoke()->Pos(BWEB::Map::getNaturalChoke()->end2);

            if (abs(p1.x - p2.x) * 8 >= 320 || abs(p1.y - p2.y) * 8 >= 256 || p1.getDistance(p2) * 8 >= 288) {
                Broodwar << "Wideboi" << endl;
                buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
            }
            else
                buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber };
        }
    }

    void onStart()
    {
        initializeWallParameters();
        findMainWall();
        findNaturalWall();
        findOtherWalls();
    }

    int needGroundDefenses(BWEB::Wall& wall)
    {
        auto groundCount = wall.getGroundDefenseCount();
        if (!Terrain::isInAllyTerritory(wall.getArea()))
            return 0;

        // Protoss
        if (Broodwar->self()->getRace() == Races::Protoss) {
            auto prepareExpansionDefenses = Util::getTime() < Time(10, 0) && vis(Protoss_Nexus) >= 2 && com(Protoss_Forge) > 0;

            if (Players::vP() && prepareExpansionDefenses && BuildOrder::isWallNat()) {
                auto cannonCount = 2 + int(1 + Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Protoss_High_Templar) - com(Protoss_Dark_Templar)) / 2;
                return cannonCount - groundCount;
            }

            if (Players::vZ() && BuildOrder::isWallNat()) {
                auto cannonCount = int(com(Protoss_Forge) > 0)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 6)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 12)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Zergling) >= 24)
                    + (Players::getCurrentCount(PlayerState::Enemy, Zerg_Hydralisk) / 2);

                // TODO: If scout died, go to 2 cannons, if next scout dies, go 3 cannons   
                if (Strategy::getEnemyTransition() == "2HatchHydra")
                    return 5 - groundCount;
                else if (Strategy::getEnemyTransition() == "3HatchHydra")
                    return 4 - groundCount;
                else if (Strategy::getEnemyTransition() == "2HatchMuta" && Util::getTime() > Time(4, 0))
                    return 3 - groundCount;
                else if (Strategy::getEnemyTransition() == "3HatchMuta" && Util::getTime() > Time(5, 0))
                    return 3 - groundCount;
                else if (Strategy::getEnemyOpener() == "4Pool")
                    return 2 + (Players::getSupply(PlayerState::Self) >= 24) - groundCount;
                return cannonCount - groundCount;
            }
        }

        // Zerg
        if (Broodwar->self()->getRace() == Races::Zerg) {

            if (Players::vP()) {
                if (Strategy::getEnemyBuild() == "1GateCore")
                    return (2 * (Util::getTime() > Time(5, 30))) + (2 * (Util::getTime() > Time(7, 00)));

                if (BuildOrder::isOpener() || Stations::getMyStations().size() < 3) {
                    const auto count = int((Players::getCurrentCount(PlayerState::Enemy, Protoss_Zealot) + Players::getCurrentCount(PlayerState::Enemy, Protoss_Dragoon)) / 1.5) - (com(Zerg_Zergling) / 6) + (2 * Players::getCurrentCount(PlayerState::Enemy, Protoss_Dark_Templar));
                    return max(int(Strategy::getEnemyBuild() == "2Gate") * 3, count) - groundCount;
                }
                else
                    return (Util::getTime().minutes - groundCount - 4) / 2;
            }

            if (Players::vT()) {
                if (Strategy::getEnemyTransition() == "1FactTanks")
                    return (5 * (Util::getTime() > Time(10, 30))) - groundCount;
                if (Strategy::getEnemyBuild() == "RaxFact")
                    return (2 * (Util::getTime() > Time(3, 00))) - groundCount;
                if (Strategy::getEnemyBuild() == "2Rax")
                    return (2 * (Util::getTime() > Time(2, 00))) + (2 * (Util::getTime() > Time(4, 30))) - groundCount;
                if (Strategy::getEnemyBuild() == "RaxCC")
                    return (6 * (Util::getTime() > Time(6, 00))) - groundCount;
                return (Players::getCurrentCount(PlayerState::Enemy, Terran_Marine) / 4) - groundCount;
            }
        }
        return 0;
    }

    int needAirDefenses(BWEB::Wall& wall)
    {
        auto airCount = wall.getAirDefenseCount();
        if (!Terrain::isInAllyTerritory(wall.getArea()))
            return 0;

        // P
        if (Broodwar->self()->getRace() == Races::Protoss) {

        }

        // Z
        if ((Players::ZvP() || Players::ZvT()) && !BuildOrder::isTechUnit(Zerg_Mutalisk)) {
            if ((Strategy::enemyFastExpand() && Util::getTime() > Time(8, 0)) || (!Strategy::enemyFastExpand() && Util::getTime() > Time(7, 0)))
                return Strategy::enemyAir() - airCount;
        }
        return 0;
    }

    BWEB::Wall* getMainWall() { return mainWall; }
    BWEB::Wall* getNaturalWall() { return naturalWall; }
}