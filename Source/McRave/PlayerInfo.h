#pragma once
#include <BWAPI.h>
#include <set>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class PlayerInfo
	{
		Player thisPlayer;
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

		Player player() { return thisPlayer; }

		void storeUpgrade(UpgradeType upgrade) { playerUpgrades.insert(upgrade); }
		void storeTech(TechType tech) { playerTechs.insert(tech); }

		void setRace(Race newRace) { race = newRace; }
		void setAlive(bool newState) { alive = newState; }
		void setPlayer(Player newPlayer) { thisPlayer = newPlayer; }

		bool hasUpgrade(UpgradeType);
		bool hasTech(TechType);
	};
}