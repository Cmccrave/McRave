#pragma once
#include "McRave.h"

// Information from: https://docs.google.com/document/d/1p7Rw4v56blhf5bzhSnFVfgrKviyrapDFHh9J4FNUXM0

namespace McRave::Events
{
    inline void onUnitDiscover(BWAPI::Unit unit)
    {
        BWEB::Map::onUnitDiscover(unit);

        if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self()))
            Units::storeUnit(unit);

        if (Terrain::isIslandMap() && unit->getPlayer() == BWAPI::Broodwar->neutral() && !unit->getType().isResourceContainer() && unit->getType().isBuilding())
            Units::storeUnit(unit);
    }

    inline void onUnitCreate(BWAPI::Unit unit)
    {
        if (unit->getPlayer() == BWAPI::Broodwar->self())
            Units::storeUnit(unit);
        if (unit->getType().isResourceContainer())
            Resources::storeResource(unit);
        if (unit->getType().isResourceDepot())
            Stations::storeStation(unit);
    }

    inline void onUnitDestroy(BWAPI::Unit unit)
    {
        BWEB::Map::onUnitDestroy(unit);
        Resources::removeResource(unit);
        Units::removeUnit(unit);

        if (unit->getType().isResourceDepot())
            Stations::removeStation(unit);        
    }

    inline void onUnitMorph(BWAPI::Unit unit)
    {
        BWEB::Map::onUnitMorph(unit);

        // My unit
        if (unit->getPlayer() == BWAPI::Broodwar->self())  
            Units::morphUnit(unit);        

        // Enemy unit
        else if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self())) {

            // Remove any stations on a canceled hatchery (because the ingame AI does this LUL)
            if (unit->getType() == BWAPI::UnitTypes::Zerg_Drone)
                Stations::removeStation(unit);
            else
                Units::storeUnit(unit);
        }

        // Refinery that morphed as an enemy
        else if (unit->getType().isResourceContainer())
            Resources::storeResource(unit);
    }

    inline void onUnitComplete(BWAPI::Unit unit)
    {
        if (unit->getPlayer() == BWAPI::Broodwar->self())
            Units::storeUnit(unit);
        if (unit->getType().isResourceDepot())
            Stations::storeStation(unit);
        if (unit->getType().isResourceContainer())
            Resources::storeResource(unit);
    }

    inline void onUnitRenegade(BWAPI::Unit unit)
    {
        // TODO: Refinery is added in onUnitDiscover for enemy units (keep resource unit the same)
        // Destroy the unit otherwise
        if (!unit->getType().isRefinery())
            Units::removeUnit(unit);

        if (unit->getPlayer() == BWAPI::Broodwar->self())
            onUnitComplete(unit);
    }
}