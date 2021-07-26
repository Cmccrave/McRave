#include "McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave
{
    namespace {
        map<Unit, UnitType> actualEggType; /// BWAPI issue #850
    }

    void PlayerInfo::update()
    {
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
        raceSupply.clear();
        pStrength.clear();
        for (auto &u : units) {
            auto &unit = *u;
            auto type = unit.getType() == UnitTypes::Zerg_Egg ? unit.unit()->getBuildType() : unit.getType();

            /// BWAPI issue #850
            if (unit.getType() == UnitTypes::Zerg_Egg) {
                if (type == UnitTypes::None)
                    type = actualEggType[unit.unit()];
                else
                    actualEggType[unit.unit()] = unit.unit()->getBuildType();
            }

            // Supply
            raceSupply[unit.getType().getRace()] += type.supplyRequired();
            raceSupply[unit.getType().getRace()] += unit.unit()->getBuildType() == Zerg_Zergling || unit.unit()->getBuildType() == Zerg_Scourge;

            // All supply
            raceSupply[Races::None] += type.supplyRequired();
            raceSupply[Races::None] += unit.unit()->getBuildType() == Zerg_Zergling || unit.unit()->getBuildType() == Zerg_Scourge;

            // Targets
            unit.getTargetedBy().clear();
            unit.setTarget(nullptr);
            unit.borrowedPath = false;

            // Strength
            if ((unit.getType().isWorker() && unit.getRole() != Role::Combat)
                || (unit.unit()->exists() && !unit.getType().isBuilding() && !unit.unit()->isCompleted())
                || (unit.unit()->isMorphing()))
                continue;

            if (unit.getType().isBuilding()) {
                pStrength.airDefense += unit.getVisibleAirStrength();
                pStrength.groundDefense += unit.getVisibleGroundStrength();
            }
            else if (unit.getType().isFlyer() && !unit.isSpellcaster() && !unit.getType().isWorker()) {
                pStrength.airToAir += unit.getVisibleAirStrength();
                pStrength.airToGround += unit.getVisibleGroundStrength();
            }
            else if (!unit.isSpellcaster() && !unit.getType().isWorker()) {
                pStrength.groundToAir += unit.getVisibleAirStrength();
                pStrength.groundToGround += unit.getVisibleGroundStrength();
            }
        }

        // Supply has to be an even number and is rounded up - TODO: Check
        for (auto &[race, supply] : raceSupply) {
            if (supply % 2 == 1)
                supply++;
        }

        // Set current allied status
        if (thisPlayer->getID() == BWAPI::Broodwar->self()->getID())
            pState = PlayerState::Self;
        else if (thisPlayer->isEnemy(BWAPI::Broodwar->self()))
            pState = PlayerState::Enemy;
        else if (thisPlayer->isAlly(BWAPI::Broodwar->self()))
            pState = PlayerState::Ally;
        else
            pState = PlayerState::Neutral;

        auto race = thisPlayer->isNeutral() || !alive ? BWAPI::Races::None : thisPlayer->getRace(); // BWAPI returns Zerg for neutral race
        currentRace = race;
    }
}