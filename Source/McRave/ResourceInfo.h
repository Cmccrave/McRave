#pragma once
#include <BWAPI.h>
#include "BWEB.h"
#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
    class ResourceInfo
    {
    private:
        int gathererCount, remainingResources;
        Unit thisUnit;
        UnitType type;
        Position position;
        TilePosition tilePosition;
        const BWEB::Stations::Station * station;
        ResourceState rState;
    public:
        ResourceInfo() {
            station = nullptr;
            thisUnit = nullptr;
            gathererCount = 0;
            remainingResources = 0;
            rState = ResourceState::None;
            type = UnitTypes::None;
            position = Positions::None;
            tilePosition = TilePositions::None;
        }

        void updateResource() {
            type				= thisUnit->getType();
            remainingResources	= thisUnit->getResources();
            position			= thisUnit->getPosition();
            tilePosition		= thisUnit->getTilePosition();
        }

        bool hasStation() { return station != nullptr; }
        const BWEB::Stations::Station * getStation() { return station; }
        void setStation(const BWEB::Stations::Station* newStation) { station = newStation; }

        int getGathererCount() { return gathererCount; };
        int getRemainingResources() { return remainingResources; }
        ResourceState getResourceState() { return rState; }
        Unit unit() { return thisUnit; }
        UnitType getType() { return type; }
        Position getPosition() { return position; }
        TilePosition getTilePosition() { return tilePosition; }

        void setGathererCount(int newInt) { gathererCount = newInt; }
        void setRemainingResources(int newInt) { remainingResources = newInt; }
        void setResourceState(ResourceState newState) { rState = newState; }
        void setUnit(Unit newUnit) { thisUnit = newUnit; }
        void setType(UnitType newType) { type = newType; }
        void setPosition(Position newPosition) { position = newPosition; }
        void setTilePosition(TilePosition newTilePosition) { tilePosition = newTilePosition; }
    };
}
