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
		UnitInfo arbiter = a.second;
		/*if (Broodwar->self()->hasResearched(TechTypes::Recall) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Arbiter) > 1 && arbiter.unit()->getEnergy() > 100 && (!recaller || (recaller && !recaller->exists())))
		{
		recaller = arbiter.unit();
		}*/

		int bestCluster = 0;
		double closestD = 0.0;
		Position bestPosition = Grids().getAllyArmyCenter();
		WalkPosition start = arbiter.getWalkPosition();

		//if (recaller && arbiter.unit()->getEnergy() > 100)
		//{
		//	if (arbiter.unit()->getDistance(Terrain().getPlayerStartingPosition()) > 320 && Grids().getStasisCluster(arbiter.getWalkPosition()) < 4)
		//	{
		//		// Move towards areas with no threat close to enemy starting position
		//		for (int x = start.x - 20; x <= start.x + 20; x++)
		//		{
		//			for (int y = start.y - 20; y <= start.y + 20; y++)
		//			{
		//				// If the Arbiter finds a tank cluster that is fairly high with little to no air threat, lock in moving there
		//				if (WalkPosition(x, y).isValid() && Grids().getStasisCluster(WalkPosition(x, y)) >= 4 && Grids().getEAirThreat(WalkPosition(x, y)) < 10)
		//				{
		//					closestD = -1;
		//					bestPosition = Position(WalkPosition(x, y));
		//				}

		//				// Else if the Arbiter finds a tile that is no threat and closer to the enemy starting position
		//				else if (WalkPosition(x, y).isValid() && (closestD == 0.0 || Terrain().getEnemyStartingPosition().getDistance(Position(WalkPosition(x, y))) < closestD))
		//				{
		//					if (Util().isSafe(start, WalkPosition(x, y), UnitTypes::Protoss_Arbiter, false, true, false))
		//					{
		//						closestD = Terrain().getEnemyStartingPosition().getDistance(Position(WalkPosition(x, y)));
		//						bestPosition = Position(WalkPosition(x, y));
		//					}
		//				}
		//			}
		//		}
		//	}
		//	else
		//	{
		//		recaller->useTech(TechTypes::Recall, Grids().getAllyArmyCenter());
		//		Strategy().recallEvent();
		//		continue;
		//	}
		//}
		//else
		//{
		// Move towards high cluster counts and closest to ally starting position
		for (int x = start.x - 20; x <= start.x + 20; x++)
		{
			for (int y = start.y - 20; y <= start.y + 20; y++)
			{
				if (WalkPosition(x, y).isValid() && Grids().getEMPGrid(x, y) == 0 && Grids().getArbiterGrid(x, y) == 0 && (closestD == 0.0 || Grids().getACluster(x, y) > bestCluster || (Grids().getACluster(x, y) == bestCluster && Terrain().getPlayerStartingPosition().getDistance(Position(WalkPosition(x, y))) < closestD)))
				{
					if (Util().isSafe(start, WalkPosition(x, y), UnitTypes::Protoss_Arbiter, false, true, false))
					{
						closestD = Terrain().getPlayerStartingPosition().getDistance(Position(WalkPosition(x, y)));
						bestCluster = Grids().getACluster(x, y);
						bestPosition = Position(WalkPosition(x, y));
					}
				}
			}
		}
		//}

		// Move and update grids	
		arbiter.setEngagePosition(bestPosition);
		arbiter.unit()->move(bestPosition);
		Grids().updateArbiterMovement(arbiter);

		// If there's a stasis target, cast stasis on it
		Unit target = Units().getAllyUnits()[arbiter.unit()].getTarget();
		if (target && target->exists() && arbiter.unit()->getEnergy() >= 100)
		{
			arbiter.unit()->useTech(TechTypes::Stasis_Field, target);
		}
	}
	return;
}

void SpecialUnitTrackerClass::updateDetectors()
{
	for (auto &d : Units().getAllyUnitsFilter(UnitTypes::Protoss_Observer))
	{
		UnitInfo detector = d.second;
		Unit target = detector.getTarget();

		// Check if there is a unit that needs revealing
		if (target && target->exists() && Grids().getEDetectorGrid(Util().getWalkPosition(target)) == 0)
		{
			detector.setEngagePosition(target->getPosition());
			detector.unit()->move(target->getPosition());
			Grids().updateDetectorMovement(detector);
			continue;
		}

		// Check if any expansions need detection on them
		if (BuildOrder().getBuildingDesired()[UnitTypes::Protoss_Nexus] > Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus))
		{
			if (Grids().getEDetectorGrid(WalkPosition(Buildings().getCurrentExpansion())) == 0)
			{
				detector.setEngagePosition(Position((Buildings().getCurrentExpansion())));
				detector.unit()->move(Position((Buildings().getCurrentExpansion())));
				Grids().updateDetectorMovement(detector);
				continue;
			}
		}

		// Move towards lowest enemy air threat, no enemy detection and closest to enemy starting position	
		double closestD = 0.0;
		Position newDestination = Grids().getAllyArmyCenter();
		WalkPosition start = detector.getWalkPosition();
		for (int x = start.x - 20; x <= start.x + 20; x++)
		{
			for (int y = start.y - 20; y <= start.y + 20; y++)
			{
				if (WalkPosition(x, y).isValid() && Grids().getEDetectorGrid(x, y) == 0 && Position(WalkPosition(x, y)).getDistance(Position(start)) > 64 && Grids().getACluster(x, y) > 0 && Grids().getADetectorGrid(x, y) == 0 && Grids().getEAirThreat(x, y) == 0.0 && (Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition()) < closestD || closestD == 0))
				{
					newDestination = Position(WalkPosition(x, y));
					closestD = Position(WalkPosition(x, y)).getDistance(Terrain().getEnemyStartingPosition());
				}
			}
		}
		if (newDestination.isValid())
		{
			detector.setEngagePosition(newDestination);
			detector.unit()->move(newDestination);
			Grids().updateDetectorMovement(detector);
		}
		continue;
	}
	return;
}

void SpecialUnitTrackerClass::updateReavers()
{
	for (auto &r : Units().getAllyUnitsFilter(UnitTypes::Protoss_Reaver))
	{
		UnitInfo &reaver = r.second;

		// If we need Scarabs
		if (reaver.unit()->getScarabCount() < 5)
		{
			reaver.unit()->train(UnitTypes::Protoss_Scarab);
		}
	}
	return;
}

void SpecialUnitTrackerClass::updateVultures()
{
	for (auto &v : Units().getAllyUnitsFilter(UnitTypes::Terran_Vulture))
	{
		UnitInfo &vulture = v.second;
		double closestD = 0.0;
		int x = rand() % 64 - 64;
		int y = rand() % 64 - 64;
		Position closestP;
		// If we have mines, plant them at a either the target or a chokepoint
		if (vulture.unit()->getSpiderMineCount() > 0 && (vulture.getStrategy() == 1 || vulture.getStrategy() == 0))
		{
			/*if (vulture.getTarget() && vulture.getTarget()->exists())
			{
			vulture.unit()->useTech(TechTypes::Spider_Mines, vulture.getTargetPosition());
			}
			else
			{
			for (auto choke : Terrain().getPath())
			{
			if ((closestD == 0.0 || Position(choke->Center()).getDistance(Terrain().getPlayerStartingPosition()) < closestD) && Broodwar->getUnitsInRadius(Position(choke->Center()), 128, Filter::GetType == UnitTypes::Terran_Vulture_Spider_Mine && Filter::IsAlly).size() < 2)
			{
			closestD = Position(choke->Center()).getDistance(Terrain().getPlayerStartingPosition());
			closestP = Position(choke->Center());
			}
			}

			if (closestP.isValid())
			{*/
			if (vulture.unit()->getLastCommand().getTechType() != TechTypes::Spider_Mines)
			{
				vulture.unit()->useTech(TechTypes::Spider_Mines, vulture.getPosition());
			}
			vulture.setLastCommandFrame(Broodwar->getFrameCount());
		}
	}
	return;
}