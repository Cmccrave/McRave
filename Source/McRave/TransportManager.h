#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TransportManager
	{
		map <WalkPosition, int> recentExplorations;
	public:
		void onFrame();
		void updateTransports();
		void updateCargo(UnitInfo&);
		void updateDecision(UnitInfo&);
		void updateMovement(UnitInfo&);
		void removeUnit(Unit);
	};
}

typedef Singleton<McRave::TransportManager> TransportSingleton;
