#pragma once
#include <BWAPI.h>
#include "..\BWEB\BWEB.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{

	//class TroopInfo {

	//};

	//class DefenseInfo {

	//};

	//class SupportInfo {

	//};

	class CombatUnit {
		double visibleGroundStrength, visibleAirStrength, maxGroundStrength, maxAirStrength, priority, engageDist, simValue, simBonus;
		TransportUnit* assignedTransport;
		UnitInfo * unitInfo;
		UnitInfo * assignedTarget;
		Path targetPath;

		void updateTarget();
	public:
		CombatUnit(UnitInfo *);

		// McRave stats
		double getVisibleGroundStrength() { return visibleGroundStrength; }	
		double getMaxGroundStrength() { return maxGroundStrength; }	
		double getVisibleAirStrength() { return visibleAirStrength; }	
		double getMaxAirStrength() { return maxAirStrength; }
		double getPriority() { return priority; }
		
		// Simulation
		void setSimValue(double newValue) { simValue = newValue; }
		double getSimValue() { return simValue; }
		double getSimBonus() { return simBonus; }
		void setSimBonus(double newValue) { simBonus = newValue; }

		// Engagement distances
		double getEngDist() { return engageDist; }
		void setEngDist(double newValue) { engageDist = newValue; }

		// Transport
		TransportUnit * getTransport() { return assignedTransport; }
		void setTransport(TransportUnit * unit) { assignedTransport = unit; }

		// Info
		UnitInfo * info() { return unitInfo; }

		// Targeting		
		bool hasTarget() { return assignedTarget != nullptr; }
		UnitInfo &getTarget() { return *assignedTarget; }
		void setTarget(UnitInfo * unit) { assignedTarget = unit; }

		// Target path
		void setTargetPath(BWEB::Path& newPath) { targetPath = newPath; }
		BWEB::Path& getTargetPath() { return targetPath; }
		bool samePath() {
			return (targetPath.getTiles().front() == assignedTarget->getTilePosition() && targetPath.getTiles().back() == unitInfo->getTilePosition());
		}
	};

	class UnitInfo {
		// StarCraft stats
		double percentHealth, groundRange, airRange, groundDamage, airDamage, speed;								
		int localStrategy, globalStrategy, lastAttackFrame, lastVisibleFrame, shields, health, minStopFrame;
		int killCount, frameCreated;
		int lastMoveFrame;

		bool burrowed;
		bool engage, retreat;

		Unit thisUnit;
		UnitType unitType;
		Player player;
		Role assignedRole;

		Position position, engagePosition, destination, simPosition, lastPos;
		WalkPosition walkPosition, lastWalk;
		TilePosition tilePosition, lastTile;		
		
		void updateStuckCheck();
	public:
		UnitInfo();

		// General use
		void updateUnit();
		void createDummy(UnitType);
		double getDistance(UnitInfo unit) { return position.getDistance(unit.getPosition()); }

		// Roles
		Role getRole() { return assignedRole; }

		// Engage/Retreat
		void resetForces() { engage = false, retreat = false; }
		void setRetreat() { retreat = true; }
		void setEngage() { engage = true; }
		bool shouldRetreat() { return retreat; }
		bool shouldEngage() { return engage; }

		// Last positions
		Position getLastPosition() { return lastPos; }
		WalkPosition getLastWalk() { return lastWalk; }
		TilePosition getLastTile() { return lastTile; }
		bool sameTile() { return lastTile == tilePosition; }
		void setLastPositions();

		// Attack frame
		bool hasAttackedRecently() {
			return (Broodwar->getFrameCount() - lastAttackFrame < 50);
		}

		// Stuck
		bool isStuck() {
			return (Broodwar->getFrameCount() - lastMoveFrame > 50);
		}

		// Creation frame
		void setCreationFrame() { frameCreated = Broodwar->getFrameCount(); }
		int getFrameCreated() { return frameCreated; }
		
		// Starcraft Stats
		double getPercentHealth()			{ return percentHealth; }				// Returns the units health and shield percentage		
		double getGroundRange()				{ return groundRange; }					// Returns the units ground range including upgrades		
		double getAirRange()				{ return airRange; }					// Returns the units air range including upgrades				
		double getGroundDamage()			{ return groundDamage; }				// Returns the units ground damage (not including most upgrades)		
		double getAirDamage()				{ return airDamage; }					// Returns the units air damage (not including most upgrades)		
		double getSpeed()					{ return speed; }						// Returns the units movement speed in pixels per frame including upgrades	
		int getShields()					{ return shields; }
		int getHealth()						{ return health; }		
		int getLastAttackFrame()			{ return lastAttackFrame; }				// Returns the frame on which isStartingAttack was last true		
		int getMinStopFrame()				{ return minStopFrame; }				// Returns the minimum number of frames that the unit needs after a shot before another command can be issued		
		int getLastVisibleFrame()			{ return lastVisibleFrame; }			// Returns the last frame since this unit was visible

		// McRave Stats
		int getLocalStrategy()				{ return localStrategy; }				// Returns the units local strategy		
		int getGlobalStrategy()				{ return globalStrategy; }				// Returns the units global strategy				

		// Kill count
		int getKillCount()					{ return killCount; }
		void setKillCount(int newValue)		{ killCount = newValue; }

		// Debug circles
		void circleGreen()								{ Broodwar->drawCircleMap(position, unitType.width(), Colors::Green); }		
		void circleYellow()								{ Broodwar->drawCircleMap(position, unitType.width(), Colors::Yellow); }
		void circleRed()								{ Broodwar->drawCircleMap(position, unitType.width(), Colors::Red); }

		bool isBurrowed()								{ return burrowed; }
		Position getSimPosition()						{ return simPosition; }
		Unit unit()										{ return thisUnit; }		
		UnitType getType()								{ return unitType; }
		Player getPlayer()								{ return player; }

		Position getPosition()							{ return position; }
		Position getEngagePosition()					{ return engagePosition; }
		Position getDestination()						{ return destination; }
		WalkPosition getWalkPosition()					{ return walkPosition; }
		TilePosition getTilePosition()					{ return tilePosition; }
		
		void setLocalStrategy(int newValue)				{ localStrategy = newValue; }
		void setGlobalStrategy(int newValue)			{ globalStrategy = newValue; }
		void setLastAttackFrame(int newValue)			{ lastAttackFrame = newValue; }

		void setUnit(Unit newUnit)						{ thisUnit = newUnit; }		
		void setType(UnitType newType)					{ unitType = newType; }
		void setPlayer(Player newPlayer)				{ player = newPlayer; }

		void setSimPosition(Position newPosition)		{ simPosition = newPosition; }
		void setPosition(Position newPosition)			{ position = newPosition; }
		void setEngagePosition(Position newPosition)	{ engagePosition = newPosition; }
		void setDestination(Position newPosition)		{ destination = newPosition; }
		void setWalkPosition(WalkPosition newPosition)	{ walkPosition = newPosition; }
		void setTilePosition(TilePosition newPosition)	{ tilePosition = newPosition; }
	};
}
