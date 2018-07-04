#include "PlayerInfo.h"

namespace McRave
{
	PlayerInfo::PlayerInfo()
	{
		thisPlayer = nullptr;
		race = Races::None;
		alive = true;
	}

	bool PlayerInfo::hasTech(TechType tech)
	{
		if (playerTechs.find(tech) != playerTechs.end())
			return true;
		return false;
	}

	bool PlayerInfo::hasUpgrade(UpgradeType upgrade)
	{
		if (playerUpgrades.find(upgrade) != playerUpgrades.end())
			return true;
		return false;
	}
}