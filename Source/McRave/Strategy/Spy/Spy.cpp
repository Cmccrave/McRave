#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Spy {

    namespace {
        StrategySpy theSpy; // TODO: Impl multiple races and players
    }

    bool finishedSooner(UnitType t1, UnitType t2)
    {
        if (theSpy.unitTimings.find(t1) == theSpy.unitTimings.end())
            return false;
        return (theSpy.unitTimings.find(t2) == theSpy.unitTimings.end()) || (theSpy.unitTimings[t1].firstCompletedWhen < theSpy.unitTimings[t2].firstCompletedWhen);
    }

    bool startedEarlier(UnitType t1, UnitType t2)
    {
        if (theSpy.unitTimings.find(t1) == theSpy.unitTimings.end())
            return false;
        return (theSpy.unitTimings.find(t2) == theSpy.unitTimings.end()) || (theSpy.unitTimings[t1].firstStartedWhen < theSpy.unitTimings[t2].firstStartedWhen);
    }

    bool completesBy(int count, UnitType type, Time beforeThis)
    {
        int current = 0;
        for (auto &time : theSpy.unitTimings[type].countCompletedWhen) {
            if (time <= beforeThis)
                current++;
        }
        return current >= count;
    }

    bool completesBy(int count, UpgradeType type, Time beforeThis)
    {
        int current = 0;
        for (auto &time : theSpy.upgradeTimings[type].countCompletedWhen) {
            if (time <= beforeThis)
                current++;
        }
        return current >= count;
    }

    bool completesBy(int count, TechType type, Time beforeThis)
    {
        int current = 0;
        for (auto &time : theSpy.researchTimings[type].countCompletedWhen) {
            if (time <= beforeThis)
                current++;
        }
        return current >= count;
    }

    bool arrivesBy(int count, UnitType type, Time beforeThis)
    {
        int current = 0;
        for (auto &time : theSpy.unitTimings[type].countArrivesWhen) {
            if (time <= beforeThis)
                current++;
        }
        return current >= count;
    }
    
    void onFrame()
    {
        if (Players::vFFA())
            return;

        General::updateGeneral(theSpy);
        Protoss::updateProtoss(theSpy);
        Terran::updateTerran(theSpy);
        Zerg::updateZerg(theSpy);

        // Verify strategy checking for confirmations
        for (auto &strat : theSpy.strats) {
            strat->debugLog();
            if (!strat->confirmed)
                strat->updateStrat();
        }
        for (auto &blueprint : theSpy.blueprints) {
            blueprint->debugLog();
            if (!blueprint->confirmed)
                blueprint->updateBlueprint();
        }

        // Set a timestamp for when we detected a piece of the enemy build order
        if (theSpy.build.confirmed && theSpy.buildTime == Time(999, 00))
            theSpy.buildTime = Util::getTime();
        if (theSpy.opener.confirmed && theSpy.openerTime == Time(999, 00))
            theSpy.openerTime = Util::getTime();
        if (theSpy.transition.confirmed && theSpy.transitionTime == Time(999, 00))
            theSpy.transitionTime = Util::getTime();
    }

    string getEnemyBuild() { return theSpy.build.name; }
    string getEnemyOpener() { return theSpy.opener.name; }
    string getEnemyTransition() { return theSpy.transition.name; }
    Time getEnemyBuildTime() { return theSpy.buildTime; }
    Time getEnemyOpenerTime() { return theSpy.openerTime; }
    Time getEnemyTransitionTime() { return theSpy.transitionTime; }
    bool enemyFastExpand() { return theSpy.expand.confirmed; }
    bool enemyRush() { return theSpy.rush.confirmed; }
    bool enemyInvis() { return theSpy.invis.confirmed; }
    bool enemyPossibleProxy() { return theSpy.early.confirmed; }
    bool enemyProxy() { return theSpy.proxy.confirmed; }
    bool enemyGasSteal() { return theSpy.steal.confirmed; }
    bool enemyPressure() { return theSpy.pressure.confirmed; }
    bool enemyWalled() { return theSpy.wall.confirmed; }
    bool enemyGreedy() { return theSpy.greedy.confirmed; }
    bool enemyTurtle() { return theSpy.turtle.confirmed; }
    bool enemyFortress() { return theSpy.fortress.confirmed; }
    int getWorkersPulled() { return theSpy.workersPulled; }
    int getEnemyGasMined() { return theSpy.gasMined; }
}