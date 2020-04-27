#pragma once
#include <BWAPI.h>

namespace McRave::Strategy {

    std::string getEnemyBuild();
    std::string getEnemyOpener();
    std::string getEnemyTransition();
    BWAPI::Position enemyScoutPosition();
    bool enemyFastExpand();
    bool enemyRush();
    bool needDetection();
    bool defendChoke();
    bool enemyAir();
    bool enemyPossibleProxy();
    bool enemyProxy();
    bool enemyGasSteal();
    bool enemyScouted();
    bool enemyBust();
    bool enemyPressure();
    bool enemyBlockedScout();
    Time enemyArrivalTime();

    void onFrame();
}
