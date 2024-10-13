#include "Main\McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Spy::Zerg {

    Time equalPoolTiming = Time(0, 30);
    map<string, Time> poolTimings =
    {
        // Pool first
        {"4Pool", Time(1, 40)},
        {"7Pool", Time(1, 50)},
        {"9Pool", Time(2, 00)},
        {"Overpool", Time(2, 10)},
        {"12Pool", Time(2, 25)},

        // Hatch first
        {"10Hatch", Time(2, 40)},
        {"12Hatch", Time(2, 50)},
    };

    map<string, Time> lingTimings;

    namespace {

        void enemyZergBuilds(PlayerInfo& player, StrategySpy& theSpy)
        {
            theSpy.build.framesRequired = 24;

            for (auto &u : player.getUnits()) {
                UnitInfo &unit =*u;

                // If this is our first time seeing a Zergling, see how soon until it arrives
                if (unit.getType() == UnitTypes::Zerg_Zergling) {
                    if (unit.timeArrivesWhen().minutes > 0 && unit.timeArrivesWhen() < theSpy.rushArrivalTime)
                        theSpy.rushArrivalTime = unit.timeArrivesWhen();
                }
            }

            // Build based on the opener seen
            if (theSpy.opener.name == "4Pool" || theSpy.opener.name == "7Pool" || theSpy.opener.name == "9Pool" || theSpy.opener.name == "Overpool" || theSpy.opener.name == "12Pool") {
                if (theSpy.productionCount > 1)
                    theSpy.build.name = "PoolHatch";
                else if (completesBy(1, Zerg_Lair, Time(3, 45)))
                    theSpy.build.name = "PoolLair";
            }
            if (theSpy.opener.name == "9Hatch" || theSpy.opener.name == "10Hatch" || theSpy.opener.name == "12Hatch")
                theSpy.build.name = "HatchPool";

            // Build based on the transition seen
            if (theSpy.transition.name == "1HatchMuta" || theSpy.transition.name == "1HatchLurker")
                theSpy.build.name = "PoolLair";
        }

        void enemyZergOpeners(PlayerInfo& player, StrategySpy& theSpy)
        {
            theSpy.opener.framesRequired = 24;

            for (int x = 1; x <= 12; x++) {
                for (auto &[opener, time] : poolTimings) {

                }
            }

            // Pool timing
            if (arrivesBy(1, Zerg_Zergling, Time(2, 40))
                || arrivesBy(8, Zerg_Zergling, Time(3, 00))
                || completesBy(1, Zerg_Zergling, Time(2, 05))
                || completesBy(1, Zerg_Spawning_Pool, Time(1, 40)))
                theSpy.opener.name = "4Pool";
            else if (arrivesBy(1, Zerg_Zergling, Time(2, 50))
                || completesBy(1, Zerg_Zergling, Time(2, 15))
                || completesBy(1, Zerg_Spawning_Pool, Time(1, 50)))
                theSpy.opener.name = "7Pool";
            else if (arrivesBy(1, Zerg_Zergling, Time(2, 55))
                || arrivesBy(8, Zerg_Zergling, Time(3, 15))
                || completesBy(1, Zerg_Zergling, Time(2, 25))
                || completesBy(8, Zerg_Zergling, Time(3, 00))
                || completesBy(10, Zerg_Zergling, Time(3, 20))
                || completesBy(12, Zerg_Zergling, Time(3, 30))
                || completesBy(14, Zerg_Zergling, Time(3, 40))
                || arrivesBy(16, Zerg_Zergling, Time(4, 20))
                || completesBy(1, Zerg_Spawning_Pool, Time(2, 00))
                || completesBy(1, Zerg_Lair, Time(3, 45)))
                theSpy.opener.name = "9Pool";
            else if (arrivesBy(1, Zerg_Zergling, Time(3, 05))
                || arrivesBy(8, Zerg_Zergling, Time(3, 20))
                || completesBy(1, Zerg_Zergling, Time(2, 40)))
                theSpy.opener.name = "Overpool";
            else if (arrivesBy(1, Zerg_Zergling, Time(3, 30))
                || arrivesBy(8, Zerg_Zergling, Time(3, 35))
                || completesBy(1, Zerg_Zergling, Time(3, 00))
                || completesBy(1, Zerg_Spawning_Pool, Time(2, 00)))
                theSpy.opener.name = "12Pool";

            // Hatch timing
            else if (completesBy(2, Zerg_Hatchery, Time(2, 35)))
                theSpy.opener.name = "9Hatch";
            else if (completesBy(2, Zerg_Hatchery, Time(2, 45)))
                theSpy.opener.name = "10Hatch";
            else if (completesBy(2, Zerg_Hatchery, Time(3, 05))
                || completesBy(6, Zerg_Zergling, Time(3, 15))
                || arrivesBy(6, Zerg_Zergling, Time(4, 00))
                || arrivesBy(10, Zerg_Zergling, Time(4, 20)))
                theSpy.opener.name = "12Hatch";
        }

        void enemyZergTransitions(PlayerInfo& player, StrategySpy& theSpy)
        {
            // Ling rush detection
            if (theSpy.opener.name == "4Pool"
                || theSpy.opener.name == "7Pool"
                || (!Players::ZvZ() && theSpy.opener.name == "9Pool" && Util::getTime() > Time(3, 30) && Util::getTime() < Time(4, 30) && Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 0 && theSpy.productionCount == 1))
                theSpy.transition.name = "LingRush";

            // Zergling all-in transitions
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 0 && Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 12) {

                // General
                if (completesBy(1, UpgradeTypes::Metabolic_Boost, Time(4, 15)) && arrivesBy(20, Zerg_Zergling, Time(4, 30))
                    || (Util::getTime() < Time(5, 00) && Scouts::gotFullScout() && Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone) <= 8 && theSpy.build.name == "PoolHatch" && (theSpy.opener.name == "9Pool" || theSpy.opener.name == "Overpool")))
                    theSpy.transition.name = "2HatchSpeedling";
                else if (!completesBy(1, UpgradeTypes::Metabolic_Boost, Time(4, 15)) && completesBy(1, UpgradeTypes::Metabolic_Boost, Time(5, 00)) && arrivesBy(28, Zerg_Zergling, Time(5, 10)))
                    theSpy.transition.name = "3HatchSpeedling";

                // ZvZ
                if (Players::ZvZ()) {
                    if (completesBy(1, UpgradeTypes::Zerg_Melee_Attacks, Time(5, 30)))
                        theSpy.transition.name = "+1Ling";
                }
            }

            // Hydralisk/Lurker build detection
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Spire) == 0 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk_Den) > 0) {
                if (theSpy.productionCount == 3)
                    theSpy.transition.name = "3HatchHydra";
                else if (theSpy.productionCount == 2)
                    theSpy.transition.name = "2HatchHydra";
                else if (theSpy.productionCount == 1)
                    theSpy.transition.name = "1HatchHydra";
                else if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) + Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) == 2)
                    theSpy.transition.name = "2HatchLurker";
                else if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair) == 1 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) == 0)
                    theSpy.transition.name = "1HatchLurker";
                else if (theSpy.productionCount >= 4)
                    theSpy.transition.name = "5HatchHydra";
            }

            // Mutalisk transition detection
            if (Players::getVisibleCount(PlayerState::Enemy, Zerg_Hydralisk_Den) == 0) {

                // General
                if (completesBy(1, Zerg_Lair, Time(3, 45)) || completesBy(1, Zerg_Spire, Time(5, 15)) || arrivesBy(1, Zerg_Mutalisk, Time(6, 00)))
                    theSpy.transition.name = "1HatchMuta";
                else if (completesBy(1, Zerg_Spire, Time(5, 45)) || arrivesBy(1, Zerg_Mutalisk, Time(6, 30)))
                    theSpy.transition.name = "2HatchMuta";
                else if (completesBy(1, Zerg_Spire, Time(6, 15)) || arrivesBy(1, Zerg_Mutalisk, Time(7, 00)))
                    theSpy.transition.name = "3HatchMuta";

                // ZvZ
                if (Players::ZvZ()) {
                    if (!theSpy.expand.confirmed && theSpy.productionCount == 1 && Util::getTime() > Time(5, 30))
                        theSpy.transition.name = "1HatchMuta";
                    else if (theSpy.expand.confirmed && theSpy.productionCount == 2 && Util::getTime() > Time(6, 00))
                        theSpy.transition.name = "2HatchMuta";
                }
            }
        }
    }

    void enemyZergMisc(PlayerInfo& player, StrategySpy& theSpy)
    {
        // Turtle detection
        if (completesBy(1, Zerg_Sunken_Colony, Time(3, 00)) || completesBy(2, Zerg_Sunken_Colony, Time(4, 00)) || completesBy(3, Zerg_Sunken_Colony, Time(5, 30)))
            theSpy.turtle.possible = true;
        
        // Fortress detection
        if (completesBy(3, Zerg_Sunken_Colony, Time(8, 00)) || completesBy(3, Zerg_Spore_Colony, Time(8, 00)))
            theSpy.fortress.possible = true;
    }

    void updateZerg(StrategySpy& theSpy)
    {
        for (auto &p : Players::getPlayers()) {
            PlayerInfo &player = p.second;
            if (player.isEnemy() && player.getCurrentRace() == Races::Zerg) {
                if (!theSpy.build.confirmed || theSpy.build.changeable)
                    enemyZergBuilds(player, theSpy);
                if (!theSpy.opener.confirmed || theSpy.opener.changeable)
                    enemyZergOpeners(player, theSpy);
                if (!theSpy.transition.confirmed || theSpy.transition.changeable)
                    enemyZergTransitions(player, theSpy);
                enemyZergMisc(player, theSpy);
            }
        }
    }

    bool enemyFasterPool() {
        auto enemyTiming = poolTimings.find(Spy::getEnemyOpener());
        auto selfTiming = poolTimings.find(BuildOrder::getCurrentOpener());

        if (enemyTiming == poolTimings.end() || selfTiming == poolTimings.end())
            return false;        

        // Self timing is later than threshold, enemy is faster
        return selfTiming->second - enemyTiming->second > equalPoolTiming;
    }

    bool enemyEqualPool() {
        auto enemyTiming = poolTimings.find(Spy::getEnemyOpener());
        auto selfTiming = poolTimings.find(BuildOrder::getCurrentOpener());

        if (enemyTiming == poolTimings.end() || selfTiming == poolTimings.end())
            return false;        

        // Self timing and enemy timing within threshold, equal
        return (enemyTiming->second > selfTiming->second && enemyTiming->second - selfTiming->second < equalPoolTiming)
            || (enemyTiming->second <= selfTiming->second && selfTiming->second - enemyTiming->second < equalPoolTiming);
    }

    bool enemySlowerPool() {
        auto enemyTiming = poolTimings.find(Spy::getEnemyOpener());
        auto selfTiming = poolTimings.find(BuildOrder::getCurrentOpener());

        if (enemyTiming == poolTimings.end() || selfTiming == poolTimings.end())
            return false;        

        // Enemy timing is later than threshold, self is faster
        return enemyTiming->second - selfTiming->second > equalPoolTiming;
    }

    // TODO: Use pool timings
    bool enemyFasterSpeed() {
        return (Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost, 1) && !Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1))
            || (enemyFasterPool() && !Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1));
    }
    
    // TODO: Equal speed

    // TODO: Use pool timings
    bool enemySlowerSpeed() {
        return (!Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Metabolic_Boost, 1) && Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1))
            || (enemySlowerPool() && Players::hasUpgraded(PlayerState::Self, UpgradeTypes::Metabolic_Boost, 1));
    }
}