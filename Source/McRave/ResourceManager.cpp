#include "McRave.h"

void ResourceManager::onFrame()
{
	Display().startClock();
	updateResources();
	Display().performanceTest(__FUNCTION__);
	return;
}

void ResourceManager::updateResources()
{
	// Assume saturation, will be changed to false if any resource isn't saturated
	minSat = true, gasSat = true;
	incomeMineral = 0, incomeGas = 0;
	gasCount = 0;

	for (auto &m : myMinerals) {
		ResourceInfo& resource = m.second;
		if (!resource.unit())
			continue;

		updateInformation(resource);
		updateIncome(resource);
	}

	for (auto &g : myGas) {
		ResourceInfo& resource = g.second;
		if (!resource.unit())
			continue;

		for (auto block = mapBWEM.GetTile(resource.getTilePosition()).GetNeutral(); block; block = block->NextStacked()) {
			if (block && block->Unit() && block->Unit()->isInvincible() && !block->IsGeyser())
				resource.setState(0);
		}

		updateInformation(resource);
		updateIncome(resource);
	}
}

void ResourceManager::updateInformation(ResourceInfo& resource)
{
	// If unit exists, update BW information
	if (resource.unit()->exists()) {
		resource.setType(resource.unit()->getType());
		resource.setRemainingResources(resource.unit()->getResources());
	}

	UnitType geyserType = Broodwar->self()->getRace().getRefinery();	

	// Update saturation
	if (resource.getType().isMineralField() && minSat && resource.getGathererCount() < 2 && resource.getState() > 0)
		minSat = false;
	else if (resource.getType() == geyserType && resource.getGathererCount() < 3 && resource.unit()->isCompleted() && resource.getState() > 0)
		gasSat = false;
	
	if (!resource.getType().isMineralField() && resource.getState() == 2)
		gasCount++;
}

void ResourceManager::updateIncome(ResourceInfo& resource)
{
	if (resource.getType().isMineralField()) {
		if (resource.getGathererCount() == 1)
			incomeMineral += 65;
		else if (resource.getGathererCount() == 2)
			incomeMineral += 126;
	}
	else {
		if (resource.getRemainingResources())
			incomeGas += 103 * resource.getGathererCount();
		else
			incomeGas += 26 * resource.getGathererCount();
	}
}

void ResourceManager::storeResource(Unit resource)
{
	if (resource->getInitialResources() > 300)
		resource->getType().isMineralField() ? storeMineral(resource) : storeGas(resource);
	else if (resource->getType().isMineralField())
		storeBoulder(resource);
	return;
}

void ResourceManager::storeMineral(Unit resource)
{
	ResourceInfo& m = myMinerals[resource];
	m.setGathererCount(0);
	m.setRemainingResources(resource->getResources());
	m.setUnit(resource);
	m.setType(resource->getType());
	m.setPosition(resource->getPosition());
	m.setWalkPosition(Util().getWalkPosition(resource));
	m.setTilePosition(resource->getTilePosition());
	return;
}

void ResourceManager::storeGas(Unit resource)
{
	ResourceInfo& g = myGas[resource];
	g.setGathererCount(0);
	g.setRemainingResources(resource->getResources());
	g.setUnit(resource);
	g.setType(resource->getType());
	g.setPosition(resource->getPosition());
	g.setWalkPosition(Util().getWalkPosition(resource));
	g.setTilePosition(resource->getTilePosition());
	return;
}

void ResourceManager::storeBoulder(Unit resource)
{
	ResourceInfo& b = myBoulders[resource];
	b.setGathererCount(0);
	b.setRemainingResources(resource->getResources());
	b.setUnit(resource);
	b.setType(resource->getType());
	b.setPosition(resource->getPosition());
	b.setWalkPosition(Util().getWalkPosition(resource));
	b.setTilePosition(resource->getTilePosition());
	return;
}

void ResourceManager::removeResource(Unit resource)
{
	// Remove dead resources
	if (myMinerals.find(resource) != myMinerals.end())
		myMinerals.erase(resource);
	else if (myBoulders.find(resource) != myBoulders.end())
		myBoulders.erase(resource);

	// Any workers that targeted that resource now have no target
	for (auto &worker : Workers().getMyWorkers()) {
		if (worker.second.hasResource() && worker.second.getResource().unit() == resource)
			worker.second.setResource(nullptr);
	}
	return;
}