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
		map <Unit, TransportInfo> myTransports;
		map <WalkPosition, int> recentExplorations;
	public:
		void onFrame();
		void updateTransports();
		void updateInformation(TransportInfo&);
		void updateCargo(TransportInfo&);
		void updateDecision(TransportInfo&);
		void updateMovement(TransportInfo&);
		void removeUnit(Unit);
		void storeUnit(Unit);
	};
}

typedef Singleton<McRave::TransportManager> TransportSingleton;
