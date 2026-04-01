#include "PlayerInfo.h"

#include "Info/Unit/UnitInfo.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave {
    namespace {
        map<Unit, UnitType> actualEggType; /// BWAPI issue #850
        int extractorsLastFrame = 0;
    } // namespace

    void PlayerInfo::update()
    {
        // Store any upgrades this player has
        for (auto &upgrade : UpgradeTypes::allUpgradeTypes()) {
            auto level = thisPlayer->getUpgradeLevel(upgrade);
            if (level > 0)
                playerUpgrades.insert_or_assign(upgrade, level);
        }

        // Store any tech this player has, enemies need to check if they have units under certain conditions
        for (auto &tech : TechTypes::allTechTypes()) {
            if (thisPlayer->hasResearched(tech))
                playerTechs.insert(tech);
        }
        for (auto &u : units) {
            if (u->unit()->exists() && u->unit()->isStimmed())
                playerTechs.insert(TechTypes::Stim_Packs);
            if (u->getType() == Terran_Siege_Tank_Siege_Mode)
                playerTechs.insert(TechTypes::Tank_Siege_Mode);
        }

        extractorsLastFrame = visibleTypeCounts[Zerg_Extractor];

        // Update player units
        raceSupply.clear();
        pStrength.clear();
        visibleTypeCounts.clear();
        completeTypeCounts.clear();

        // Offset Zerg supply if an extractor is being made, since the drone is destroyed/created 1 frame off of an extractor creation/destruction
        if (extractorsLastFrame != Broodwar->self()->visibleUnitCount(Zerg_Extractor))
            raceSupply[Races::Zerg] += 2 * (Broodwar->self()->visibleUnitCount(Zerg_Extractor) - extractorsLastFrame);

        auto totalGrndDamage = 0.0;
        auto totalAirDamage  = 0.0;
        auto grndCount       = 0;
        auto airCount        = 0;
        for (auto &u : units) {
            auto &unit = *u;
            auto isEgg = (unit.getType() == Zerg_Egg || unit.getType() == Zerg_Lurker_Egg);
            auto type  = isEgg ? unit.unit()->getBuildType() : unit.getType();

            /// BWAPI issue #850
            if (type == None && actualEggType.find(unit.unit()) != actualEggType.end())
                type = actualEggType[unit.unit()];

            if (isEgg && unit.unit()->getBuildType() != None) {
                actualEggType[unit.unit()] = unit.unit()->getBuildType();
                type                       = unit.unit()->getBuildType();
            }

            // Supply
            raceSupply[type.getRace()] += type.supplyRequired();
            raceSupply[type.getRace()] += !unit.isCompleted() && (type == Zerg_Zergling || type == Zerg_Scourge);

            // All supply
            raceSupply[Races::None] += type.supplyRequired();
            raceSupply[Races::None] += !unit.isCompleted() && (type == Zerg_Zergling || type == Zerg_Scourge);

            // Counts
            int eggOffset = int(isSelf() && !unit.isCompleted() && (type == Zerg_Zergling || type == Zerg_Scourge));
            if (type != None && type.isValid()) {
                visibleTypeCounts[type] += 1 + eggOffset;
                if (unit.isCompleted())
                    completeTypeCounts[type] += 1;
            }

            // Strength
            if ((type.isWorker() && unit.getRole() != Role::Combat) || (unit.unit()->exists() && !type.isBuilding() && !unit.unit()->isCompleted()))
                continue;

            if (type.isBuilding()) {
                pStrength.airDefense += unit.getVisibleAirStrength();
                pStrength.groundDefense += unit.getVisibleGroundStrength();
            }
            else if (type.isFlyer() && !unit.isSpellcaster() && !type.isWorker()) {
                pStrength.airToAir += unit.getVisibleAirStrength();
                pStrength.airToGround += unit.getVisibleGroundStrength();
            }
            else if (!unit.isSpellcaster() && !type.isWorker()) {
                pStrength.groundToAir += unit.getVisibleAirStrength();
                pStrength.groundToGround += unit.getVisibleGroundStrength();
            }

            if (!type.isBuilding() && !type.isWorker()) {

                if (unit.getAirDamage() > 0) {
                    totalAirDamage += unit.getAirDamage();
                    airCount++;
                    pStrength.maxAirRange = max(pStrength.maxAirRange, unit.getAirRange());
                }
                if (unit.getGroundDamage() > 0) {
                    totalGrndDamage += unit.getGroundDamage();
                    grndCount++;
                    pStrength.maxAirRange = max(pStrength.maxGroundRange, unit.getGroundRange());
                }
            }
        }
        pStrength.avgGroundDamage = grndCount > 0 ? totalGrndDamage / double(grndCount) : 5;
        pStrength.avgAirDamage    = airCount > 0 ? totalAirDamage / double(airCount) : 5;

        // Round up to nearest 2 (for actual Broodwar supply)
        for (auto &supply : raceSupply)
            supply.second += (supply.second % 2);

        // Set current allied status
        if (thisPlayer->getID() == BWAPI::Broodwar->self()->getID())
            pState = PlayerState::Self;
        else if (thisPlayer->isEnemy(BWAPI::Broodwar->self()))
            pState = PlayerState::Enemy;
        else if (thisPlayer->isAlly(BWAPI::Broodwar->self()))
            pState = PlayerState::Ally;
        else
            pState = PlayerState::Neutral;

        auto race   = thisPlayer->isNeutral() || !alive ? BWAPI::Races::None : thisPlayer->getRace(); // BWAPI returns Zerg for neutral race
        currentRace = race;
    }
} // namespace McRave