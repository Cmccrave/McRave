#include "McRave.h"

void StationTrackerClass::update()
{
	Display().startClock();
	updateStations();
	Display().performanceTest(__FUNCTION__);
	return;
}

void StationTrackerClass::updateStations()
{
	for (auto &b : myStations)
	{

	}
	for (auto &b : enemyStations)
	{
		//BaseInfo& base = b.second;
		//if (!base.unit()) continue;
		//if (base.unit()->exists())
		//{
		//	base.setLastVisibleFrame(Broodwar->getFrameCount());
		//}
		//if (!base.unit()->exists() && Broodwar->isVisible(base.getTilePosition()))
		//{
		//	enemyBases.erase(base.unit());
		//	break;
		//}
	}
	return;
}

//void StationTrackerClass::updateProduction(BaseInfo& base)
//{
//	if (base.getType() == UnitTypes::Terran_Command_Center && !base.unit()->getAddon())
//	{
//		base.unit()->buildAddon(UnitTypes::Terran_Comsat_Station);
//	}
//
//	int ovie = 0;
//	for (auto &u : Units().getAllyUnitsFilter(UnitTypes::Zerg_Egg))
//	{
//		UnitInfo& unit = Units().getUnitInfo(u);
//		if (unit.unit()->getBuildType() == UnitTypes::Zerg_Overlord)
//		{
//			ovie++;
//		}
//	}
//
//	if (base.unit() && base.unit()->isIdle() && ((!Resources().isMinSaturated() || !Resources().isGasSaturated()) || BuildOrder().getCurrentBuild() == "Sparks"))
//	{
//		for (auto &unit : base.getType().buildsWhat())
//		{
//			if (unit.isWorker())
//			{
//				if (Broodwar->self()->completedUnitCount(unit) < 75 && (Broodwar->self()->minerals() >= unit.mineralPrice() + Production().getReservedMineral() + Buildings().getQueuedMineral()))
//				{
//					base.unit()->train(unit);
//				}
//			}
//		}
//	}
//
//	for (auto &unit : base.unit()->getLarva())
//	{
//		if ((Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Overlord) + ovie - 1) < min(22, (int)floor((Units().getSupply() / max(14, (16 - Broodwar->self()->allUnitCount(UnitTypes::Zerg_Overlord)))))))
//		{
//			base.unit()->morph(UnitTypes::Zerg_Overlord);
//		}
//		else if (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Spawning_Pool) > 0 && (Units().getGlobalEnemyGroundStrength() > Units().getGlobalAllyGroundStrength() || Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Zergling) < 6))
//		{
//			base.unit()->morph(UnitTypes::Zerg_Zergling);
//		}
//		else if ((!Resources().isMinSaturated() || !Resources().isGasSaturated()) && Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Drone) < 60 && (Broodwar->self()->minerals() >= UnitTypes::Zerg_Drone.mineralPrice() + Production().getReservedMineral() + Buildings().getQueuedMineral()))
//		{
//			base.unit()->morph(UnitTypes::Zerg_Drone);
//		}
//		else
//		{
//			base.unit()->morph(UnitTypes::Zerg_Zergling);
//		}
//	}
//	return;
//}

Position StationTrackerClass::getClosestEnemyStation(Position here)
{
	double distBest = DBL_MAX;
	Position best;
	for (auto &station : enemyStations)
	{
		double dist = here.getDistance(station.BWEMBase()->Center());
		if (dist < distBest)
			best = station.BWEMBase()->Center(), distBest = dist;
	}
	return best;
}

void StationTrackerClass::storeStation(Unit unit)
{
	BWEB::Station station = mapBWEB.getClosestStation(unit->getTilePosition());
	unit->getPlayer() == Broodwar->self() ? myStations.push_back(station) : enemyStations.push_back(station);
	int state = 1 + (unit->isCompleted());

	if (unit->getPlayer() == Broodwar->self())
	{
		for (auto &mineral : station.BWEMBase()->Minerals())
			Resources().getMyMinerals()[mineral->Unit()].setState(state);
		for (auto &gas : station.BWEMBase()->Geysers())
			Resources().getMyGas()[gas->Unit()].setState(state);
	}

	if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition()))
	{
		if (unit->getPlayer() == Broodwar->self())
			Terrain().getAllyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition())->Id());
		else
			Terrain().getEnemyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition())->Id());
	}
	return;
}

void StationTrackerClass::removeStation(Unit unit)
{
	BWEB::Station station = mapBWEB.getClosestStation(unit->getTilePosition());

	if (unit->getPlayer() == Broodwar->self())
	{
		for (auto &mineral : station.BWEMBase()->Minerals())
			Resources().getMyMinerals()[mineral->Unit()].setState(0);
		for (auto &gas : station.BWEMBase()->Geysers())
			Resources().getMyGas()[gas->Unit()].setState(0);
	}

	if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition()))
	{
		if (unit->getPlayer() == Broodwar->self())
			Terrain().getAllyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition())->Id());
		else
			Terrain().getEnemyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition())->Id());
	}
	return;
}