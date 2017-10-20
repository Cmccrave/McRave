#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

class TransportInfo
{
	int cargoSize, lastDropFrame;
	bool harassing, loading, unloading;
	Unit thisUnit;
	UnitType transportType;
	set<Unit> assignedCargo;
	Position position, destination;
	WalkPosition walkPosition;
	TilePosition tilePosition;
public:
	TransportInfo();

	int getCargoSize() { return cargoSize; }
	int getLastDropFrame() { return lastDropFrame; }
	bool isLoading() { return loading; }
	bool isUnloading() { return unloading; }
	bool isHarassing() { return harassing; }
	Unit unit() { return thisUnit; }
	UnitType getType() { return transportType; }
	set<Unit>& getAssignedCargo() { return assignedCargo; }
	Position getPosition() { return position; }
	Position getDestination() { return destination; }
	WalkPosition getWalkPosition() { return walkPosition; }
	TilePosition getTilePosition() { return tilePosition; }

	void setCargoSize(int newSize) { cargoSize = newSize; }
	void setLastDropFrame(int newDropFrame) { lastDropFrame = newDropFrame; }
	void setLoading(bool newState) { loading = newState; }
	void setUnloading(bool newState) { unloading = newState; }
	void setHarassing(bool newState) { harassing = newState; }
	void setUnit(Unit newTransport) { thisUnit = newTransport; }
	void setType(UnitType newType) { transportType = newType; }
	void setPosition(Position newPosition) { position = newPosition; }
	void setDestination(Position newPosition) { destination = newPosition; }
	void setWalkPosition(WalkPosition newWalkPosition) { walkPosition = newWalkPosition; }
	void setTilePosition(TilePosition newTilePosition) { tilePosition = newTilePosition; }

	// Add cargo to the assigned cargo set
	void assignCargo(Unit);

	// Remove cargo from the assigned cargo set
	void removeCargo(Unit);
};