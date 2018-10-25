#pragma once
#include <BWAPI.h>

namespace McRave {
	class UnitInfo;
	namespace Support {
		void onFrame();
		void updateDecision(UnitInfo&);
	}
}
