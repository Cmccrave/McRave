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

        const auto unlockedOrVis = [&](auto &t) {
            return vis(t) > 0 || BuildOrder::isUnitUnlocked(t);
        };

        if (Units::getImmThreat() > 0.0)
            return;

        // Hydralisks
        if (!BuildOrder::isPressure() && (unlockedOrVis(Zerg_Hydralisk) || BuildOrder::getCurrentTransition() == "4HatchHydra" || BuildOrder::getCurrentTransition() == "6HatchHydra")) {
            const auto hydraSpeed = Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Muscular_Augments);
            const auto hydraRange = Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Grooved_Spines);
            const auto defendTiming = Spy::getEnemyBuild() == "FFE" && !BuildOrder::isPressure() && Util::getTime() < Time(12, 00);
            if (!hydraRange || !hydraSpeed || BuildOrder::isAllIn() || defendTiming)
                staticRetreatTypes.push_back(Zerg_Hydralisk);
        }

        // Mutalisks
        if (unlockedOrVis(Zerg_Mutalisk) || BuildOrder::getCurrentTransition() == "2HatchMuta" || BuildOrder::getCurrentTransition() == "3HatchMuta") {
            if (Players::ZvZ()) {
                const auto lessMutas = com(Zerg_Mutalisk) < Players::getCompleteCount(PlayerState::Enemy, Zerg_Mutalisk);
                if (lessMutas)
                    staticRetreatTypes.push_back(Zerg_Mutalisk);
            }
            if (Players::ZvP()) {
                const auto lowCount = com(Zerg_Mutalisk) < 5 && total(Zerg_Mutalisk) < 9;
                const auto corsairPump = Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Protoss_Air_Weapons, 1) && vis(Protoss_Corsair) >= 5 && Util::getTime() < Time(12, 00);
                if (lowCount || corsairPump)
                    staticRetreatTypes.push_back(Zerg_Mutalisk);
            }
            if (Players::ZvT()) {
                if (com(Zerg_Mutalisk) < 6 && total(Zerg_Mutalisk) < 9)
                    staticRetreatTypes.push_back(Zerg_Mutalisk);
            }
        }

        // Zerglings
        if (!BuildOrder::isPressure() && unlockedOrVis(Zerg_Zergling)) {
            const auto speedLing = Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Metabolic_Boost);
            const auto crackling = Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Adrenal_Glands);
            const auto volume = speedLing && Players::getTotalCount(PlayerState::Self, Zerg_Zergling) >= 64;

            if (!crackling && !volume && !BuildOrder::isRush() && !BuildOrder::isAllIn()) {
                if (Players::ZvP()) {
                    const auto killedWorkers = Players::getDeadCount(PlayerState::Enemy, Protoss_Probe) >= 8;
                    const auto scaryOpeners = Spy::getEnemyBuild() != "FFE" && Util::getTime() < Time(8, 00);
                    const auto hideCheese = BuildOrder::isHideTech() && BuildOrder::isOpener() && !BuildOrder::isPressure();
                    const auto defendProxy = Spy::enemyProxy() && !speedLing && Util::getTime() < Time(5, 00) && Players::getDeadCount(PlayerState::Enemy, Protoss_Pylon) == 0;
                    const auto defendTiming = Spy::getEnemyBuild() == "FFE" && Util::getTime() > Time(6, 00) && Util::getTime() < Time(8, 00);
                    if (!killedWorkers && (scaryOpeners || hideCheese || defendProxy || defendTiming))
                        staticRetreatTypes.push_back(Zerg_Zergling);
                }
                if (Players::ZvT()) {
                    const auto defendSunkens = com(Zerg_Mutalisk) == 0 && com(Zerg_Sunken_Colony) > 0 && !speedLing;
                    const auto vulturesExist = Players::getCompleteCount(PlayerState::Enemy, Terran_Vulture) > 0;
                    const auto vultureThreat = Util::getTime() < Time(12, 00) && Util::getTime() > Time(3, 30) && !Spy::enemyGreedy() && !Spy::enemyProxy()
                        && (Spy::getEnemyBuild() == "RaxFact" || Spy::enemyWalled());
                    if (defendSunkens || vulturesExist || vultureThreat)
                        staticRetreatTypes.push_back(Zerg_Zergling);
                }
                if (Players::ZvZ()) {
                    const auto enemyLingVomit = (Spy::getEnemyTransition() == "2HatchSpeedling" || Spy::getEnemyTransition() == "3HatchSpeedling") && Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) < 9;
                    const auto avoidDiceRoll = (Broodwar->getStartLocations().size() >= 3 && Util::getTime() < Time(3, 15) && !Terrain::getEnemyStartingPosition().isValid())
                        || (BuildOrder::getCurrentOpener() == "12Pool")
                        || (BuildOrder::getCurrentOpener() == "12Hatch");
                    const auto enemyDroneScouted = Players::getCompleteCount(PlayerState::Enemy, Zerg_Drone) > 0 && !Terrain::getEnemyStartingPosition().isValid() && Util::getTime() < Time(3, 15);
                    const auto confidentLingLead = Players::getVisibleCount(PlayerState::Self, Zerg_Zergling) * 2 > Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) * 3;

                    // 1hm early
                    if (!confidentLingLead) {
                        if (BuildOrder::getCurrentTransition() == "1HatchMuta" && Util::getTime() < Time(7, 00)) {
                            if (Spy::Zerg::enemyFasterPool() || Spy::Zerg::enemyEqualPool() || enemyLingVomit || enemyDroneScouted || Spy::enemyTurtle())
                                staticRetreatTypes.push_back(Zerg_Zergling);
                        }

                        // 1hm mid
                        if (BuildOrder::getCurrentTransition() == "1HatchMuta" && Spy::getEnemyBuild() == "HatchPool" && Util::getTime() > Time(3, 15) && Util::getTime() < Time(10, 00))
                            staticRetreatTypes.push_back(Zerg_Zergling);

                        // 2hm early
                        if (BuildOrder::getCurrentTransition() == "2HatchMuta" && Util::getTime() < Time(5, 30) && !Spy::enemyFastExpand() && !Spy::Zerg::enemySlowerSpeed()) {
                            if (Spy::Zerg::enemyFasterPool() || Spy::Zerg::enemyEqualPool() || avoidDiceRoll || enemyDroneScouted)
                                staticRetreatTypes.push_back(Zerg_Zergling);
                        }

                        // 2hm mid
                        if (BuildOrder::getCurrentTransition() == "2HatchMuta" && Util::getTime() < Time(10, 00) && !Spy::enemyFastExpand() && !Spy::Zerg::enemySlowerSpeed()) {
                            if (enemyLingVomit || Spy::Zerg::enemyFasterSpeed())
                                staticRetreatTypes.push_back(Zerg_Zergling);
                        }
                    }
                }
            }
        }

        // Corsairs
        if (unlockedOrVis(Protoss_Corsair)) {
            if (Players::PvZ()) {
                if (Players::getCompleteCount(PlayerState::Enemy, Zerg_Scourge) > 0 && !Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Protoss_Air_Weapons) && com(Protoss_Corsair) < 6)
                    staticRetreatTypes.push_back(Protoss_Corsair);
            }
        }

        // Carriers
        static bool carrierCountReady = carrierCountReady || com(Protoss_Carrier) >= 4;
        if (unlockedOrVis(Protoss_Carrier)) {
            if (Players::PvT()) {
                if (!carrierCountReady)
                    staticRetreatTypes.push_back(Protoss_Carrier);
            }
        }

        const auto lockGateways = [&]() {
            staticRetreatTypes.push_back(Protoss_Zealot);
            staticRetreatTypes.push_back(Protoss_Dragoon);
        };

        // Zealots / Dragoons
        if (unlockedOrVis(Protoss_Zealot) || unlockedOrVis(Protoss_Dragoon)) {
            if (Players::PvZ()) {
                auto gateUnits = total(Protoss_Zealot) + total(Protoss_Dragoon);
                if ((gateUnits < 24 && int(Stations::getStations(PlayerState::Enemy).size()) < 2) || gateUnits < 3)
                    lockGateways();
            }
        }

        // Marines
        if (unlockedOrVis(Terran_Marine)) {
            if (Players::TvZ()) {
                auto stim = Players::getPlayerInfo(Broodwar->self())->hasTech(TechTypes::Stim_Packs);
                if (!stim)
                    staticRetreatTypes.push_back(Terran_Marine);
            }
            if (Players::TvP() || Players::TvT()) {
                if (!BuildOrder::isRush())
                    staticRetreatTypes.push_back(Terran_Marine);
            }
        }

        // Tanks / Goliaths
        if (unlockedOrVis(Terran_Siege_Tank_Tank_Mode) || unlockedOrVis(Terran_Siege_Tank_Siege_Mode)) {
            if (Players::TvP()) {
                if (Util::getTime() < Time(12, 00)) {
                    staticRetreatTypes.push_back(Terran_Siege_Tank_Tank_Mode);
                    staticRetreatTypes.push_back(Terran_Siege_Tank_Siege_Mode);
                    staticRetreatTypes.push_back(Terran_Goliath);
                }
            }
        }
    }

    bool forceLocalAttack(UnitInfo& unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        auto countDefensesInRange = 0.0;
        if (unit.getType() == Zerg_Mutalisk) {

            // If we're not in range, calculate the risk
            if (!unit.isWithinRange(target)) {
                for (auto &e : Units::getUnits(PlayerState::Enemy)) {
                    auto &enemy = *e;
                    if (enemy.canAttackAir() && enemy != target) {
                        if (enemy.getPosition().getDistance(target.getPosition()) < enemy.getAirRange() + 96.0) {
                            if (enemy.getType().isBuilding())
                                countDefensesInRange += 1.0;
                            else if (enemy.hasAttackedRecently())
                                countDefensesInRange += 0.2;
                        }
                    }
                }
            }

            // One shotting units for free
            if (unit.canOneShot(target) && !unit.isTargetedBySplash() && !unit.isNearSplash()) {
                if ((countDefensesInRange > 0.0 && Players::ZvZ() && !target.getType().isWorker())
                    || (countDefensesInRange <= 2.0 && Util::getTime() < Time(8, 00))
                    || (countDefensesInRange <= 3.0 && Util::getTime() > Time(8, 00) && Util::getTime() < Time(10, 00))
                    || Util::getTime() > Time(10, 00))
                    return true;
            }

            // Two shotting buildings is good too
            if (unit.canTwoShot(target) && target.getType().isBuilding())
                return true;
        }

        auto engageWhenInRange = (target.isSiegeTank() || target.isLightAir() || target.isTransport() || target.getType() == Protoss_Reaver || target.getType() == Protoss_High_Templar);

        // Forced local attacks:
        return ((!unit.isFlying() && engageWhenInRange && unit.getType() != Zerg_Lurker && unit.isWithinRange(target) && unit.getGroundRange() > 32.0)
            || (unit.getType() == Protoss_Reaver && !unit.unit()->isLoaded() && unit.isWithinRange(target))
            || (target.getType() == Protoss_Dragoon && Util::getTime() > Time(5, 00) && unit.getType() == Zerg_Zergling && unit.isWithinRange(target))
            //|| (Util::getTime() < Time(8, 00) && unit.getType() == Zerg_Mutalisk && target.getType() == Protoss_Photon_Cannon && !target.isCompleted())
            || (target.getType() == Terran_Vulture_Spider_Mine && !target.isBurrowed())
            || (unit.isLightAir() && target.isLightAir() && target.hasAttackedRecently())
            || (unit.attemptingRunby() && target.getType().isWorker() && (unit.getHealth() > 15 || Util::getTime() > Time(6, 00)))
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
            const auto slowZealotVsVulture = unit.getType() == Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && target.getType() == Terran_Vulture;
            const auto sparseCorsairVsScourge = unit.getType() == Protoss_Corsair && target.isSuicidal() && com(Protoss_Corsair) < 6; // TODO: Check density instead
            const auto lowShieldFlyer = false;// (unit.isLightAir() && unit.getType().maxShields() > 0 && target.getType() == Zerg_Overlord && Grids::getAirThreat(unit.getEngagePosition(), PlayerState::Enemy) * 5.0 > (double)unit.getShields());
            const auto oomMedic = unit.getType() == Terran_Medic && unit.getEnergy() <= TechTypes::Healing.energyCost();
            const auto hurtLingVsWorker = (unit.getType() == Zerg_Zergling && unit.getHealth() <= 15 && target.getType().isWorker() && Util::getTime() < Time(6, 00));
            const auto regroupingAir = unit.isLightAir() && !unit.getGoal().isValid() && !target.isThreatening() && unit.attemptingRegroup();

            if (slowZealotVsVulture || sparseCorsairVsScourge || lowShieldFlyer || oomMedic || hurtLingVsWorker || regroupingAir)
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

        const auto nearGrdToAir = [&]() {
            const auto closestDefense = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                return u->canAttackAir() && !u->isFlying();
            });
            return closestDefense && closestDefense->getPosition().getDistance(target.getPosition()) < 256.0;
        };

        const auto engagingWithWorkers = [&]() {
            const auto closestCombatWorker = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                return unit != *u && u->getType().isWorker() && u->getRole() == Role::Combat;
            });
            return closestCombatWorker && closestCombatWorker->getPosition().getDistance(target.getPosition()) < 96.0;
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
        return (atHome && unit.getSimState() == SimState::Win && !Players::ZvZ())
            || (atHome && freeAttacks())
            || (atHome && !unit.getType().isWorker() && !Spy::enemyRush() && (unit.getGroundRange() > target.getGroundRange() || target.getType().isWorker()) && !target.isHidden())
            || (target.isThreatening() && !target.isHidden() && Util::getTime() < Time(6, 00))
            || (unit.isSuicidal() && (Terrain::inTerritory(PlayerState::Self, target.getPosition()) || target.isThreatening()))
            || (unit.isSuicidal() && !nearGrdToAir())
            || (unit.isHidden() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
            || (unit.getType() == Zerg_Lurker && unit.isBurrowed() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))
            || (!unit.isFlying() && unit.getGroundRange() < 32.0 && unit.getGoalType() == GoalType::Explore && Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()) && Util::getTime() > Time(8, 00) && !Players::ZvZ() && nearEnemyStation())
            || (!unit.isFlying() && Actions::overlapsActions(unit.unit(), unit.getEngagePosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, 96))
            || (!unit.isFlying() && Actions::overlapsActions(unit.unit(), unit.getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, 96))
            || (unit.isTargetedBySuicide() && !unit.isFlying())
            || (unit.getType() == Terran_Ghost && com(Terran_Nuclear_Missile) > 0 && unit.unit()->isLoaded())
            //|| (atHome && engagingWithWorkers())
            ;
    }

    bool forceGlobalRetreat(UnitInfo& unit)
    {
        if (unit.hasTarget()) {
            auto &target = *unit.getTarget().lock();

            // Try to save Mutas that are low hp when the firepower isn't needed
            const auto mutaSavingRequired = unit.getType() == Zerg_Mutalisk &&
                (Players::ZvZ() ? (Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) == 0) : (Util::getTime() > Time(8, 00)))
                && !unit.isWithinRange(target) && !target.isWithinRange(unit) && unit.getHealth() <= 30
                && !Terrain::inTerritory(PlayerState::Enemy, unit.getPosition());

            // Try to save scouts as they have high shield counts
            const auto scoutSavingRequired = unit.getType() == Protoss_Scout && !unit.isWithinRange(target) && unit.getHealth() + unit.getShields() <= 80;

            // Try to save zerglings in ZvZ
            const auto zerglingSaving = Players::ZvZ() && unit.getType() == Zerg_Zergling && !unit.isWithinRange(target) && unit.getHealth() <= 10;

            // Save the units
            if (mutaSavingRequired || scoutSavingRequired /*|| zerglingSaving*/)
                unit.saveUnit = true;
            if (unit.saveUnit) {
                if (unit.getType() == Zerg_Mutalisk && unit.getHealth() >= 100)
                    unit.saveUnit = false;
                if (unit.getType() == Protoss_Scout && unit.getShields() >= 90)
                    unit.saveUnit = false;
                if (unit.getType() == Zerg_Zergling && unit.getHealth() >= 30)
                    unit.saveUnit = false;
                if (unit.getGoal().isValid())
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

        const auto distSim = (unit.isFlying() || simTarget.isFlying() || unit.isWithinRange(simTarget) || simTarget.isWithinRange(unit))
            ? double(Util::boxDistance(unit.getType(), unit.getPosition(), simTarget.getType(), simTarget.getPosition()))
            : BWEB::Map::getGroundDistance(unit.getPosition(), simTarget.getPosition());
        
        const auto distTarget = (unit.isFlying() || target.isFlying() || unit.isWithinRange(target) || target.isWithinRange(unit)) ?
            double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()))
            : BWEB::Map::getGroundDistance(unit.getPosition(), target.getPosition());

        const auto insideRetreatRadius = distSim < unit.getRetreatRadius() && !atHome;
        const auto insideEngageRadius = distTarget < unit.getEngageRadius() && (unit.getGlobalState() == GlobalState::Attack || atHome);
        const auto exploringGoal = unit.getGoal().isValid() && unit.getGoalType() == GoalType::Explore && unit.getUnitsInReachOfThis().empty() && Util::getTime() > Time(4, 00);

        // Regardless of any decision, determine if Unit is in danger and needs to retreat
        if ((unit.inDanger && !unit.isTargetedBySuicide())
            || (unit.isNearSuicide() && unit.isFlying())) {
            unit.setLocalState(LocalState::Retreat);
        }

        // Forced states
        else if (insideEngageRadius && forceLocalAttack(unit))
            unit.setLocalState(LocalState::Attack);
        else if (insideRetreatRadius && forceLocalRetreat(unit))
            unit.setLocalState(LocalState::Retreat);

        // If within local decision range, determine if Unit needs to attack or retreat
        else if (insideEngageRadius && (unit.getSimState() == SimState::Win /*|| exploringGoal*/))
            unit.setLocalState(LocalState::Attack);
        else if (Terrain::inTerritory(PlayerState::Self, unit.getPosition()) && (unit.getGlobalState() == GlobalState::Retreat || unit.getGoalType() == GoalType::Defend) && !unit.isLightAir())
            unit.setLocalState(LocalState::Hold);
        else if (Terrain::inTerritory(PlayerState::Self, unit.getPosition()) && insideEngageRadius && unit.getSimState() != SimState::Win && !unit.isLightAir())
            unit.setLocalState(LocalState::Hold);
        else if (insideRetreatRadius && (!unit.attemptingRunby() || Terrain::inTerritory(PlayerState::Enemy, unit.getPosition())) && unit.getSimState() == SimState::Loss)
            unit.setLocalState(LocalState::Retreat);

        // Respect global states for overall direction
        else if (!Terrain::inTerritory(PlayerState::Self, unit.getPosition()) && unit.getGlobalState() == GlobalState::Retreat)
            unit.setLocalState(LocalState::Retreat);

        // Within engage and not retreat, but not winning
        else if (insideEngageRadius && !insideRetreatRadius && unit.getSimState() != SimState::Win && !unit.isLightAir())
            unit.setLocalState(LocalState::Hold);
    }

    void updateGlobalState(UnitInfo& unit)
    {
        if (unit.getGlobalState() != GlobalState::None)
            return;

        if (forceGlobalAttack(unit)) {
            unit.setGlobalState(GlobalState::Attack);
            unit.setLocalState(LocalState::Attack);
        }
        else if (forceGlobalRetreat(unit)) {
            unit.setGlobalState(GlobalState::Retreat);
            unit.setLocalState(LocalState::Retreat);
        }
        else if (isStaticRetreat(unit.getType()) && !unit.attemptingRunby())
            unit.setGlobalState(GlobalState::Retreat);
        else
            unit.setGlobalState(GlobalState::Attack);
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