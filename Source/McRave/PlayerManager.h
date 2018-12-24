#pragma once
#include "McRave.h"
#include "PlayerInfo.h"
#include "Singleton.h"

namespace McRave
{
	class PlayerManager
	{
		map <Player, PlayerInfo> thePlayers;
		map <Race, int> raceCount;
	public:
		map <Player, PlayerInfo>& getPlayers() { return thePlayers; }
		void onStart(), onFrame(), update(PlayerInfo&);

		int getNumberZerg() { return raceCount[Races::Zerg]; }
		int getNumberProtoss() { return raceCount[Races::Protoss]; }
		int getNumberTerran() { return raceCount[Races::Terran]; }
		int getNumberRandom() { return raceCount[Races::Unknown]; }

		bool vP() { return (thePlayers.size() == 1 && raceCount[Races::Protoss] > 0); }
		bool vT() { return (thePlayers.size() == 1 && raceCount[Races::Terran] > 0); }
		bool vZ() { return (thePlayers.size() == 1 && raceCount[Races::Zerg] > 0 ); }
	};
}

typedef Singleton<McRave::PlayerManager> PlayerSingleton;
