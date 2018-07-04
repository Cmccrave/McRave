#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UnitInfo {
		double percentHealth, groundRange, airRange, groundDamage, airDamage, speed;						// StarCraft stats
		double visibleGroundStrength, visibleAirStrength, maxGroundStrength, maxAirStrength, priority;		// McRave stats
		double engageDist;
		int localStrategy, globalStrategy, lastAttackFrame, lastCommandFrame, lastVisibleFrame, shields, health, minStopFrame;
		int killCount, frameCreated;

		bool burrowed;
		bool forceE, forceR;

		Unit thisUnit, transport;
		UnitType unitType;
		Player who;

		UnitInfo* target;

		Position position, engagePosition, destination, simPosition, lastPos;
		WalkPosition walkPosition, lastWalk;
		TilePosition tilePosition, lastTile;

		vector<TilePosition> targetPath;
	public:
		UnitInfo();

		void resetForces() { forceE = false, forceR = false; }
		void setRetreat() { forceR = true; }
		void setEngage() { forceE = true; }
		bool shouldRetreat() { return forceR; }
		bool shouldEngage() { return forceE; }

		// Target
		bool hasTarget() { return target != nullptr; }
		UnitInfo &getTarget() { return *target; }
		void setTarget(UnitInfo * unit) { target = unit; }
		double getDistance(UnitInfo unit) { return position.getDistance(unit.getPosition()); }

		// Engagement distance
		double getEngDist() { return engageDist; }
		void setEngDist(double newValue) { engageDist = newValue; }

		// Last positions
		Position getLastPosition() { return lastPos; }
		WalkPosition getLastWalk() { return lastWalk; }
		TilePosition getLastTile() { return lastTile; }
		bool sameTile() { return lastTile == tilePosition; }
		void setLastPositions();

		// Target path
		void setTargetPath(vector<TilePosition>& newPath) { targetPath = newPath; }
		vector<TilePosition>& getTargetPath() { return targetPath; }
		bool samePath() {
			return (targetPath.front() == target->getTilePosition() && targetPath.back() == tilePosition);
		}

		// Creation frame
		void setCreationFrame() { frameCreated = Broodwar->getFrameCount(); }
		int getFrameCreated() { return frameCreated; }

		// Returns the units health and shield percentage
		double getPercentHealth() { return percentHealth; }

		// Returns the units visible ground strength based on: (%hp * ground dps * range * speed)
		double getVisibleGroundStrength() { return visibleGroundStrength; }

		// Returns the units max ground strength based on: (ground dps * range * speed)
		double getMaxGroundStrength() { return maxGroundStrength; }

		// Returns the units visible air strength based on: (%hp * air dps * range * speed)
		double getVisibleAirStrength() { return visibleAirStrength; }

		// Returns the units max air strength based on: (air dps * range * speed)
		double getMaxAirStrength() { return maxAirStrength; }

		// Returns the units ground range including upgrades
		double getGroundRange() { return groundRange; }

		// Returns the units air range including upgrades
		double getAirRange() { return airRange; }

		// Returns the units priority for targeting purposes based on strength (not including value)
		double getPriority() { return priority; }

		// Returns the units ground damage (not including most upgrades)
		double getGroundDamage() { return groundDamage; }

		// Returns the units air damage (not including most upgrades)
		double getAirDamage() { return airDamage; }

		// Returns the units movement speed in pixels per frame
		double getSpeed() { return speed; }

		// Returns the units local strategy
		int getLocalStrategy() { return localStrategy; }

		// Returns the units global strategy
		int getGlobalStrategy() { return globalStrategy; }

		// Returns the frame on which isStartingAttack was last true for purposes of avoiding moving before a shot has fired
		// This is important for units with a minStopFrame > 0 such as Dragoons, where moving before the shot is fully off will result in a dud
		int getLastAttackFrame() { return lastAttackFrame; }

		// Returns the frame on which you sent a command to the unit (different from BWAPI as it stores it during the frame, rather than after)
		int getLastCommandFrame() { return lastCommandFrame; }

		// Returns the minimum number of frames that the unit needs after a shot before another command can be issued
		int getMinStopFrame() { return minStopFrame; }

		// Returns the last frame since this unit was visible
		int getLastVisibleFrame() { return lastVisibleFrame; }

		// Returns kill count
		int getKillCount() { return killCount; }
		void setKillCount(int newValue) { killCount = newValue; }

		int getShields()								{ return shields; }
		int getHealth()									{ return health; }

		bool isBurrowed()								{ return burrowed; }

		Position getSimPosition()						{ return simPosition; }

		Unit unit()										{ return thisUnit; }
		Unit getTransport()								{ return transport; }
		UnitType getType()								{ return unitType; }
		Player getPlayer()								{ return who; }

		Position getPosition()							{ return position; }
		Position getEngagePosition()					{ return engagePosition; }
		Position getDestination()						{ return destination; }
		WalkPosition getWalkPosition()					{ return walkPosition; }
		TilePosition getTilePosition()					{ return tilePosition; }

		void setPercentHealth(double newValue)			{ percentHealth = newValue; }
		void setVisibleGroundStrength(double newValue)	{ visibleGroundStrength = newValue; }
		void setMaxGroundStrength(double newValue)		{ maxGroundStrength = newValue; }
		void setVisibleAirStrength(double newValue)		{ visibleAirStrength = newValue; }
		void setMaxAirStrength(double newValue)			{ maxAirStrength = newValue; }
		void setGroundRange(double newValue)			{ groundRange = newValue; }
		void setAirRange(double newValue)				{ airRange = newValue; }
		void setPriority(double newValue)				{ priority = newValue; }
		void setGroundDamage(double newValue)			{ groundDamage = newValue; }
		void setAirDamage(double newValue)				{ airDamage = newValue; }
		void setSpeed(double newValue)					{ speed = newValue; }
		
		void setHealth(int newValue)					{ health = newValue; }
		void setShields(int newValue)					{ shields = newValue; }
		void setLocalStrategy(int newValue)				{ localStrategy = newValue; }
		void setGlobalStrategy(int newValue)			{ globalStrategy = newValue; }
		void setLastAttackFrame(int newValue)			{ lastAttackFrame = newValue; }
		void setLastCommandFrame(int newValue)			{ lastCommandFrame = newValue; }
		void setMinStopFrame(int newValue)				{ minStopFrame = newValue; }
		void setLastVisibleFrame(int newValue)			{ lastVisibleFrame = newValue; }

		void setBurrowed(bool b)						{ burrowed = b; }

		void setUnit(Unit newUnit)						{ thisUnit = newUnit; }
		void setTransport(Unit newUnit)					{ transport = newUnit; }
		void setType(UnitType newType)					{ unitType = newType; }
		void setPlayer(Player newPlayer)				{ who = newPlayer; }

		void setSimPosition(Position newPosition)		{ simPosition = newPosition; }
		void setPosition(Position newPosition)			{ position = newPosition; }
		void setEngagePosition(Position newPosition)	{ engagePosition = newPosition; }
		void setDestination(Position newPosition)		{ destination = newPosition; }
		void setWalkPosition(WalkPosition newPosition)	{ walkPosition = newPosition; }
		void setTilePosition(TilePosition newPosition)	{ tilePosition = newPosition; }
	};
}
