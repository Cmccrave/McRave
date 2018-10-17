#include "McRave.h"

void StationManager::onFrame()
{
	Display().startClock();
	updateStations();
	Display().performanceTest(__FUNCTION__);
}

void StationManager::updateStations()
{

}

Position StationManager::getClosestEnemyStation(Position here)
{
	double distBest = DBL_MAX;
	Position best;
	for (auto &station : enemyStations) {
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

	// Change the resource states and store station
	if (unit->getPlayer() == Broodwar->self()) {
		for (auto &mineral : station->BWEMBase()->Minerals()) {
			auto &resource = Resources().getMyMinerals()[mineral->Unit()];
			resource.setState(state);

			if (state == 2)
				resource.setStation(station);
		}
		for (auto &gas : station->BWEMBase()->Geysers()) {
			auto &resource = Resources().getMyGas()[gas->Unit()];
			resource.setState(state);

			if (state == 2)
				resource.setStation(station);
		}
	}

	if (unit->getTilePosition().isValid() && mapBWEM.GetArea(unit->getTilePosition())) {
		if (unit->getPlayer() == Broodwar->self())
			Terrain().getAllyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition()));
		else
			Terrain().getEnemyTerritory().insert(mapBWEM.GetArea(unit->getTilePosition()));
	}
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
}

bool StationManager::needDefenses(const Station station)
{
	auto centroid = TilePosition(station.ResourceCentroid());
	auto defenseCount = station.getDefenseCount();
	auto main = station.BWEMBase()->Location() == mapBWEB.getMainTile();
	auto nat = station.BWEMBase()->Location() == mapBWEB.getNaturalTile();

	if (!Pylons().hasPower(centroid, UnitTypes::Protoss_Photon_Cannon))
		return false;

	if ((nat || main) && !Terrain().isIslandMap() && defenseCount <= 0)
		return true;
	else if (defenseCount <= 1)
		return true;
	else if ((Players().getPlayers().size() > 1 || Broodwar->enemy()->getRace() == Races::Zerg) && !main && !nat && defenseCount < int(station.DefenseLocations().size()))
		return true;
	else if (station.getDefenseCount() < 1 && (Units().getGlobalEnemyAirStrength() > 0.0 || Strategy().getEnemyBuild() == "Z2HatchMuta" || Strategy().getEnemyBuild() == "Z3HatchMuta"))
		return true;
	return false;
}