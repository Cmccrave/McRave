#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class ResourceInfo
	{
	private:
		int gathererCount, remainingResources, miningState;
		Unit storedUnit;
		UnitType unitType;
		Position position;
		TilePosition tilePosition;
		WalkPosition walkPosition;

	public:
		ResourceInfo();

		int getGathererCount()								{ return gathererCount; };
		int getRemainingResources()							{ return remainingResources; }
		int getState()										{ return miningState; }
		Unit unit()											{ return storedUnit; }
		UnitType getType()									{ return unitType; }
		Position getPosition()								{ return position; }
		WalkPosition getWalkPosition()						{ return walkPosition; }
		TilePosition getTilePosition()						{ return tilePosition; }

		void setGathererCount(int newInt)					{ gathererCount = newInt; }
		void setRemainingResources(int newInt)				{ remainingResources = newInt; }
		void setState(int newInt)							{ miningState = newInt; }
		void setUnit(Unit newUnit)							{ storedUnit = newUnit; }
		void setType(UnitType newType)						{ unitType = newType; }
		void setPosition(Position newPosition)				{ position = newPosition; }
		void setWalkPosition(WalkPosition newPosition)		{ walkPosition = newPosition; }
		void setTilePosition(TilePosition newTilePosition)	{ tilePosition = newTilePosition; }
	};
}