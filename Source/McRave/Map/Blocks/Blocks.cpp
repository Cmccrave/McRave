#include "Blocks.h"

#include "Micro/Combat/Combat.h"
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
        if (block->isDefensive() && !Combat::isDefendNatural()) {
            if (Players::TvP()) {
                if (Spy::getEnemyBuild() == P_2Gate || Spy::getEnemyBuild() == P_1GateCore)
                    return 1 - count;
            }

            if (Players::TvZ()) {
                if (Spy::getEnemyOpener() == Z_9Pool || Spy::getEnemyBuild() == Z_Overpool || Util::getTime() > Time(4, 00))
                    return 1 - count;
            }

            // Maybe shield battery logic here?
        }
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

            // Make mutas as we see them
            auto mutaBuild = Spy::getEnemyTransition().find("Muta") != string::npos ||
                             Players::getTotalCount(PlayerState::Enemy, Zerg_Spire, Zerg_Mutalisk, Zerg_Scourge, Zerg_Greater_Spire, Zerg_Guardian, Zerg_Devourer) > 0;
            if (Spy::enemyFastExpand() && !Spy::enemyPressure() && !Spy::enemyRush()) {
                if (mutaBuild && Util::getTime() > Time(6, 30))
                    return 2 - count;
            }
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