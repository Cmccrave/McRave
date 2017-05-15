#pragma once
#include <BWAPI.h>
#include <BWTA.h>

using namespace BWAPI;
using namespace BWTA;
using namespace std;

class SpecialUnitInfoClass
{
	Position position, destination;
public:

	SpecialUnitInfoClass(Position, Position);

	Position getPosition() { return position; }
	Position getDestination() { return destination; }

	void setPosition(Position newPosition) { position = newPosition; }
	void setDestination(Position newDestination) { destination = newDestination; }
};