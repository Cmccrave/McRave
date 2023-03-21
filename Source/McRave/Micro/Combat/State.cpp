#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::State {

    vector<UnitType> staticRetreatTypes;

    // Certain unit types are vulnerable under certain group sizes / lack of upgrades
    void updateStaticStates()
    {
        staticRetreatTypes.clear();

        // Hydralisks
        if (!Spy::enemyFastExpand() && (BuildOrder::isUnitUnlocked(Zerg_Hydralisk) || vis(Zerg_Hydralisk) > 0)) {
            auto hydraSpeed = Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Muscular_Augments);
            auto hydraRange = Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Grooved_Spines);
            if (BuildOrder::getCurrentTransition() == "2HatchHydra" || BuildOrder::getCurrentTransition() == "3HatchHydra") {
                if (!hydraRange || !hydraSpeed)
                    staticRetreatTypes.push_back(Zerg_Hydralisk);
                //if (Spy::enemyInvis() && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0)
                //    staticRetreatTypes.push_back(Zerg_Hydralisk);
            }
            else {
                if (!hydraRange || !hydraSpeed/* || BuildOrder::getCompositionPercentage(Zerg_Lurker) >= 1.00*/)
                    staticRetreatTypes.push_back(Zerg_Hydralisk);
            }
        }

        // Mutalisks
        if (BuildOrder::isUnitUnlocked(Zerg_Mutalisk) || vis(Zerg_Mutalisk) > 0) {
            if (!Players::ZvZ() && com(Zerg_Mutalisk) < (Stations::getStations(PlayerState::Self).size() >= 2 ? 5 : 3) && total(Zerg_Mutalisk) < 9)
                staticRetreatTypes.push_back(Zerg_Mutalisk);
        }

        // Zerglings
        if (BuildOrder::isUnitUnlocked(Zerg_Zergling) || vis(Zerg_Zergling) > 0) {
            const auto scaryOpeners = Players::ZvP() && Spy::getEnemyBuild() != "FFE"&& Spy::getEnemyBuild() != "CannonRush";
            if (scaryOpeners && !staticRetreatTypes.empty() && int(BuildOrder::getTechList().size()) == 1)
                staticRetreatTypes.push_back(Zerg_Zergling);

            if ((Players::ZvT() && Util::getTime() < Time(12, 00) && Util::getTime() > Time(3, 30) && !Spy::enemyGreedy() && (Spy::getEnemyBuild() == "RaxFact" || Spy::enemyWalled() || Players::getCompleteCount(PlayerState::Enemy, Terran_Vulture) > 0))
                || (Players::ZvT() && com(Zerg_Mutalisk) == 0 && com(Zerg_Sunken_Colony) > 0)
                || (Players::ZvP() && find(staticRetreatTypes.begin(), staticRetreatTypes.end(), Zerg_Hydralisk) != staticRetreatTypes.end())
                || (Players::ZvZ() && Spy::getEnemyOpener() == "9Pool" && BuildOrder::getCurrentOpener() == "12Pool")
                || (Players::ZvZ() && Spy::getEnemyTransition() == "2HatchSpeedling" && Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) == 0)
                || (Players::ZvZ() && Spy::getEnemyTransition() == "3HatchSpeedling" && Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) == 0)
                || (Players::ZvZ() && Broodwar->getStartLocations().size() >= 3 && Util::getTime() < Time(3, 00) && !Terrain::getEnemyStartingPosition().isValid())
                || (Players::ZvZ() && Players::getCompleteCount(PlayerState::Enemy, Zerg_Drone) > 0 && !Terrain::getEnemyStartingPosition().isValid() && Util::getTime() < Time(2, 45)))
                staticRetreatTypes.push_back(Zerg_Zergling);
        }

        // Corsairs

        // Carriers
    }

    bool forceLocalAttack(UnitInfo& unit)
    {
        if (!unit.hasTarget())
            return false;
        auto target = *unit.getTarget().lock();

        auto countDefensesInRange = 0.0;
        if (unit.getType() == Zerg_Mutalisk && unit.hasTarget() && unit.canOneShot(target)) {
            for (auto &e : Units::getUnits(PlayerState::Enemy)) {
                auto enemy = *e;
                if (enemy.canAttackAir() && enemy != target) {
                    if (enemy.getPosition().getDistance(target.getPosition()) < enemy.getAirRange() + 32.0
                        || enemy.getPosition().getDistance(unit.getEngagePosition()) < enemy.getAirRange() + 32.0
                        || enemy.getPosition().getDistance(unit.getPosition()) < enemy.getAirRange() + 32.0)
                        countDefensesInRange += (enemy.getType().isBuilding() ? 1.0 : 0.25);
                }
            }

            if (unit.canOneShot(target)) {
                if ((countDefensesInRange < 2.0 && Util::getTime() < Time(8, 00))
                    || (countDefensesInRange < 3.0 && Util::getTime() < Time(10, 00)))
                    return true;
            }
            else {
                if (countDefensesInRange <= 0.0)
                    return true;
            }
        }

        // Forced local attacks:
        return ((!unit.isFlying() && target.isSiegeTank() && unit.getType() != Zerg_Lurker && unit.isWithinRange(target) && unit.getGroundRange() > 32.0)
            || (unit.getType() == Protoss_Reaver && !unit.unit()->isLoaded() && unit.isWithinRange(target))
            || (target.getType() == Terran_Vulture_Spider_Mine && !target.isBurrowed())
            || (unit.hasTransport() && !unit.unit()->isLoaded() && unit.getType() == Protoss_High_Templar && unit.canStartCast(TechTypes::Psionic_Storm, target.getPosition()) && unit.isWithinRange(target))
            || (unit.hasTransport() && !unit.unit()->isLoaded() && unit.getType() == Protoss_Reaver && unit.canStartAttack()) && unit.isWithinRange(target));
    }

    bool forceLocalRetreat(UnitInfo& unit)
    {
        // Forced local retreats:
        // ... Zealot without speed targeting a Vulture
        // ... Corsair targeting a Scourge with less than 6 completed Corsairs
        // ... Corsair/Scout targeting a Overlord under threat greater than shields
        // ... Medic with no energy
        // ... Zergling with low health targeting a worker
        if (unit.hasTarget()) {
            auto &target = *unit.getTarget().lock();
            if ((unit.getType() == Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && target.getType() == Terran_Vulture)
                || (unit.getType() == Protoss_Corsair && target.isSuicidal() && com(Protoss_Corsair) < 6) // TODO: Check density instead
                || (unit.getType() == Terran_Medic && unit.getEnergy() <= TechTypes::Healing.energyCost())
                || (unit.isLightAir() && unit.getType().maxShields() > 0 && target.getType() == Zerg_Overlord && Grids::getAirThreat(unit.getEngagePosition(), PlayerState::Enemy) * 5.0 > (double)unit.getShields())
                || (unit.getType() == Zerg_Zergling && !unit.isHealthy() && target.getType().isWorker()))
                return true;
        }
        return unit.unit()->isIrradiated();
    }

    bool forceGlobalAttack(UnitInfo& unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();
        const auto atHome = Terrain::inTerritory(PlayerState::Self, target.getPosition());

        const auto nearEnemyStation = [&]() {
            const auto closestEnemyStation = Stations::getClosestStationGround(unit.getPosition(), PlayerState::Enemy);
            return (closestEnemyStation && unit.getPosition().getDistance(closestEnemyStation->getBase()->Center()) < 400.0);
        };

        const auto nearEnemyDefenseStructure = [&]() {
            const auto closestDefense = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->getType().isBuilding() && ((u->canAttackGround() && u->isFlying()) || (u->canAttackAir() && !u->isFlying()));
            });
            return closestDefense && closestDefense->getPosition().getDistance(target.getPosition()) < 256.0;
        };

        const auto engagingWithWorkers = [&]() {
            const auto closestCombatWorker = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                return u->getType().isWorker() && u->getRole() == Role::Combat;
            });
            return closestCombatWorker && closestCombatWorker->getPosition().getDistance(target.getPosition()) < unit.getPosition().getDistance(target.getPosition()) + 64.0;
        };

        const auto freeAttacks = [&]() {
            return (!target.canAttackAir() && unit.isFlying()) || (!target.canAttackGround() && !unit.isFlying());
        };

        // Forced global attacks:
        // ... Unit at home and in a winning sim state
        // ... Unit at home is higher range than target
        // ... Target is a threat and we need to engage
        // ... Unit is suicidal and at home or targeting something close
        // ... Unit is suicidal and enemy isn't near a defense
        // ... Unit is hidden and enemy doesn't have detection at engagement
        // ... Unit is a burrowed Lurker and enemy has no detection
        // ... Non-flying unit is melee and in enemy territory
        // ... Non-flying unit has a dark swarm at engagement position - TODO: Check if we're melee?
        // ... Non-flying unit has a dark swarm at current position - TODO: Check if we're melee?
        // ... Non-flying unit is targeted by suicide
        // ... Ghost with nuke made is loaded in a transport
        // ... Unit is fighting with a worker
        return (atHome && unit.getSimState() == SimState::Win)
            || (atHome && freeAttacks())
            || (atHome && !unit.getType().isWorker() && !Spy::enemyRush() && (unit.getGroundRange() > target.getGroundRange() || target.getType().isWorker()) && !target.isHidden())
            || (target.isThreatening() && !target.isHidden() && (Util::getTime() < Time(10, 00) || unit.getSimState() == SimState::Win || Players::ZvZ() || unit.getType() == Zerg_Zergling))
            || (unit.isSuicidal() && (Terrain::inTerritory(PlayerState::Self, target.getPosition()) || target.isThreatening() || target.getPosition().getDistance(unit.getGoal()) < 160.0))
            || (unit.isSuicidal() && Players::getStrength(PlayerState::Enemy).groundToAir <= 0.0 && !nearEnemyDefenseStructure())
            || (unit.isHidden() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
            || (unit.getType() == Zerg_Lurker && unit.isBurrowed() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
            || (!unit.isFlying() && unit.getGroundRange() < 32.0 && Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()) && Util::getTime() > Time(8, 00) && !Players::ZvZ() && nearEnemyStation())
            || (!unit.isFlying() && Actions::overlapsActions(unit.unit(), unit.getEngagePosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, 96))
            || (!unit.isFlying() && Actions::overlapsActions(unit.unit(), unit.getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, 96))
            || (unit.isTargetedBySuicide() && !unit.isFlying())
            || (unit.getType() == Terran_Ghost && com(Terran_Nuclear_Missile) > 0 && unit.unit()->isLoaded())
            || engagingWithWorkers();
    }

    bool forceGlobalRetreat(UnitInfo& unit)
    {
        if (unit.hasTarget()) {
            auto &target = *unit.getTarget().lock();

            // Try to save Mutas that are low hp when the firepower isn't needed
            const auto mutaSavingRequired = unit.getType() == Zerg_Mutalisk &&
                (Players::ZvZ() ? (Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) == 0) : (Util::getTime() > Time(8, 00)))
                && !unit.isWithinRange(target) && !target.isWithinRange(unit) && unit.getHealth() <= 60
                && !Terrain::inTerritory(PlayerState::Enemy, unit.getPosition());

            // Try to save scouts as they have high shield counts
            const auto scoutSavingRequired = unit.getType() == Protoss_Scout && !unit.isWithinRange(target) && unit.getHealth() + unit.getShields() <= 80;

            if (mutaSavingRequired || scoutSavingRequired)
                unit.saveUnit = true;
            if (unit.saveUnit) {
                if (unit.getType() == Zerg_Mutalisk && unit.getHealth() >= 100)
                    unit.saveUnit = false;
                if (unit.getType() == Protoss_Scout && unit.getShields() >= 90)
                    unit.saveUnit = false;
            }
        }

        // Forced global retreat:
        // ... unit is near a hidden enemy
        // ... unit should be sent home to heal
        return unit.isNearHidden() || unit.saveUnit;
    }

    void updateLocalState(UnitInfo& unit)
    {
        if (!unit.hasSimTarget() || !unit.hasTarget() || unit.getLocalState() != LocalState::None)
            return;

        auto &simTarget = *unit.getSimTarget().lock();
        auto &target = *unit.getTarget().lock();
        const auto atHome = Terrain::inTerritory(PlayerState::Self, target.getPosition());
        const auto distSim = double(Util::boxDistance(unit.getType(), unit.getPosition(), simTarget.getType(), simTarget.getPosition()));
        const auto distTarget = double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));

        const auto insideRetreatRadius = distSim < unit.getRetreatRadius();
        const auto insideEngageRadius = distTarget < unit.getEngageRadius() && (unit.getGlobalState() == GlobalState::Attack || unit.getGlobalState() == GlobalState::ForcedAttack || atHome);
        const auto exploringGoal = unit.getGoal().isValid() && unit.getGoalType() == GoalType::Explore && unit.getUnitsInRangeOfThis().empty() && Util::getTime() > Time(4, 00);

        // Regardless of any decision, determine if Unit is in danger and needs to retreat
        if ((Actions::isInDanger(unit, unit.getPosition()) && !unit.isTargetedBySuicide())
            || (Actions::isInDanger(unit, unit.getEngagePosition()) && insideEngageRadius && !unit.isTargetedBySuicide())
            || (unit.isNearSuicide() && unit.isFlying())) {
            unit.setLocalState(LocalState::ForcedRetreat);
        }

        // Forced states
        else if (insideEngageRadius && forceLocalAttack(unit))
            unit.setLocalState(LocalState::ForcedAttack);
        else if (insideRetreatRadius && forceLocalRetreat(unit))
            unit.setLocalState(LocalState::ForcedRetreat);

        // If within local decision range, determine if Unit needs to attack or retreat
        else if (insideEngageRadius && (unit.getSimState() == SimState::Win || exploringGoal))
            unit.setLocalState(LocalState::Attack);
        else if (insideRetreatRadius && (!unit.attemptingRunby() || Terrain::inTerritory(PlayerState::Enemy, unit.getPosition())) && unit.getSimState() == SimState::Loss)
            unit.setLocalState(LocalState::Retreat);
    }

    void updateGlobalState(UnitInfo& unit)
    {
        if (unit.getGlobalState() != GlobalState::None)
            return;

        // Protoss
        if (Broodwar->self()->getRace() == Races::Protoss) {
            if ((!BuildOrder::takeNatural() && Spy::enemyFastExpand())
                || (Spy::enemyProxy() && !Spy::enemyRush())
                || BuildOrder::isRush()
                || unit.getType() == Protoss_Dark_Templar
                || (Players::getVisibleCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 && com(Protoss_Observer) == 0 && Broodwar->getFrameCount() < 15000))
                unit.setGlobalState(GlobalState::Attack);

            else if (unit.getType().isWorker()
                || (unit.getType() == Protoss_Corsair && !BuildOrder::firstReady() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0)
                || (unit.getType() == Protoss_Carrier && com(Protoss_Interceptor) < 16 && !Spy::enemyPressure()))
                unit.setGlobalState(GlobalState::Retreat);
            else
                unit.setGlobalState(GlobalState::Attack);
        }

        // Zerg
        else if (Broodwar->self()->getRace() == Races::Zerg) {
            if (forceGlobalAttack(unit))
                unit.setGlobalState(GlobalState::ForcedAttack);
            else if (forceGlobalRetreat(unit))
                unit.setGlobalState(GlobalState::ForcedRetreat);
            else if (isStaticRetreat(unit.getType()))
                unit.setGlobalState(GlobalState::Retreat);
            else
                unit.setGlobalState(GlobalState::Attack);
        }

        // Terran
        else if (Broodwar->self()->getRace() == Races::Terran) {
            if (!BuildOrder::firstReady())
                unit.setGlobalState(GlobalState::Retreat);
            else
                unit.setGlobalState(GlobalState::Attack);
        }
    }

    void onFrame()
    {
        updateStaticStates();
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;
            if (unit.getRole() == Role::Combat) {
                updateGlobalState(unit);
                updateLocalState(unit);
            }
        }
    }

    bool isStaticRetreat(UnitType type) {
        auto itr = find(staticRetreatTypes.begin(), staticRetreatTypes.end(), type);
        return itr != staticRetreatTypes.end();
    }
}