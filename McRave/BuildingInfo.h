#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class BuildingInfo
	{
		int energy;
		double strength;
		bool idleStatus;
		Unit thisUnit;
		UnitType unitType, nextUnit;
		Position position;
		WalkPosition walkPosition;
		TilePosition tilePosition;

	public:
		BuildingInfo();

		int getEnergy() { return energy; }
		double getStrength() { return strength; }
		bool isIdle() { return idleStatus; }
		Unit unit() { return thisUnit; }
		UnitType getType() { return unitType; }
		UnitType getNextUnit() { return nextUnit; }
		Position getPosition() { return position; }
		WalkPosition getWalkPosition() { return walkPosition; }
		TilePosition getTilePosition() { return tilePosition; }

		void setEnergy(int newEnergy) { energy = newEnergy; }
		void setStrength(double newStrength) { strength = newStrength; }
		void setIdleStatus(bool newIdleStatus) { idleStatus = newIdleStatus; }
		void setUnit(Unit newUnit) { thisUnit = newUnit; }
		void setType(UnitType newBuildingType) { unitType = newBuildingType; }
		void setNextUnit(UnitType newNextUnit) { nextUnit = newNextUnit; }
		void setPosition(Position newPosition) { position = newPosition; }
		void setWalkPosition(WalkPosition newPosition) { walkPosition = newPosition; }
		void setTilePosition(TilePosition newPosition) { tilePosition = newPosition; }

	};
}