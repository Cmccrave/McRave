#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Pylons
{
    namespace {
        map<TilePosition, int> smallMediumLocations, largeLocations;
        map<UnitSizeType, int> poweredPositions;

        void updatePowerGrid()
        {
            // Update power of every Pylon
            smallMediumLocations.clear();
            largeLocations.clear();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getType() != UnitTypes::Protoss_Pylon)
                    continue;

                int count = 1 + unit.unit()->isCompleted();
                for (int x = 0; x <= 15; x++) {
                    for (int y = 0; y <= 9; y++) {
                        const auto tile = TilePosition(x, y) + unit.getTilePosition() - TilePosition(8, 5);

                        if (!tile.isValid())
                            continue;

                        if (y == 0) {
                            if (x >= 4 && x <= 9)
                                largeLocations[tile] = max(largeLocations[tile], count);
                        }
                        else if (y == 1 || y == 8) {
                            if (x >= 2 && x <= 13)
                                smallMediumLocations[tile] = max(smallMediumLocations[tile], count);
                            if (x >= 1 && x <= 12)
                                largeLocations[tile] = max(largeLocations[tile], count);
                        }
                        else if (y == 2 || y == 7) {
                            if (x >= 1 && x <= 14)
                                smallMediumLocations[tile] = max(smallMediumLocations[tile], count);
                            if (x <= 13)
                                largeLocations[tile] = max(largeLocations[tile], count);
                        }
                        else if (y == 3 || y == 4 || y == 5 || y == 6) {
                            if (x >= 1)
                                smallMediumLocations[tile] = max(smallMediumLocations[tile], count);
                            if (x <= 14)
                                largeLocations[tile] = max(largeLocations[tile], count);
                        }
                        else if (y == 9) {
                            if (x >= 5 && x <= 10)
                                smallMediumLocations[tile] = max(smallMediumLocations[tile], count);
                            if (x >= 4 && x <= 9)
                                largeLocations[tile] = max(largeLocations[tile], count);
                        }
                    }
                }
            }
        }

        void updatePoweredPositions()
        {
            // Update power of every Block
            poweredPositions.clear();
            for (auto &block : BWEB::Blocks::getBlocks()) {
                for (auto &large : block.getLargeTiles()) {
                    if (largeLocations[large] > 0 && BWEB::Map::isUsed(large) == UnitTypes::None)
                        poweredPositions[UnitSizeTypes::Large]++;
                }
                for (auto &medium : block.getMediumTiles()) {
                    if (smallMediumLocations[medium] > 0 && BWEB::Map::isUsed(medium) == UnitTypes::None)
                        poweredPositions[UnitSizeTypes::Medium]++;
                }
            }
        }
    }

    void onFrame()
    {
        updatePowerGrid();
        updatePoweredPositions();
    }

    bool hasPowerNow(TilePosition here, UnitType building)
    {
        if (building.tileHeight() == 2 && smallMediumLocations[here] == 2)
            return true;
        else if (building.tileHeight() == 3 && largeLocations[here] == 2)
            return true;
        return false;
    }

    bool hasPowerSoon(TilePosition here, UnitType building)
    {
        if (building.tileHeight() == 2 && smallMediumLocations[here] >= 1)
            return true;
        else if (building.tileHeight() == 3 && largeLocations[here] >= 1)
            return true;
        return false;
    }

    int countPoweredPositions(UnitSizeType type) { return poweredPositions[type]; }
}