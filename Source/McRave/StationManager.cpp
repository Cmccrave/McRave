#include "McRave.h"

void StationManager::onFrame()
{
	Display().startClock();
	updateStations();
	Display().performanceTest(__FUNCTION__);
	return;
}

void StationManager::updateStations()
{
	for (auto &station : enemyStations)	{
		Station s = station.second;
		if (Broodwar->isVisible(TilePosition(s.BWEMBase()->Center())))		
			lastVisible[TilePosition(s.BWEMBase()->Center())] = Broodwar->getFrameCount();
	}
	return;
}

Position StationManager::getClosestEnemyStation(Position here)
{
	double distBest = DBL_MAX;
	Position best;
	for (auto &station : enemyStations)	{
		Station s = station.second;
		double dist = here.getDistance(s.BWEMBase()->Center());
		if (dist < distBest)
			best = s.BWEMBase()->Center(), distBest = dist;
	}
	return best;
}

void StationManager::storeStation(Unit unit)
{
	const Station * station = mapBWEB.getClosestStation(unit->getTilePosition());
	if (!station)
		return;
	if (unit->getTilePosition() != station->BWEMBase()->Location())
		return;

	unit->getPlayer() == Broodwar->self() ? myStations.emplace(unit, *station) : enemyStations.emplace(unit, *station);
	int state = 1 + (unit->isCompleted());
	if (unit->getPlayer() == Broodwar->self()) {
		for (auto &mineral : station->BWEMBase()->Minerals())
			Resources().getMyMinerals()[mineral->Unit()].setState(state);
		for (auto &gas : station->BWEMBase()->Geysers())
			Resources().getMyGas()[gas->Unit()].setState(state);
	}

	if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition())) {
		if (unit->getPlayer() == Broodwar->self())
			Terrain().getAllyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition()));
		else
			Terrain().getEnemyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition()));
	}
	return;
}

void StationManager::removeStation(Unit unit)
{
	const Station * station = mapBWEB.getClosestStation(unit->getTilePosition());
	if (!station)
		return;

	if (unit->getPlayer() == Broodwar->self()) {
		for (auto &mineral : station->BWEMBase()->Minerals())
			Resources().getMyMinerals()[mineral->Unit()].setState(0);
		for (auto &gas : station->BWEMBase()->Geysers())
			Resources().getMyGas()[gas->Unit()].setState(0);
		myStations.erase(unit);
	}
	else
		enemyStations.erase(unit);


	if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition())) {
		if (unit->getPlayer() == Broodwar->self())
			Terrain().getAllyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
		else
			Terrain().getEnemyTerritory().erase(mapBWEM.GetArea(unit->getTilePosition()));
	}
	return;
}