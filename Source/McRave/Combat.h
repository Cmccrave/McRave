#pragma once
#include <BWAPI.h>

namespace McRave::Combat {

    enum class Shape {
        None, Concave, Line, Box
    };

    struct Cluster {
        BWAPI::Position sharedPosition;
        BWAPI::Position sharedTarget;
        std::map<BWAPI::UnitType, int> typeCounts;
        double sharedRadius = 160.0;
        Shape shape;
        std::vector<std::weak_ptr<UnitInfo>> units;

        Cluster(BWAPI::Position _sp, BWAPI::Position _st, BWAPI::UnitType _t) {
            sharedPosition = _sp;
            sharedTarget = _st;
            typeCounts[_t] = 1;
        }
    };

    struct Formation {
        BWAPI::Position center;
        BWAPI::UnitType type;
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
    BWAPI::Position getClosestRetreatPosition(UnitInfo&);
    BWAPI::Position getAirClusterCenter();
    std::set<BWAPI::Position>& getDefendPositions();
    std::multimap<double, UnitInfo&> getCombatUnitsByDistance();
}