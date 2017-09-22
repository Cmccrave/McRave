#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

class SupportUnitInfo
{
	Position position, destination;
	WalkPosition walkPosition;
	Unit thisUnit, target, transport;
	bool hasTransport;
public:
	SupportUnitInfo();

	Unit unit() { return thisUnit; }
	Unit getTransport() { return transport; }
	Position getPosition() { return position; }
	Position getDestination() { return destination; }
	WalkPosition getWalkPosition() { return walkPosition; }
	void setUnit(Unit newUnit) { thisUnit = newUnit; }
	void setTransport(Unit newTransport) { transport = newTransport; }
	void setTarget(Unit newUnit) { target = newUnit; }
	void setPosition(Position newPosition) { position = newPosition; }
	void setDestination(Position newPosition) { destination = newPosition; }
	void setWalkPosition(WalkPosition newWalkPosition) { walkPosition = newWalkPosition; }
};

class TransportInfo
{
	int loadState, cargoSize;
	bool harassing;
	Unit thisUnit;
	UnitType transportType;
	map <Unit, UnitInfo> assignedCargo;
	Position position, destination, drop;
	WalkPosition walkPosition;
public:
	TransportInfo();
	~TransportInfo();

	int getCargoSize() { return cargoSize; }
	int getLoadState() { return loadState; }
	bool isHarassing() { return harassing; }
	Unit unit() { return thisUnit; }
	UnitType getType() { return transportType; }
	map <Unit, UnitInfo>& getAssignedCargo() { return assignedCargo; }
	Position getDrop() { return drop; }
	Position getPosition() { return position; }
	Position getDestination() { return destination; }
	WalkPosition getWalkPosition() { return walkPosition; }

	void setCargoSize(int newSize) { cargoSize = newSize; }
	void setLoadState(int newState) { loadState = newState; }
	void setHarassing(bool newState) { harassing = newState; }
	void setUnit(Unit newTransport) { thisUnit = newTransport; }
	void setType(UnitType newType) { transportType = newType; }
	void setDrop(Position newDrop) { drop = newDrop; }
	void setPosition(Position newPosition) { position = newPosition; }
	void setDestination(Position newPosition) { destination = newPosition; }
	void setWalkPosition(WalkPosition newWalkPosition) { walkPosition = newWalkPosition; }

	// Add cargo to the assigned cargo set
	void assignCargo(UnitInfo&);

	// Remove cargo from the assigned cargo set
	void removeCargo(UnitInfo&);
};

void TransportInfo::assignCargo(UnitInfo& unit)
{
	assignedCargo[unit.unit()] = unit;
	cargoSize = cargoSize + unit.getType().spaceRequired();
}

void TransportInfo::removeCargo(UnitInfo& unit)
{
	assignedCargo.erase[unit];
	cargoSize = cargoSize - unit.getType().spaceRequired();
}