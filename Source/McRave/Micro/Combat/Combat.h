#pragma once
#include <BWAPI.h>

namespace McRave::Combat {

    enum class Shape {
        None, Concave, Line, Box, Choke
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
        std::vector<UnitInfo*> units;
        std::weak_ptr<UnitInfo> commander;
        CommandShare commandShare;
        Shape shape;
        double spacing = 32.0;
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
        double radius, leash;
        std::vector<BWAPI::Position> positions;
    };

    namespace Formations {
        void onFrame();
        std::vector<Formation>& getFormations();
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

    const BWEM::ChokePoint * getDefendChoke();
    const BWEM::Area * getDefendArea();
    BWEB::Station * getDefendStation();

    BWAPI::Position getAttackPosition();
    BWAPI::Position getDefendPosition();
    BWAPI::Position getHarassPosition();
    bool isDefendNatural();

    void onFrame();
    void onStart();
    bool holdAtChoke();
}