#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class BuildingInfo
	{
		int remainingTrainFrame;
		int energy;
		Unit thisUnit;
		UnitType type, nextUnit;
		Position position;
		WalkPosition walkPosition;
		TilePosition tilePosition;

	public:
		BuildingInfo();
		void update();

		int getRemainingTrainFrames()					{ return remainingTrainFrame; }
		int getEnergy()									{ return energy; }
		Unit unit()										{ return thisUnit; }
		UnitType getType()								{ return type; }
		Position getPosition()							{ return position; }
		WalkPosition getWalkPosition()					{ return walkPosition; }
		TilePosition getTilePosition()					{ return tilePosition; }

		void setRemainingTrainFrame(int newFrame)		{ remainingTrainFrame = newFrame; }		
		void setUnit(Unit newUnit)						{ thisUnit = newUnit; }
	};
}