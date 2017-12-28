#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

class PlayerInfo
{
	Race race;
	bool alive;
	TilePosition startLocation;
	set <UpgradeType> playerUpgrades;
	set <TechType> playerTechs;
public:
	PlayerInfo();
	Race getRace() { return race; }
	bool isAlive() { return alive; }
	TilePosition getStartingLocation() { return startLocation; }

	void setRace(Race newRace) { race = newRace; }
	void setAlive(bool newState) { alive = newState; }
	void hasUpgrade(UpgradeType);
	void hasTech(TechType);
};