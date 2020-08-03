#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave
{
    namespace {
        map<Unit, UnitType> actualEggType; // BWAPI issue #850
    }

    void PlayerInfo::update()
    {
        set<shared_ptr<UnitInfo>> deadUnits;

        // Store any upgrades this player has
        for (auto &upgrade : BWAPI::UpgradeTypes::allUpgradeTypes()) {
            if (thisPlayer->getUpgradeLevel(upgrade) > 0)
                playerUpgrades.insert(upgrade);
        }

        // Store any tech this player has
        for (auto &tech : BWAPI::TechTypes::allTechTypes()) {
            if (thisPlayer->hasResearched(tech))
                playerTechs.insert(tech);
        }

        // Update player units
        supply = 0;
        pStrength.clear();
        for (auto &u : units) {
            auto &unit = *u;
            auto type = unit.getType() == UnitTypes::Zerg_Egg ? unit.unit()->getBuildType() : unit.getType();

            if (unit.getType() == UnitTypes::Zerg_Egg) {
                if (type == UnitTypes::None)
                    type = actualEggType[unit.unit()];
                else
                    actualEggType[unit.unit()] = unit.unit()->getBuildType();
            }

            // Supply
            supply += type.supplyRequired();
            supply += unit.unit()->getBuildType() == Zerg_Zergling || unit.unit()->getBuildType() == Zerg_Scourge;

            // Targets
            unit.getTargetedBy().clear();
            unit.setTarget(nullptr);

            // Strength
            if ((unit.getType().isWorker() && unit.getRole() != Role::Combat)
                || (unit.unit()->exists() && !unit.getType().isBuilding() && !unit.unit()->isCompleted()))
                continue;

            if (unit.getType().isBuilding()) {
                pStrength.airDefense += unit.getVisibleAirStrength();
                pStrength.groundDefense += unit.getVisibleGroundStrength();
            }
            else if (unit.getType().isFlyer()) {
                pStrength.airToAir += unit.getVisibleAirStrength();
                pStrength.airToGround += unit.getVisibleGroundStrength();
            }
            else {
                pStrength.groundToAir += unit.getVisibleAirStrength();
                pStrength.groundToGround += unit.getVisibleGroundStrength();
            }
        }

        // Supply has to be an even number and is rounded up
        if (supply % 2 == 1)
            supply++;

        // Set current allied status
        if (thisPlayer->getID() == BWAPI::Broodwar->self()->getID())
            pState = PlayerState::Self;
        else if (thisPlayer->isEnemy(BWAPI::Broodwar->self()))
            pState = PlayerState::Enemy;
        else if (thisPlayer->isAlly(BWAPI::Broodwar->self()))
            pState = PlayerState::Ally;
        else if (thisPlayer->isNeutral())
            pState = PlayerState::Neutral;
        else
            pState = PlayerState::None;

        auto race = thisPlayer->isNeutral() || !alive ? BWAPI::Races::None : thisPlayer->getRace(); // BWAPI returns Zerg for neutral race
        currentRace = race;
    }
}