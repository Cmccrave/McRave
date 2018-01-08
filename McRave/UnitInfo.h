#pragma once
#include <BWAPI.h>

using namespace BWAPI;
using namespace std;

namespace McRave
{
	class UnitInfo {
		double percentHealth, visibleGroundStrength, visibleAirStrength, maxGroundStrength, maxAirStrength, groundLocal, airLocal, groundRange, airRange, priority, groundDamage, airDamage, speed;
		int strategy, lastAttackFrame, lastCommandFrame, minStopFrame, lastVisibleFrame, shields, health;

		Unit /*target,*/ thisUnit, transport;
		UnitType unitType;
		UnitCommand command;
		Player who;

		UnitInfo* target;

		Position position, engagePosition, destination, simPosition;
		WalkPosition walkPosition;
		TilePosition tilePosition;
	public:
		UnitInfo();

		bool hasTarget() { return target != nullptr; }
		UnitInfo &getTarget() { return *target; }
		void setTarget(UnitInfo * unit) { target = unit; }

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

		// Returns the units current ground local calculation based on: (allyLocalGroundStrength - enemyLocalGroundStrength)
		double getGroundLocal() { return groundLocal; }

		// Returns the units current airlocal calculation based on: (allyLocalAirStrength - enemyLocalAirStrength)
		double getAirLocal() { return airLocal; }

		// Returns the units ground range including upgrades
		double getGroundRange() { return groundRange; }

		// Returns the units air range including upgrades
		double getAirRange() { return airRange; }

		// Returns the units priority for targeting purposes based on strength (not including value)
		double getPriority() { return priority; }

		// Returns the units ground damage (not including most upgrades)
		double getGroundDamage(){ return groundDamage; }

		// Returns the units air damage (not including most upgrades)
		double getAirDamage() { return airDamage; }

		// Returns the units movement speed in pixels per 24 frames (1 second)
		double getSpeed() { return speed; }

		// Returns the units strategy, see StrategyManager for details on what each correspond to
		int getStrategy() { return strategy; }

		// Returns the frame on which isStartingAttack was last true for purposes of avoiding moving before a shot has fired
		// This is important for units with a minStopFrame > 0 such as Dragoons, where moving before the shot is fully off will result in a dud
		int getLastAttackFrame() { return lastAttackFrame; }

		// Returns the frame on which you sent a command to the unit (different from BWAPI as it stores it during the frame, rather than after)
		int getLastCommandFrame() { return lastCommandFrame; }

		// Returns the minimum number of frames that the unit needs after a shot before another command can be issued
		int getMinStopFrame() { return minStopFrame; }

		// Returns the last frame since this unit was visible
		int getLastVisibleFrame() { return lastVisibleFrame; }

		int getShields() { return shields; }
		int getHealth() { return health; }

		Position getSimPosition() { return simPosition; }

		Unit unit() { return thisUnit; }
		//Unit getTarget() { return target; }
		Unit getTransport() { return transport; }
		UnitType getType(){ return unitType; }
		UnitCommand getLastCommand() { return command; }
		Player getPlayer() { return who; }

		Position getPosition(){ return position; }
		Position getEngagePosition() { return engagePosition; }
		Position getDestination() { return destination; }
		WalkPosition getWalkPosition() { return walkPosition; }
		TilePosition getTilePosition() { return tilePosition; }

		void setPercentHealth(double newValue) { percentHealth = newValue; }
		void setVisibleGroundStrength(double newValue) { visibleGroundStrength = newValue; }
		void setMaxGroundStrength(double newValue) { maxGroundStrength = newValue; }
		void setVisibleAirStrength(double newValue) { visibleAirStrength = newValue; }
		void setMaxAirStrength(double newValue) { maxAirStrength = newValue; }
		void setGroundLocal(double newValue) { groundLocal = newValue; }
		void setAirLocal(double newValue) { airLocal = newValue; }
		void setGroundRange(double newValue) { groundRange = newValue; }
		void setAirRange(double newValue) { airRange = newValue; }
		void setPriority(double newValue) { priority = newValue; }
		void setGroundDamage(double newValue) { groundDamage = newValue; }
		void setAirDamage(double newValue) { airDamage = newValue; }
		void setSpeed(double newValue) { speed = newValue; }
		void setHealth(int newValue) { health = newValue; }
		void setShields(int newValue) { shields = newValue; }
		void setStrategy(int newValue){ strategy = newValue; }
		void setLastAttackFrame(int newValue) { lastAttackFrame = newValue; }
		void setLastCommandFrame(int newValue) { lastCommandFrame = newValue; }
		void setMinStopFrame(int newValue) { minStopFrame = newValue; }
		void setLastVisibleFrame(int newValue) { lastVisibleFrame = newValue; }

		void setUnit(Unit newUnit) { thisUnit = newUnit; }
		//void setTarget(Unit newUnit){ target = newUnit; }
		void setTransport(Unit newUnit) { transport = newUnit; }
		void setType(UnitType newType) { unitType = newType; }
		void setLastCommand(UnitCommand newCommand) { command = newCommand; }
		void setPlayer(Player newPlayer) { who = newPlayer; }

		void setSimPosition(Position newPosition) { simPosition = newPosition; }
		void setPosition(Position newPosition){ position = newPosition; }
		void setEngagePosition(Position newPosition) { engagePosition = newPosition; }
		void setDestination(Position newPosition) { destination = newPosition; }
		void setWalkPosition(WalkPosition newPosition) { walkPosition = newPosition; }
		void setTilePosition(TilePosition newPosition) { tilePosition = newPosition; }
	};
}
