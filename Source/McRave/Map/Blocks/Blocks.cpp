#include "Blocks.h"

#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Blocks {

    int needGroundDefenses(BWEB::Block *block) // NOIMPL
    {
        // TODO: Move to BWEB
        auto count = 0;
        for (auto &def : block->getSmallTiles()) {
            if (BWEB::Map::isUsed(def) != UnitTypes::None)
                count++;
        }

        // Bunker or shield battery
        if (block->isDefensive()) {
        }

        // Turrets or cannons
        else {
        }

        // Reasons we want a bunker that is not in a wall
        // 4pool up to overpool
        // or ling all-in (pressure?/rush?)
        // 2bunker if lurker all-in

        // Maybe shield battery logic here?
        return 0;
    }

    int needAirDefenses(BWEB::Block *block) //
    {
        // TODO: Move to BWEB
        auto count = 0;
        for (auto &def : block->getSmallTiles()) {
            if (BWEB::Map::isUsed(def) != UnitTypes::None)
                count++;
        }

        auto valid = false;
        for (auto &large : block->getLargeTiles()) {
            if (BWEB::Map::isUsed(large) != UnitTypes::None)
                valid = true;
        }
        if (!valid)
            return 0;

        if (Players::TvZ()) {

            // 2hm
            if (Spy::getEnemyTransition() == Z_2HatchMuta && Util::getTime() > Time(5, 45))
                return 2 - count;

            // 3hm
            if (Spy::getEnemyTransition() == Z_3HatchMuta && Util::getTime() > Time(6, 15))
                return 2 - count;

            // Fallback
            if (Spy::enemyFastExpand() && !Spy::enemyPressure() && !Spy::enemyRush() && Util::getTime() > Time(6, 30))
                return 2 - count;
        }
        return 0;
    }

    int getGroundDefenseCount(BWEB::Block *) // NOIMPL
    {
        return 0;
    }

    int getAirDefenseCount(BWEB::Block *) // NOIMPL
    {
        return 0;
    }
} // namespace McRave::Blocks