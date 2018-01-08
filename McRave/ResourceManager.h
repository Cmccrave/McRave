#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "ResourceInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class ResourceTrackerClass
	{
		map <Unit, ResourceInfo> myMinerals;
		map <Unit, ResourceInfo> myGas;
		map <Unit, ResourceInfo> myBoulders;
		set <Position> myMineralLines;
		bool minSat, gasSat;
		int gasNeeded, tempGasCount, lastGas, lastMineral;
		double MPM, GPM;
	public:
		int getTempGasCount() { return tempGasCount; }

		map <Unit, ResourceInfo>& getMyMinerals() { return myMinerals; }
		map <Unit, ResourceInfo>& getMyGas() { return myGas; }
		map <Unit, ResourceInfo>& getMyBoulders() { return myBoulders; }

		double getMPM() { return MPM; }
		double getGPM() { return GPM; }
		
		void update();
		void updateResources();
		void storeResource(Unit);
		void storeMineral(Unit);
		void storeGas(Unit);
		void storeBoulder(Unit);
		void removeResource(Unit);

		bool isMinSaturated() { return minSat; }
		bool isGasSaturated() { return gasSat; }
	};
}

typedef Singleton<McRave::ResourceTrackerClass> ResourceTracker;