#include "McRave.h"

void ResourceTrackerClass::update()
{
	Display().startClock();
	updateResources();
	Display().performanceTest(__FUNCTION__);
	return;
}

void ResourceTrackerClass::updateResources()
{
	// Assume mineral saturation, will be changed to false if any mineral field has less than 2 gatherers
	minSat = true;
	for (auto &m : myMinerals)
	{
		ResourceInfo& resource = m.second;	
		if (!resource.unit()) continue;
		if (resource.unit()->exists())
		{
			resource.setRemainingResources(resource.unit()->getResources());
		}
		if (minSat && resource.getGathererCount() < 2 && resource.getState() > 0)
		{
			minSat = false;
		}
	}

	// Assume gas saturation, will be changed to false if any gas geyser has less than 3 gatherers
	gasSat = true;
	tempGasCount = 0;
	for (auto &g : myGas)
	{
		ResourceInfo& resource = g.second;
		if (!resource.unit()) continue;
		if (resource.unit()->exists())
		{
			resource.setType(resource.unit()->getType());
			resource.setRemainingResources(resource.unit()->getResources());
		}
		if (resource.getGathererCount() < 3 && resource.getType() != UnitTypes::Resource_Vespene_Geyser && resource.unit()->isCompleted() && resource.getState() > 0)
		{			
			gasSat = false;
		}
		if (resource.getState() == 2)
		{
			tempGasCount++;
		}
	}
	
	// Calculate minerals and gas per minute
	int gas = Broodwar->self()->gas() + Broodwar->self()->spentGas() + Broodwar->self()->refundedGas() + Broodwar->self()->repairedGas();
	int mineral = Broodwar->self()->minerals() + Broodwar->self()->spentMinerals() + Broodwar->self()->refundedMinerals() + Broodwar->self()->repairedMinerals();
	GPM = (GPM * 1439 / 1440) + (gas - lastGas);
	MPM = (MPM * 1439 / 1440) + (mineral - lastMineral);
	lastGas = gas;
	lastMineral = mineral;
	return;
}

void ResourceTrackerClass::storeResource(Unit resource)
{
	if (resource->getInitialResources() > 0)
	{
		resource->getType().isMineralField() ? storeMineral(resource) : storeGas(resource);		
	}
	else
	{
		storeBoulder(resource);
	}
	return;
}

void ResourceTrackerClass::storeMineral(Unit resource)
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

void ResourceTrackerClass::storeGas(Unit resource)
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

void ResourceTrackerClass::storeBoulder(Unit resource)
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

void ResourceTrackerClass::removeResource(Unit resource)
{
	// Remove dead resources
	if (myMinerals.find(resource) != myMinerals.end())
	{
		myMinerals.erase(resource);
	}
	else if (myBoulders.find(resource) != myBoulders.end())
	{
		myBoulders.erase(resource);
	}

	// Any workers that targeted that resource now have no target
	for (auto &worker : Workers().getMyWorkers())
	{
		if (worker.second.hasResource() && worker.second.getResource().unit() == resource)
		{
			worker.second.setResource(nullptr);
		}
	}
	return;
}