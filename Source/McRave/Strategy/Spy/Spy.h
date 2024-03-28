#pragma once
#include <BWAPI.h>

namespace McRave::Spy {

    struct Strat {
        bool possible = false;
        bool confirmed = false;
        bool changeable = false;
        int framesTrue = 0;
        int framesRequired = 0;
        int framesChangeable = 200;
        std::string name = "Unknown";

        void debugLog() {
            McRave::easyWrite(name + " confirmed at " + Util::getTime().toString());
        }

        void updateStrat() {
            possible ? framesTrue++ : framesTrue = 0;
            if (framesTrue > framesRequired) {
                debugLog();
                confirmed = true;
            }
            else {
                possible = false;
            }
            changeable = framesTrue < framesChangeable;
        }

        void updateBlueprint() {
            name != "Unknown" ? framesTrue++ : framesTrue = 0;
            if (framesTrue > framesRequired) {
                debugLog();
                confirmed = true;
            }
            else {
                name = "Unknown";
            }
            changeable = framesTrue < framesChangeable;
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
        std::map<BWAPI::UnitType, UnitTimings> enemyTimings;
        std::map<BWAPI::UpgradeType, int> upgradeLevel;
        std::map<BWAPI::TechType, bool> techResearched; // TODO: Impl
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

    namespace Protoss {
        void updateProtoss(StrategySpy&);
    }
    namespace Terran {
        void updateTerran(StrategySpy&);
    }
    namespace Zerg {
        void updateZerg(StrategySpy&);        
    }
    namespace General {
        void updateGeneral(StrategySpy&);
    }

    bool finishedSooner(BWAPI::UnitType, BWAPI::UnitType);
    bool startedEarlier(BWAPI::UnitType, BWAPI::UnitType);
    bool completesBy(int, BWAPI::UnitType, Time);
    bool arrivesBy(int, BWAPI::UnitType, Time);
    void onFrame();

    std::string getEnemyBuild();
    std::string getEnemyOpener();
    std::string getEnemyTransition();
    Time getEnemyBuildTime();
    Time getEnemyOpenerTime();
    Time getEnemyTransitionTime();
    Time whenArrival(int, BWAPI::UnitType);
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
}
