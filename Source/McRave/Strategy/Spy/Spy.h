#pragma once
#include <BWAPI.h>

namespace McRave::Spy {

    struct Strat {
        bool possible = false;
        bool confirmed = false;
        bool changeable = false;
        bool loggedPossible = false;
        bool loggedConfirmed = false;
        int framesTrue = 0;
        int framesRequired = 0;
        int framesChangeable = 200;
        std::string name = "Unknown";

        void debugLog() {
            if (possible && !loggedPossible) {
                Util::debug("[Spy]: " + name + " possible.");
                loggedPossible = true;
            }
            if (confirmed && !loggedConfirmed) {
                Util::debug("[Spy]: " + name + " confirmed.");
                loggedConfirmed = true;
            }
        }

        void updateStrat() {
            possible ? framesTrue++ : framesTrue = 0;            
            if (framesTrue > framesRequired)
                confirmed = true;
            else
                possible = false;
        }

        void updateBlueprint() {
            if (name != "Unknown") {
                framesTrue++;
                possible = true;
            }
            else
                framesTrue = 0;

            if (framesTrue > framesRequired)
                confirmed = true;
            else {
                name = "Unknown";
                possible = false;
            }
        }
    };

    struct UnitTimings {
        std::vector<Time> countStartedWhen;
        std::vector<Time> countCompletedWhen;
        std::vector<Time> countArrivesWhen;
        Time firstStartedWhen;
        Time firstCompletedWhen;
        Time firstArrivesWhen;
    };

    struct StrategySpy { // TODO: Impl multiple players

        Strat build, opener, transition, expand, rush, wall, proxy, early, steal, pressure, greedy, invis, allin, turtle;
        Time buildTime, openerTime, transitionTime, rushArrivalTime;
        std::vector<Strat*> strats;
        std::vector<Strat*> blueprints;
        std::map<BWAPI::UnitType, UnitTimings> unitTimings;
        std::map<BWAPI::UpgradeType, UnitTimings> upgradeTimings;
        std::map<BWAPI::TechType, UnitTimings> researchTimings; // TODO: Impl
        int workersPulled = 0;
        int gasMined = 0;
        int productionCount = 0;
        std::set<BWAPI::UnitType> typeUpgrading; // TODO: Better impl (doesn't look at current state)

        StrategySpy() {
            strats ={ &expand, &rush, &wall, &proxy, &early, &steal, &pressure, &greedy, &invis, &allin, &turtle };
            blueprints ={ &build, &opener, &transition };
            build.framesRequired = 24;
            build.framesChangeable = 500;
            opener.framesRequired = 24;
            opener.framesChangeable = 500;
            transition.framesRequired = 240;
            transition.framesChangeable = 500;

            // Attaching names to strats for logging purposes
            expand.name = "Expand";
            rush.name = "Rush";
            wall.name = "Wall";
            proxy.name = "Proxy";
            early.name = "Early";
            steal.name = "Steal";
            pressure.name = "Pressure";
            greedy.name = "Greedy";
            invis.name = "Invis";
            allin.name = "Allin";
            turtle.name = "Turtle";
        }
    };

    bool finishedSooner(BWAPI::UnitType, BWAPI::UnitType);
    bool startedEarlier(BWAPI::UnitType, BWAPI::UnitType);
    bool completesBy(int, BWAPI::UnitType, Time);
    bool completesBy(int, BWAPI::UpgradeType, Time);
    bool completesBy(int, BWAPI::TechType, Time);
    bool arrivesBy(int, BWAPI::UnitType, Time);
    void onFrame();

    std::string getEnemyBuild();
    std::string getEnemyOpener();
    std::string getEnemyTransition();
    Time getEnemyBuildTime();
    Time getEnemyOpenerTime();
    Time getEnemyTransitionTime();
    bool enemyFastExpand();
    bool enemyRush();
    bool enemyInvis();
    bool enemyPossibleProxy();
    bool enemyProxy();
    bool enemyGasSteal();
    bool enemyPressure();
    bool enemyWalled();
    bool enemyGreedy();
    bool enemyTurtle();
    int getWorkersPulled();
    int getEnemyGasMined();

    namespace Protoss {
        void updateProtoss(StrategySpy&);
    }
    namespace Terran {
        void updateTerran(StrategySpy&);
    }
    namespace Zerg {
        void updateZerg(StrategySpy&);

        bool enemyFasterPool();
        bool enemyEqualPool();
        bool enemyFasterSpeed();
    }
    namespace General {
        void updateGeneral(StrategySpy&);
    }
}
