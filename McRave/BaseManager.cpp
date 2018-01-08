#include "McRave.h"

void BaseTrackerClass::update()
{
	Display().startClock();
	updateBases();
	Display().performanceTest(__FUNCTION__);
	return;
}

void BaseTrackerClass::updateBases()
{
	for (auto &b : myStations)
	{	

	}
	for (auto &b : enemyBases)
	{
		BaseInfo& base = b.second;
		if (!base.unit()) continue;
		if (base.unit()->exists())
		{
			base.setLastVisibleFrame(Broodwar->getFrameCount());
		}
		if (!base.unit()->exists() && Broodwar->isVisible(base.getTilePosition()))
		{
			enemyBases.erase(base.unit());
			break;
		}
	}
	return;
}

void BaseTrackerClass::updateProduction(BaseInfo& base)
{
	if (base.getType() == UnitTypes::Terran_Command_Center && !base.unit()->getAddon())
	{
		base.unit()->buildAddon(UnitTypes::Terran_Comsat_Station);
	}

	int ovie = 0;
	for (auto &u : Units().getAllyUnitsFilter(UnitTypes::Zerg_Egg))
	{
		UnitInfo& unit = Units().getUnitInfo(u);
		if (unit.unit()->getBuildType() == UnitTypes::Zerg_Overlord)
		{
			ovie++;
		}
	}

	if (base.unit() && base.unit()->isIdle() && ((!Resources().isMinSaturated() || !Resources().isGasSaturated()) || BuildOrder().getCurrentBuild() == "Sparks"))
	{
		for (auto &unit : base.getType().buildsWhat())
		{
			if (unit.isWorker())
			{
				if (Broodwar->self()->completedUnitCount(unit) < 75 && (Broodwar->self()->minerals() >= unit.mineralPrice() + Production().getReservedMineral() + Buildings().getQueuedMineral()))
				{
					base.unit()->train(unit);
				}
			}
		}
	}

	for (auto &unit : base.unit()->getLarva())
	{
		if ((Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Overlord) + ovie - 1) < min(22, (int)floor((Units().getSupply() / max(14, (16 - Broodwar->self()->allUnitCount(UnitTypes::Zerg_Overlord)))))))
		{
			base.unit()->morph(UnitTypes::Zerg_Overlord);
		}
		else if (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Spawning_Pool) > 0 && (Units().getGlobalEnemyGroundStrength() > Units().getGlobalAllyGroundStrength() || Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Zergling) < 6))
		{
			base.unit()->morph(UnitTypes::Zerg_Zergling);
		}
		else if ((!Resources().isMinSaturated() || !Resources().isGasSaturated()) && Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Drone) < 60 && (Broodwar->self()->minerals() >= UnitTypes::Zerg_Drone.mineralPrice() + Production().getReservedMineral() + Buildings().getQueuedMineral()))
		{
			base.unit()->morph(UnitTypes::Zerg_Drone);
		}
		else
		{
			base.unit()->morph(UnitTypes::Zerg_Zergling);
		}
	}
	return;
}

Position BaseTrackerClass::getClosestEnemyBase(Position here)
{
	double closestD = 0.0;
	Position closestP;
	for (auto &b : enemyBases)
	{
		BaseInfo& base = b.second;
		if (here.getDistance(base.getPosition()) < closestD || closestD == 0.0)
		{
			closestP = base.getPosition();
			closestD = here.getDistance(base.getPosition());
		}
	}
	return closestP;
}

void BaseTrackerClass::storeBase(Unit base)
{
	// TODO - Find BWEB Station based on closest
	BWEB::Station station = mapBWEB.getClosestStation(base->getTilePosition());
	base->getPlayer() == Broodwar->self() ? myStations.push_back(station) : enemyStations.push_back(station);
	
	if (base->getPlayer() == Broodwar->self())
	{
		if (base->isCompleted())
		{
			// Update resources to state 2
			for (auto &mineral : station.BWEMBase()->Minerals())
			{
				Resources().getMyMinerals()[mineral->Unit()].setState(2);
			}
			// Update resources to state 2
			for (auto &gas : station.BWEMBase()->Geysers())
			{
				Resources().getMyGas()[gas->Unit()].setState(2);
			}
		}
		else
		{
			// Update resources to state 2
			for (auto &mineral : station.BWEMBase()->Minerals())
			{
				Resources().getMyMinerals()[mineral->Unit()].setState(1);
			}
			// Update resources to state 2
			for (auto &gas : station.BWEMBase()->Geysers())
			{
				Resources().getMyGas()[gas->Unit()].setState(1);
			}
		}
	}

	/*BaseInfo& b = base->getPlayer() == Broodwar->self() ? myBases[base] : enemyBases[base];
	b.setUnit(base);
	b.setType(base->getType());
	b.setPosition(base->getPosition());
	b.setWalkPosition(Util().getWalkPosition(base));
	b.setTilePosition(base->getTilePosition());
	b.setPosition(base->getPosition());*/

	if (base->getTilePosition().isValid() && mapBWEM.GetArea(base->getTilePosition()))
	{
		if (base->getPlayer() == Broodwar->self())
		{
			Terrain().getAllyTerritory().insert(mapBWEM.GetArea(base->getTilePosition())->Id());
		}
		else
		{
			Terrain().getEnemyTerritory().insert(mapBWEM.GetArea(base->getTilePosition())->Id());
		}
	}
	return;
}

void BaseTrackerClass::removeBase(Unit base)
{
	BWEB::Station station = mapBWEB.getClosestStation(base->getTilePosition());

	// Update resources to state 2
	for (auto &mineral : station.BWEMBase()->Minerals())
	{
		Resources().getMyMinerals()[mineral->Unit()].setState(0);
	}

	if (base->getPlayer() == Broodwar->self())
	{
		Terrain().getAllyTerritory().erase(mapBWEM.GetArea(base->getTilePosition())->Id());
	}
	else
	{
		Terrain().getEnemyTerritory().erase(mapBWEM.GetArea(base->getTilePosition())->Id());
	}
	return;
}