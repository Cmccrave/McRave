#pragma once
#include <BWAPI.h>

namespace McRave::Strategy {

    std::string getEnemyBuild();
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
    int enemyArrivalFrame();

    void onFrame();
}
