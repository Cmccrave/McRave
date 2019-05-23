#pragma once
#include <BWAPI.h>
#include "bwem.h"
#include "BWEB.h"

#define STORM_LIMIT 0.75
#define STASIS_LIMIT 2.0
#define LOW_SHIELD_PERCENT_LIMIT 0.2
#define LOW_MECH_PERCENT_LIMIT 0.2
#define LOW_BIO_PERCENT_LIMIT 0.3
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

    struct Strength {
        double airToAir = 0.0;
        double airToGround = 0.0;
        double groundToAir = 0.0;
        double groundToGround = 0.0;
        double airDefense = 0.0;
        double groundDefense = 0.0;

        void operator+= (const Strength &second) {
            airToAir += second.airToAir;
            airToGround += second.airToGround;
            groundToAir += second.groundToAir;
            groundToGround += second.groundToGround;
            airDefense += second.airDefense;
            groundDefense += second.groundDefense;
        }

        void clear() {
            airToAir = 0.0;
            airToGround = 0.0;
            groundToAir = 0.0;
            groundToGround = 0.0;
            airDefense = 0.0;
            groundDefense = 0.0;
        }
    };

    enum class Role {
        None, Worker, Combat, Transport, Scout, Production, Defender, Support
    };

    // Maybe use this?
    enum class CombatType {
        None, AirToAir, GroundToGround, AirToGround, GroundToAir
    };

    enum class TransportState {
        None, Loading, Unloading, Monitoring, Engaging, Retreating
    };

    enum class ResourceState {
        None, Assignable, Mineable, Stealable
    };

    enum class GlobalState {
        None, Attack, Retreat
    };

    enum class LocalState {
        None, Attack, Retreat
    };

    enum class SimState {
        None, Win, Loss, HighWin, HighLoss
    };

    enum class PlayerState {
        None, Self, Ally, Enemy, Neutral
    };
}

#include "Horizon.h"
#include "BuildingManager.h"
#include "BuildOrder.h"
#include "CommandManager.h"
#include "CombatManager.h"
#include "GoalManager.h"
#include "GridManager.h"
#include "StationManager.h"
#include "WorkerManager.h"
#include "LearningManager.h"
#include "PlayerManager.h"
#include "ProductionManager.h"
#include "PylonManager.h"
#include "ResourceInfo.h"
#include "ResourceManager.h"
#include "ScoutManager.h"
#include "StrategyManager.h"
#include "SupportManager.h"
#include "TargetManager.h"
#include "TerrainManager.h"
#include "TransportManager.h"
#include "UnitManager.h"
#include "UnitMath.h"
#include "UnitInfo.h"
#include "Util.h"
#include "Visuals.h"

namespace
{
    auto &mapBWEM = BWEM::Map::Instance();
}

// Has to be after the includes to prevent compiler error
// TODO: Fix includes and move this up with the States
namespace McRave {
    static int vis(BWAPI::UnitType t) {
        return Units::getMyVisible(t);
    }
    static int com(BWAPI::UnitType t) {
        return Units::getMyComplete(t);
    }
    //static int mins() {
    //    return Broodwar->self()->minerals();
    //}
    //static int gas() {
    //    return Broodwar->self()->gas();
    //}
}