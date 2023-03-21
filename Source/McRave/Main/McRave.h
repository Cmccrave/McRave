#pragma once
#include <BWAPI.h>
#include "bwem.h"
#include "BWEB.h"

namespace McRave
{
    class Strength;
    class UnitInfo;
    class ResourceInfo;
    class PlayerInfo;

    /// Writes to a file, very slow function!
    static void easyWrite(std::string& stuff)
    {
        std::ofstream writeFile;
        writeFile.open("bwapi-data/write/McRave_Debug_Log.txt", std::ios::app);
        writeFile << stuff << std::endl;
    }

    struct Time {
        int minutes;
        int seconds;
        int frames;
        Time(int f) {
            frames = f;
            minutes = int(double(f) / 1428.6);
            seconds = int(double(f) / 23.81) % 60;
        }
        Time(int m, int s) {
            minutes = m, seconds = s;
            frames = int(23.81 * ((double(m) * 60.0) + double(s)));
        }
        Time() {
            minutes = 999;
            seconds = 0;
        }
        std::string toString() {
            return std::to_string(minutes) + ":" + (seconds < 10 ? "0" + std::to_string(seconds) : std::to_string(seconds));
        }

        bool operator< (const Time t2) {
            return (minutes < t2.minutes)
                || (minutes == t2.minutes && seconds < t2.seconds);
        }

        bool operator<= (const Time t2) {
            return (minutes < t2.minutes)
                || (minutes == t2.minutes && seconds <= t2.seconds);
        }

        bool operator> (const Time t2) {
            return (minutes > t2.minutes)
                || (minutes == t2.minutes && seconds > t2.seconds);
        }

        bool operator>= (const Time t2) {
            return (minutes > t2.minutes)
                || (minutes == t2.minutes && seconds >= t2.seconds);
        }

        bool operator== (const Time t2) {
            return minutes == t2.minutes && seconds == t2.seconds;
        }

        Time operator- (const Time t2) {
            auto op = (minutes * 60 + seconds) - (t2.minutes * 60 + t2.seconds);
            return Time(op / 60, op % 60);
        }

        Time operator+ (const Time t2) {
            auto op = (minutes * 60 + seconds) + (t2.minutes * 60 + t2.seconds);
            return Time(op / 60, op % 60);
        }
    };

    inline BWAPI::GameWrapper& operator<<(BWAPI::GameWrapper& bw, const Time& t) {
        bw << t.minutes << ":";
        t.seconds < 10 ? bw << "0" << t.seconds : bw << t.seconds;
        bw << std::endl;
        return bw;
    }




    enum class GoalType {
        None, Attack, Contain, Explore, Escort, Defend
    };

    enum class Role {
        None, Worker, Combat, Transport, Scout, Production, Defender, Support, Consumable
    };

    enum class TransportState {
        None, Loading, Engaging, Retreating, Reinforcing
    };

    enum class ResourceState {
        None, Assignable, Mineable, Stealable
    };

    enum class GlobalState {
        None, ForcedAttack, Attack, Hold, Retreat, ForcedRetreat
    };

    enum class LocalState {
        None, ForcedAttack, Attack, Hold, Retreat, ForcedRetreat
    };

    enum class SimState {
        None, Win, Loss
    };

    enum class PlayerState {
        None, Self, Ally, Enemy, Neutral, All
    };


    inline double logLookup16[1024]={};
}

#include "Horizon.h"
#include "Util.h"
#include "Visuals.h"

#include "Micro/All/Commands.h"
#include "Micro/Combat/Combat.h"
#include "Micro/Defender/Defender.h"
#include "Micro/Scout/Scouts.h"
#include "Micro/Support/Support.h"
#include "Micro/Transport/Transports.h"
#include "Micro/Worker/Workers.h"

#include "Info/Roles.h"
#include "Info/Building/Buildings.h"
#include "Info/Player/PlayerInfo.h"
#include "Info/Player/Players.h"
#include "Info/Resource/ResourceInfo.h"
#include "Info/Resource/Resources.h"
#include "Info/Unit/Pathing.h"
#include "Info/Unit/Targeting.h"
#include "Info/Unit/Units.h"
#include "Info/Unit/UnitMath.h"
#include "Info/Unit/UnitInfo.h"

#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Map/Walls/Walls.h"

#include "Builds/All/BuildOrder.h"
#include "Builds/All/Learning.h"

#include "Strategy/Actions/Actions.h"
#include "Strategy/Goals/Goals.h"
#include "Strategy/Spy/Spy.h"
#include "Strategy/Zones/Zones.h"

#include "Macro/Expanding/Expanding.h"
#include "Macro/Planning/Planning.h"
#include "Macro/Planning/Pylons.h"
#include "Macro/Producing/Producing.h"
#include "Macro/Researching/Researching.h"
#include "Macro/Upgrading/Upgrading.h"

namespace
{
    auto &mapBWEM = BWEM::Map::Instance();
}