#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat {

    namespace {

        bool holdChoke = false;

        bool lightUnitNeedsRegroup(UnitInfo& unit)
        {
            if (!unit.isLightAir())
                return false;
            return unit.hasCommander() && unit.getPosition().getDistance(unit.getCommander().lock()->getPosition()) > 64.0;
        }

        void checkHoldChoke()
        {
            auto defensiveAdvantage = (Players::ZvZ() && BuildOrder::getCurrentOpener() == Spy::getEnemyOpener())
                || (Players::ZvZ() && BuildOrder::getCurrentOpener() == "12Pool" && Spy::getEnemyOpener() == "9Pool")
                || (Players::ZvZ() && vis(Zerg_Zergling) >= Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) + 2);

            // UMS Setting
            if (Broodwar->getGameType() == BWAPI::GameTypes::Use_Map_Settings) {
                holdChoke = true;
                return;
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss && Players::getSupply(PlayerState::Self, Races::None) > 40) {
                holdChoke = BuildOrder::takeNatural()
                    || vis(Protoss_Dragoon) > 0
                    || com(Protoss_Shield_Battery) > 0
                    || BuildOrder::isWallNat()
                    || (BuildOrder::isHideTech() && !Spy::enemyRush())
                    || Players::getSupply(PlayerState::Self, Races::None) > 60
                    || Players::vT();
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran && Players::getSupply(PlayerState::Self, Races::None) > 40)
                holdChoke = true;

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                holdChoke = (!Players::ZvZ() && (Spy::getEnemyBuild() != "2Gate" || !Spy::enemyProxy()))
                    || (defensiveAdvantage && !Spy::enemyPressure() && vis(Zerg_Sunken_Colony) == 0 && com(Zerg_Mutalisk) < 3 && Util::getTime() > Time(3, 30))
                    || (BuildOrder::takeNatural() && total(Zerg_Zergling) >= 10)
                    || Players::getSupply(PlayerState::Self, Races::None) > 60;
            }
        }       
    }

    void onFrame() {
        Visuals::startPerfTest();
        checkHoldChoke();
        Simulation::onFrame();
        State::onFrame();
        Destination::onFrame();
        Clusters::onFrame();
        Formations::onFrame();
        Navigation::onFrame();
        Decision::onFrame();
        Visuals::endPerfTest("Combat");
    }

    void onStart() {

    }

    bool defendChoke() { return holdChoke; }
}