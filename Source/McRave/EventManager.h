#pragma once
#include "McRave.h"

// Information from: https://docs.google.com/document/d/1p7Rw4v56blhf5bzhSnFVfgrKviyrapDFHh9J4FNUXM0

namespace McRave::Events
{
    inline void onUnitDiscover(BWAPI::Unit unit)
    {
        BWEB::Map::onUnitDiscover(unit);
        Units::storeUnit(unit);
    }

    inline void onUnitCreate(BWAPI::Unit unit)
    {
        Units::storeUnit(unit);
        if (unit->getType().isResourceContainer())
            Resources::storeResource(unit);
        if (unit->getType().isResourceDepot())
            Stations::storeStation(unit);
    }

    inline void onUnitDestroy(BWAPI::Unit unit)
    {
        BWEB::Map::onUnitDestroy(unit);
        Units::removeUnit(unit);

        if (unit->getType().isResourceDepot())
            Stations::removeStation(unit);
        if (unit->getType().isResourceContainer())
            Resources::removeResource(unit);
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
                Units::morphUnit(unit);
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
        // HACK: Changing players is kind of annoying, so we just remove and re-store
        if (!unit->getType().isRefinery()) {
            Units::removeUnit(unit);
            Units::storeUnit(unit);
        }
    }

    inline void customOnUnitLift(UnitInfo& unit)
    {
        BWEB::Map::removeUsed(unit.getTilePosition(), unit.getType().tileWidth(), unit.getType().tileHeight());

        if (unit.getType().isResourceDepot())
            Stations::removeStation(unit.unit());
    }

    inline void customOnUnitLand(UnitInfo& unit)
    {
        BWEB::Map::addUsed(unit.getTilePosition(), unit.getType().tileWidth(), unit.getType().tileHeight());

        if (unit.getType().isResourceDepot())
            Stations::storeStation(unit.unit());
    }

    inline void customOnUnitDisappear(UnitInfo& unit)
    {
        bool move = true;
        for (int x = unit.getTilePosition().x - 1; x < unit.getTilePosition().x + 1; x++) {
            for (int y = unit.getTilePosition().y - 1; y < unit.getTilePosition().y + 1; y++) {
                BWAPI::TilePosition t(x, y);
                if (t.isValid() && !BWAPI::Broodwar->isVisible(t))
                    move = false;
            }
        }
        if (move) {
            unit.setPosition(BWAPI::Positions::Invalid);
            unit.setTilePosition(BWAPI::TilePositions::Invalid);
            unit.setWalkPosition(BWAPI::WalkPositions::Invalid);
        }
    }
}