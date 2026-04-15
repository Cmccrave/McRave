#include "Builds/All/BuildOrder.h"
#include "Combat.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Macro/Upgrading/Upgrading.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
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
            if (Players::PvZ()) {
                auto gateUnits = total(Protoss_Zealot) + total(Protoss_Dragoon);
                if ((gateUnits < 12 && int(Stations::getStations(PlayerState::Enemy).size()) < 2) || gateUnits < 3)
                    lockGateways();
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
    }

    void updateTStaticStates()
    {
        // Marines
        if (!BuildOrder::isPressure(Terran_Marine) && unlockedOrVis(Terran_Marine)) {
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
        if (!BuildOrder::isPressure(Terran_Siege_Tank_Tank_Mode) && !BuildOrder::isPressure(Terran_Siege_Tank_Siege_Mode) &&
            (unlockedOrVis(Terran_Siege_Tank_Tank_Mode) || unlockedOrVis(Terran_Siege_Tank_Siege_Mode))) {
            if (Players::TvP()) {
                if (Util::getTime() < Time(12, 00)) {
                    staticRetreatTypes.push_back(Terran_Siege_Tank_Tank_Mode);
                    staticRetreatTypes.push_back(Terran_Siege_Tank_Siege_Mode);
                    staticRetreatTypes.push_back(Terran_Goliath);
                }
            }
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
                    const auto scaryOpeners = Spy::getEnemyBuild() != P_FFE && Util::getTime() < Time(8, 00) && vis(Zerg_Sunken_Colony) > 0;
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
                    const auto enemyHydraBuild = Spy::getEnemyTransition() == Z_1HatchLurker || Spy::getEnemyTransition() == Z_1HatchHydra || Spy::getEnemyTransition() == Z_2HatchHydra;
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
                        if (BuildOrder::getCurrentTransition() == Z_2HatchMuta && Util::getTime() < Time(5, 30) && !Spy::enemyFastExpand() && !Spy::Zerg::enemySlowerSpeed()) {
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
        if (!unit.hasTarget())
            return false;
        auto &target = *unit.getTarget().lock();
        if (isStaticRetreat(unit.getType()))
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
        const auto atHome  = Terrain::isAtHome(target.getPosition());
        const auto inRange = unit.isWithinRange(target);

        auto meleeAttack = [&]() {
            if (!unit.isMelee())
                return false;

            return unit.getType() == Zerg_Broodling                                                                                                                                                //
                   || (unit.getType() == Zerg_Ultralisk && unit.unit()->isIrradiated())                                                                                                            //
                   || (unit.getSurroundPosition().isValid() && inRange)                                                                                                                            //
                   || (!target.isMelee() && Actions::overlapsActions(unit.unit(), target.getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, Util::getCastRadius(TechTypes::Dark_Swarm))); //
        };

        auto cargoAttack = [&]() {
            return (unit.getType() == Protoss_Reaver && !unit.unit()->isLoaded() && unit.canStartAttack() && inRange)                                           //
                   || (unit.getType() == Protoss_High_Templar && !unit.unit()->isLoaded() && unit.canStartCast(TechTypes::Psionic_Storm, target.getPosition())) //
                   || (unit.getType() == Terran_Ghost && com(Terran_Nuclear_Missile) > 0 && unit.unit()->isLoaded());                                           //
        };

        auto harassAttack = [&]() {
            if (unit.getType() == Zerg_Mutalisk) {
                if (Clusters::canDecimate(unit, target))
                    return true;
            }
            return false;
        };

        auto inRangeAttack = [&]() {
            if (!inRange)
                return false;

            const auto vulnerableTarget = (target.isSiegeTank() || target.isLightAir() || target.isTransport() || target.getType() == Protoss_Reaver || target.getType() == Protoss_High_Templar);

            return (!unit.isFlying() && vulnerableTarget && unit.getType() != Zerg_Lurker)      //
                   || (unit.isTargetedBySuicide() && !unit.isFlying())                          //
                   || (target.getType() == Terran_Vulture_Spider_Mine && !target.isBurrowed()); //
        };

        auto runbyAttack = [&]() {
            const auto runbyVsWorker = (unit.attemptingRunby() && target.getType().isWorker() && (unit.getHealth() > 15 || Util::getTime() > Time(6, 00) || Players::ZvZ()));

            return runbyVsWorker;
        };

        auto invisAttack = [&]() {
            return (unit.isSuicidal() && !nearEnemyDefense())                                                                                                                                   //
                   || (unit.isHidden() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy))                                                               //
                   || (unit.getType() == Zerg_Lurker && unit.isBurrowed() && !Spy::enemyDetection() && !Actions::overlapsDetection(unit.unit(), unit.getEngagePosition(), PlayerState::Enemy)); //
        };

        // If inside self territory, likely forcing an attack is best
        auto atHomeAttack = [&]() {

            // If both sides are melee vs melee, we don't need to force engagement until something is in range
            if (target.isThreatening() && !target.isHidden()) {
                if (unit.isMelee() && target.isMelee() && !target.hasAttackedRecently() && !unit.isWithinRange(target) && !Combat::isDefendNatural() && nearMainRamp())
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
        return !Terrain::isAtHome(unit.getPosition()) && unit.getGlobalState() == GlobalState::Retreat;
    }

    bool forceGlobalRetreat(UnitInfo &unit)
    {
        if (unit.hasTarget()) {
            auto &target = *unit.getTarget().lock();

            // No saving if the enemy is threatening
            if (target.isThreatening())
                return false;

            // Try to save Mutas that are low hp when the firepower isn't needed
            const auto mutaSavingRequired = unit.getType() == Zerg_Mutalisk &&
                                            (Players::ZvZ() ? (Players::getVisibleCount(PlayerState::Enemy, Zerg_Mutalisk) == 0) : (Util::getTime() > Time(8, 00))) && !unit.isWithinRange(target) &&
                                            unit.getHealth() <= 40 && !Terrain::inTerritory(PlayerState::Enemy, unit.getPosition());

            // Try to save scouts as they have high shield counts
            const auto scoutSavingRequired = unit.getType() == Protoss_Scout && !unit.isWithinRange(target) && unit.getHealth() + unit.getShields() <= 80;

            // Try to save zerglings in ZvZ
            const auto zerglingSaving = Players::ZvZ() && unit.getType() == Zerg_Zergling && !unit.isWithinRange(target) && unit.getHealth() <= 10;

            const auto queenSaving = unit.getType() == Zerg_Queen && unit.getEnergy() < TechTypes::Spawn_Broodlings.energyCost();

            // Save the units
            if (mutaSavingRequired || scoutSavingRequired || queenSaving /*|| zerglingSaving*/)
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
        if (!unit.hasSimTarget() || !unit.hasTarget()) {
            if (unit.getGlobalState() == GlobalState::Retreat)
                unit.setLocalState(LocalState::Hold);
            return;
        }

        if (unit.getLocalState() != LocalState::None)
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

        const auto insideRetreatRadius = distSim < unit.getRetreatRadius();
        const auto insideEngageRadius  = distSim < unit.getEngageRadius();
        const auto insideHoldRadius    = distSim >= unit.getRetreatRadius() || selfTerritory;

        const auto localRetreat = unit.getSimState() == SimState::Loss && (!unit.attemptingRunby() || Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()));
        const auto localEngage  = unit.getSimState() == SimState::Win && (unit.getGlobalState() == GlobalState::Attack || targetAtHome);
        const auto localHold    = unit.getSimState() != SimState::Win && !unit.isLightAir() && selfTerritory && (unit.getGlobalState() == GlobalState::Retreat || unit.getGoalType() == GoalType::Defend);

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
        else if (insideEngageRadius && localEngage)
            unit.setLocalState(LocalState::Attack);
        else if (insideHoldRadius && localHold)
            unit.setLocalState(LocalState::Hold);
        else if (insideRetreatRadius && localRetreat)
            unit.setLocalState(LocalState::Retreat);
    }

    void updateGlobalState(UnitInfo &unit)
    {
        if (unit.getGlobalState() != GlobalState::None)
            return;

        if (forceGlobalRetreat(unit) || (isStaticRetreat(unit.getType()) && !unit.attemptingRunby() && !Units::enemyThreatening()))
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

    bool isStaticRetreat(UnitType type)
    {
        auto itr = find(staticRetreatTypes.begin(), staticRetreatTypes.end(), type);
        return itr != staticRetreatTypes.end();
    }
} // namespace McRave::Combat::State