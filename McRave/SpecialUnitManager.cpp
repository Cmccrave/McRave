#include "McRave.h"

void SpecialUnitTrackerClass::update()
{
	Display().startClock();
	updateArbiters();
	updateDetectors();
	updateReavers();
	updateVultures();
	Display().performanceTest(__FUNCTION__);
	return;
}

void SpecialUnitTrackerClass::updateArbiters()
{
	for (auto &a : Units().getAllyUnitsFilter(UnitTypes::Protoss_Arbiter))
	{
		UnitInfo &arbiter = Units().getUnitInfo(a);
		int bestCluster = 0;
		double closestD = 0.0;
		Position bestPosition = Grids().getAllyArmyCenter();
		WalkPosition start = arbiter.getWalkPosition();

		for (int x = start.x - 20; x <= start.x + 20; x++)
		{
			for (int y = start.y - 20; y <= start.y + 20; y++)
			{
				// If not valid, has an EMP on it, an Arbiter moving to it or isn't safe, continue
				if (!WalkPosition(x, y).isValid() || Grids().getEMPGrid(x, y) > 0 || Grids().getArbiterGrid(x, y) > 0 || !Util().isSafe(WalkPosition(x, y), UnitTypes::Protoss_Arbiter, false, true)) continue;

				// If this position is closer or has a higher cluster of ally units
				if (closestD == 0.0 || Grids().getAGroundCluster(x, y) > bestCluster || (Grids().getAGroundCluster(x, y) == bestCluster && Terrain().getPlayerStartingPosition().getDistance(Position(WalkPosition(x, y))) < closestD))
				{
					closestD = Terrain().getPlayerStartingPosition().getDistance(Position(WalkPosition(x, y)));
					bestCluster = Grids().getAGroundCluster(x, y);
					bestPosition = Position(WalkPosition(x, y));
				}
			}
		}

		// Move and update grids	
		arbiter.setEngagePosition(bestPosition);
		arbiter.unit()->move(bestPosition);
		Grids().updateArbiterMovement(arbiter);

		// If there's a stasis target, cast stasis on it		
		if (arbiter.hasTarget() && arbiter.getTarget().unit()->exists() && arbiter.unit()->getEnergy() >= 100)
		{
			arbiter.unit()->useTech(TechTypes::Stasis_Field, arbiter.getTarget().unit());
		}
	}
	return;
}

void SpecialUnitTrackerClass::updateDetectors()
{
	for (auto &d : Units().getAllyUnitsFilter(UnitTypes::Protoss_Observer))
	{
		UnitInfo &detector = Units().getUnitInfo(d);

		// Check if there is a unit that needs revealing
		if (detector.hasTarget() && detector.getTarget().unit()->exists() && Grids().getAAirThreat(detector.getTarget().getWalkPosition()) > Grids().getEAirThreat(detector.getTarget().getWalkPosition()))
		{
			detector.setEngagePosition(detector.getTarget().getPosition());
			detector.unit()->move(detector.getTarget().getPosition());
			Grids().updateDetectorMovement(detector);
			continue;
		}

		// Check if any expansions need detection on them
		if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observer) > 1 && BuildOrder().getBuildingDesired()[UnitTypes::Protoss_Nexus] > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) && Grids().getADetectorGrid(WalkPosition(Buildings().getCurrentExpansion())) == 0)
		{
			detector.setEngagePosition(Position((Buildings().getCurrentExpansion())));
			detector.unit()->move(Position((Buildings().getCurrentExpansion())));
			Grids().updateDetectorMovement(detector);
			continue;
		}

		// Move towards lowest enemy air threat, no enemy detection and closest to enemy starting position	
		double closestD = 0.0;
		Position bestPosition = Grids().getAllyArmyCenter();
		WalkPosition start = detector.getWalkPosition();
		for (int x = start.x - 20; x <= start.x + 20; x++)
		{
			for (int y = start.y - 20; y <= start.y + 20; y++)
			{
				if (!WalkPosition(x, y).isValid()) continue;

				if (Grids().getEDetectorGrid(x, y) == 0 && Position(WalkPosition(x, y)).getDistance(Position(start)) > 64 && (Grids().getAAirThreat(x, y) > 0 || Grids().getAGroundThreat(x, y) > 0) && Grids().getADetectorGrid(x, y) == 0 && (Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition()) < closestD || closestD == 0.0))
				{
					bestPosition = Position(WalkPosition(x, y));
					closestD = Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition());
				}
			}
		}
		if (bestPosition.isValid())
		{
			detector.setEngagePosition(bestPosition);
			detector.unit()->move(bestPosition);
			Grids().updateDetectorMovement(detector);
		}
	}

	for (auto &d : Buildings().getAllyBuildingsFilter(UnitTypes::Terran_Comsat_Station))
	{
		// ??? Should I store as a unit?
	}
	return;
}

void SpecialUnitTrackerClass::updateReavers()
{
	for (auto &r : Units().getAllyUnitsFilter(UnitTypes::Protoss_Reaver))
	{
		UnitInfo &reaver = Units().getUnitInfo(r);

		// If we need Scarabs
		if (!reaver.unit()->isLoaded() && reaver.unit()->getScarabCount() < 5)
		{
			reaver.unit()->train(UnitTypes::Protoss_Scarab);
			reaver.setLastAttackFrame(Broodwar->getFrameCount());
		}
	}
	return;
}

void SpecialUnitTrackerClass::updateVultures()
{
	for (auto &v : Units().getAllyUnitsFilter(UnitTypes::Terran_Vulture))
	{
		UnitInfo &vulture = Units().getUnitInfo(v);
		double closestD = 0.0;
		int x = rand() % 64 - 64;
		int y = rand() % 64 - 64;
		Position closestP;
		// If we have mines, plant them at a either the target or a chokepoint
		if (Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && vulture.unit()->getSpiderMineCount() > 0 && (vulture.getStrategy() == 1 || vulture.getStrategy() == 0) && Broodwar->getUnitsInRadius(vulture.getPosition(), 64, Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine).size() <= 0)
		{
			if (vulture.unit()->getLastCommand().getTechType() != TechTypes::Spider_Mines || vulture.unit()->getLastCommand().getTargetPosition().getDistance(vulture.getPosition()) > 10)
			{
				vulture.unit()->useTech(TechTypes::Spider_Mines, vulture.getPosition());
			}
			vulture.setLastCommandFrame(Broodwar->getFrameCount());
		}
	}
	return;
}