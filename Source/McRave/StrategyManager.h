#pragma once
#include <BWAPI.h>

namespace McRave::Strategy {

    std::string getEnemyBuild();
    bool enemyFastExpand();
    bool enemyRush();
    bool needDetection();
    bool defendChoke();
    bool enemyProxy();
    bool enemyGasSteal();
    bool enemyScouted();
    bool enemyBust();
    bool enemyPressure();
    int getPoolFrame();
    std::map <BWAPI::UnitType, double>& getUnitScores();

    void onFrame();
    double getUnitScore(BWAPI::UnitType);
    BWAPI::UnitType getHighestUnitScore();
}
