#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

class TransportInfo
{
	int loadState, cargoSize;
	bool harassing;
	Unit thisUnit;
	UnitType transportType;
	set<Unit> assignedCargo;
	Position position, destination;
	WalkPosition walkPosition;
public:
	TransportInfo();

	int getCargoSize() { return cargoSize; }
	int getLoadState() { return loadState; }
	bool isHarassing() { return harassing; }
	Unit unit() { return thisUnit; }
	UnitType getType() { return transportType; }
	set<Unit>& getAssignedCargo() { return assignedCargo; }
	Position getPosition() { return position; }
	Position getDestination() { return destination; }
	WalkPosition getWalkPosition() { return walkPosition; }

	void setCargoSize(int newSize) { cargoSize = newSize; }
	void setLoadState(int newState) { loadState = newState; }
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