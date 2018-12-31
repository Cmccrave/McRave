#pragma once
#include <BWAPI.h>
#include "Singleton.h"

namespace McRave
{
    class TransportManager
    {
        std::map <BWAPI::WalkPosition, int> recentExplorations;
    public:
        void onFrame();
        void updateTransports();
        void updateCargo(UnitInfo&);
        void updateDecision(UnitInfo&);
        void updateMovement(UnitInfo&);
        void removeUnit(BWAPI::Unit);
    };
}

typedef Singleton<McRave::TransportManager> TransportSingleton;
