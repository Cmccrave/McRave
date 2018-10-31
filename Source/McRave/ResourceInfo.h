#pragma once
#include <BWAPI.h>
#include "BWEB.h"
#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace BWEB {
	class Station;
}

namespace McRave
{
	class ResourceInfo
	{
	private:
		int gathererCount, remainingResources;
		Unit thisUnit;
		UnitType type;
		Position position;
		TilePosition tilePosition;
		const BWEB::Station * station;
		ResourceState rState;
	public:
		ResourceInfo();

		void updateResource();

		bool hasStation()									{ return station != nullptr; }
		const BWEB::Station * getStation()					{ return station; }
		void setStation(const BWEB::Station* newStation)	{ station = newStation; }

		int getGathererCount()								{ return gathererCount; };
		int getRemainingResources()							{ return remainingResources; }
		ResourceState getResourceState()					{ return rState; }
		Unit unit()											{ return thisUnit; }
		UnitType getType()									{ return type; }
		Position getPosition()								{ return position; }
		TilePosition getTilePosition()						{ return tilePosition; }

		void setGathererCount(int newInt)					{ gathererCount = newInt; }
		void setRemainingResources(int newInt)				{ remainingResources = newInt; }
		void setResourceState(ResourceState newState)		{ rState = newState; }
		void setUnit(Unit newUnit)							{ thisUnit = newUnit; }
		void setType(UnitType newType)						{ type = newType; }
		void setPosition(Position newPosition)				{ position = newPosition; }
		void setTilePosition(TilePosition newTilePosition)	{ tilePosition = newTilePosition; }
	};
}
