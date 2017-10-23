#pragma once
#include "McRave.h"
#include "PlayerInfo.h"
#include "Singleton.h"

class PlayerTrackerClass
{
	map <Player, PlayerInfo> thePlayers;
	int eZerg, eProtoss, eTerran, eRandom;
public:
	map <Player, PlayerInfo>& getPlayers() { return thePlayers; }

	void onStart();
	void update();
	void updateUpgrades(PlayerInfo&);

	int getNumberZerg() { return eZerg; }
	int getNumberProtoss() { return eProtoss; }
	int getNumberTerran() { return eTerran; }
	int getNumberRandom() { return eRandom; }
};

typedef Singleton<PlayerTrackerClass> PlayerTracker;