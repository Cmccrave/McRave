#pragma once
#include <BWAPI.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "bwem.h"

using namespace BWAPI;

namespace McRave {
    class Strength;
    class UnitInfo;
    class UnitData;
    class UnitFrames;
    class ResourceInfo;
    class PlayerInfo;

    // clang-format off
    enum class Diagnostic { None, Goal, Destination, Command, Order };
    enum class GoalType { None, Attack, Contain, Explore, Escort, Defend, Block, Harass, Runby };
    enum class Role { None, Worker, Combat, Transport, Scout, Production, Defender, Support, Consumable };
    enum class TransportState { None, Loading, Engaging, Retreating, Reinforcing };
    enum class ResourceState { None, Assignable, Mineable, Stealable };
    enum class GlobalState { None, Attack, Hold, Retreat };
    enum class LocalState { None, Attack, Hold, Retreat };
    enum class SimState { None, Win, Loss };
    enum class PlayerState { None, Self, Ally, Enemy, Neutral, All };
    // clang-format on

    inline double log8Lookup[2048]    = {};
    inline double invLog8Lookup[2048] = {};
} // namespace McRave

namespace {
    auto &mapBWEM = BWEM::Map::Instance();
}

#include "Main/Helpers.h"
#include "Main/Time.h"
#include "Main/Util.h"
#include "Main/Visuals.h"
