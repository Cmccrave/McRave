#include "ZergBuildOrder.h"

#include "Info/Player/Players.h"
#include "Info/Resource/Resources.h"
#include "Info/Roles.h"
#include "Info/Unit/Units.h"
#include "Macro/Planning/Planning.h"
#include "Macro/Researching/Researching.h"
#include "Macro/Upgrading/Upgrading.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Map/Walls/Walls.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

// Notes:
// Lings: 3 drones per hatch
// Hydra: 5.5 drones per hatch - 0.5 gas per hatch
// Muta: 8 drones per hatch - 1 gas per hatch

namespace McRave::BuildOrder::Zerg {

    namespace {
        bool againstRandom = false;
        bool needSpores    = false;
        bool needSunks     = false;
        bool needHatch     = false;

        void queueGasTrick()
        {
            // Executing gas trick
            if (gasTrick) {
                if (Broodwar->self()->minerals() >= 80 && total(Zerg_Extractor) == 0)
                    buildQueue[Zerg_Extractor] = 1;
                if (vis(Zerg_Extractor) > 0)
                    buildQueue[Zerg_Extractor] = (vis(Zerg_Drone) < 9);
            }
        }

        void queueDefenses()
        {
            needSunks                     = false;
            needSpores                    = false;
            auto colonyCount              = vis(Zerg_Creep_Colony) + vis(Zerg_Spore_Colony) + vis(Zerg_Sunken_Colony);
            buildQueue[Zerg_Creep_Colony] = colonyCount;

            // Adding Wall defenses
            if ((vis(Zerg_Drone) >= 8 || Players::ZvZ()) && !isPreparingAllIn()) {
                auto i = 1;
                for (auto &[_, wall] : BWEB::Walls::getWalls()) {
                    if (!Terrain::inTerritory(PlayerState::Self, wall.getArea()))
                        continue;

                    auto colonies     = 0;
                    auto stationNeeds = wall.getStation() && (Stations::needAirDefenses(wall.getStation()) > 0 || Stations::needGroundDefenses(wall.getStation()) > 0);
                    for (auto &tile : wall.getDefenses()) {
                        if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                            colonies++;
                        if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony && stationNeeds && wall.getStation()->getDefenses().find(tile) != wall.getStation()->getDefenses().end())
                            colonies--;
                    }

                    auto airNeeded = Walls::needAirDefenses(wall);
                    auto grdNeeded = Walls::needGroundDefenses(wall);
                    if (airNeeded > colonies)
                        needSpores = true;
                    if (grdNeeded > colonies)
                        needSunks = true;

                    if ((atPercent(Zerg_Spawning_Pool, 0.66) && grdNeeded > colonies) || (atPercent(Zerg_Evolution_Chamber, 0.50) && airNeeded > colonies)) {
                        buildQueue[Zerg_Creep_Colony] = colonyCount + 1;
                        LOG_SLOW("Wall ", i, " needs a sunken/spore");
                        i++;
                    }
                }
            }

            // Adding Station defenses
            if (vis(Zerg_Drone) >= 8 || Players::ZvZ()) {
                auto i = 1;
                for (auto &station : Stations::getStations(PlayerState::Self)) {
                    auto colonies  = Stations::getColonyCount(station);
                    auto airNeeded = Stations::needAirDefenses(station);
                    auto grdNeeded = Stations::needGroundDefenses(station);

                    if (airNeeded > colonies)
                        needSpores = true;
                    if (grdNeeded > colonies)
                        needSunks = true;

                    if ((atPercent(Zerg_Spawning_Pool, 0.66) && grdNeeded > colonies) || (atPercent(Zerg_Evolution_Chamber, 0.50) && airNeeded > colonies)) {
                        buildQueue[Zerg_Creep_Colony] = colonyCount + 1;
                        LOG_SLOW("Station ", i, " needs a sunken/spore");
                        i++;
                    }
                }
            }

            // Prepare evo chamber just in case and not a hydra build
            if (focusUnit != Zerg_Hydralisk && focusUnit != Zerg_Lurker) {
                if (Players::ZvT() && !Spy::enemyFastExpand() && (Spy::getEnemyBuild() == T_RaxFact || Spy::enemyWalled()) && Util::getTime() > Time(4, 15)) {
                    needSpores = true;
                    wallNat    = true;
                }
                if (Players::ZvP() && !Spy::enemyFastExpand() && Spy::getEnemyTransition() == "Unknown" && !Spy::enemyFastExpand() &&
                    ((Spy::getEnemyBuild() == P_2Gate && Util::getTime() > Time(4, 45)) || (Spy::getEnemyBuild() == P_1GateCore && Util::getTime() > Time(4, 15)))) {
                    needSpores = true;
                    wallNat    = true;
                }
                if (Players::ZvZ() && !Spy::enemyFastExpand() && Spy::getEnemyOpener() != Z_4Pool && !Spy::enemyTurtle() && Spy::getEnemyOpener() != Z_7Pool &&
                    Spy::getEnemyTransition() == "Unknown" && Util::getTime() > Time(5, 00)) {
                    needSpores = true;
                    wallNat    = true;
                }
            }

            // Delay any extra hatchery early on
            if (Util::getTime() < Time(3, 30) && hatchCount() >= 2 && needSunks && buildQueue[Zerg_Hatchery] > 2) {
                buildQueue[Zerg_Hatchery] = min(buildQueue[Zerg_Hatchery], 2);
                LOG_ONCE("Delaying third hatchery for sunkens");
            }

            // Log needing sunks
            if (needSunks)
                LOG_ONCE("Need sunkens");

            // Log needing spores
            if (needSpores)
                LOG_ONCE("Need spores");
        }

        void queueSupply()
        {
            // Adding Overlords outside opening book supply, offset by large hatch counts
            if (!inBookSupply) {
                int supplyPerOvie         = max(16 - (hatchCount() / 5), 12);
                int count                 = 1 + min(26, s / supplyPerOvie);
                buildQueue[Zerg_Overlord] = min(25, count);
            }

            // Queue enough overlords to fit the reservations
            if (reserveLarva > 0 && atPercent(Zerg_Spire, 0.33) && total(focusUnit) < reserveLarva && com(Zerg_Spire) == 0) {
                auto expectedSupply       = s + ((reserveLarva - total(focusUnit)) * 4);
                auto expectedOverlords    = int(ceil(double(expectedSupply) / 16.0));
                buildQueue[Zerg_Overlord] = expectedOverlords;
            }

            // Adding Overlords if we are sacrificing a scout or know we will lose one
            if (!Players::ZvZ() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0 && Util::getTime() < Time(8, 00)) {
                buildQueue[Zerg_Overlord]++;
            }

            // Check if any are low, add an extra if so
            auto injuredOverlord = Util::getFurthestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) { return u->getType() == Zerg_Overlord && !u->isHealthy() && u->isCompleted(); });
            if (injuredOverlord) {
                buildQueue[Zerg_Overlord]++;
            }

            // Remove spending money on Overlords if they'll have no protection anyways
            if (Players::ZvP() && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) > 0 && vis(Zerg_Lair) == 0 && vis(Zerg_Hydralisk_Den) == 0 && vis(Zerg_Spore_Colony) == 0 &&
                Util::getTime() < Time(7, 00))
                buildQueue[Zerg_Overlord] = 0;
        }

        void queueGeysers()
        {
            // Adding Extractors
            if (!inOpening) {

                auto minDronesAllGas = 40;
                if (Players::ZvZ())
                    minDronesAllGas = 10;

                auto minDronesPerGas = 10 + (4 * vis(Zerg_Extractor));
                auto takeAllGeysers  = com(Zerg_Drone) >= minDronesAllGas;
                auto allowNewGeyser  = com(Zerg_Drone) >= minDronesPerGas && (Resources::isHalfMineralSaturated() || productionSat || takeAllGeysers);
                auto needGeyser      = gasLimit > vis(Zerg_Extractor) * 3;
                gasDesired           = allowNewGeyser && needGeyser;

                buildQueue[Zerg_Extractor] = min(vis(Zerg_Extractor) + gasDesired, Resources::getGasCount());
            }
        }

        void queueUpgradeStructures()
        {
            // Adding Evolution Chambers
            if ((s >= 200 && Stations::getStations(PlayerState::Self).size() >= 3) || (s >= 50 && (unitOrder == mutalingdefiler || unitOrder == ultraling) && hatchCount() >= 3))
                buildQueue[Zerg_Evolution_Chamber] = 1 + (Stations::getStations(PlayerState::Self).size() >= 4);
            if (needSpores)
                buildQueue[Zerg_Evolution_Chamber] = max(buildQueue[Zerg_Evolution_Chamber], 1);

            // Lair upgrades
            if (Util::getTime() > Time(8, 00) && int(Stations::getStations(PlayerState::Self).size()) >= 3 && vis(Zerg_Drone) >= 28) {
                buildQueue[Zerg_Lair] = 1;
            }

            // Hive upgrades
            auto hiveStations  = Players::ZvT() ? 3 : 4;
            auto hiveDrones    = Players::ZvT() ? 28 : 36;
            auto lateGameHive  = int(Stations::getStations(PlayerState::Self).size()) >= hiveStations && vis(Zerg_Drone) >= hiveDrones;
            auto earlyGameHive = isFocusUnit(Zerg_Zergling);
            if (!inOpening && com(Zerg_Lair) > 0 && (lateGameHive || earlyGameHive)) {
                buildQueue[Zerg_Queens_Nest] = 1;
                buildQueue[Zerg_Hive]        = com(Zerg_Queens_Nest) >= 1;
                buildQueue[Zerg_Lair]        = com(Zerg_Queens_Nest) < 1;
            }
        }

        void queueExpansions()
        {
            expandDesired = false;
            if (!inOpening) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto availableGas      = Broodwar->self()->gas() - BuildOrder::getGasQueued();
                const auto incompleteHatch   = vis(Zerg_Hatchery) - com(Zerg_Hatchery);

                const auto waitForMinerals = 200 + (200 * incompleteHatch);
                const auto resourceSat     = (availableMinerals >= waitForMinerals || vis(Zerg_Larva) <= Stations::getStations(PlayerState::Self).size()) && Resources::isHalfMineralSaturated() &&
                                         Resources::isGasSaturated();
                const auto excessResources = (availableMinerals >= waitForMinerals * 2 && vis(Zerg_Larva) <= 3);

                expandDesired = (resourceSat && techSat && productionSat && Stations::getMiningStationsCount() <= 5) || (excessResources && productionSat) ||
                                (Players::ZvZ() && Players::getTotalCount(PlayerState::Enemy, Zerg_Spore_Colony) > 0 &&
                                 Stations::getStations(PlayerState::Self).size() < Stations::getStations(PlayerState::Enemy).size() && availableMinerals > waitForMinerals && availableGas < 150) ||
                                (Stations::getStations(PlayerState::Self).size() >= 4 && Stations::getMiningStationsCount() <= 2) ||
                                (Stations::getStations(PlayerState::Self).size() >= 4 && Stations::getGasingStationsCount() <= 1) ||
                                (!Players::ZvZ() && Stations::getMiningStationsCount() < 2 && Util::getTime() > Time(12, 00));

                buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + expandDesired);

                if (expandDesired && int(Stations::getStations(PlayerState::Self).size()) <= 1)
                    wantNatural = true;
                if (expandDesired && int(Stations::getStations(PlayerState::Self).size()) <= 2)
                    wantThird = true;

                if (expandDesired)
                    LOG_SLOW("Expanding to station ", int(Stations::getStations(PlayerState::Self).size() + 1));
            }
        }

        void queueProduction()
        {
            rampDesired = false;
            if (!inOpening) {

                // Calculate hatcheries per base
                map<int, int> hatchPerBase;
                if (Players::ZvZ()) {
                    hatchPerBase = {{1, 1}, {2, 4}, {3, 5}, {4, 6}};
                }
                if (Players::ZvP()) {
                    hatchPerBase = {{1, 1}, {2, 3}, {3, 5}, {4, 6}};
                }
                if (Players::ZvT()) {
                    hatchPerBase = {{1, 1}, {2, 3}, {3, 4}, {4, 8}};
                }

                // Check if we are maxed on production
                auto current           = clamp(int(Stations::getStations(PlayerState::Self).size()), 1, 4);
                auto desiredProduction = hatchPerBase[current];

                if (isFocusUnit(Zerg_Zergling))
                    desiredProduction *= 1.5;

                productionSat = hatchCount() >= min(7, desiredProduction);

                // Queue production if we need it
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto incompleteHatch   = vis(Zerg_Hatchery) - com(Zerg_Hatchery);
                const auto waitForMinerals   = 200 + (200 * incompleteHatch);

                const auto resourceSat     = (availableMinerals >= waitForMinerals && Resources::isHalfMineralSaturated() && Resources::isGasSaturated() && !productionSat && vis(Zerg_Larva) <= 3);
                const auto excessResources = (availableMinerals >= waitForMinerals * 2 && !productionSat && vis(Zerg_Larva) <= 3);
                const auto larvaBankrupt   = (availableMinerals >= waitForMinerals && (vis(Zerg_Larva) + (incompleteHatch)) < min(3, hatchCount()) && !productionSat);

                rampDesired               = resourceSat || excessResources || larvaBankrupt;
                buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + rampDesired);
            }

            // Above 3 hatcheries we need to carefully add, don't add hatcheries if we have larva to spend
            if (inOpening && hatchCount() >= 3 && inTransition) {
                if (vis(Zerg_Larva) >= (2 + hatchCount()))
                    buildQueue[Zerg_Hatchery] = hatchCount();
            }

            // Determine if we've tried to expand for a while and seem to be getting denied, ramp instead
            static auto denialTime = Util::getTime();
            if (!expandDesired || !minerals(300))
                denialTime = Util::getTime();

            auto expansionDenied = Util::getTime() > denialTime + Time(1, 00);
            if (expandDesired && expansionDenied && hatchCount() < 5) {
                expandDesired = false;
                rampDesired   = true;
                LOG_SLOW("Ramping instead of expanding due to denial");
            }
        }

        void queueNydusNetwork()
        {
            if (Stations::getStations(PlayerState::Self).size() >= 4 && Players::ZvT()) {
                buildQueue[Zerg_Nydus_Canal] = com(Zerg_Hive) > 0;
            }
        }

        void calculateGasLimit()
        {
            // Calculate the number of gas workers we need outside of the opening book
            if (!inOpening) {
                if (Players::ZvP())
                    gasLimit = vis(Zerg_Drone) / 5;
                else
                    gasLimit = vis(Zerg_Drone) / 3;
            }

            // If enemy has invis and we are trying to get lair for detection
            if (Spy::enemyInvis() && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0 && Util::getTime() > Time(4, 45) && atPercent(Zerg_Lair, 0.5))
                gasLimit += 1;

            //// If we have very limited mineral mining (maybe we pulled workers to fight) we need to cut all gas
            // if (!Players::ZvZ() && Workers::getMineralWorkers() <= 5 && Util::getTime() < Time(5, 00))
            //    gasLimit = 0;

            if (!Players::ZvZ() && Spy::getEnemyTransition() == U_WorkerRush && Util::getTime() < Time(3, 00))
                gasLimit = 0;

            // If we have to pull everything we have
            if (Players::ZvZ() && Spy::getEnemyOpener() == Z_4Pool && currentOpener == Z_12Pool && total(Zerg_Zergling) < 18)
                gasLimit = 0;
        }

        void removeExcessGas()
        {
            // Removing gas workers if we are adding Sunkens or have excess gas (and we already have an extractor complete)
            if (com(Zerg_Extractor) > 0) {
                if (Util::getTime() > Time(3, 00) || Players::ZvZ()) {
                    auto gasRemaining    = Broodwar->self()->gas() - BuildOrder::getGasQueued();
                    auto minRemaining    = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                    auto dropGasRush     = !Players::ZvZ() && Spy::enemyRush();
                    auto dropGasLarva    = !Players::ZvZ() && vis(Zerg_Larva) >= hatchCount() && unitReservations.empty();
                    auto dropGasDefenses = needSunks && (Players::ZvZ() || Spy::enemyProxy() || Spy::getEnemyOpener() == P_9_9 || Spy::getEnemyOpener() == T_8Rax);

                    auto gasPer            = (vis(Zerg_Spire) > 0) ? 20 : 13;
                    auto mineralToGasRatio = minRemaining < max(100, 8 * vis(Zerg_Drone)) && gasRemaining > max(150, gasPer * vis(Zerg_Drone));

                    if (mineralToGasRatio && !rush && !pressure) {
                        if (dropGasRush || dropGasLarva || dropGasDefenses || Roles::getMyRoleCount(Role::Worker) < 5 || Players::ZvZ())
                            gasLimit = 0;
                    }
                }
            }

            // Drop extractors if we don't want gas right now
            if (gasLimit == 0)
                buildQueue[Zerg_Extractor] = 0;

            if (Players::ZvZ()) {
                if (Spy::enemyPressure() && vis(Zerg_Drone) < 12 && vis(Zerg_Larva) > 0 && com(Zerg_Spire) == 0)
                    gasLimit = 0;
            }
        }

        void queueUpgrades()
        {
            using namespace UpgradeTypes;

            // Overlord speed can be done inside openings
            auto zvpOvieSpeed = Players::ZvP() &&                                                                                                                                  //
                                ((Players::getStrength(PlayerState::Enemy).airToAir > 0 && Players::getSupply(PlayerState::Self, Races::Zerg) >= 140 && total(Zerg_Hydralisk) > 0) //
                                 || (Players::getTotalCount(PlayerState::Enemy, Protoss_Arbiter) > 0)                                                                              //
                                 || (Util::getTime() > Time(9, 00)));                                                                                                              //

            auto zvtOvieSpeed = Players::ZvT() &&                                                              //
                                (Spy::getEnemyTransition() == T_2PortWraith && Util::getTime() > Time(6, 00)); //

            const auto queueOvieSpeed = zvpOvieSpeed || zvpOvieSpeed ||                                                                                //
                                        (Spy::enemyInvis() && (BuildOrder::isFocusUnit(Zerg_Hydralisk) || BuildOrder::isFocusUnit(Zerg_Ultralisk))) || //
                                        (!Players::ZvZ() && Players::getSupply(PlayerState::Self, Races::Zerg) >= 200);                                //

            // Need to do this otherwise our queue isnt empty
            if (queueOvieSpeed)
                upgradeQueue[Pneumatized_Carapace] = 1;

            // Drone burrow to avoid a targeted tech build
            auto zvtBurrow                  = (Players::ZvT() && Spy::getEnemyBuild() == T_RaxFact && Util::getTime() > Time(4, 15) && !Spy::enemyFastExpand());
            auto zvpBurrow                  = (Players::ZvP() && Spy::getEnemyTransition() == P_Robo && Util::getTime() > Time(5, 00) && !Spy::enemyFastExpand());
            techQueue[TechTypes::Burrowing] = zvtBurrow || zvpBurrow;

            if (inOpening)
                return;

            // Speed upgrades
            upgradeQueue[Metabolic_Boost]    = vis(Zerg_Zergling) >= 20 || total(Zerg_Hive) > 0 || total(Zerg_Defiler_Mound) > 0;
            upgradeQueue[Muscular_Augments]  = (BuildOrder::isFocusUnit(Zerg_Hydralisk) && Players::ZvP()) || Broodwar->self()->getUpgradeLevel(Grooved_Spines) > 0;
            upgradeQueue[Anabolic_Synthesis] = Players::getTotalCount(PlayerState::Enemy, Terran_Marine) < 20 || Broodwar->self()->getUpgradeLevel(Chitinous_Plating) > 0;

            // Range upgrades
            upgradeQueue[Grooved_Spines] = (BuildOrder::isFocusUnit(Zerg_Hydralisk) && !Players::ZvP()) || Broodwar->self()->getUpgradeLevel(Muscular_Augments) > 0;

            // Other upgrades
            upgradeQueue[Chitinous_Plating] = Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 20 || Broodwar->self()->getUpgradeLevel(Anabolic_Synthesis) > 0;
            upgradeQueue[Adrenal_Glands]    = com(Zerg_Hive) > 0 && vis(Zerg_Zergling) >= 36 && Broodwar->self()->getUpgradeLevel(Metabolic_Boost) > 0;

            // Ground unit upgrades
            const auto upgradingGrdArmor   = (Broodwar->self()->getUpgradeLevel(Zerg_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Melee_Attacks)) || Broodwar->self()->isUpgrading(Zerg_Carapace);
            const auto compositionAndCount = [&](auto &t) { return BuildOrder::getCompositionPercentage(t) > 0.0 && total(t) >= (t.supplyRequired() / 2); };
            upgradeQueue[Zerg_Melee_Attacks]   = upgradingGrdArmor && (compositionAndCount(Zerg_Zergling) || compositionAndCount(Zerg_Ultralisk));
            upgradeQueue[Zerg_Missile_Attacks] = upgradingGrdArmor && (compositionAndCount(Zerg_Hydralisk) || compositionAndCount(Zerg_Lurker));
            upgradeQueue[Zerg_Carapace]        = s >= 100 &&
                                          (compositionAndCount(Zerg_Hydralisk) || compositionAndCount(Zerg_Lurker) || compositionAndCount(Zerg_Zergling) || compositionAndCount(Zerg_Ultralisk));

            // Want 3x upgrades by default
            upgradeQueue[Zerg_Carapace] *= 3;
            upgradeQueue[Zerg_Missile_Attacks] *= 3;
            upgradeQueue[Zerg_Melee_Attacks] *= 3;

            // Air unit upgrades
            auto upgradingAirArmor = (Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Attacks)) ||
                                     Broodwar->self()->isUpgrading(Zerg_Flyer_Carapace);
            auto ZvZAirArmor = (Players::ZvZ() && int(Stations::getStations(PlayerState::Self).size()) >= 2);
            auto ZvTAirArmor = Players::ZvT();
            auto ZvPAirArmor = (Players::ZvP() && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0);

            upgradeQueue[Zerg_Flyer_Attacks]  = BuildOrder::getCompositionPercentage(Zerg_Mutalisk) > 0.0 && total(Zerg_Mutalisk) >= 12 && upgradingAirArmor;
            upgradeQueue[Zerg_Flyer_Carapace] = BuildOrder::getCompositionPercentage(Zerg_Mutalisk) > 0.0 && total(Zerg_Mutalisk) >= 12 && (ZvZAirArmor || ZvTAirArmor || ZvPAirArmor);

            // Want 3x upgrades by default
            upgradeQueue[Zerg_Flyer_Attacks] *= 3;
            upgradeQueue[Zerg_Flyer_Carapace] *= 3;
        }

        void queueResearch()
        {
            using namespace TechTypes;

            // Sometimes burrow can stop enemy all-ins that aim to kill drones
            if (Spy::getEnemyTransition() == P_Speedlot)
                techQueue[Burrowing] = true;

            if (inOpening)
                return;

            techQueue[Lurker_Aspect]    = BuildOrder::isFocusUnit(Zerg_Lurker) && (!BuildOrder::isFocusUnit(Zerg_Hydralisk) || (Upgrading::haveOrUpgrading(UpgradeTypes::Grooved_Spines, 1) &&
                                                                                                                             Upgrading::haveOrUpgrading(UpgradeTypes::Muscular_Augments, 1)));
            techQueue[Burrowing]        = Stations::getStations(PlayerState::Self).size() >= 3 && Players::getSupply(PlayerState::Self, Races::Zerg) > 140;
            techQueue[Consume]          = true;
            techQueue[Plague]           = Broodwar->self()->hasResearched(Consume);
            techQueue[Spawn_Broodlings] = BuildOrder::isFocusUnit(Zerg_Queen) && vis(Zerg_Queen) >= 5;
        }

        void queueAllin()
        {
            Allin Z_9HatchCrackling;
            Z_9HatchCrackling.workerCount     = 30 + (6 * vis(Zerg_Hive));
            Z_9HatchCrackling.productionCount = 9;
            Z_9HatchCrackling.typeCount       = 64;
            Z_9HatchCrackling.type            = Zerg_Zergling;
            Z_9HatchCrackling.name            = "9HatchCrackling";

            Allin Z_5HatchSpeedling;
            Z_5HatchSpeedling.workerCount     = 20;
            Z_5HatchSpeedling.productionCount = 5;
            Z_5HatchSpeedling.typeCount       = 16;
            Z_5HatchSpeedling.type            = Zerg_Zergling;
            Z_5HatchSpeedling.name            = "5HatchSpeedling";

            Allin Z_3HatchSpeedling;
            Z_3HatchSpeedling.workerCount     = 9;
            Z_3HatchSpeedling.productionCount = 3;
            Z_3HatchSpeedling.typeCount       = 6;
            Z_3HatchSpeedling.type            = Zerg_Zergling;
            Z_3HatchSpeedling.name            = "3HatchSpeedling";

            Allin Z_None;

            // Get allin struct
            if (activeAllinType == AllinType::Z_9HatchCrackling)
                activeAllin = Z_9HatchCrackling;
            if (activeAllinType == AllinType::Z_5HatchSpeedling)
                activeAllin = Z_5HatchSpeedling;
            if (activeAllinType == AllinType::Z_3HatchSpeedling)
                activeAllin = Z_3HatchSpeedling;

            // Turn off all-ins against splash or weird tech
            auto turnOffAllin = Players::getTotalCount(PlayerState::Enemy, Protoss_Reaver) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_High_Templar) >= 6 ||
                                Players::getTotalCount(PlayerState::Enemy, Protoss_Dark_Templar) >= 2 || Players::getTotalCount(PlayerState::Enemy, Protoss_Archon) >= 4 ||
                                Players::getTotalCount(PlayerState::Enemy, Protoss_Arbiter) > 0 || Players::getTotalCount(PlayerState::Enemy, Protoss_Robotics_Facility) > 0 ||
                                Players::getTotalCount(PlayerState::Enemy, Protoss_Robotics_Support_Bay) > 0 || (activeAllinType == AllinType::None);
            if (turnOffAllin) {
                activeAllin = Z_None;
                return;
            }

            // Common attributes
            if (activeAllin.name != "") {
                armyComposition.clear();
                reserveLarva = 0;
                focusUnit    = None;

                if (inOpening) {
                    inOpening = false;
                    wantThird = false;
                }

                // Clear typical buildings we wont need
                buildQueue[Zerg_Lair]          = 0;
                buildQueue[Zerg_Hydralisk_Den] = 0;
                buildQueue[Zerg_Spire]         = 0;
                buildQueue[Zerg_Extractor]     = 0;

                // Pumping
                if (vis(Zerg_Drone) < activeAllin.workerCount)
                    armyComposition[Zerg_Drone] = 1.00;
                else
                    armyComposition[activeAllin.type] = 1.00;

                // Log active all-in
                if (activeAllin.name != "")
                    LOG_ONCE("All in started: ", activeAllin.name);

                // Common
                auto lowLarvaCount = vis(Zerg_Larva) <= 2;
                auto highEconomy   = vis(Zerg_Drone) >= activeAllin.workerCount && vis(Zerg_Hatchery) == com(Zerg_Hatchery);
                if (hatchCount() < activeAllin.productionCount && (lowLarvaCount || highEconomy))
                    buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + 1);
            }

            // 9HatchCrackling
            if (activeAllin.name == "9HatchCrackling") {
                gasLimit = 3;

                // Buildings
                buildQueue[Zerg_Extractor] = 1;
                buildQueue[Zerg_Lair]      = 1;
                if (hatchCount() < activeAllin.productionCount && (vis(Zerg_Lair) > 0 || vis(Zerg_Hive) > 0) && vis(Zerg_Larva) <= 2)
                    buildQueue[Zerg_Hatchery] = max(buildQueue[Zerg_Hatchery], hatchCount() + 1);
                if (hatchCount() >= 5)
                    buildQueue[Zerg_Evolution_Chamber] = 2;

                // Hive tech
                buildQueue[Zerg_Queens_Nest] = 1;
                buildQueue[Zerg_Hive]        = atPercent(Zerg_Queens_Nest, 0.95);

                expandDesired = hatchCount() >= 7;

                // Upgrades
                upgradeQueue[UpgradeTypes::Metabolic_Boost]    = 1;
                upgradeQueue[UpgradeTypes::Zerg_Carapace]      = 3;
                upgradeQueue[UpgradeTypes::Zerg_Melee_Attacks] = 3;
                upgradeQueue[UpgradeTypes::Adrenal_Glands]     = com(Zerg_Hive) > 0;
            }

            // 5HatchSpeedling
            if (activeAllin.name == "5HatchSpeedling") {
                gasLimit                                    = lingSpeed() ? 1 : 3;
                buildQueue[Zerg_Extractor]                  = 1;
                upgradeQueue[UpgradeTypes::Metabolic_Boost] = gas(100);
            }

            // 3HatchSpeedling
            if (activeAllin.name == "3HatchSpeedling") {
                gasLimit                                    = lingSpeed() ? 0 : capGas(100);
                buildQueue[Zerg_Extractor]                  = (vis(Zerg_Spawning_Pool) > 0);
                upgradeQueue[UpgradeTypes::Metabolic_Boost] = gas(100);
            }
        }
    } // namespace

    void opener()
    {
        againstRandom = false;
        if (Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0 && !Players::ZvFFA() && !Players::ZvTVB())
            againstRandom = true;

        // TODO: Team melee / Team FFA support
        if (Broodwar->getGameType() == GameTypes::Team_Free_For_All || Broodwar->getGameType() == GameTypes::Team_Melee) {
            buildQueue[Zerg_Hatchery] = Players::getSupply(PlayerState::Self, Races::None) >= 30;
            currentBuild              = Z_HatchPool;
            currentOpener             = Z_10Hatch;
            currentTransition         = Z_3HatchMuta;
            ZvFFA();
            return;
        }

        zergUnitPump.clear();
        if (Players::vT())
            ZvT();
        else if (Players::vP() || againstRandom)
            ZvP();
        else if (Players::vZ())
            ZvZ();
        else if (Players::ZvFFA())
            ZvFFA();
    }

    void tech()
    {
        getTech = false;
        if (!inOpening) {

            auto techOffset = 0;

            // ZvP
            if (Players::ZvP()) {
                techOffset = 2;
            }

            // ZvT
            if (Players::ZvT()) {
                techOffset = 1 + Spy::Terran::enemyMech();
            }

            // ZvZ
            if (Players::ZvZ()) {
                techOffset = 0;
            }

            // ZvFFA
            if (Players::ZvFFA())
                unitOrder = {Zerg_Mutalisk, Zerg_Hydralisk, Zerg_Lurker};

            // Adding tech
            const auto endOfTech   = !unitOrder.empty() && isFocusUnit(unitOrder.back());
            const auto techVal     = int(focusUnits.size()) + techOffset + mineralThird;
            const auto readyToTech = (vis(Zerg_Extractor) >= 2 || int(Stations::getStations(PlayerState::Self).size()) >= 4 || focusUnits.empty()) && vis(Zerg_Drone) >= 10;
            techSat                = (techVal >= int(Stations::getStations(PlayerState::Self).size()) || endOfTech);

            getTech = readyToTech && !techSat && productionSat;
            getNewTech();
            getTechBuildings();
        }
    }

    void situational()
    {
        // Queue up defenses
        queueDefenses();

        // Queue up supply, upgrade structures
        queueSupply();
        queueUpgradeStructures();
        queueGasTrick();

        // Optimize our gas mining by dropping gas mining at specific excessive values
        calculateGasLimit();
        removeExcessGas();

        // Outside of opening book, book no longer is in control of queuing production, expansions or gas limits
        needHatch = false;
        queueProduction();
        queueExpansions();
        queueGeysers();

        // Queue upgrades/research
        queueUpgrades();
        queueResearch();
        queueNydusNetwork();

        // Queue an all-in if it will win
        queueAllin();
    }

    void composition()
    {
        armyComposition.clear();
        auto availGas = Broodwar->self()->gas() - (Upgrading::getReservedGas() + Researching::getReservedGas() + Planning::getPlannedGas());
        auto droneCap = 60;

        if (inOpening) {

            // Pump drones explicitly or we're trying to pump anything else but can't afford it (fall through)
            zergUnitPump[Zerg_Drone] |= zergUnitPump[Zerg_Defiler] || zergUnitPump[Zerg_Scourge] || zergUnitPump[Zerg_Mutalisk] || zergUnitPump[Zerg_Hydralisk];

            // Unit based
            if (zergUnitPump[Zerg_Lurker] && availGas > 100 && com(Zerg_Hydralisk) > 0)
                armyComposition[Zerg_Lurker] = 1.00;

            // Larva based
            else if (zergUnitPump[Zerg_Queen] && availGas >= 100)
                armyComposition[Zerg_Queen] = 1.00;
            else if (zergUnitPump[Zerg_Defiler] && availGas >= 150)
                armyComposition[Zerg_Defiler] = 1.00;
            else if (zergUnitPump[Zerg_Ultralisk] && availGas >= 200)
                armyComposition[Zerg_Ultralisk] = 1.00;
            else if (zergUnitPump[Zerg_Scourge] && availGas >= 75)
                armyComposition[Zerg_Scourge] = 1.00;
            else if (zergUnitPump[Zerg_Mutalisk] && availGas >= 100)
                armyComposition[Zerg_Mutalisk] = 1.00;
            else if (zergUnitPump[Zerg_Hydralisk] && availGas >= 25)
                armyComposition[Zerg_Hydralisk] = 1.00;
            else if (zergUnitPump[Zerg_Zergling])
                armyComposition[Zerg_Zergling] = 1.00;
            else if (zergUnitPump[Zerg_Drone])
                armyComposition[Zerg_Drone] = 1.00;
            return;
        }

        if (!inOpening) {
            static vector<pair<UnitType, int>> priorityOrder;

            // ZvP
            if (Players::ZvP() || Players::ZvTVB() || Players::ZvFFA()) {

                // Higher muta count, no hive tech
                if (unitOrder == mutaling) {
                    priorityOrder = {
                        {Zerg_Drone, 30},     {Zerg_Mutalisk, 16}, //
                        {Zerg_Drone, 45},     {Zerg_Mutalisk, 24}, //
                        {Zerg_Drone, 60},     {Zerg_Mutalisk, 48}, //
                        {Zerg_Mutalisk, 100},
                    };
                }

                // No mutas, purely ground muscle
                else if (unitOrder == hydralurk) {
                    priorityOrder = {
                        {Zerg_Drone, 30},      {Zerg_Hydralisk, 24}, {Zerg_Lurker, 2}, //
                        {Zerg_Drone, 44},      {Zerg_Hydralisk, 48}, {Zerg_Lurker, 4}, //
                        {Zerg_Drone, 60},      {Zerg_Hydralisk, 96}, {Zerg_Lurker, 8}, //
                        {Zerg_Hydralisk, 200},
                    };
                }

                // Mix of both
                else {
                    priorityOrder = {{Zerg_Drone, 30}, {Zerg_Hydralisk, 32}, {Zerg_Defiler, 2}, {Zerg_Lurker, 2}, {Zerg_Mutalisk, 9},  //
                                     {Zerg_Drone, 45}, {Zerg_Hydralisk, 64}, {Zerg_Defiler, 2}, {Zerg_Lurker, 2}, {Zerg_Mutalisk, 12}, //
                                     {Zerg_Drone, 60}, {Zerg_Hydralisk, 96}, {Zerg_Defiler, 2}, {Zerg_Lurker, 2}, {Zerg_Mutalisk, 24}};
                }
            }

            // ZvT
            if (Players::ZvT()) {

                // Higher muta count, hive tech is just defilers
                if (unitOrder == mutalingdefiler) {
                    priorityOrder = {
                        {Zerg_Drone, 30}, {Zerg_Mutalisk, 16}, {Zerg_Defiler, 1}, //
                        {Zerg_Drone, 45}, {Zerg_Mutalisk, 24}, {Zerg_Defiler, 2}, //
                        {Zerg_Drone, 60}, {Zerg_Mutalisk, 48},
                    };
                }

                // Keep a consistent muta count, get ultras and defilers eventually
                else if (unitOrder == ultraling || unitOrder == defilerling) {
                    priorityOrder = {
                        {Zerg_Drone, 30}, {Zerg_Mutalisk, 16}, {Zerg_Ultralisk, 4},  {Zerg_Defiler, 1}, //
                        {Zerg_Drone, 45}, {Zerg_Mutalisk, 16}, {Zerg_Ultralisk, 8},  {Zerg_Defiler, 2}, //
                        {Zerg_Drone, 60}, {Zerg_Mutalisk, 48}, {Zerg_Ultralisk, 12}, {Zerg_Defiler, 2},
                    };
                    if (total(Zerg_Ultralisk) > 0 || total(Zerg_Defiler) > 0) {
                        priorityOrder = {{Zerg_Drone, 30},   {Zerg_Ultralisk, 4},  {Zerg_Defiler, 1}, //
                                         {Zerg_Drone, 45},   {Zerg_Ultralisk, 8},  {Zerg_Defiler, 2}, //
                                         {Zerg_Drone, 60},   {Zerg_Ultralisk, 12}, {Zerg_Defiler, 2}, //
                                         {Zerg_Mutalisk, 16}};
                    }
                }

                else if (unitOrder == mutalingqueen) {
                    priorityOrder = {
                        {Zerg_Drone, 30}, {Zerg_Mutalisk, 16}, {Zerg_Queen, 6},  //
                        {Zerg_Drone, 45}, {Zerg_Mutalisk, 24}, {Zerg_Queen, 12}, //
                        {Zerg_Drone, 60}, {Zerg_Mutalisk, 48},
                    };
                }

                else if (unitOrder == mutalurk) {
                    priorityOrder = {
                        {Zerg_Drone, 30}, {Zerg_Mutalisk, 6}, {Zerg_Hydralisk, 2}, {Zerg_Lurker, 2}, //
                        {Zerg_Drone, 30}, {Zerg_Mutalisk, 12}, {Zerg_Hydralisk, 4}, {Zerg_Lurker, 4}, //
                        {Zerg_Drone, 45}, {Zerg_Mutalisk, 18}, {Zerg_Hydralisk, 8}, {Zerg_Lurker, 8}, //
                        {Zerg_Drone, 60}, {Zerg_Mutalisk, 24}, {Zerg_Hydralisk, 16}, {Zerg_Lurker, 16},
                    };
                }
            }

            // ZvZ
            if (Players::ZvZ()) {
                priorityOrder = {{Zerg_Mutalisk, 200}};
            }

            // Cleanup enemy
            if (Util::getTime() > Time(15, 0) && Stations::getStations(PlayerState::Enemy).size() == 0 && Terrain::foundEnemy() && availGas > 0) {
                armyComposition.clear();
                armyComposition[Zerg_Mutalisk] = 1.00;
            }

            for (auto &[type, count] : priorityOrder) {
                auto typeAvailable = (unlockReady(type) && isFocusUnit(type) && vis(type) < count) ||                                                                               //
                                     (!type.isWorker() && any_of(type.buildsWhat().begin(), type.buildsWhat().end(), [&](auto &t) { return unlockReady(t) && isFocusUnit(t); })) || //
                                     (type.isWorker() && (!Resources::isMineralSaturated() || !Resources::isGasSaturated()) && vis(type) < count);                                  //
                if (!typeAvailable)
                    continue;

                // Queue if affordable, break otherwise to prevent spending gas on other units
                if (availGas >= type.gasPrice())
                    armyComposition[type] = 1.00;
                break;
            }

            // Always need 2 lings
            if (vis(Zerg_Drone) >= 30 && vis(Zerg_Zergling) <= 2) {
                armyComposition.clear();
                armyComposition[Zerg_Zergling] = 1.0;
            }
        }

        // Pump lings or drones if nothing else to do
        if (availGas <= 0 || armyComposition.empty()) {
            if (zergUnitPump[Zerg_Zergling] || vis(Zerg_Drone) >= droneCap || (Resources::isMineralSaturated() && Resources::isGasSaturated())) {
                armyComposition[Zerg_Zergling] = 1.0;
                armyComposition[Zerg_Drone]    = 0.0;
            }
            else {
                armyComposition[Zerg_Drone]    = 1.0;
                armyComposition[Zerg_Zergling] = 0.0;
            }
        }
    }

    void unlocks()
    {
        // Saving larva to burst out tech units
        const auto reserveAt = Players::ZvZ() ? 10 : 16;
        unitReservations.clear();
        if (vis(Zerg_Drone) >= reserveAt && inOpening && reserveLarva > 0) {
            if (atPercent(Zerg_Spire, 0.50) && focusUnit == Zerg_Mutalisk)
                unitReservations[Zerg_Mutalisk] = max(0, reserveLarva - total(Zerg_Mutalisk));
            if (atPercent(Zerg_Spire, 0.50) && focusUnit == Zerg_Scourge)
                unitReservations[Zerg_Scourge] = max(0, reserveLarva - total(Zerg_Scourge) * 2);
            if (vis(Zerg_Hydralisk_Den) > 0 && focusUnit == Zerg_Hydralisk)
                unitReservations[Zerg_Hydralisk] = max(0, reserveLarva - total(Zerg_Hydralisk));

            if (zergUnitPump[Zerg_Scourge] && reserveLarva > 0)
                unitReservations[Zerg_Scourge] = 1;
        }

        // Unlocking units
        unlockedType.clear();
        for (auto &[type, per] : armyComposition) {
            if (per > 0.0)
                unlockedType.insert(type);
        }

        // Always allowed to make overlords
        unlockedType.insert(Zerg_Overlord);
    }

    bool lingSpeed() { return Upgrading::haveOrUpgrading(UpgradeTypes::Metabolic_Boost, 1); }

    bool hydraSpeed() { return Upgrading::haveOrUpgrading(UpgradeTypes::Muscular_Augments, 1); }

    bool hydraRange() { return Upgrading::haveOrUpgrading(UpgradeTypes::Grooved_Spines, 1); }

    bool gas(int amount) { return Broodwar->self()->gas() >= amount; }

    bool minerals(int amount) { return Broodwar->self()->minerals() >= amount; }

    int capGas(int value)
    {
        auto onTheWay = 0;
        for (auto &w : Units::getUnits(PlayerState::Self)) {
            auto &worker = *w;
            if (worker.unit()->isCarryingGas() || (worker.hasResource() && worker.getResource().lock()->getType().isRefinery()))
                onTheWay += 8;
        }

        return int(round(double(value - Broodwar->self()->gas() - onTheWay) / 8.0));
    }
} // namespace McRave::BuildOrder::Zerg