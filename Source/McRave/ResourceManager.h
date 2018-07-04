#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "ResourceInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class ResourceManager
	{
		map <Unit, ResourceInfo> myMinerals;
		map <Unit, ResourceInfo> myGas;
		map <Unit, ResourceInfo> myBoulders;
		bool minSat, gasSat;
		int gasCount;
		int incomeMineral, incomeGas;

		void storeMineral(Unit), storeGas(Unit), storeBoulder(Unit);
		void updateResources(), updateIncome(ResourceInfo&), updateInformation(ResourceInfo&);
	public:
		int getGasCount() { return gasCount; }
		int getIncomeMineral() { return incomeMineral; }
		int getIncomeGas() { return incomeGas; }
		bool isMinSaturated() { return minSat; }
		bool isGasSaturated() { return gasSat; }
		map <Unit, ResourceInfo>& getMyMinerals() { return myMinerals; }
		map <Unit, ResourceInfo>& getMyGas() { return myGas; }
		map <Unit, ResourceInfo>& getMyBoulders() { return myBoulders; }

		void onFrame();
		void storeResource(Unit), removeResource(Unit);
	};
}

typedef Singleton<McRave::ResourceManager> ResourceSingleton;
