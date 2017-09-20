#pragma once
// Include API files
#include <BWAPI.h>
#include "Singleton.h"
#include "..\BWEM\bwem.h"

// Namespaces
using namespace BWAPI;
using namespace std;
using namespace BWEM;

namespace
{
	auto & theMap = Map::Instance();
}

// Include standard libraries that are needed
#include <Windows.h>
#include <set>
#include <ctime>
#include <chrono>

// Forward declare classes needed in other header files than their defined header
class BaseInfo;
class UnitInfo;
class SupportUnitInfo;
class ResourceInfo;

// Include other source files
#include "BuildingManager.h"
#include "BuildOrder.h"
#include "CommandManager.h"
#include "GridManager.h"
#include "Interface.h"
#include "BaseManager.h"
#include "WorkerManager.h"
#include "PlayerManager.h"
#include "ProductionManager.h"
#include "PylonManager.h"
#include "ResourceManager.h"
#include "SpecialUnitManager.h"
#include "StrategyManager.h"
#include "TargetManager.h"
#include "TerrainManager.h"
#include "TransportManager.h"
#include "UnitManager.h"
#include "Util.h"

// Namespace to access all trackers globally
namespace McRave
{	
	inline BuildingTrackerClass& Buildings() { return BuildingTracker::Instance(); }
	inline BuildOrderTrackerClass& BuildOrder() { return BuildOrderTracker::Instance(); }
	inline CommandTrackerClass& Commands() { return CommandTracker::Instance(); }
	inline GridTrackerClass& Grids() { return GridTracker::Instance(); }
	inline InterfaceTrackerClass& Display() { return InterfaceTracker::Instance(); }
	inline BaseTrackerClass& Bases() { return BaseTracker::Instance(); }
	inline PlayerTrackerClass& Players() { return PlayerTracker::Instance(); }
	inline ProductionTrackerClass& Production() { return ProductionTracker::Instance(); }
	inline PylonTrackerClass& Pylons() { return PylonTracker::Instance(); }
	inline ResourceTrackerClass& Resources() { return ResourceTracker::Instance(); }
	inline SpecialUnitTrackerClass& SpecialUnits() { return SpecialUnitTracker::Instance(); }
	inline StrategyTrackerClass& Strategy() { return StrategyTracker::Instance(); }
	inline TargetTrackerClass& Targets() { return TargetTracker::Instance(); }
	inline TerrainTrackerClass& Terrain() { return TerrainTracker::Instance(); }
	inline TransportTrackerClass& Transport() { return TransportTracker::Instance(); }
	inline UnitTrackerClass& Units() { return UnitTracker::Instance(); }
	inline UtilTrackerClass& Util() { return UtilTracker::Instance(); }
	inline WorkerTrackerClass& Workers() { return WorkerTracker::Instance(); }
}
using namespace McRave;
