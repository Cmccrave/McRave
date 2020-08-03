#pragma once
#include <BWAPI.h>
#include "bwem.h"
#include "BWEB.h"

#define STORM_LIMIT 2.0
#define STASIS_LIMIT 4.0
#define LOW_SHIELD_PERCENT_LIMIT 0.5
#define LOW_MECH_PERCENT_LIMIT 0.25
#define LOW_BIO_PERCENT_LIMIT 0.25
#define MIN_THREAT 0.01f
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

    struct Time {
        int minutes;
        int seconds;
        int frames;
        Time(int m, int s) {
            minutes = m, seconds = s;
            frames = ((m * 60) + s) * 24;
        }
        Time() {
            minutes = 999;
            seconds = 0;
        }

        bool operator< (const Time t2) {
            return (minutes < t2.minutes)
                || (minutes == t2.minutes && seconds < t2.seconds);
        }

        bool operator> (const Time t2) {
            return (minutes > t2.minutes)
                || (minutes == t2.minutes && seconds > t2.seconds);
        }

        bool operator== (const Time t2) {
            return minutes == t2.minutes && seconds == t2.seconds;
        }

        Time operator- (const Time t2) {
            auto secondsDiff = (minutes * 60 + seconds) - (t2.minutes * 60 + t2.seconds);
            return Time(secondsDiff / 60, secondsDiff % 60);
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
        None, Win, Loss
    };

    enum class PlayerState {
        None, Self, Ally, Enemy, Neutral
    };

    enum class ScoutState {
        None, Base, Proxy, Expand, Rush
    };
}

#include "Horizon.h"
#include "ActionManager.h"
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
#include "WallManager.h"

namespace
{
    auto &mapBWEM = BWEM::Map::Instance();
}

// Has to be after the includes to prevent compiler error
// TODO: Fix includes and move this up with the States
namespace McRave {

    /// Returns the self owned visible unit count of this UnitType
    static int vis(BWAPI::UnitType t) {
        return Units::getMyVisible(t);
    }

    /// Returns the self owned completed unit count of this UnitType
    static int com(BWAPI::UnitType t) {
        return Units::getMyComplete(t);
    }

    /// Returns the self total unit count of this UnitType
    static int total(BWAPI::UnitType t) {
        return Players::getTotalCount(PlayerState::Self, t);
    }

    static void easyWrite(std::string stuff)
    {
        std::ofstream writeFile;
        writeFile.open("bwapi-data/write/McRave_Debug_Log.txt", std::ios::app);
        writeFile << stuff << std::endl;
    }
}