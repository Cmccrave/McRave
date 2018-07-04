#pragma once
// Include API files
#include <BWAPI.h>
#include "Singleton.h"
#include "..\BWEM\bwem.h"
#include "..\BWEB\BWEB.h"

#define STORM_LIMIT 5.0
#define STASIS_LIMIT 8.0

// Namespaces
using namespace BWAPI;
using namespace BWEM;
using namespace std;
using namespace BWEB;

namespace McRave
{
	class UnitInfo;
	class WorkerInfo;
}

namespace
{
	auto &mapBWEM = BWEM::Map::Instance();
	auto &mapBWEB = BWEB::Map::Instance();
}

// Include standard libraries that are needed
#include <set>
#include <ctime>
#include <chrono>

// Include other source files
#include "BuildingManager.h"
#include "BuildOrder.h"
#include "CommandManager.h"
#include "GridManager.h"
#include "Interface.h"
#include "StationManager.h"
#include "WorkerManager.h"
#include "PlayerManager.h"
#include "ProductionManager.h"
#include "PylonManager.h"
#include "ResourceManager.h"
#include "StrategyManager.h"
#include "TargetManager.h"
#include "TerrainManager.h"
#include "TransportManager.h"
#include "UnitManager.h"
#include "Util.h"

// Namespace to access all managers globally
namespace McRave
{
	inline BuildingManager& Buildings() { return BuildingSingleton::Instance(); }
	inline BuildOrderManager& BuildOrder() { return BuildOrderSingleton::Instance(); }
	inline CommandManager& Commands() { return CommandSingleton::Instance(); }
	inline GridManager& Grids() { return GridSingleton::Instance(); }
	inline InterfaceManager& Display() { return InterfaceSingleton::Instance(); }
	inline StationManager& Stations() { return StationSingleton::Instance(); }
	inline PlayerManager& Players() { return PlayerSingleton::Instance(); }
	inline ProductionManager& Production() { return ProductionSingleton::Instance(); }
	inline PylonManager& Pylons() { return PylonSingleton::Instance(); }
	inline ResourceManager& Resources() { return ResourceSingleton::Instance(); }
	inline StrategyManager& Strategy() { return StrategySingleton::Instance(); }
	inline TargetManager& Targets() { return TargetSingleton::Instance(); }
	inline TerrainManager& Terrain() { return TerrainSingleton::Instance(); }
	inline TransportManager& Transport() { return TransportSingleton::Instance(); }
	inline UnitManager& Units() { return UnitSingleton::Instance(); }
	inline UtilManager& Util() { return UtilSingleton::Instance(); }
	inline WorkerManager& Workers() { return WorkerSingleton::Instance(); }
}
using namespace McRave;
