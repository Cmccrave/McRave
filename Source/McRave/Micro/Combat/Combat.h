#pragma once
#include <BWAPI.h>

namespace McRave::Combat {

    enum class Shape {
        None, Concave, Line, Box
    };

    enum class CommandShare {
        None, Exact, Parallel
    };

    struct Cluster {
        BWAPI::Position sharedPosition, sharedDestination, sharedNavigation;
        std::map<BWAPI::UnitType, int> typeCounts;
        double sharedRadius = 160.0;
        std::vector<UnitInfo*> units;
        std::weak_ptr<UnitInfo> commander;
        CommandShare commandShare;
        Shape shape;
        bool mobileCluster = false;
        BWAPI::Color color;
        BWEB::Path path;

        Cluster(BWAPI::Position _sp, BWAPI::Position _sd, BWAPI::UnitType _t) {
            sharedPosition = _sp;
            sharedDestination = _sd;
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