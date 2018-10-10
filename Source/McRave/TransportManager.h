#pragma once
#include <BWAPI.h>
#include "Singleton.h"
#include "TransportInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class TransportManager
	{
		map <Unit, TransportUnit> myTransports;
		map <WalkPosition, int> recentExplorations;
	public:
		void onFrame();
		void updateCargo(TransportUnit&);
		void updateDecision(TransportUnit&);
		void updateMovement(TransportUnit&);
		void removeUnit(Unit);
		void storeUnit(Unit);
	};
}

typedef Singleton<McRave::TransportManager> TransportSingleton;
