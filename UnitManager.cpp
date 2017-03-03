#include "UnitManager.h"
#include "TargetManager.h"
#include "BWTA.h"

using namespace BWAPI;
using namespace std;
using namespace BWTA;

bool harassing = false;

int unitGetLocalStrategy(Unit unit)
{
	// Check for local strength, based on that make an adjustment
	//	1) If our local strength is higher, fight
	//  2) If our local strength is lower, regroup at regroupPosition

	//// Check if we are in ally territory, if so, fight
	if (find(allyTerritory.begin(), allyTerritory.end(), BWTA::getRegion(unit->getPosition())) != allyTerritory.end() && unit->getUnitsInRadius(640, Filter::IsEnemy && !Filter::IsWorker && Filter::CanAttack).size() > 0)
	{
		return 1;
	}
	// Based on units around, check if we can fight it
	if ((unit->getUnitsInRadius(640, Filter::IsEnemy && !Filter::IsWorker && Filter::CanAttack).size() > unit->getUnitsInRadius(640, Filter::IsAlly && !Filter::IsWorker && Filter::CanAttack).size() || enemyStrength > allyStrength) && unit->getUnitsInRadius(640, Filter::IsEnemy && !Filter::IsWorker && Filter::CanAttack).size() > 0)
	{
		double enemyLocalStrength = 0, allyLocalStrength = 0;
		// Add one frame to each to prevent divide by zero/huge numbers
		for (Unit enemy : unit->getUnitsInRadius(640, Filter::IsEnemy && !Filter::IsWorker))
		{
			enemyLocalStrength += unitGetStrength(enemy);
		}
		for (Unit ally : unit->getUnitsInRadius(640, Filter::IsAlly && !Filter::IsBuilding && !Filter::IsWorker))
		{
			// If shuttle, add units inside
			if (ally->getType() == UnitTypes::Protoss_Shuttle && ally->getLoadedUnits().size() > 0)
			{
				for (Unit u : ally->getLoadedUnits())
				{
					// allyLocalStrength += (double(u->getShields() + u->getHitPoints()) / 100) + ally->getType().groundWeapon().maxRange()/32 + (double(u->getType().groundWeapon().damageAmount()) / double(u->getType().groundWeapon().damageCooldown() + 1));
					allyLocalStrength += unitGetStrength(u);
				}
			}
			else
			{
				allyLocalStrength += unitGetStrength(ally);
			}
		}
		if (enemyLocalStrength > allyLocalStrength || enemyStrength > allyStrength)
		{
			// We have lower strength, regroup
			return 0;
		}
		else
		{
			// We have higher strength
			return 1;
		}
	}
	// We have higher number count
	return 1;
}

int unitGetGlobalStrategy()
{
	//// If we have 6 dragoons ready, timing attack is ready
	//if (Broodwar->self()->supplyUsed() > 300)
	//{
	//	return 1;
	//}
	if (allyStrength > enemyStrength)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void unitGetCommand(Unit unit)
{
	// ++ No shield batteries currently ++
	//// Check if low shield and shield battery can be used
	//if (unit->getShields() < 10 && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shield_Battery) > 0)
	//{
	//	Unit battery = Broodwar->getClosestUnit(unit->getPosition(), Filter::IsAlly && Filter::GetType == UnitTypes::Protoss_Shield_Battery && Filter::Energy > 10);
	//	if (unit->getDistance(battery) < 256 && unit->getLastCommandFrame() > Broodwar->getFrameCount() + 20)
	//	{
	//		unit->rightClick(battery);
	//		return;
	//	}
	//}

	// Check local strategy manager to see what our next task is
	// If 0, regroup unless forced to engage
	if (unitGetLocalStrategy(unit) == 0)
	{
		Position regroupPosition = unitRegroup(unit);
		if (find(allyTerritory.begin(), allyTerritory.end(), getRegion(TilePosition(regroupPosition))) != allyTerritory.end())
		{
			forceEngage = true;
			unitMicro(unit);
			return;
		}
		// If we are close to a chokepoint, let's fight so we don't fight inside a chokepoint and have poor positioning
		for (auto position : defendHere)
		{
			if (regroupPosition.getDistance(position) < 640 && Broodwar->self()->supplyUsed() > 100)
			{
				forceEngage = true;
				unitMicro(unit);
				return;
			}
			else if (regroupPosition.getDistance(position) > 2560)
			{
				forceEngage = false;
			}
		}

		// If regrouping, get reaver back into shuttle and run
		if (unit->getType() == UnitTypes::Protoss_Reaver)
		{
			if (find(reaverID.begin(), reaverID.end(), unit->getID()) - reaverID.begin() < (int)shuttleID.size())
			{
				unit->rightClick(Broodwar->getUnit(shuttleID.at(find(reaverID.begin(), reaverID.end(), unit->getID()) - reaverID.begin())));
				return;
			}
			else
			{
				unitMicro(unit);
				return;
			}
		}

		// If we are on top of our ramp, let's hold with zealots
		if (unit->getDistance(defendHere.at(0)) < 32 && unit->getType() == UnitTypes::Protoss_Zealot && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Zealot) > 2 && BWTA::getRegion(unit->getPosition()) == BWTA::getRegion(playerStartingPosition))
		{
			unit->holdPosition();
			return;
		}

		// If regroup position is in low threat and no enemies around, stop and move to it
		if (Broodwar->getUnitsInRadius(regroupPosition, 512, Filter::IsEnemy).size() < 1 && threatArray[regroupPosition.x / 32][regroupPosition.y / 32] < 1)
		{
			//unit->move(regroupPosition);
			unit->move(Position(defendHere.at(rand() % defendHere.size()).x + rand() % -50 + 50, defendHere.at(rand() % defendHere.size()).y + rand() % -50 + 50));
			return;
		}
	}
	// If fighting and there's an enemy around, micro
	else if (unitGetLocalStrategy(unit) == 1 && unit->getClosestUnit(Filter::IsEnemy))
	{
		forceEngage = false;
		unitMicro(unit);
		return;
	}
	// Check our global strategy
	else if (unitGetGlobalStrategy() == 0)
	{
		if (enemyBasePositions.size() > 0 && allyTerritory.size() > 0)
		{
			// Check if we are close enough to any defensive position
			for (auto position : defendHere)
			{
				if (unit->getDistance(position) < 200)
				{
					return;
				}
			}
			// Else defend at one of the vulnerable chokepoints
			unit->move(Position(defendHere.at(rand() % defendHere.size()).x + rand() % -50 + 50, defendHere.at(rand() % defendHere.size()).y + rand() % -50 + 50));
			return;
		}
		else
		{
			// Else just defend at nearest chokepoint (starting of game without scout information)
			unit->move((BWTA::getNearestChokepoint(unit->getPosition()))->getCenter());
			return;
		}
	}

	// Check if we should attack
	else if (unitGetGlobalStrategy() == 1)
	{
		if (enemyBasePositions.size() > 0)
		{
			unit->attack(enemyBasePositions.front());
			return;
		}
	}
}

double unitGetStrength(Unit unit)
{
	// Some hardcoded values
	if (unit->getType() == UnitTypes::Terran_Bunker)
	{
		return 20.0;
	}
	if (unit->getType() == UnitTypes::Terran_Medic)
	{
		return 5.0;
	}
	if (unit->getType() == UnitTypes::Zerg_Lurker)
	{
		return 6.0;
	}

	if (!unit->getType().isWorker() && unit->isCompleted() && unit->getType() != UnitTypes::Protoss_Scarab && unit->getType() != UnitTypes::Terran_Vulture_Spider_Mine && (unit->getType().groundWeapon().damageAmount() > 0 || !unit->getType().isBuilding()))
	{
		double range, damage, hp;
		// Range upgrade check
		if (unit->getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge))
		{
			range = 192.0;
		}
		else
		{
			range = double(unit->getType().groundWeapon().maxRange());
		}

		// Damage
		damage = double(unit->getType().groundWeapon().damageAmount());

		// Hp
		if (unit->exists())
		{
			hp = double(unit->getHitPoints() + unit->getShields());
		}
		else
		{
			hp = double(unit->getType().maxHitPoints() + unit->getType().maxShields());
		}
		return sqrt((1 + (range / 320.0)) * damage + hp / 1000);
	}
	else
	{
		return 0;
	}
}

Position unitGetPath(Unit unit)
{
	int x = unit->getTilePosition().x;
	int y = unit->getTilePosition().y;
	int cnt = 0;
	//TilePosition target = Broodwar->getClosestUnit(enemyStartingPosition, Filter::IsMineralField)->getTilePosition();
	TilePosition target = enemyStartingTilePosition;

	// Reset path
	for (int i = 0; i <= Broodwar->mapWidth(); i++)
	{
		for (int j = 0; j <= Broodwar->mapHeight(); j++)
		{
			if (pathArray[i][j] < 256)
			{
				pathArray[i][j] = 0;
			}
		}
	}

	pathArray[x][y] = 0;
	pathArray[target.x][target.y] = 1;

	// While current units tile is 0 (no path currently)
	while (pathArray[x][y] < 1)
	{
		cnt++;
		// For each tile equal to cnt, move up and to the left, apply cnt + 1 to a 3x3 grid
		for (int i = 0; i <= Broodwar->mapWidth(); i++)
		{
			for (int j = 0; j <= Broodwar->mapHeight(); j++)
			{
				// If it's equal
				if (pathArray[i][j] == cnt)
				{
					// Get 3x3 grid, set each tile to cnt + 1
					for (int a = i - 1; a <= i + 1; a++)
					{
						for (int b = j - 1; b <= j + 1; b++)
						{
							if (a >= 0 && a <= 256 && b >= 0 && b <= 256 && pathArray[a][b] == 0)
							{
								pathArray[a][b] = cnt + 1;
							}
						}
					}
				}
			}
		}
	}
	int lowestInt = 257;
	Position lowestPosition = unit->getPosition();
	for (int i = x - 3; i <= x + 4; i++)
	{
		for (int j = y - 3; j <= y + 4; j++)
		{
			if (airThreatArray[i][j] == 0 && pathArray[i][j] > 0 && pathArray[i][j] < lowestInt && i > 0 && i < Broodwar->mapWidth() && j > 0 && j < Broodwar->mapHeight())
			{
				lowestPosition = Position((i * 32) + 16, (j * 32) + 16);
				lowestInt = pathArray[i][j];
			}
		}
	}
	return lowestPosition;
}

void unitMicro(Unit unit)
{
	// Variables
	bool kite;
	int range = unit->getType().groundWeapon().maxRange();
	Unit currentTarget;

	if (unit->getType() == UnitTypes::Protoss_Reaver)
	{
		currentTarget = clusterTargetPriority(unit);
	}
	else
	{
		currentTarget = targetPriority(unit);
	}

	// Range upgrade check
	if (unit->getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge))
	{
		range = 192;
	}

	// If the target and unit exists, let's micro the unit
	if (currentTarget && unit)
	{
		// If unit is attacking or in an attack frame, don't want to micro
		if (unit->isAttackFrame() || unit->isStartingAttack())
		{
			return;
		}

		// If recently issued a command to attack a unit, don't want to micro
		if (unit->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && unit->getTarget() == currentTarget)
		{
			return;
		}

		// If units too far away, move back
		if (unit->getDistance(unit->getClosestUnit(Filter::IsAlly && Filter::IsBuilding)) > 640 && unitGetGlobalStrategy == 0)
		{
			unit->move((Position(defendHere.at(rand() % defendHere.size()).x + rand() % 100, defendHere.at(rand() % defendHere.size()).y + rand() % 100)));
		}

		// If kiting unnecessary, disable
		if (currentTarget->getType().isFlyer() || currentTarget->getType().isBuilding())
		{
			kite = false;
		}

		// If kiting is a good idea, enable
		else if (currentTarget->getType() == UnitTypes::Terran_Vulture_Spider_Mine || (range > 32 && unit->isUnderAttack()) || (currentTarget->getType().groundWeapon().maxRange() <= range && (unit->getDistance(currentTarget) < range - currentTarget->getType().groundWeapon().maxRange() || unit->getHitPoints() < 40)))
		{
			kite = true;
		}

		// If kite is true and weapon on cooldown, move
		if (kite && unit->getLastCommand().getType() != UnitCommandTypes::Move && Broodwar->getLatencyFrames() / 2 <= unit->getGroundWeaponCooldown())
		{
			Position correctedFleePosition = unitFlee(unit, currentTarget);
			if (correctedFleePosition != BWAPI::Positions::None)
			{
				Broodwar->drawLineMap(unit->getPosition(), correctedFleePosition, Colors::Red);
				unit->move(correctedFleePosition);
			}
		}
		// Else, regardless of if kite is true or not, attack if weapon is off cooldown
		else if (Broodwar->getLatencyFrames() / 2 > unit->getGroundWeaponCooldown())
		{
			unit->attack(currentTarget);
		}
	}
}

Position unitRegroup(Unit unit)
{
	Unitset regroupSet = unit->getUnitsInRadius(50000, Filter::IsAlly && !Filter::IsBuilding && !Filter::IsWorker);
	Position regroupPosition;
	for (Unit re : regroupSet)
	{
		regroupPosition += re->getPosition();
	}
	if (regroupSet.size() > 0)
	{
		return (regroupPosition / regroupSet.size());
	}
	else
	{
		return Broodwar->getClosestUnit(unit->getPosition(), Filter::IsAlly && Filter::IsResourceDepot)->getPosition();
	}
}

Position unitFlee(Unit unit, Unit currentTarget)
{
	if (!unit || !currentTarget)
	{
		return BWAPI::Positions::None;
	}

	double slopeDegree;
	int x, y;
	Position currentTargetPosition = currentTarget->getPosition();

	// Divide by zero check, if zero then we are fleeing horizontally, no problem if fleeing vertically.
	if ((unit->getPosition().x - currentTargetPosition.x) == 0)
	{
		slopeDegree = 1.571;
	}

	else
	{
		slopeDegree = atan((unit->getPosition().y - currentTargetPosition.y) / (unit->getPosition().x - currentTargetPosition.x));
	}

	// Need to make sure we are fleeing in the correct direction
	if (unit->getPosition().x > currentTargetPosition.x)
	{
		x = (int)(5 * cos(slopeDegree)) + unit->getTilePosition().x;
	}
	else
	{
		x = (int)(unit->getTilePosition().x - (5 * cos(slopeDegree)));
	}
	if (unit->getPosition().y > currentTargetPosition.y)
	{
		y = (int)(unit->getTilePosition().y + abs(5 * sin(slopeDegree)));
	}
	else
	{
		y = (int)(unit->getTilePosition().y - abs(5 * sin(slopeDegree)));
	}

	Position initialPosition = Position(x * 32, y * 32);
	// Spiral variables
	int length = 1;
	int j = 0;
	bool first = true;
	int dx = 0;
	int dy = 1;
	// Searches in a spiral around the specified tile position
	while (length < 2000)
	{
		//If threat is low, move there
		if (x >= 0 && x < BWAPI::Broodwar->mapWidth() && y >= 0 && y < BWAPI::Broodwar->mapHeight())
		{
			if (threatArray[x][y] < 1 && (Position(x * 32, y * 32).getDistance(BWTA::getNearestChokepoint(unit->getPosition())->getCenter()) < 256 || BWTA::getRegion(unit->getPosition()) == BWTA::getRegion(Position(x * 32, y * 32))))
			{
				return BWAPI::Position(x * 32, y * 32);
			}
		}
		//Otherwise, move to another position
		x = x + dx;
		y = y + dy;
		//Count how many steps we take in this direction
		j++;
		if (j == length) //if we've reached the end, its time to turn
		{
			//reset step counter
			j = 0;

			//Increment step counter
			if (!first)
				length++;

			//First=true for every other turn so we spiral out at the right rate
			first = !first;

			//Turn counter clockwise 90 degrees:
			if (dx == 0)
			{
				dx = dy;
				dy = 0;
			}
			else
			{
				dy = -dx;
				dx = 0;
			}
		}
	}
	return initialPosition;
}

void shuttleManager(Unit unit)
{
	bool harassing = false;
	if (find(shuttleID.begin(), shuttleID.end(), unit->getID()) == shuttleID.end())
	{
		shuttleID.push_back(unit->getID());
	}

	if (find(harassShuttleID.begin(), harassShuttleID.end(), unit->getID()) != harassShuttleID.end())
	{
		harassing = true;
	}

	if (unit->getUnitsInRadius(256, Filter::IsEnemy && !Filter::IsFlyer).size() >= 1)
	{
		Unitset loadedUnits = unit->getLoadedUnits();
		for (Unitset::iterator itr = loadedUnits.begin(); itr != loadedUnits.end(); itr++)
		{
			// If we are loaded and either under attack, cant unload or trying to retreat, move away
			if (unit->isUnderAttack() || !unit->canUnloadAll())
			{
				unit->move(unitFlee(unit, unit->getClosestUnit(Filter::IsEnemy)));
				return;
			}
			else if (Broodwar->getLatencyFrames() > (*itr)->getGroundWeaponCooldown() && (unitGetLocalStrategy(unit) == 1 || forceEngage == true || harassing == true))
			{
				unit->unload(*itr);
				return;
			}
		}
		if (find(shuttleID.begin(), shuttleID.end(), unit->getID()) - shuttleID.begin() < (int)reaverID.size())
		{
			if (!unit->getTarget() && loadedUnits.size() < 1 && Broodwar->getLatencyFrames() + 30 >= Broodwar->getUnit(reaverID.at(find(shuttleID.begin(), shuttleID.end(), unit->getID()) - shuttleID.begin()))->getGroundWeaponCooldown())
			{
				Position correctedFleePosition = unitFlee(unit, unit->getClosestUnit(Filter::IsEnemy && !Filter::IsFlyer && !Filter::IsWorker && Filter::CanAttack));
				if (correctedFleePosition != BWAPI::Positions::None)
				{
					Broodwar->drawLineMap(unit->getPosition(), correctedFleePosition, Colors::Orange);
					unit->move(correctedFleePosition);
				}
			}
		}
	}
	else if (find(shuttleID.begin(), shuttleID.end(), unit->getID()) - shuttleID.begin() < (int)reaverID.size() && unit->getLoadedUnits().size() < 1)
	{
		unit->follow(Broodwar->getUnit(reaverID.at(find(shuttleID.begin(), shuttleID.end(), unit->getID()) - shuttleID.begin())));
	}
	else
	{
		unit->follow(Broodwar->getClosestUnit(enemyStartingPosition, Filter::GetType == UnitTypes::Protoss_Dragoon && Filter::IsAlly));
	}
}

void shuttleHarass(Unit unit)
{
	if (unit->getLoadedUnits().size() > 0 && (pathArray[unit->getTilePosition().x][unit->getTilePosition().y] > 2 || pathArray[unit->getTilePosition().x][unit->getTilePosition().y] == 0))
	{
		unit->move(unitGetPath(unit));
	}
	else
	{
		shuttleManager(unit);
	}
}

void reaverManager(Unit unit)
{
	bool harassing = false;
	if (find(reaverID.begin(), reaverID.end(), unit->getID()) == reaverID.end())
	{
		reaverID.push_back(unit->getID());
	}
	if (find(harassReaverID.begin(), harassReaverID.end(), unit->getID()) != harassReaverID.end())
	{
		harassing = true;
	}
	if (unit->getScarabCount() < 10)
	{
		unit->train(UnitTypes::Protoss_Scarab);
	}
	if ((!harassing && unitGetLocalStrategy(unit) == 0 && forceEngage == 0) || unit->getUnitsInRadius(256, Filter::IsEnemy && !Filter::IsFlyer).size() < 1 || Broodwar->getLatencyFrames() + 30 < unit->getGroundWeaponCooldown())
	{
		// If a shuttle is following a unit, it is empty, see shuttle manager
		if (find(reaverID.begin(), reaverID.end(), unit->getID()) - reaverID.begin() < (int)shuttleID.size())
		{
			unit->rightClick(Broodwar->getUnit(shuttleID.at(find(reaverID.begin(), reaverID.end(), unit->getID()) - reaverID.begin())));
		}
		else
		{
			unitMicro(unit);
		}
	}
	else if (unitGetLocalStrategy(unit) == 1 || forceEngage == 1)
	{
		unitGetCommand(unit);
	}
}

void observerManager(Unit unit)
{
	Unit currentTarget;
	if (unit->isIdle())
	{
		if (unit->getUnitsInRadius(640, Filter::IsEnemy && (Filter::IsBurrowed || Filter::IsCloaked)).size() > 0)
		{
			currentTarget = unit->getClosestUnit(Filter::IsEnemy && (Filter::IsBurrowed || Filter::IsCloaked));
		}
		else
		{
			currentTarget = Broodwar->getClosestUnit(enemyStartingPosition, Filter::IsAlly && Filter::GetType == UnitTypes::Enum::Protoss_Dragoon);
		}
		if (currentTarget)
		{
			unit->follow(currentTarget);
		}
		else
		{
			unit->follow(unit->getClosestUnit(Filter::IsAlly));
		}
	}
}

void templarManager(Unit unit)
{
	if (unit->getUnitsInRadius(320, Filter::IsEnemy).size() < 4)
	{
		unit->follow(Broodwar->getClosestUnit(enemyStartingPosition, Filter::IsAlly && Filter::GetType == UnitTypes::Protoss_Dragoon));
	}
	else
	{
		Unit target = clusterTargetPriority(unit);
		if (target)
		{
			unit->useTech(TechTypes::Psionic_Storm, target);
		}
	}
}

void arbiterManager(Unit unit)
{
	if (unit->getUnitsInRadius(640, Filter::IsEnemy).size() < 4)
	{
		unit->follow(Broodwar->getClosestUnit(enemyStartingPosition, Filter::IsAlly && Filter::GetType == UnitTypes::Enum::Protoss_Dragoon));
	}
	else
	{
		Unit target = clusterTargetPriority(unit);
		if (target)
		{
			unit->useTech(TechTypes::Stasis_Field, target);
		}
	}
}

void carrierManager(Unit unit)
{
	if (unit->getInterceptorCount() < 8)
	{
		unit->train(UnitTypes::Protoss_Interceptor);
	}
	else
	{
		unitGetCommand(unit);
	}
}

int storeEnemyUnit(Unit unit, map<int, UnitInfo>& enemyUnits)
{
	int before = enemyUnits.size();
	UnitInfo newBuilding(unit->getType(), unit->getPosition());
	enemyUnits.emplace(unit->getID(), newBuilding);
	int after = enemyUnits.size();
	// If size is equal, we did not add a building, return 0
	if (before == after)
	{
		return 0;
	}
	// If size changed, we added a building, return 1
	return 1;
}

UnitInfo::UnitInfo(UnitType newType, Position newPosition)
{
	unitType = newType;
	position = newPosition;
}

UnitInfo::UnitInfo()
{
	unitType = UnitTypes::Enum::None;
	position = Positions::None;
}

UnitInfo::~UnitInfo()
{
}

Position UnitInfo::getPosition() const
{
	return position;
}

UnitType UnitInfo::getUnitType() const
{
	return unitType;
}