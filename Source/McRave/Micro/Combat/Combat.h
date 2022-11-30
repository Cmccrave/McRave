#pragma once
#include <BWAPI.h>

namespace McRave::Combat {

    enum class Shape {
        None, Concave, Line, Box
    };

    enum class CommandShare {
        None, Exact, Parallel
    };

    struct ClusterNode {
        int id = 0;
        BWAPI::Position position = BWAPI::Positions::Invalid;
        UnitInfo * unit;
    };

    struct Cluster {
        BWAPI::Position avgPosition, marchPosition, retreatPosition, marchNavigation, retreatNavigation;
        std::map<BWAPI::UnitType, int> typeCounts;
        double radius;
        std::vector<UnitInfo*> units;
        std::weak_ptr<UnitInfo> commander;
        CommandShare commandShare;
        Shape shape;
        bool mobileCluster = false;
        BWAPI::Color color;
        BWEB::Path marchPath, retreatPath;

        Cluster(BWAPI::Position _avg, BWAPI::Position _march, BWAPI::Position _retreat, BWAPI::UnitType _t) {
            marchPosition = _march;
            retreatPosition = _retreat;
            typeCounts[_t] = 1;
        }
        Cluster() {};
    };

    struct Formation {
        Cluster* cluster;
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

    namespace Simulation {
        void onFrame();
    }

    namespace State {
        void onFrame();
        bool isStaticRetreat(BWAPI::UnitType);
    }

    namespace Decision {
        void onFrame();
    }

    namespace Destination {
        void onFrame();
    }

    namespace Navigation {
        void onFrame();
    }

    void onFrame();
    void onStart();
    bool defendChoke();
}