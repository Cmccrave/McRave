#pragma once
#include <BWAPI.h>

namespace McRave::Combat {

    enum class Shape {
        None, Concave, Line, Box
    };

    struct Cluster {
        BWAPI::Position sharedPosition, sharedRetreat, sharedObjective;
        std::map<BWAPI::UnitType, int> typeCounts;
        double sharedRadius = 160.0;
        Shape shape = Shape::Concave;
        std::vector<std::weak_ptr<UnitInfo>> units;
        std::weak_ptr<UnitInfo> commander;
        bool mobileCluster = false;

        Cluster(BWAPI::Position _sp, BWAPI::Position _sr, BWAPI::Position _so, BWAPI::UnitType _t) {
            sharedPosition = _sp;
            sharedRetreat = _sr;
            sharedObjective = _so;
            typeCounts[_t] = 1;
        }
        Cluster() {};
    };

    struct Formation {
        Cluster cluster;
        BWAPI::Position center;
        std::vector<BWAPI::Position> positions;
    };

    namespace Formations {
        void onFrame();
        std::vector<Formation>& getConcaves();
    }
    namespace Clusters {
        void onFrame();
        std::vector<Cluster>& getClusters();
    }

    void onFrame();

    bool defendChoke();
    std::multimap<double, BWAPI::Position>& getCombatClusters();
    BWAPI::Position getAirClusterCenter();
    std::set<BWAPI::Position>& getDefendPositions();
    std::multimap<double, UnitInfo&> getCombatUnitsByDistance();
}