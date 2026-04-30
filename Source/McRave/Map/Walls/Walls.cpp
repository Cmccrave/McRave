#include "Walls.h"

#include "BWEB.h"
#include "Builds/All/BuildOrder.h"
#include "Builds/All/Learning.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"
#include "Micro/Scout/Scouts.h"
#include "ProtossWalls.h"
#include "Strategy/Spy/Spy.h"
#include "TerranWalls.h"
#include "ZergWalls.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;
using namespace UnitTypes;

namespace McRave::Walls {

    namespace {
        BWEB::Wall *mainWall    = nullptr;
        BWEB::Wall *naturalWall = nullptr;
        vector<vector<UnitType>> naturaltestingOrder;
        vector<vector<UnitType>> thirdTestingOrder;
        vector<UnitType> defenses;
        bool tight;
        bool openWall;
        UnitType tightType   = None;

        map<BWEB::Wall *, int> desiredGroundDefenses;
        map<BWEB::Wall *, int> desiredAirDefenses;

        void generateWall(const BWEB::Station *const station, const BWEM::ChokePoint *choke)
        {
            auto area = station->getBase()->GetArea();

            // HACK: Zerg walls in the main are just necessary on pocket natural maps, we don't want extra buildings
            if (Broodwar->self()->getRace() == Races::Zerg && area == Terrain::getMainArea()) {
                vector<UnitType> hatch = {Zerg_Hatchery};
                BWEB::Walls::createWall(hatch, area, choke, tightType, defenses, openWall, true);
                return;
            }

            auto &list = station->isNatural() ? naturaltestingOrder : thirdTestingOrder;

            for (auto &buildings : list) {
                if (!BWEB::Walls::getWall(choke)) {
                    BWEB::Walls::createWall(buildings, area, choke, tightType, defenses, openWall, tight);
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
                defenses = {Protoss_Photon_Cannon};
                if (Players::vZ()) {
                    tight               = false;
                    naturaltestingOrder = {{Protoss_Gateway, Protoss_Forge, Protoss_Pylon}};
                }
                else {
                    tight               = false;
                    naturaltestingOrder = {{Protoss_Forge, Protoss_Pylon, Protoss_Pylon, Protoss_Pylon}};
                }
            }

            // Terran wall parameters
            if (Broodwar->self()->getRace() == Races::Terran) {
                tight             = false;
                defenses          = {Terran_Missile_Turret};
                thirdTestingOrder = {{Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot},
                                     {Terran_Supply_Depot, Terran_Supply_Depot, Terran_Supply_Depot},
                                     {Terran_Supply_Depot, Terran_Supply_Depot}};

                if (Players::vZ())
                    naturaltestingOrder = {{Terran_Supply_Depot, Terran_Supply_Depot}};
                else
                    naturaltestingOrder = {{Terran_Barracks, Terran_Supply_Depot}};
            }

            // Zerg wall parameters
            if (Broodwar->self()->getRace() == Races::Zerg) {
                tight               = false;
                defenses            = {Zerg_Sunken_Colony};
                naturaltestingOrder = {{Zerg_Evolution_Chamber, Zerg_Hatchery}, {Zerg_Hatchery, Zerg_Evolution_Chamber}, {Zerg_Evolution_Chamber, Zerg_Evolution_Chamber}, {}};
                thirdTestingOrder   = naturaltestingOrder;
            }
        }

        void findWalls()
        {
            // Create an open wall at every natural
            openWall = true;

            // In FFA just make a wall at our natural (if we have one)
            if (Players::vFFA() && Terrain::getMyNatural() && Terrain::getNaturalChoke()) {
                if (Terrain::getMyNatural()->getChokepoint())
                    generateWall(Terrain::getMyNatural(), Terrain::getNaturalChoke());
            }
            else {
                for (auto &station : BWEB::Stations::getStations()) {
                    if ((station.isMain() && !Terrain::isPocketNatural()) || (station.isNatural() && Terrain::isPocketNatural()))
                        continue;
                    if (station.getChokepoint())
                        generateWall(&station, station.getChokepoint());
                }
            }

            naturalWall = BWEB::Walls::getWall(Terrain::getNaturalChoke());
            mainWall    = BWEB::Walls::getWall(Terrain::getMainChoke());
        }

        int calcDesiredGroundDefenses(BWEB::Wall &wall)
        {
            if (BuildOrder::isRush() || BuildOrder::isPressure() || Spy::getEnemyTransition() == P_Carrier)
                return 0;

            auto groundCount = wall.getGroundDefenseCount();
            auto colonyCount = Walls::getColonyCount(&wall);

            if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()) || BuildOrder::isAllIn() || (!Combat::isDefendNatural() && wall.getStation()->isNatural()) ||
                Stations::isPocket(wall.getStation()))
                return 0;

            // (ZvZ) If enemy adds defenses, we can start to cut defenses too
            if (Players::ZvZ() && Util::getTime() > Time(4, 00))
                groundCount += Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Creep_Colony) +
                               Players::getVisibleCount(PlayerState::Enemy, Zerg_Spore_Colony);

            // (ZvP) If they expanded, we can skip a sunk after a delay
            if (Players::ZvP() && colonyCount == 0 && Spy::enemyFastExpand() && Spy::getEnemyBuild() != P_FFE && Util::getTime() > Time(4, 30)) {
                static Time now = Util::getTime();
                if (Util::getTime() > now + Time(0, 45))
                    groundCount++;
            }

            // (Zv) Can't build defensives early until closest hatch almost completes
            if (Broodwar->self()->getRace() == Races::Zerg && Util::getTime() < Time(3, 30)) {
                auto nearestHatch = Util::getClosestUnit(Position(wall.getChokePoint()->Center()), PlayerState::Self, [&](auto &u) { return u->getType().isResourceDepot(); });
                if (nearestHatch && nearestHatch->frameCompletesWhen() > Broodwar->getFrameCount() + 200)
                    return 0;
            }

            // (Zv) If the natural is narrow, it's fair to skip one after we hit 2
            if (Broodwar->self()->getRace() == Races::Zerg && wall.getStation() && wall.getStation()->isNatural() && Terrain::isNarrowNatural() && wall.getGroundDefenseCount() >= 2) {
                groundCount++;
            }

            // If they're only at home and not proxying units, don't make any defenses for a bit
            if (Broodwar->self()->getRace() == Races::Zerg) {
                auto seconds             = Walls::getColonyCount(&wall) > 0 ? 20 : 30;
                auto minimumColonyNeeded = (Util::getTime() > Time(4, 00)) + (Util::getTime() > Time(6, 00));
                auto morphAlways         = (Spy::getEnemyBuild() == T_RaxFact && wall.getGroundDefenseCount() == 0) || Players::vFFA() || Spy::enemyProxy();
                if (!morphAlways && (wall.getGroundDefenseCount() >= minimumColonyNeeded || getColonyCount(&wall) >= minimumColonyNeeded)) {
                    auto closestUnit = Util::getClosestUnit(Position(wall.getChokePoint()->Center()), PlayerState::Enemy, [&](auto &u) { return Units::inBoundUnit(*u, seconds); });
                    if (!closestUnit)
                        return 0;
                }
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::PvP())
                    return Protoss::PvP_Defenses(wall) - groundCount;
                if (Players::PvZ())
                    return Protoss::PvZ_Defenses(wall) - groundCount;
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                if (Players::TvZ())
                    return Terran::TvZ_Defenses(wall) - groundCount;
                if (Players::TvZ())
                    return Terran::TvZ_Defenses(wall) - groundCount;
                if (Players::TvZ())
                    return Terran::TvZ_Defenses(wall) - groundCount;
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Stations::ownedBy(wall.getStation()) != PlayerState::Self)
                    return 0;

                if (isDefenseFilled(&wall))
                    wall.requestAddedLayer();

                if (Players::ZvP())
                    return Zerg::ZvP_Defenses(wall) - groundCount;
                if (Players::ZvT())
                    return Zerg::ZvT_Defenses(wall) - groundCount;
                if (Players::ZvZ())
                    return Zerg::ZvZ_Defenses(wall) - groundCount;
                if (Players::ZvFFA())
                    return Zerg::ZvFFA_Defenses(wall) - groundCount;
            }
            return 0;
        }

        int calcDesiredAirDefenses(BWEB::Wall &wall)
        {
            auto airCount       = wall.getAirDefenseCount();
            const auto enemyAir = Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Scout) > 0 ||
                                  Players::getTotalCount(PlayerState::Enemy, Protoss_Stargate) > 0 || Players::getTotalCount(PlayerState::Enemy, Terran_Wraith) > 0 ||
                                  Players::getTotalCount(PlayerState::Enemy, Terran_Valkyrie) > 0 || Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) > 0 ||
                                  (Players::getTotalCount(PlayerState::Enemy, Zerg_Spire) > 0 && Util::getTime() > Time(4, 45));

            if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()))
                return 0;

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                return 0;
            }

            // Terran

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                return 0;
            }
            return 0;
        }

        void updateDefenses(BWEB::Wall &wall)
        {
            desiredGroundDefenses[&wall] = calcDesiredGroundDefenses(wall);
            desiredAirDefenses[&wall]    = calcDesiredAirDefenses(wall);
        }

    } // namespace

    void onStart()
    {
        initializeWallParameters();
        findWalls();
    }

    void onFrame()
    {
        static vector<const BWEB::Station *> stationsTried;

        for (auto &station : BWEB::Stations::getStations()) {
            if (station.isMain() || station.isNatural() || find(stationsTried.begin(), stationsTried.end(), &station) != stationsTried.end())
                continue;

            auto defendPos = Stations::getDefendPosition(&station);
            if (defendPos.isValid()) {
                auto defendChoke = Util::getClosestChokepoint(defendPos);
                if (defendChoke && !BWEB::Walls::getWall(defendChoke)) {
                    generateWall(&station, defendChoke);
                    stationsTried.push_back(&station);
                }
            }
        }

        for (auto &[choke, wall] : BWEB::Walls::getWalls()) {
            updateDefenses(wall);
        }
    }

    bool isDefenseFilled(BWEB::Wall *const wall)
    {
        for (auto &defense : wall->getDefenses(1)) {
            if (BWEB::Map::isUsed(defense) == None)
                return false;
        }
        for (auto &defense : wall->getDefenses(2)) {
            if (BWEB::Map::isUsed(defense) == None)
                return false;
        }
        return true;
    }

    bool isComplete(BWEB::Wall *const wall)
    {
        for (auto large : wall->getLargeTiles()) {
            if (BWEB::Map::isUsed(large) == UnitTypes::None)
                return false;
        }
        for (auto medium : wall->getMediumTiles()) {
            if (BWEB::Map::isUsed(medium) == UnitTypes::None)
                return false;
        }
        for (auto small : wall->getSmallTiles()) {
            if (BWEB::Map::isUsed(small) == UnitTypes::None)
                return false;
        }
        return true;
    }

    int getColonyCount(BWEB::Wall *const wall)
    {
        auto colonies = 0;
        for (auto &tile : wall->getDefenses()) {
            if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                colonies++;
        }
        return colonies;
    }

    int needGroundDefenses(BWEB::Wall &wall) { return desiredGroundDefenses[&wall]; }
    int needAirDefenses(BWEB::Wall &wall) { return desiredAirDefenses[&wall]; }
    int getCompleteTypeCount(UnitType type) {}
    int getIncompleteTypeCount(UnitType type) {}
    int getTotalTypeCount(UnitType type) {}
    BWEB::Wall *const getMainWall() { return mainWall; }
    BWEB::Wall *const getNaturalWall() { return naturalWall; }
} // namespace McRave::Walls