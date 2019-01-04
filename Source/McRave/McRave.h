#pragma once
#include <BWAPI.h>
#include "bwem.h"
#include "BWEB.h"

#define STORM_LIMIT 5.0
#define STASIS_LIMIT 8.0
#define LOW_SHIELD_PERCENT_LIMIT 0.2
#define LOW_MECH_PERCENT_LIMIT 0.2
#define MIN_THREAT 0.01f
#define SIM_RADIUS 640.0
#define MAX_SCARAB 5 + (BWAPI::Broodwar->self()->getUpgradeLevel(UpgradeTypes::Reaver_Capacity) * 5)
#define MAX_INTERCEPTOR 4 + (BWAPI::Broodwar->self()->getUpgradeLevel(UpgradeTypes::Carrier_Capacity) * 4)

namespace McRave
{
    class UnitInfo;
    class ResourceInfo;
    class PlayerInfo;

    struct Line {
        double yInt;
        double slope;
        double y(int x) { return (slope * double(x)) + yInt; }
        Line(double y, double s) {
            yInt = y, slope = s;
        }
    };

    enum class Role {
        None, Working, Fighting, Transporting, Scouting, Producing, Defending, Supporting
    };

    enum class TransportState {
        None, Loading, Unloading, Monitoring, Engaging, Retreating
    };

    enum class ResourceState {
        None, Assignable, Mineable
    };

    enum class GlobalState {
        None, Engaging, Retreating
    };

    enum class LocalState {
        None, Engaging, Retreating
    };

    enum class SimState {
        None, Win, Loss, HighWin, HighLoss
    };

    static int vis(BWAPI::UnitType t) {
        return BWAPI::Broodwar->self()->visibleUnitCount(t);
    }
    static int com(BWAPI::UnitType t) {
        return BWAPI::Broodwar->self()->completedUnitCount(t);
    }
}

namespace
{
    auto &mapBWEM = BWEM::Map::Instance();
}

// Namespaces
using namespace McRave;

#include "BuildingManager.h"
#include "BuildOrder.h"
#include "CommandManager.h"
#include "GoalManager.h"
#include "GridManager.h"
#include "StationManager.h"
#include "WorkerManager.h"
#include "PlayerManager.h"
#include "ProductionManager.h"
#include "PylonManager.h"
#include "ResourceManager.h"
#include "ScoutManager.h"
#include "StrategyManager.h"
#include "SupportManager.h"
#include "TargetManager.h"
#include "TerrainManager.h"
#include "TransportManager.h"
#include "UnitManager.h"
#include "UnitInfo.h"
#include "Util.h"
#include "Visuals.h"