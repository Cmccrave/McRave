#pragma once
#include <BWAPI.h>
#include "BWEB.h"

namespace McRave
{
    class UnitInfo;
    class ResourceInfo : public std::enable_shared_from_this<ResourceInfo>
    {
    private:
        int remainingResources;
        BWAPI::Unit thisUnit;
        BWAPI::UnitType type;
        BWAPI::Position position;
        BWAPI::TilePosition tilePosition;
        BWEB::Station * station;
        ResourceState rState;
        std::vector<std::weak_ptr<UnitInfo>> targetedBy;
    public:
        ResourceInfo(BWAPI::Unit newResource) {
            thisUnit = newResource;

            station = nullptr;
            remainingResources = 0;
            rState = ResourceState::None;
            type = BWAPI::UnitTypes::None;
            position = BWAPI::Positions::None;
            tilePosition = BWAPI::TilePositions::None;
        }

        void updateResource() {
            type                = thisUnit->getType();
            remainingResources    = thisUnit->getResources();
            position            = thisUnit->getPosition();
            tilePosition        = thisUnit->getTilePosition();
        }

        void addTargetedBy(std::weak_ptr<UnitInfo> unit) {
            targetedBy.push_back(unit);
        }

        void removeTargetedBy(std::weak_ptr<UnitInfo> unit) {
            for (auto itr = targetedBy.begin(); itr != targetedBy.end(); itr++) {
                if ((itr)->lock() == unit.lock()) {
                    targetedBy.erase(itr);
                    break;
                }
            }
        }

        // 
        bool hasStation() { return station != nullptr; }
        BWEB::Station * getStation() { return station; }
        void setStation(BWEB::Station* newStation) { station = newStation; }
        int getGathererCount() { return int(targetedBy.size()); }
        int getRemainingResources() { return remainingResources; }
        ResourceState getResourceState() { return rState; }
        BWAPI::Unit unit() { return thisUnit; }
        BWAPI::UnitType getType() { return type; }
        BWAPI::Position getPosition() { return position; }
        BWAPI::TilePosition getTilePosition() { return tilePosition; }
        std::vector<std::weak_ptr<UnitInfo>>& targetedByWhat() { return targetedBy; }

        // 
        void setRemainingResources(int newInt) { remainingResources = newInt; }
        void setResourceState(ResourceState newState) { rState = newState; }
        void setUnit(BWAPI::Unit newUnit) { thisUnit = newUnit; }
        void setType(BWAPI::UnitType newType) { type = newType; }
        void setPosition(BWAPI::Position newPosition) { position = newPosition; }
        void setTilePosition(BWAPI::TilePosition newTilePosition) { tilePosition = newTilePosition; }

        // 
        bool operator== (ResourceInfo& p) {
            return thisUnit == p.unit();
        }

        bool operator< (ResourceInfo& p) {
            return thisUnit < p.unit();
        }
    };
}
