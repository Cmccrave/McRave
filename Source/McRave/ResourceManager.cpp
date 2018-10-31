#include "McRave.h"

void ResourceManager::onFrame()
{
	Display().startClock();
	updateResources();
	Display().performanceTest(__FUNCTION__);
}

void ResourceManager::updateResources()
{
	// Assume saturation, will be changed to false if any resource isn't saturated
	minSat = true, gasSat = true;
	incomeMineral = 0, incomeGas = 0;
	gasCount = 0;

	const auto update = [&](ResourceInfo& resource) {
		if (!resource.unit())
			return;
		updateInformation(resource);
		updateIncome(resource);
	};

	for (auto &m : myBoulders) {
		ResourceInfo& resource = m.second;
		update(resource);		
	}

	for (auto &m : myMinerals) {
		ResourceInfo& resource = m.second;
		update(resource);
	}

	for (auto &g : myGas) {
		ResourceInfo& resource = g.second;
		update(resource);
		
		// If resource is blocked from usage
		if (resource.getTilePosition().isValid()) {
			for (auto block = mapBWEM.GetTile(resource.getTilePosition()).GetNeutral(); block; block = block->NextStacked()) {
				if (block && block->Unit() && block->Unit()->exists() && block->Unit()->isInvincible() && !block->IsGeyser())
					resource.setState(0);
			}
		}
	}
}

void ResourceManager::updateInformation(ResourceInfo& resource)
{
	// If unit exists, update BW information
	if (resource.unit()->exists())
		resource.updateResource();	

	UnitType geyserType = Broodwar->self()->getRace().getRefinery();	

	// Update saturation
	if (resource.getType().isMineralField() && minSat && resource.getGathererCount() < 2 && resource.getState() > 0)
		minSat = false;
	else if (resource.getType() == geyserType && resource.unit()->isCompleted() && resource.getState() > 0 && ((BuildOrder().isOpener() && resource.getGathererCount() < min(3, BuildOrder().gasWorkerLimit())) || (!BuildOrder().isOpener() && resource.getGathererCount() < 3)))
		gasSat = false;
	
	if (!resource.getType().isMineralField() && resource.getState() == 2)
		gasCount++;
}

void ResourceManager::updateIncome(ResourceInfo& resource)
{
	// Estimate income
	auto cnt = resource.getGathererCount();
	if (resource.getType().isMineralField())
		incomeMineral += cnt == 1 ? 65 : 126;	
	else
		incomeGas += resource.getRemainingResources() ? 103 * cnt : 26 * cnt;	
}

void ResourceManager::storeResource(Unit resource)
{
	auto &r = (resource->getInitialResources() > 0 ? (resource->getType().isMineralField() ? myMinerals[resource] : myGas[resource]) : myBoulders[resource]);
	r.setUnit(resource);

	// If we are not on an inital frame, a geyser was just created and we need to see if we own it
	if (Broodwar->getFrameCount() > 0) {
		auto newStation = mapBWEB.getClosestStation(resource->getTilePosition());

		if (newStation) {
			for (auto &s : Stations().getMyStations()) {
				auto &station = *s.second;
				if (station.BWEMBase() == newStation->BWEMBase()) {
					r.setState(2);
					break;
				}
			}
		}
	}
}

void ResourceManager::removeResource(Unit resource)
{
	// Remove dead resources
	if (myMinerals.find(resource) != myMinerals.end())
		myMinerals.erase(resource);
	else if (myBoulders.find(resource) != myBoulders.end())
		myBoulders.erase(resource);
	else if (myGas.find(resource) != myGas.end())
		myGas.erase(resource);

	// Any workers that targeted that resource now have no target
	for (auto &u : Units().getMyUnits()) {
		UnitInfo& unit = u.second;
		if (unit.hasResource() && unit.getResource().unit() == resource)
			unit.setResource(nullptr);
	}
}