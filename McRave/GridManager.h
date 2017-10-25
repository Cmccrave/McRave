#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

#pragma warning(disable : 4351)

class GridTrackerClass
{
	bool resetGrid[1024][1024] = {};

	// Ally grids
	double aGroundThreat[1024][1024] = {};
	double aAirThreat[1024][1024] = {};
	int aGroundClusterGrid[1024][1024] = {};
	int aAirClusterGrid[1024][1024] = {};
	int buildingGrid[256][256] = {};
	int baseGrid[256][256] = {};
	int pylonGrid[256][256] = {};
	int batteryGrid[256][256] = {};
	int bunkerGrid[256][256] = {};
	int defenseGrid[256][256] = {};

	// Enemy grids
	double eGroundThreat[1024][1024] = {};
	double eAirThreat[1024][1024] = {};
	int eDetectorGrid[1024][1024] = {};
	int eGroundClusterGrid[1024][1024] = {};
	int eAirClusterGrid[1024][1024] = {};
	int stasisClusterGrid[1024][1024] = {};

	// Neutral grids	
	int resourceGrid[256][256] = {};

	// Mobility grids
	int mobilityGrid[1024][1024] = {};
	int antiMobilityGrid[1024][1024] = {};
	int distanceGridHome[1024][1024] = {};
	int reservePathHome[256][256] = {};

	// Special Unit grids
	int aDetectorGrid[1024][1024] = {};
	int arbiterGrid[1024][1024] = {};
	int psiStormGrid[1024][1024] = {};
	int EMPGrid[1024][1024] = {};

	// Other
	bool distanceAnalysis = false;
	bool mobilityAnalysis = false;
	Position allyArmyCenter, enemyArmyCenter;
public:

	// Check if we are done analyzing stuff
	bool isAnalyzed() { return distanceAnalysis; }

	// Update functions
	void reset();
	void update();
	void updateAllyGrids();
	void updateEnemyGrids();
	void updateNeutralGrids();
	void updateMobilityGrids();

	// Unit and building based functions
	void updateArbiterMovement(UnitInfo&);
	void updateDetectorMovement(UnitInfo&);
	void updateAllyMovement(Unit, WalkPosition);

	// Update if there is a storm active on the map or we are attempting to cast one
	void updatePsiStorm(WalkPosition);
	void updatePsiStorm(Bullet);

	// Update if there is an EMP active on the map
	void updateEMP(Bullet);

	// On start functions
	void updateDistanceGrid();

	// Updates a resource if it is destroyed or created
	void updateResourceGrid(ResourceInfo&);

	// Updates a building if it is destroyed or created
	void updateBuildingGrid(BuildingInfo&);

	// Updates a base if it is destroyed or created
	void updateBaseGrid(BaseInfo&);

	// Updates a defense if it is destroyed or created
	void updateDefenseGrid(UnitInfo&);

	// Returns the combined ground strength of ally units within range and moving distance (based on how fast the unit is) of the given WalkPosition
	double getAGroundThreat(int x, int y) { return aGroundThreat[x][y]; }
	double getAGroundThreat(WalkPosition here) { return aGroundThreat[here.x][here.y]; }

	// Returns the combined air strength of ally units within range and moving distance (based on how fast the unit is) of the given WalkPosition
	double getAAirThreat(int x, int y) { return aAirThreat[x][y]; }
	double getAAirThreat(WalkPosition here) { return aAirThreat[here.x][here.y]; }

	// Returns the number of allied ground units within range of most area of effect abilities
	int getAGroundCluster(int x, int y) { return aGroundClusterGrid[x][y]; }
	int getAGroundCluster(WalkPosition here) { return aGroundClusterGrid[here.x][here.y]; }

	// Returns the number of allied air units within range of most area of effect abilities
	int getAAirCluster(int x, int y) { return aAirClusterGrid[x][y]; }
	int getAAirCluster(WalkPosition here) { return aAirClusterGrid[here.x][here.y]; }

	// Returns 1 if the given TilePosition has a building on it, 0 otherwise
	int getBuildingGrid(int x, int y) { return buildingGrid[x][y]; }
	int getBuildingGrid(TilePosition here) { return buildingGrid[here.x][here.y]; }

	// Returns 2 if the given TilePosition is within range of a base that is completed, 1 if constructing and 0 otherwise
	int getBaseGrid(int x, int y) { return baseGrid[x][y]; }
	int getBaseGrid(TilePosition here) { return baseGrid[here.x][here.y]; }

	// Returns 1 if a Pylon is within range of the given TilePosition, 0 otherwise
	int getPylonGrid(int x, int y) { return pylonGrid[x][y]; }
	int getPylonGrid(TilePosition here) { return pylonGrid[here.x][here.y]; }

	// Returns 1 if within range of an allied Shield Battery of the given TilePosition, 0 otherwise
	int getBatteryGrid(int x, int y) { return batteryGrid[x][y]; }
	int getBatteryGrid(TilePosition here) { return batteryGrid[here.x][here.y]; }

	// Returns 1 if within range of an allied Bunker of the given TilePosition, 0 otherwise
	int getBunkerGrid(int x, int y) { return bunkerGrid[x][y]; }
	int getBunkerGrid(TilePosition here) { return bunkerGrid[here.x][here.y]; }

	// Returns the number of static defenses within range of the given TilePosition
	int getDefenseGrid(int x, int y) { return defenseGrid[x][y]; }
	int getDefenseGrid(TilePosition here) { return defenseGrid[here.x][here.y]; }

	// Returns the combined ground strength of enemy units within range and moving distance (based on how fast the unit is) of the given WalkPosition
	double getEGroundThreat(int x, int y) { return eGroundThreat[x][y]; }
	double getEGroundThreat(WalkPosition here) { return eGroundThreat[here.x][here.y]; }

	// Returns the combined air strength of enemy units within range and moving distance (based on how fast the unit is) of the given WalkPosition
	double getEAirThreat(int x, int y) { return eAirThreat[x][y]; }
	double getEAirThreat(WalkPosition here) { return eAirThreat[here.x][here.y]; }

	// Returns 1 if there is enemy detection on the given walk position, 0 otherwise
	int getEDetectorGrid(int x, int y) { return eDetectorGrid[x][y]; }
	int getEDetectorGrid(WalkPosition here) { return eDetectorGrid[here.x][here.y]; }

	// Returns the number of enemy ground units within range of most area of effect abilities
	int getEGroundCluster(int x, int y) { return eGroundClusterGrid[x][y]; }
	int getEGroundCluster(WalkPosition here) { return eGroundClusterGrid[here.x][here.y]; }

	// Returns the number of enemy air units within range of most area of effect abilities
	int getEAirCluster(int x, int y) { return eAirClusterGrid[x][y]; }
	int getEAirCluster(WalkPosition here) { return eAirClusterGrid[here.x][here.y]; }

	// Returns the number of valuable stasis targets within range of stasis
	int getStasisCluster(int x, int y) { return stasisClusterGrid[x][y]; }
	int getStasisCluster(WalkPosition here) { return stasisClusterGrid[here.x][here.y]; }

	// Returns 1 if the tile is between a resource and a base, 0 otherwise
	int getResourceGrid(int x, int y) { return resourceGrid[x][y]; }
	int getResourceGrid(TilePosition here) { return resourceGrid[here.x][here.y]; }

	// Returns the ground mobility of the given walk position
	int getMobilityGrid(int x, int y) { return mobilityGrid[x][y]; }
	int getMobilityGrid(WalkPosition here) { return mobilityGrid[here.x][here.y]; }

	// Returns 1 if a unit would reduce the mobility of a tile, 0 otherwise
	int getAntiMobilityGrid(int x, int y) { return antiMobilityGrid[x][y]; }
	int getAntiMobilityGrid(WalkPosition here) { return antiMobilityGrid[here.x][here.y]; }

	// Returns the ground distance from the given WalkPosition to the players starting position, -1 if not reachable on ground
	int getDistanceHome(int x, int y) { return distanceGridHome[x][y]; }
	int getDistanceHome(WalkPosition here) { return distanceGridHome[here.x][here.y]; }

	// Returns 1 if an allied Observer is within range of the given WalkPosition, 0 otherwise
	int getADetectorGrid(int x, int y) { return aDetectorGrid[x][y]; }
	int getADetectorGrid(WalkPosition here) { return aDetectorGrid[here.x][here.y]; }

	// Returns 1 if an allied Arbiter is within range of the given WalkPosition, 0 otherwise
	int getArbiterGrid(int x, int y) { return arbiterGrid[x][y]; }
	int getArbiterGrid(WalkPosition here) { return arbiterGrid[here.x][here.y]; }

	// Returns 1 if an active psi storm exists or will exist at this location
	int getPsiStormGrid(int x, int y) { return psiStormGrid[x][y]; }
	int getPsiStormGrid(WalkPosition here) { return psiStormGrid[here.x][here.y]; }

	// Returns 1 if an active EMP is targeted at this location
	int getEMPGrid(int x, int y) { return EMPGrid[x][y]; }
	int getEMPGrid(WalkPosition here) { return EMPGrid[here.x][here.y]; }

	// Other functions
	Position getAllyArmyCenter(){ return allyArmyCenter; }
	Position getEnemyArmyCenter() { return enemyArmyCenter; }
};

typedef Singleton<GridTrackerClass> GridTracker;