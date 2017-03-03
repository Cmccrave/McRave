#include <BWAPI.h>
#include <BWTA.h>
#include <vector>
#include "src\bwem.h"

using namespace BWAPI;
using namespace std;
using namespace BWEM;

// External position variables
extern BWEM::CPPath path;
extern vector <Position> enemyBasePositions;
extern vector <TilePosition> nextExpansion;
extern Position enemyStartingPosition;
extern TilePosition enemyStartingTilePosition;
extern Position playerStartingPosition;
extern Position holdingPosition;
extern Position zealotPosition;

extern int nexusDesired;
extern int firstAttack;

extern double threatArray[256][256];
extern double airThreatArray[256][256];
extern int pathArray[256][256];
extern double enemyStrength, allyStrength;

// Shuttle ID and Reaver ID pairing
extern vector<int> shuttleID;
extern vector<int> harassShuttleID;
extern vector<int> reaverID;
extern vector<int> harassReaverID;

// Internal position variables
extern Position fleePosition;
extern vector<Position> fleePositions;
extern vector<Position> unitsFleeing;

// Other
extern bool forceEngage;
extern int currentSize;
extern vector<BWTA::Region*> allyTerritory;
extern vector<Position> defendHere;
extern int enemyCountNearby;
extern int defendingUnitCount;


// Function declarations
void unitGetCommand(Unit unit);
void unitMicro(Unit unit);
double unitGetStrength(Unit unit);
Position unitRegroup(Unit unit);
Position unitFlee(Unit unit, Unit currentTarget);
void shuttleManager(Unit unit);
void shuttleHarass(Unit unit);
void observerManager(Unit unit);
void reaverManager(Unit unit);
void carrierManager(Unit unit);
void templarManager(Unit unit);
void arbiterManager(Unit unit);
int unitGetGlobalStrategy();

// Classes for enemy building tracking

class UnitInfo{
	UnitType unitType;
	Position position;
public:
	UnitInfo();
	UnitInfo(UnitType, Position);
	~UnitInfo();

	UnitType getUnitType() const;
	// getUnitType
	// @return UnitType - BWAPI type of unit
	Position getPosition() const;
	// getPosition
	// @return Position - BWAPI position of unit
	//void setUnitType(UnitType) const;
	//	// setUnitType
	//	// @param UnitType - BWAPI type of unit
	//void setPosition(Position) const;
	//	// setPosition
	//	// @param Position - BWAPI position of unit
};

extern map <int, UnitInfo> enemyUnits;
int storeEnemyUnit(Unit building, map<int, UnitInfo>& enemyUnits);
