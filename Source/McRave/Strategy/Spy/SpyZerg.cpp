#include "Builds/All/BuildOrder.h"
#include "Info/Player/PlayerInfo.h"
#include "Info/Player/Players.h"
#include "Info/Unit/UnitInfo.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Micro/Scout/Scouts.h"
#include "Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Spy::Zerg {

    Time equalPoolTiming          = Time(0, 30);
    map<string, Time> poolTimings = {
        // Pool first
        {static_cast<string>(Z_4Pool), Time(1, 40)},
        {static_cast<string>(Z_7Pool), Time(1, 50)},
        {static_cast<string>(Z_9Pool), Time(2, 00)},
        {static_cast<string>(Z_Overpool), Time(2, 10)},
        {static_cast<string>(Z_12Pool), Time(2, 25)},

        // Hatch first
        {static_cast<string>(Z_9Hatch), Time(2, 30)},
        {static_cast<string>(Z_10Hatch), Time(2, 40)},
        {static_cast<string>(Z_12Hatch), Time(2, 50)},
    };

    map<string, Time> lingTimings;
    map<string, Time> hatchTimings;

    namespace {

        void enemyZergBuilds(PlayerInfo &player, StrategySpy &theSpy)
        {
            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // If this is our first time seeing a Zergling, see how soon until it arrives
                if (unit.getType() == UnitTypes::Zerg_Zergling) {
                    if (unit.timeArrivesWhen().minutes > 0 && unit.timeArrivesWhen() < theSpy.rushArrivalTime)
                        theSpy.rushArrivalTime = unit.timeArrivesWhen();
                }
            }

            // Build based on the opener seen
            if (theSpy.opener.name == Z_4Pool || theSpy.opener.name == Z_7Pool || theSpy.opener.name == Z_9Pool || theSpy.opener.name == Z_Overpool || theSpy.opener.name == Z_12Pool) {
                if (theSpy.productionCount > 1)
                    theSpy.build.name = Z_PoolHatch;
                else if (completesBy(1, Zerg_Lair, Time(3, 45)))
                    theSpy.build.name = Z_PoolLair;
            }
            if (theSpy.opener.name == Z_9Hatch || theSpy.opener.name == Z_10Hatch || theSpy.opener.name == Z_12Hatch)
                theSpy.build.name = Z_HatchPool;

            // Build based on the transition seen
            if (theSpy.transition.name == Z_1HatchMuta || theSpy.transition.name == Z_1HatchLurker)
                theSpy.build.name = Z_PoolLair;
        }

        void enemyZergOpeners(PlayerInfo &player, StrategySpy &theSpy)
        {
            // Hatch timing
            // 9Hatch
            if (Players::getCompleteCount(PlayerState::Enemy, Zerg_Hatchery) > 0) {
                auto cnt = 1 + (Terrain::getEnemyMain() && Stations::isBaseExplored(Terrain::getEnemyMain()));
                if (completesBy(cnt, Zerg_Hatchery, Time(2, 35)))
                    theSpy.opener.name = Z_9Hatch;

                // 10Hatch
                else if (completesBy(cnt, Zerg_Hatchery, Time(2, 45)))
                    theSpy.opener.name = Z_10Hatch;

                // 12Hatch
                else if (completesBy(cnt, Zerg_Hatchery, Time(3, 05)))
                    theSpy.opener.name = Z_12Hatch;
            }

            // Pool timing
            // 4Pool
            else if (Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) == 0 && Players::getCompleteCount(PlayerState::Enemy, Zerg_Spawning_Pool) == 0) {
                if (completesBy(1, Zerg_Spawning_Pool, Time(1, 40)))
                    theSpy.opener.name = Z_4Pool;

                // 7Pool
                else if (completesBy(1, Zerg_Spawning_Pool, Time(1, 50)))
                    theSpy.opener.name = Z_7Pool;

                // 9Pool
                else if (completesBy(1, Zerg_Spawning_Pool, Time(2, 00)) || completesBy(1, Zerg_Lair, Time(3, 45)))
                    theSpy.opener.name = Z_9Pool;

                // 12Pool
                else if (completesBy(1, Zerg_Spawning_Pool, Time(2, 30)))
                    theSpy.opener.name = Z_12Pool;

                // 12Hatch
                else if (completesBy(1, Zerg_Spawning_Pool, Time(3, 15)))
                    theSpy.opener.name = Z_12Pool;
            }

            // Ling timings
            // 4Pool
            else if (Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) > 0) {
                if (arrivesBy(1, Zerg_Zergling, Time(2, 40)) || arrivesBy(8, Zerg_Zergling, Time(3, 00)) || completesBy(1, Zerg_Zergling, Time(2, 05)))
                    theSpy.opener.name = Z_4Pool;

                // 7Pool
                else if (arrivesBy(1, Zerg_Zergling, Time(2, 45)) || completesBy(1, Zerg_Zergling, Time(2, 15)))
                    theSpy.opener.name = Z_7Pool;

                // 9Pool
                else if (arrivesBy(1, Zerg_Zergling, Time(2, 55)) || arrivesBy(8, Zerg_Zergling, Time(3, 15)) || completesBy(1, Zerg_Zergling, Time(2, 25)) ||
                         completesBy(8, Zerg_Zergling, Time(3, 00)) || completesBy(10, Zerg_Zergling, Time(3, 20)) || completesBy(12, Zerg_Zergling, Time(3, 30)) ||
                         completesBy(14, Zerg_Zergling, Time(3, 40)) || arrivesBy(16, Zerg_Zergling, Time(4, 20)))
                    theSpy.opener.name = Z_9Pool;

                // Overpool
                else if (arrivesBy(1, Zerg_Zergling, Time(3, 05)) || arrivesBy(8, Zerg_Zergling, Time(3, 20)) || completesBy(1, Zerg_Zergling, Time(2, 40)))
                    theSpy.opener.name = Z_Overpool;

                // 12Pool
                else if (arrivesBy(1, Zerg_Zergling, Time(3, 30)) || arrivesBy(8, Zerg_Zergling, Time(3, 35)) || completesBy(1, Zerg_Zergling, Time(3, 00)))
                    theSpy.opener.name = Z_12Pool;

                // 12Hatch
                else if (completesBy(6, Zerg_Zergling, Time(3, 15)) || arrivesBy(6, Zerg_Zergling, Time(4, 00)) || arrivesBy(10, Zerg_Zergling, Time(4, 20)))
                    theSpy.opener.name = Z_12Hatch;
            }
        }

        void enemyZergTransitions(PlayerInfo &player, StrategySpy &theSpy)
        {
            // Ling rush detection
            if (theSpy.opener.name == Z_4Pool || theSpy.opener.name == Z_7Pool ||
                (!Players::ZvZ() && theSpy.opener.name == Z_9Pool && Util::getTime() > Time(3, 30) && Util::getTime() < Time(4, 30) && Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 0 &&
                 theSpy.productionCount == 1))
                theSpy.transition.name = Z_Rush;

            // Zergling all-in transitions
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0 &&
                Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 0 && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 12) {

                // General
                if (completesBy(1, UpgradeTypes::Metabolic_Boost, Time(4, 15)) && arrivesBy(20, Zerg_Zergling, Time(4, 30)) ||
                    (Util::getTime() < Time(5, 00) && Scouts::gotFullScout() && Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone) <= 8 && theSpy.build.name == Z_PoolHatch &&
                     (theSpy.opener.name == Z_9Pool || theSpy.opener.name == Z_Overpool)))
                    theSpy.transition.name = Z_2HatchSpeedling;
                else if (!completesBy(1, UpgradeTypes::Metabolic_Boost, Time(4, 15)) && completesBy(1, UpgradeTypes::Metabolic_Boost, Time(5, 00)) && arrivesBy(28, Zerg_Zergling, Time(5, 10)))
                    theSpy.transition.name = Z_3HatchSpeedling;

                // ZvZ
                if (Players::ZvZ()) {
                    if (completesBy(1, UpgradeTypes::Zerg_Melee_Attacks, Time(5, 30)))
                        theSpy.transition.name = Z_UpgradeLing;
                }
            }

            // Hydralisk/Lurker build detection
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0) {

                if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 0) {
                    if (theSpy.productionCount >= 4)
                        theSpy.transition.name = Z_5HatchHydra;
                    else if (theSpy.productionCount == 3)
                        theSpy.transition.name = Z_3HatchHydra;
                    else if (theSpy.productionCount == 2)
                        theSpy.transition.name = Z_2HatchHydra;
                    else if (theSpy.productionCount == 1)
                        theSpy.transition.name = Z_1HatchHydra;
                }
                else {
                    if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) == 2)
                        theSpy.transition.name = Z_2HatchLurker;
                    else if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 1 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) == 0)
                        theSpy.transition.name = Z_1HatchLurker;
                }
            }

            // Mutalisk transition detection
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0) {

                // General
                if (completesBy(1, Zerg_Lair, Time(3, 45)) || completesBy(1, Zerg_Spire, Time(5, 15)) || arrivesBy(1, Zerg_Mutalisk, Time(6, 00)))
                    theSpy.transition.name = Z_1HatchMuta;
                else if (completesBy(1, Zerg_Spire, Time(5, 45)) || arrivesBy(1, Zerg_Mutalisk, Time(6, 30)))
                    theSpy.transition.name = Z_2HatchMuta;
                else if (completesBy(1, Zerg_Spire, Time(6, 15)) || arrivesBy(1, Zerg_Mutalisk, Time(7, 00)))
                    theSpy.transition.name = Z_3HatchMuta;

                // ZvZ
                if (Players::ZvZ()) {
                    if (!theSpy.expand.likely && theSpy.productionCount == 1 && Util::getTime() > Time(5, 30))
                        theSpy.transition.name = Z_1HatchMuta;
                    else if (theSpy.expand.likely && theSpy.productionCount == 2 && Util::getTime() > Time(6, 00))
                        theSpy.transition.name = Z_2HatchMuta;
                }
            }
        }
    } // namespace

    void enemyZergMisc(PlayerInfo &player, StrategySpy &theSpy)
    {
        // Turtle detection
        if (completesBy(2, Zerg_Sunken_Colony, Time(4, 00)) || completesBy(3, Zerg_Sunken_Colony, Time(5, 30)))
            theSpy.turtle.possible = true;
        if (Players::ZvZ() && Spy::enemyFastExpand() && completesBy(15, Zerg_Drone, Time(3, 15)))
            theSpy.turtle.possible = true;

        // Fortress detection
        if (completesBy(3, Zerg_Sunken_Colony, Time(8, 00)) || completesBy(3, Zerg_Spore_Colony, Time(8, 00)))
            theSpy.fortress.possible = true;
    }

    void updateZerg(StrategySpy &theSpy)
    {
        for (auto &p : Players::getPlayers()) {
            PlayerInfo &player = p.second;
            if (player.isEnemy() && player.getCurrentRace() == Races::Zerg) {
                if (!theSpy.build.confirmed)
                    enemyZergBuilds(player, theSpy);
                if (!theSpy.opener.confirmed)
                    enemyZergOpeners(player, theSpy);
                if (!theSpy.transition.confirmed)
                    enemyZergTransitions(player, theSpy);
                enemyZergMisc(player, theSpy);
            }
        }
    }

    bool enemyFasterPool()
    {
        auto enemyTiming = poolTimings.find(Spy::getEnemyOpener());
        auto selfTiming  = poolTimings.find(BuildOrder::getCurrentOpener());

        if (enemyTiming == poolTimings.end() || selfTiming == poolTimings.end())
            return false;

        // Self timing is later than threshold, enemy is faster
        return selfTiming->second - enemyTiming->second > equalPoolTiming;
    }

    bool enemyEqualPool()
    {
        auto enemyTiming = poolTimings.find(Spy::getEnemyOpener());
        auto selfTiming  = poolTimings.find(BuildOrder::getCurrentOpener());

        if (enemyTiming == poolTimings.end() || selfTiming == poolTimings.end())
            return false;

        // Self timing and enemy timing within threshold, equal
        return (enemyTiming->second > selfTiming->second && enemyTiming->second - selfTiming->second <= equalPoolTiming) ||
               (enemyTiming->second <= selfTiming->second && selfTiming->second - enemyTiming->second <= equalPoolTiming);
    }

    bool enemySlowerPool()
    {
        auto enemyTiming = poolTimings.find(Spy::getEnemyOpener());
        auto selfTiming  = poolTimings.find(BuildOrder::getCurrentOpener());

        if (enemyTiming == poolTimings.end() || selfTiming == poolTimings.end())
            return false;

        // Enemy timing is later than threshold, self is faster
        return enemyTiming->second - selfTiming->second > equalPoolTiming;
    }

    // TODO: Use pool timings
    bool enemyFasterSpeed()
    {
        return (Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost, 1) && !Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1)) ||
               (enemyFasterPool() && !Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1));
    }

    // TODO: Equal speed

    // TODO: Use pool timings
    bool enemySlowerSpeed()
    {
        return (!Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost, 1) && Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1)) ||
               (enemySlowerPool() && Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1));
    }
} // namespace McRave::Spy::Zerg