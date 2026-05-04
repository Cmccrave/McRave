#include "Builds/All/BuildOrder.h"
#include "Combat.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Macro/Upgrading/Upgrading.h"
#include "Map/Stations/Stations.h"
#include "Map/Terrain/Terrain.h"
#include "Strategy/Actions/Actions.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat::State {

    vector<UnitType> staticRetreatTypes;

    const auto unlockedOrVis = [&](auto &t) { return vis(t) > 0 || BuildOrder::isUnitUnlocked(t); };

    void updatePStaticStates()
    {
        // Corsairs
        if (!BuildOrder::isPressure(Protoss_Corsair) && unlockedOrVis(Protoss_Corsair)) {
            if (Players::PvZ()) {
                if (Players::getCompleteCount(PlayerState::Enemy, Zerg_Scourge) > 0 && !Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Protoss_Air_Weapons) &&
                    com(Protoss_Corsair) < 6)
                    staticRetreatTypes.push_back(Protoss_Corsair);
            }
        }

        // Carriers
        static bool carrierCountReady = carrierCountReady || com(Protoss_Carrier) >= 4;
        if (!BuildOrder::isPressure(Protoss_Carrier) && unlockedOrVis(Protoss_Carrier)) {
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
        if (!BuildOrder::isPressure(Protoss_Dragoon) && !BuildOrder::isPressure(Protoss_Zealot) && (unlockedOrVis(Protoss_Zealot) || unlockedOrVis(Protoss_Dragoon))) {

            auto slowLots  = !Upgrading::haveUpgrade(UpgradeTypes::Leg_Enhancements);
            auto rangeless = !Upgrading::haveUpgrade(UpgradeTypes::Singularity_Charge);

            if (Players::PvZ()) {
                auto gateUnits = total(Protoss_Zealot) + total(Protoss_Dragoon);
                if ((gateUnits < 12 && int(Stations::getStations(PlayerState::Enemy).size()) < 2) || gateUnits < 3)
                    lockGateways();
                if (Spy::getEnemyBuild() == P_FFE) {
                    if (slowLots && rangeless)
                        lockGateways();
                }
            }
            if (Players::PvP()) {
                auto rush = Spy::getEnemyOpener() == P_9_9 || Spy::getEnemyOpener() == P_Proxy_9_9;
                if (rush && !Upgrading::haveUpgrade(UpgradeTypes::Singularity_Charge))
                    lockGateways();
            }
            if (Players::PvT()) {
                auto rush = Spy::getEnemyOpener() == T_BBS;
                if (rush && !Upgrading::haveUpgrade(UpgradeTypes::Singularity_Charge))
                    lockGateways();
            }
        }

        // Probes
        if (!BuildOrder::isPressure(Protoss_Probe) && unlockedOrVis(Protoss_Probe)) {
            if (!unlockedOrVis(Protoss_Zealot) || !staticRetreatTypes.empty())
                staticRetreatTypes.push_back(Zerg_Drone);
        }
    }

    void updateTStaticStates()
    {
        // Lock barracks units except ghosts
        const auto lockBarracks = [&]() {
            staticRetreatTypes.push_back(Terran_Marine);
            staticRetreatTypes.push_back(Terran_Medic);
            staticRetreatTypes.push_back(Terran_Firebat);
        };

        // Lock factory units except vultures
        const auto lockFactory = [&]() {
            staticRetreatTypes.push_back(Terran_Siege_Tank_Tank_Mode);
            staticRetreatTypes.push_back(Terran_Siege_Tank_Siege_Mode);
            staticRetreatTypes.push_back(Terran_Goliath);
        };

        // Barracks
        if (!BuildOrder::isPressure(Terran_Marine) && unlockedOrVis(Terran_Marine)) {
            if (Players::TvZ()) {
                auto stim         = Players::getPlayerInfo(Broodwar->self())->hasTech(TechTypes::Stim_Packs);
                auto enemyOneBase = !Spy::enemyFastExpand() && Util::getTime() < Time(8, 00);
                if (!stim || Spy::enemyPressure() || enemyOneBase)
                    lockBarracks();
            }
            if (Players::TvP() || Players::TvT()) {
                if (!BuildOrder::isRush())
                    lockBarracks();
            }
        }

        // Factory
        if (!BuildOrder::isPressure(Terran_Siege_Tank_Tank_Mode) && !BuildOrder::isPressure(Terran_Siege_Tank_Siege_Mode) && !BuildOrder::isPressure(Terran_Goliath) &&
            (unlockedOrVis(Terran_Siege_Tank_Tank_Mode) || unlockedOrVis(Terran_Siege_Tank_Siege_Mode) || unlockedOrVis(Terran_Goliath))) {
            if (Players::TvP()) {
                if (Util::getTime() < Time(12, 00)) {
                    staticRetreatTypes.push_back(Terran_Siege_Tank_Tank_Mode);
                    staticRetreatTypes.push_back(Terran_Siege_Tank_Siege_Mode);
                    staticRetreatTypes.push_back(Terran_Goliath);
                }
            }

            if (Players::TvT()) {
                if (!Spy::enemyFastExpand() && Util::getTime() < Time(8, 00))
                    lockFactory();
            }

            if (Players::TvZ()) {
                if (Util::getTime() < Time(9, 00) || !Upgrading::haveUpgrade(UpgradeTypes::Charon_Boosters))
                    lockFactory();
            }
        }

        // SCVs
        if (!BuildOrder::isPressure(Terran_SCV) && unlockedOrVis(Terran_SCV)) {
            if (!unlockedOrVis(Terran_Marine) || !staticRetreatTypes.empty())
                staticRetreatTypes.push_back(Terran_SCV);
        }
    }

    void updateZStaticStates()
    {
        // Hydralisks
        if (!BuildOrder::isPressure(Zerg_Hydralisk) && (unlockedOrVis(Zerg_Hydralisk) || BuildOrder::getCurrentTransition() == Z_4HatchHydra || BuildOrder::getCurrentTransition() == Z_6HatchHydra)) {
            const auto hydraSpeed   = Upgrading::haveUpgrade(UpgradeTypes::Muscular_Augments);
            const auto hydraRange   = Upgrading::haveUpgrade(UpgradeTypes::Grooved_Spines);
            const auto defendTiming = Spy::getEnemyBuild() == P_FFE && Util::getTime() < Time(12, 00);
            const auto enemyTeched  = Players::getTotalCount(PlayerState::Enemy, Protoss_Shuttle) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) > 0 ||
                                     Players::getTotalCount(PlayerState::Enemy, Protoss_Robotics_Facility) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Robotics_Support_Bay) > 0;

            if (Players::ZvP() || Players::ZvFFA()) {
                if (!enemyTeched) {
                    if (!hydraRange || !hydraSpeed || BuildOrder::isAllIn() || defendTiming)
                        staticRetreatTypes.push_back(Zerg_Hydralisk);
                }
            }
            if (Players::ZvT() || Players::ZvFFA()) {
                if (!hydraRange || !hydraSpeed)
                    staticRetreatTypes.push_back(Zerg_Hydralisk);
            }
        }

        // Mutalisks
        if (!BuildOrder::isPressure(Zerg_Mutalisk) && (unlockedOrVis(Zerg_Mutalisk) || BuildOrder::getCurrentTransition() == Z_2HatchMuta || BuildOrder::getCurrentTransition() == Z_3HatchMuta)) {
            if (Players::ZvZ()) {
                const auto lessMutas = com(Zerg_Mutalisk) < Players::getCompleteCount(PlayerState::Enemy, Zerg_Mutalisk);
                const auto moreGas   = Stations::getStations(PlayerState::Self).size() > Stations::getStations(PlayerState::Enemy).size() && Util::getTime() < Time(9, 00);
                const auto betterEco = Players::getVisibleCount(PlayerState::Self, Zerg_Drone) > Players::getVisibleCount(PlayerState::Enemy, Zerg_Drone) &&
                                       Players::getVisibleCount(PlayerState::Self, Zerg_Hatchery) > Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery) && Util::getTime() < Time(7, 00);
                if (lessMutas || moreGas || betterEco)
                    staticRetreatTypes.push_back(Zerg_Mutalisk);
            }
            if (Players::ZvP()) {
                const auto lowCount    = com(Zerg_Mutalisk) < 5 && total(Zerg_Mutalisk) < 9;
                const auto corsairPump = Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Protoss_Air_Weapons, 1) && vis(Protoss_Corsair) >= 5 && Util::getTime() < Time(12, 00);
                if (lowCount || corsairPump)
                    staticRetreatTypes.push_back(Zerg_Mutalisk);
            }
            if (Players::ZvT()) {
                if (com(Zerg_Mutalisk) < 6 && total(Zerg_Mutalisk) < 9)
                    staticRetreatTypes.push_back(Zerg_Mutalisk);
            }

            // If we don't have enough for a reasonable ball, we should just go home
            if (Util::getTime() < Time(10, 0) && (Players::ZvT() || Players::ZvP())) {
                auto healthyCount = 0;
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    if (u->isLightAir() && !u->saveUnit)
                        healthyCount++;
                }
                if (healthyCount < 5)
                    staticRetreatTypes.push_back(Zerg_Mutalisk);
            }
        }

        // Zerglings
        if (!BuildOrder::isPressure(Zerg_Zergling) && unlockedOrVis(Zerg_Zergling)) {
            const auto speedLing = Upgrading::haveUpgrade(UpgradeTypes::Metabolic_Boost);
            const auto crackling = Upgrading::haveUpgrade(UpgradeTypes::Adrenal_Glands);
            const auto volume    = speedLing && Players::getTotalCount(PlayerState::Self, Zerg_Zergling) >= 64;

            if (!crackling && !volume && !BuildOrder::isRush() && !BuildOrder::isAllIn()) {

                if (Players::ZvFFA()) {
                    staticRetreatTypes.push_back(Zerg_Zergling);
                }

                if (Players::ZvP()) {
                    const auto killWorkers  = Players::getDeadCount(PlayerState::Enemy, Protoss_Probe) >= 8;
                    const auto scaryOpeners = (Spy::getEnemyBuild() != P_FFE && Util::getTime() < Time(8, 00));
                    const auto hideCheese   = BuildOrder::isHideTech() && BuildOrder::isOpener();
                    const auto deniedProxy  = Spy::enemyProxy() && Players::getDeadCount(PlayerState::Enemy, Protoss_Pylon) > 0;
                    const auto defendProxy  = Spy::enemyProxy() && !speedLing && Util::getTime() < Time(5, 00) && Players::getDeadCount(PlayerState::Enemy, Protoss_Pylon) == 0;
                    const auto defendTiming = Spy::getEnemyBuild() == P_FFE && Util::getTime() > Time(6, 00) && Util::getTime() < Time(8, 00);

                    if (!killWorkers && !deniedProxy && Spy::getEnemyBuild() != P_CannonRush) {
                        if (scaryOpeners || hideCheese || defendProxy || defendTiming)
                            staticRetreatTypes.push_back(Zerg_Zergling);
                    }
                }
                if (Players::ZvT()) {
                    const auto counterAttack = com(Zerg_Sunken_Colony) > 0 && Spy::getEnemyTransition() == U_WorkerRush;
                    const auto speedVultures = Players::hasUpgraded(PlayerState::Enemy, UpgradeTypes::Ion_Thrusters, 1);
                    const auto defendSunkens = com(Zerg_Mutalisk) == 0 && !speedLing && vis(Zerg_Sunken_Colony) > 0;
                    const auto vulturesExist = Players::getCompleteCount(PlayerState::Enemy, Terran_Vulture) > 0;
                    const auto vultureThreat = Util::getTime() < Time(8, 00) && Util::getTime() > Time(3, 30) && !Spy::enemyGreedy() && !Spy::enemyProxy() &&
                                               (Spy::getEnemyBuild() == T_RaxFact || Spy::enemyWalled());
                    if (!counterAttack) {
                        if (vulturesExist || vultureThreat || defendSunkens) {
                            staticRetreatTypes.push_back(Zerg_Zergling);
                        }
                    }
                }
                if (Players::ZvZ()) {
                    const auto enemyLingVomit = ((Spy::getEnemyTransition() == Z_2HatchSpeedling && Util::getTime() > Time(4, 00)) || Spy::getEnemyTransition() == Z_3HatchSpeedling) &&
                                                Players::getTotalCount(PlayerState::Enemy, Zerg_Mutalisk) < 9;
                    const auto avoidDiceRoll = (Broodwar->getStartLocations().size() >= 3 && Util::getTime() < Time(3, 15) && !Terrain::getEnemyStartingPosition().isValid()) ||
                                               (BuildOrder::getCurrentOpener() == Z_12Pool) || (BuildOrder::getCurrentOpener() == Z_12Hatch);
                    const auto enemyDroneScouted = Players::getCompleteCount(PlayerState::Enemy, Zerg_Drone) > 0 && !Terrain::getEnemyStartingPosition().isValid() && Util::getTime() < Time(3, 15);

                    const auto hatchAdvatange = Players::getVisibleCount(PlayerState::Self, Zerg_Hatchery, Zerg_Lair, Zerg_Hive) >
                                                Players::getVisibleCount(PlayerState::Enemy, Zerg_Hatchery, Zerg_Lair, Zerg_Hive);
                    const auto lingAdvantage = Players::getVisibleCount(PlayerState::Self, Zerg_Zergling) * 2 > Players::getVisibleCount(PlayerState::Enemy, Zerg_Zergling) * 3 &&
                                               Players::getTotalCount(PlayerState::Enemy, Zerg_Zergling) >= 6 && Players::getVisibleCount(PlayerState::Enemy, Zerg_Sunken_Colony) < 2;
                    const auto expansionAdvantage = Stations::getStations(PlayerState::Self).size() > Stations::getStations(PlayerState::Enemy).size();
                    const auto techAdvantage      = Players::getVisibleCount(PlayerState::Enemy, Zerg_Lair, Zerg_Hydralisk_Den) == 0;

                    //
                    const auto enemyHydraBuild  = Spy::getEnemyTransition() == Z_1HatchLurker || Spy::getEnemyTransition() == Z_1HatchHydra || Spy::getEnemyTransition() == Z_2HatchHydra;
                    const auto enemyTimingBuild = Spy::getEnemyTransition() == Z_2HatchSpeedling || Spy::getEnemyTransition() == Z_UpgradeLing;
                    if (!enemyHydraBuild) {
                        if (!lingAdvantage && (expansionAdvantage || hatchAdvatange || techAdvantage))
                            staticRetreatTypes.push_back(Zerg_Zergling);

                        // 1hm early
                        if (BuildOrder::getCurrentTransition() == Z_1HatchMuta && Util::getTime() < Time(7, 00)) {
                            if (Spy::Zerg::enemyFasterPool() || Spy::Zerg::enemyEqualPool() || Spy::enemyTurtle() || enemyLingVomit || enemyDroneScouted)
                                staticRetreatTypes.push_back(Zerg_Zergling);
                        }

                        // 1hm mid
                        if (BuildOrder::getCurrentTransition() == Z_1HatchMuta && Util::getTime() > Time(3, 15) && Util::getTime() < Time(10, 00)) {
                            staticRetreatTypes.push_back(Zerg_Zergling);
                        }

                        // 2hm early
                        if (BuildOrder::getCurrentTransition() == Z_2HatchMuta && Util::getTime() < Time(4, 00) && !Spy::enemyFastExpand()) {
                            if (Spy::Zerg::enemyFasterPool() || Spy::Zerg::enemyEqualPool() || avoidDiceRoll || enemyDroneScouted)
                                staticRetreatTypes.push_back(Zerg_Zergling);
                        }

                        // 2hm mid
                        if (BuildOrder::getCurrentTransition() == Z_2HatchMuta && Util::getTime() < Time(10, 00) && !Spy::enemyFastExpand() && !Spy::Zerg::enemySlowerSpeed()) {
                            if (enemyLingVomit || Spy::Zerg::enemyFasterSpeed())
                                staticRetreatTypes.push_back(Zerg_Zergling);
                        }
                    }
                }
            }
        }

        // Drones
        if (!BuildOrder::isPressure(Zerg_Drone) && unlockedOrVis(Zerg_Drone)) {
            if (!Spy::enemyProxy()) {
                if (!unlockedOrVis(Zerg_Zergling) || !staticRetreatTypes.empty())
                    staticRetreatTypes.push_back(Zerg_Drone);
            }
        }
    }

    // Certain unit types are vulnerable under certain group sizes / lack of upgrades
    void updateStaticStates()
    {
        staticRetreatTypes.clear();
        updatePStaticStates();
        updateTStaticStates();
        updateZStaticStates();
    }

    bool forceLocalHold(UnitInfo &unit)
    {
        if (!unit.hasTarget() || unit.isFlying())
            return false;
        auto &target = *unit.getTarget().lock();

        auto holdAtHome = Terrain::inTerritory(PlayerState::Self, unit.getPosition()) && (unit.getGlobalState() == GlobalState::Retreat || unit.getGoalType() == GoalType::Defend);

        if (holdAtHome)
            return true;
        return false;
    }

    bool forceLocalAttack(UnitInfo &unit)
    {
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();

        const auto nearEnemyStation = [&]() {
            const auto closestEnemyStation = Stations::getClosestStationGround(unit.getPosition(), PlayerState::Enemy);
            return (closestEnemyStation && unit.getPosition().getDistance(closestEnemyStation->getBase()->Center()) < 400.0);
        };

        const auto nearEnemyDefense = [&]() {
            const auto closestDefense = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) { return u->getType().isBuilding() && u->canAttack(unit); });
            return closestDefense && closestDefense->getPosition().getDistance(target.getPosition()) < 256.0;
        };

        const auto nearMainRamp = [&]() {
            auto center = Terrain::getMainRamp().center;
            return center.isValid() && target.getPosition().getDistance(center) < 64.0;
        };

        // General commonly used checks
        const auto atHome        = Terrain::isAtHome(target.getPosition());
        const auto inRange       = unit.isWithinRange(target);
        const auto targetInRange = target.isWithinRange(unit);

        // If this unit is melee and forcing engagement is ideal
        auto meleeAttack = [&]() {
            if (!unit.isMelee())
                return false;

            return unit.getType() == Zerg_Broodling                                                                                                                                               //
                   || (unit.getType() == Zerg_Ultralisk && unit.unit()->isIrradiated())                                                                                                           //
                   || (unit.getSurroundPosition().isValid() && inRange)                                                                                                                           //
                   || (unit.getType().isWorker() && target.getType().isWorker() && Util::getTime() < Time(2, 00))                                                                                 //
                   || (!target.isMelee() && Actions::overlapsActions(unit.unit(), target.getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, Util::getCastRadius(TechTypes::Dark_Swarm))) //
                   || (unit.isSuicidal() && !nearEnemyDefense());                                                                                                                                 //
        };

        // Cargo that is dropped from a transport should engage
        auto cargoAttack = [&]() {
            return (unit.getType() == Protoss_Reaver && !unit.unit()->isLoaded() && unit.canStartAttack() && inRange)                                           //
                   || (unit.getType() == Protoss_High_Templar && !unit.unit()->isLoaded() && unit.canStartCast(TechTypes::Psionic_Storm, target.getPosition())) //
                   || (unit.getType() == Terran_Ghost && com(Terran_Nuclear_Missile) > 0 && unit.unit()->isLoaded());                                           //
        };

        // Harassing units should engage when they can get value
        auto harassAttack = [&]() {
            if (unit.getType() == Zerg_Mutalisk) {
                if (Clusters::canDecimate(unit, target))
                    return true;
            }
            if (unit.getType() == Terran_Vulture) {
                if (target.isMelee() && Util::getTime() < Time(5, 00))
                    return true;
            }
            return false;
        };

        // Units in range that can kite or want to kill something deadly should engage
        auto inRangeAttack = [&]() {
            if (!inRange)
                return false;

            const auto vulnerableTarget = (target.isSiegeTank() || target.isLightAir() || target.isTransport() || target.getType() == Protoss_Reaver || target.getType() == Protoss_High_Templar);

            return (!unit.isFlying() && vulnerableTarget && unit.getType() != Zerg_Lurker)      //
                   || atHome                                                                    //
                   || (unit.isTargetedBySuicide() && !unit.isFlying())                          //
                   || (target.getType() == Terran_Vulture_Spider_Mine && !target.isBurrowed()); //
        };

        // Runby units should always engage once they've found workers
        auto runbyAttack = [&]() {
            const auto runbyVsWorker = (unit.attemptingRunby() && target.getType().isWorker() && (unit.getHealth() > 15 || Util::getTime() > Time(6, 00) || Players::ZvZ()));

            return runbyVsWorker;
        };

        // Invis units should always engage when detection isnt present
        auto invisAttack = [&]() {
            return (unit.isHidden() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))                                                                  //
                   || (unit.getType() == Zerg_Lurker && unit.isBurrowed() && !Spy::enemyDetection() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy)); //
        };

        // If inside self territory, likely forcing an attack is best
        auto atHomeAttack = [&]() {
            // If both sides are melee vs melee, we don't need to force engagement until something is in range
            if (target.isThreatening() && !target.isHidden()) {
                if (unit.isMelee() && target.isMelee() && !target.hasAttackedRecently() && !inRange && !Combat::isDefendNatural() && nearMainRamp())
                    return false;
                if (unit.getType() == Zerg_Zergling && Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) > 0 && !inRange && !target.hasAttackedRecently())
                    return false;
                return true;
            }

            if (!atHome)
                return false;

            return (unit.getSimState() == SimState::Win && !Players::ZvZ())                                                                                                        //
                   || unit.isSuicidal()                                                                                                                                            //
                   || (!target.canAttackAir() && unit.isFlying())                                                                                                                  //
                   || (!target.canAttackGround() && !unit.isFlying())                                                                                                              //
                   || (!unit.getType().isWorker() && !Spy::enemyRush() && (unit.getGroundRange() > target.getGroundRange() || target.getType().isWorker()) && !target.isHidden()); //
        };

        // If inside enemy territory, likely forcing an attack is best
        auto atEnemyAttack = [&]() {
            return (!unit.isFlying() && unit.isMelee() && unit.getGoalType() == GoalType::Explore && Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()) && Util::getTime() > Time(8, 00) &&
                    !Players::ZvZ() && nearEnemyStation());
        };

        return meleeAttack() || cargoAttack() || harassAttack() || inRangeAttack() || runbyAttack() || invisAttack() || atHomeAttack() || atEnemyAttack();
    }

    bool forceLocalRetreat(UnitInfo &unit)
    {
        // Forced local retreats:
        // ... Zealot without speed targeting a Vulture
        // ... Corsair targeting a Scourge with less than 6 completed Corsairs
        // ... Corsair/Scout targeting a Overlord under threat greater than shields
        // ... Medic with no energy
        // ... Zergling with low health targeting a worker
        if (unit.hasTarget()) {
            auto &target                      = *unit.getTarget().lock();
            const auto slowZealotVsVulture    = unit.getType() == Protoss_Zealot && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements) == 0 && target.getType() == Terran_Vulture;
            const auto sparseCorsairVsScourge = unit.getType() == Protoss_Corsair && target.isSuicidal() && com(Protoss_Corsair) < 6; // TODO: Check density instead
            const auto lowShieldFlyer         = false; // (unit.isLightAir() && unit.getType().maxShields() > 0 && target.getType() == Zerg_Overlord && Grids::getAirThreat(unit.getEngagePosition(),
                                                       // PlayerState::Enemy) * 5.0 > (double)unit.getShields());
            const auto oomMedic = unit.getType() == Terran_Medic && unit.getEnergy() <= TechTypes::Healing.energyCost();

            if (unit.getType() == Zerg_Zergling) {
                if (BuildOrder::isRush() && unit.getHealth() < 20 && !target.getType().isBuilding())
                    return true;
            }
            const auto hurtLingVsWorker = (unit.getType() == Zerg_Zergling && unit.getHealth() <= 15 && target.getType().isWorker() && Util::getTime() < Time(6, 00) && !Players::ZvZ());

            if (slowZealotVsVulture || sparseCorsairVsScourge || lowShieldFlyer || oomMedic || hurtLingVsWorker)
                return true;
        }
        return unit.getGlobalState() == GlobalState::Retreat;
    }

    bool forceGlobalRetreat(UnitInfo &unit)
    {
        if (unit.getGoalType() == GoalType::Escort || unit.getGoalType() == GoalType::Runby)
            return false;

        if (isStaticRetreat(unit.getType()))
            return true;

        if (unit.hasTarget()) {
            auto &target = *unit.getTarget().lock();

            // No saving if the enemy is threatening
            if (target.isThreatening())
                return false;

            // Try to save Mutas that are low hp when the firepower isn't needed
            const auto mutaSavingRequired = unit.getType() == Zerg_Mutalisk && (!Players::ZvZ() || Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) == 0) && !unit.isWithinRange(target) &&
                                            !Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()) && unit.getHealth() <= 40;

            // Try to save scouts as they have high shield counts
            const auto scoutSavingRequired = unit.getType() == Protoss_Scout && !unit.isWithinRange(target) && unit.getHealth() + unit.getShields() <= 80;

            // Try to save wraiths after only a few hits, want to keep a large count
            const auto wraithSavingRequired = unit.getType() == Terran_Wraith && unit.getHealth() < 75;

            // Try to save zerglings in ZvZ
            const auto zerglingSaving = Players::ZvZ() && unit.getType() == Zerg_Zergling && !unit.isWithinRange(target) && unit.getHealth() <= 10;

            const auto queenSaving = unit.getType() == Zerg_Queen && unit.getEnergy() < TechTypes::Spawn_Broodlings.energyCost();

            // Save the units
            if (mutaSavingRequired || scoutSavingRequired || queenSaving || wraithSavingRequired /*|| zerglingSaving*/)
                unit.saveUnit = true;
            if (unit.saveUnit) {
                if (unit.getType() == Zerg_Mutalisk && unit.getHealth() >= 100)
                    unit.saveUnit = false;
                if (unit.getType() == Protoss_Scout && unit.getShields() >= 90)
                    unit.saveUnit = false;
                if (unit.getType() == Zerg_Zergling && unit.getHealth() >= 30)
                    unit.saveUnit = false;
                if (unit.getType() == Terran_Wraith && unit.getHealth() >= 120)
                    unit.saveUnit = false;
                if (unit.getGoal().isValid())
                    unit.saveUnit = false;
                if (unit.getType() == Zerg_Queen && unit.getEnergy() >= TechTypes::Spawn_Broodlings.energyCost())
                    unit.saveUnit = false;
            }
        }

        // Forced global retreat:
        // ... unit is near a hidden enemy
        // ... unit should be sent home to heal
        return unit.isNearHidden() || unit.saveUnit;
    }

    void updateLocalState(UnitInfo &unit)
    {
        if (!unit.hasSimTarget() || !unit.hasTarget() || unit.getLocalState() != LocalState::None)
            return;

        auto &simTarget          = *unit.getSimTarget().lock();
        auto &target             = *unit.getTarget().lock();
        const auto targetAtHome  = Terrain::isAtHome(target.getPosition());
        const auto selfTerritory = Terrain::inTerritory(PlayerState::Self, unit.getPosition());

        const auto distSim = (unit.isFlying() || simTarget.isFlying() || unit.isWithinRange(simTarget) || simTarget.isWithinRange(unit) || unit.hasTransport())
                                 ? double(Util::boxDistance(unit.getType(), unit.getPosition(), simTarget.getType(), simTarget.getPosition()))
                                 : BWEB::Map::getGroundDistance(unit.getPosition(), simTarget.getPosition());

        const auto distTarget = (unit.isFlying() || target.isFlying() || unit.isWithinRange(target) || target.isWithinRange(unit) || unit.hasTransport())
                                    ? double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()))
                                    : BWEB::Map::getGroundDistance(unit.getPosition(), target.getPosition());

        const auto insideRetreatRadius = distSim < unit.getRetreatRadius() || unit.getGlobalState() == GlobalState::Retreat;
        const auto insideEngageRadius  = distSim < unit.getEngageRadius();
        const auto insideHoldRadius    = distSim >= unit.getRetreatRadius() || selfTerritory;

        const auto localRetreat = unit.getSimState() == SimState::Loss && insideRetreatRadius && (!unit.attemptingRunby() || Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()));
        const auto localEngage  = unit.getSimState() == SimState::Win && insideEngageRadius && (unit.getGlobalState() == GlobalState::Attack || targetAtHome);
        const auto localHold    = unit.getSimState() != SimState::Win && insideHoldRadius && !unit.isLightAir();

        // Regardless of any decision, determine if Unit is in danger and needs to retreat
        if (unit.inDanger && !unit.isTargetedBySuicide()) {
            unit.setLocalState(LocalState::Retreat);
        }

        // Forced states
        else if (insideEngageRadius && forceLocalAttack(unit))
            unit.setLocalState(LocalState::Attack);
        else if (insideHoldRadius && forceLocalHold(unit))
            unit.setLocalState(LocalState::Hold);
        else if (insideRetreatRadius && forceLocalRetreat(unit))
            unit.setLocalState(LocalState::Retreat);

        // If within local decision range
        else if (localEngage)
            unit.setLocalState(LocalState::Attack);
        // else if (localHold)
        //    unit.setLocalState(LocalState::Hold);
        else if (localRetreat)
            unit.setLocalState(LocalState::Retreat);
    }

    void updateGlobalState(UnitInfo &unit)
    {
        if (unit.getGlobalState() != GlobalState::None)
            return;

        if (forceGlobalRetreat(unit))
            unit.setGlobalState(GlobalState::Retreat);
        else
            unit.setGlobalState(GlobalState::Attack);
    }

    void updateStates()
    {
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;
            if (unit.getRole() == Role::Combat) {
                updateGlobalState(unit);
                updateLocalState(unit);
            }
        }
    }

    void drawStates()
    {
        if (!Visuals::isDrawingEnabled(DrawingType::States))
            return;

        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;
            int width  = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            width += -32;

            if (unit.getRole() == Role::Combat) {
                auto color = unit.getLocalState() == LocalState::Attack ? Text::Green : Text::Red;
                Broodwar->drawTextMap(unit.getPosition() + Position(-width, -8), "L: %c%d", color, unit.getLocalState());
            }

            if (unit.getRole() == Role::Combat) {
                auto color = unit.getGlobalState() == GlobalState::Attack ? Text::Green : Text::Red;
                Broodwar->drawTextMap(unit.getPosition() + Position(-width, 0), "G: %c%d", color, unit.getGlobalState());
            }

            if (unit.getRole() == Role::Combat) {
                auto color = unit.getSimState() == SimState::Win ? Text::Green : Text::Red;
                Broodwar->drawTextMap(unit.getPosition() + Position(-width, 8), "S: %c%.2f", color, unit.getSimValue());
            }
        }
    }

    void onFrame()
    {
        updateStaticStates();
        updateStates();
        drawStates();
    }

    bool isStaticRetreat(UnitType type)
    {
        auto itr = find(staticRetreatTypes.begin(), staticRetreatTypes.end(), type);
        return itr != staticRetreatTypes.end();
    }
} // namespace McRave::Combat::State