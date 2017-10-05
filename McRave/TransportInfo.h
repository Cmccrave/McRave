#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

class TransportInfo
{
	int cargoSize;
	bool harassing, loading, unloading;
	Unit thisUnit;
	UnitType transportType;
	set<Unit> assignedCargo;
	Position position, destination;
	WalkPosition walkPosition;
public:
	TransportInfo();

	int getCargoSize() { return cargoSize; }
	bool isLoading() { return loading; }
	bool isUnloading() { return unloading; }
	bool isHarassing() { return harassing; }
	Unit unit() { return thisUnit; }
	UnitType getType() { return transportType; }
	set<Unit>& getAssignedCargo() { return assignedCargo; }
	Position getPosition() { return position; }
	Position getDestination() { return destination; }
	WalkPosition getWalkPosition() { return walkPosition; }

	void setCargoSize(int newSize) { cargoSize = newSize; }
	void setLoading(int newState) { loading = newState; }
	void setUnloading(bool newState) { unloading = newState; }
	void setHarassing(bool newState) { harassing = newState; }
	void setUnit(Unit newTransport) { thisUnit = newTransport; }
	void setType(UnitType newType) { transportType = newType; }
	void setPosition(Position newPosition) { position = newPosition; }
	void setDestination(Position newPosition) { destination = newPosition; }
	void setWalkPosition(WalkPosition newWalkPosition) { walkPosition = newWalkPosition; }

	// Add cargo to the assigned cargo set
	void assignCargo(Unit);

	// Remove cargo from the assigned cargo set
	void removeCargo(Unit);
};