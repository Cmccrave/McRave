#include "McRave.h"

void StationManager::onFrame()
{
	Display().startClock();
	updateStations();
	Display().performanceTest(__FUNCTION__);
}

void StationManager::onStart()
{
	// Add paths to our station network
	for (auto &s1 : mapBWEB.Stations()) {
		for (auto &s2 : mapBWEB.Stations()) {
			const Station * ptrs1 = &s1;
			const Station * ptrs2 = &s2;
			if (stationNetworkExists(ptrs1, ptrs2) || ptrs1 == ptrs2)
				continue;

			Path newPath;
			newPath.createUnitPath(mapBWEB, mapBWEM, ptrs1->ResourceCentroid(), ptrs2->ResourceCentroid());
			stationNetwork[ptrs1][ptrs2] = newPath;
		}
	}
}

void StationManager::updateStations()
{
	return; // Turn this off to draw station network
	for (auto &s1 : stationNetwork) {
		auto connectedPair = s1.second;
		for (auto &path : connectedPair) {
			Display().displayPath(path.second.getTiles());
		}
	}
}

Position StationManager::getClosestEnemyStation(Position here)
{
	double distBest = DBL_MAX;
	Position best;
	for (auto &station : enemyStations) {
		auto s = *station.second;
		double dist = here.getDistance(s.BWEMBase()->Center());
		if (dist < distBest)
			best = s.BWEMBase()->Center(), distBest = dist;
	}
	return best;
}

void StationManager::storeStation(Unit unit)
{
	auto newStation = mapBWEB.getClosestStation(unit->getTilePosition());
	if (!newStation
		|| !unit->getType().isResourceDepot()
		|| unit->getTilePosition() != newStation->BWEMBase()->Location())
		return;

	// 1) Change the resource states and store station
	unit->getPlayer() == Broodwar->self() ? myStations.emplace(unit, newStation) : enemyStations.emplace(unit, newStation);
	ResourceState state = unit->isCompleted() ? ResourceState::Mineable : ResourceState::Assignable;
	if (unit->getPlayer() == Broodwar->self()) {
		for (auto &mineral : newStation->BWEMBase()->Minerals()) {
			auto &resource = Resources().getMyMinerals()[mineral->Unit()];
			resource.setResourceState(state);

			// HACK: Added this to fix some weird gas steal stuff
			if (state == ResourceState::Mineable)
				resource.setStation(newStation);
		}
		for (auto &gas : newStation->BWEMBase()->Geysers()) {
			auto &resource = Resources().getMyGas()[gas->Unit()];
			resource.setResourceState(state);

			// HACK: Added this to fix some weird gas steal stuff
			if (state == ResourceState::Mineable)
				resource.setStation(newStation);
		}
	}

	// 2) Add to territory
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
	if (!station || !unit->getType().isResourceDepot())
		return;

	// 1) Change resource state to not mineable
	if (unit->getPlayer() == Broodwar->self()) {
		for (auto &mineral : station->BWEMBase()->Minerals())
			Resources().getMyMinerals()[mineral->Unit()].setResourceState(ResourceState::None);
		for (auto &gas : station->BWEMBase()->Geysers())
			Resources().getMyGas()[gas->Unit()].setResourceState(ResourceState::None);
		myStations.erase(unit);
	}
	else
		enemyStations.erase(unit);

	// 2) Remove any territory it was in
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
	else if ((Players().getPlayers().size() > 1 || Players().vZ()) && !main && !nat && defenseCount < int(station.DefenseLocations().size()))
		return true;
	else if (station.getDefenseCount() < 1 && (Units().getGlobalEnemyAirStrength() > 0.0 || Strategy().getEnemyBuild() == "Z2HatchMuta" || Strategy().getEnemyBuild() == "Z3HatchMuta"))
		return true;
	return false;
}

bool StationManager::stationNetworkExists(const Station * start, const Station * finish)
{
	for (auto &s : stationNetwork) {
		auto s1 = s.first;

		// For each connected station, check if it exists
		auto connectedStations = s.second;
		for (auto &pair : connectedStations) {
			auto s2 = pair.first;
			if ((s1 == start && s2 == finish) || (s1 == finish && s2 == start))
				return true;
		}
	}
	return false;
}

Path& StationManager::pathStationToStation(const Station * start, const Station * finish)
{
	for (auto &s : stationNetwork) {
		auto s1 = s.first;

		// For each connected station, check if it exists
		auto connectedStations = s.second;
		for (auto &pair : connectedStations) {
			auto s2 = pair.first;
			auto &path = pair.second;
			if ((s1 == start && s2 == finish) || (s1 == finish && s2 == start))
				return path;
		}
	}
	// Return an empty path
	return Path();
}