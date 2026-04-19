#include "TerranBuildOrder.h"

#include "Info/Player/Players.h"
#include "Info/Resource/Resources.h"
#include "Macro/Planning/Planning.h"
#include "Macro/Researching/Researching.h"
#include "Macro/Upgrading/Upgrading.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Terran {

    namespace {
        bool againstRandom = false;
        bool needTurrets   = false;

        void queueDefenses()
        {
            // Adding Station Defenses
            for (auto &station : Stations::getStations(PlayerState::Self)) {
                auto airNeeded = Stations::needAirDefenses(station);
                auto grdNeeded = Stations::needGroundDefenses(station);

                if (airNeeded > 0)
                    needTurrets = true;

                if (airNeeded > 0 && atPercent(Terran_Engineering_Bay, 0.80))
                    buildQueue[Terran_Missile_Turret] = vis(Terran_Missile_Turret) + 1;
                if (grdNeeded > 0)
                    buildQueue[Terran_Bunker] = vis(Terran_Bunker) + 1;
            }

            // Add comsats
            if (com(Terran_Academy) > 0) {
                buildQueue[Terran_Comsat_Station] = com(Terran_Command_Center);
            }
        }

        void queueSupply()
        {
            if (!inBookSupply) {
                if (vis(Terran_Supply_Depot) > 0) {
                    int count                       = min(25, s / 14) - (com(Terran_Command_Center) - 1);
                    buildQueue[Terran_Supply_Depot] = count;
                }
            }
        }

        void queueGeysers()
        {
            if (!inOpening) {
                gasDesired                  = ((Broodwar->self()->minerals() > 600 && Broodwar->self()->gas() < 200) || Resources::isMineralSaturated()) && com(Terran_SCV) >= 30;
                buildQueue[Terran_Refinery] = min(vis(Terran_Refinery) + gasDesired, Resources::getGasCount());
            }
        }

        void queueUpgradeStructures()
        {
            // Control Tower
            if (com(Terran_Starport) >= 1)
                buildQueue[Terran_Control_Tower] = com(Terran_Starport);

            // Machine Shop
            if (com(Terran_Factory) >= 3 && rampType == Terran_Factory)
                buildQueue[Terran_Machine_Shop] = Stations::getGasingStationsCount();

            // Academy
            if (Spy::enemyInvis()) {
                buildQueue[Terran_Engineering_Bay] = 1;
                buildQueue[Terran_Academy]         = 1;
                buildQueue[Terran_Comsat_Station]  = 2;
            }

            // Turrets
            if (needTurrets)
                buildQueue[Terran_Engineering_Bay] = 1;

            // If we're not in our opener
            if (!inOpening) {

                // Armory
                if (rampType == Terran_Factory)
                    buildQueue[Terran_Armory] = (s > 160) + (s > 200);

                if (com(Terran_Science_Facility) > 0 && isFocusUnit(Terran_Battlecruiser) && vis(Terran_Physics_Lab) == 0)
                    buildQueue[Terran_Physics_Lab] = 1;

                // Engineering Bay
                if ((rampType == Terran_Barracks && s > 200) || (Util::getTime() > Time(5, 00) && Players::TvZ()))
                    buildQueue[Terran_Engineering_Bay] = 1;
            }
        }

        void queueExpansions()
        {
            // If we're not in our opener
            if (!inOpening) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto incompleteProd    = vis(rampType) - com(rampType);
                const auto waitForMinerals   = 300 + (100 * incompleteProd);
                const auto resourceSat       = availableMinerals >= waitForMinerals && Resources::isMineralSaturated() && Resources::isGasSaturated();
                const auto excessResources   = availableMinerals >= waitForMinerals * 2;

                auto selfCount  = Stations::getStations(PlayerState::Self).size();
                auto enemyCount = Stations::getStations(PlayerState::Enemy).size();

                expandDesired = (resourceSat && techSat && productionSat && Stations::getMiningStationsCount() <= 5) //
                                || (excessResources && productionSat)                                                //
                                || (selfCount >= 4 && Stations::getMiningStationsCount() <= 2)                       //
                                || (selfCount >= 4 && Stations::getGasingStationsCount() <= 1)                       //
                                || (Stations::getMiningStationsCount() < 2 && Util::getTime() > Time(12, 00));

                buildQueue[Terran_Command_Center] = com(Terran_Command_Center) + expandDesired;
            }
        }

        void queueProduction()
        {
            // If we're not in our opener
            if (!inOpening) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();

                // Adding production
                // Mech play
                if (rampType == Terran_Factory) {
                    auto maxFacts     = 8;
                    auto factsPerBase = 3;
                    productionSat     = (vis(Terran_Factory) >= int(factsPerBase * vis(Terran_Command_Center)) || vis(Terran_Factory) >= maxFacts);
                    rampDesired       = !productionSat && ((focusUnit == None && availableMinerals >= 150 && (techSat || com(Terran_Command_Center) >= 3)) || availableMinerals >= 300);

                    if (rampDesired) {
                        auto factCount             = min({maxFacts, int(round(com(Terran_Command_Center) * factsPerBase)), vis(Terran_Factory) + 1});
                        buildQueue[Terran_Factory] = factCount;
                    }
                }

                // Bio play
                if (rampType == Terran_Barracks) {
                    auto maxRax     = 10;
                    auto raxPerBase = 2.5;
                    productionSat   = (vis(Terran_Barracks) >= int(raxPerBase * vis(Terran_Command_Center)) || vis(Terran_Command_Center) >= maxRax);
                    rampDesired     = !productionSat && ((focusUnit == None && availableMinerals >= 150 && (techSat || com(Terran_Command_Center) >= 3)) || availableMinerals >= 300);

                    if (rampDesired) {
                        auto raxCount               = min({maxRax, int(round(com(Terran_Command_Center) * raxPerBase)), vis(Terran_Barracks) + 1});
                        buildQueue[Terran_Barracks] = raxCount;
                    }
                }
            }
        }

        void calculateGasLimit()
        {
            // Gas limits
            if ((buildCount(Terran_Refinery) == 0 && com(Terran_SCV) <= 12) || com(Terran_SCV) <= 8)
                gasLimit = 0;
            else if (com(Terran_SCV) < 20)
                gasLimit = min(gasLimit, com(Terran_SCV) / 4);
            else if (!inOpening && com(Terran_SCV) >= 20)
                gasLimit = INT_MAX;
        }

        void removeExcessGas() {}

        void queueUpgrades()
        {
            using namespace UpgradeTypes;
            if (inOpening)
                return;

            // Speed upgrades
            upgradeQueue[Ion_Thrusters] = true;

            // Range upgrades
            upgradeQueue[Charon_Boosters] = vis(Terran_Goliath) > 0;
            upgradeQueue[U_238_Shells]    = Researching::haveOrResearching(TechTypes::Stim_Packs);

            // Mech unit upgrades
            auto upgradingMechAttack = (Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Plating)) ||
                                       Broodwar->self()->isUpgrading(Terran_Vehicle_Weapons);
            auto TvZMechAttack = Players::TvZ() && rampType == Terran_Factory;
            auto TvTMechAttack = Players::TvT();
            auto TvPMechAttack = Players::TvP();

            if (vis(Terran_Armory) > 0) {
                upgradeQueue[Terran_Vehicle_Weapons] = TvZMechAttack || TvTMechAttack || TvPMechAttack;
                upgradeQueue[Terran_Vehicle_Plating] = upgradingMechAttack;
            }

            // Want 3x upgrades by default
            upgradeQueue[Terran_Vehicle_Weapons] *= 3;
            upgradeQueue[Terran_Vehicle_Plating] *= 3;

            // Bio unit upgrades
            auto upgradingBioAttack = (Broodwar->self()->getUpgradeLevel(Terran_Infantry_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Infantry_Armor)) ||
                                      Broodwar->self()->isUpgrading(Terran_Infantry_Weapons);
            auto TvZBioAttack = Players::TvZ() && rampType == Terran_Barracks;
            auto TvTBioAttack = false;
            auto TvPBioAttack = false;

            if (vis(Terran_Engineering_Bay) > 0) {
                upgradeQueue[Terran_Infantry_Weapons] = TvZBioAttack || TvTBioAttack || TvPBioAttack;
                upgradeQueue[Terran_Infantry_Armor]   = upgradingBioAttack;
            }

            // Want 3x upgrades by default
            upgradeQueue[Terran_Infantry_Weapons] *= 3;
            upgradeQueue[Terran_Infantry_Armor] *= 3;
        }

        void queueResearch()
        {
            using namespace TechTypes;
            if (inOpening)
                return;

            techQueue[Stim_Packs]         = Players::TvZ();
            techQueue[Spider_Mines]       = true;
            techQueue[Tank_Siege_Mode]    = Researching::haveOrResearching(Spider_Mines);
            techQueue[Cloaking_Field]     = isFocusUnit(Terran_Wraith);
            techQueue[Yamato_Gun]         = isFocusUnit(Terran_Battlecruiser);
            techQueue[Personnel_Cloaking] = isFocusUnit(Terran_Ghost);
        }
    } // namespace

    void opener()
    {
        if (Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0 && !Players::ZvFFA() && !Players::ZvTVB())
            againstRandom = true;

        // TODO: Team melee / Team FFA support
        if (Broodwar->getGameType() == GameTypes::Team_Free_For_All || Broodwar->getGameType() == GameTypes::Team_Melee) {
            TvA();
            return;
        }

        terranUnitPump.clear();
        if (Players::TvP() || Players::TvFFA() || Players::TvTVB())
            TvP();
        else if (Players::TvT())
            TvT();
        else if (Players::TvZ())
            TvZ();
    }

    void tech()
    {
        getTech = false;
        if (inOpening)
            return;

        auto techOffset = 0;

        // TvZ
        if (Players::TvZ()) {
            techOffset = 0;
            unitOrder  = {Terran_Medic, Terran_Science_Vessel};
        }

        // TvP
        if (Players::TvP()) {
            techOffset = 1;
            unitOrder  = {Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Science_Vessel};
        }

        // TvT
        if (Players::TvT()) {
            techOffset = 1;
            unitOrder  = {Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Science_Vessel};
        }

        // TvFFA
        if (Players::TvFFA()) {
            techOffset = 1;
            unitOrder  = {Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Science_Vessel};
        }

        // Ensure anything we already made is added into the list
        for (auto unit : unitOrder) {
            if (unlockReady(unit))
                focusUnits.insert(unit);
        }

        // Adding tech
        const auto endOfTech   = !unitOrder.empty() && isFocusUnit(unitOrder.back());
        const auto techVal     = int(focusUnits.size()) + techOffset;
        const auto readyToTech = (vis(Terran_Refinery) > 0 || int(Stations::getStations(PlayerState::Self).size()) >= 3 || focusUnits.empty()) && vis(Terran_SCV) >= 20;
        techSat                = (techVal >= int(Stations::getStations(PlayerState::Self).size()) || endOfTech);

        getTech = readyToTech && !techSat && productionSat;
        getNewTech();
        getTechBuildings();
    }

    void situational()
    {
        // Queue up defenses
        queueDefenses();

        // Queue up supply, upgrade structures
        queueSupply();
        queueUpgradeStructures();
        queueGeysers();

        // Outside of opening book, book no longer is in control of queuing production, expansions or gas limits
        queueExpansions();
        queueProduction();
        calculateGasLimit();

        // Optimize our gas mining by dropping gas mining at specific excessive values
        removeExcessGas();

        // Queue upgrades/research
        queueUpgrades();
        queueResearch();
    }

    void composition()
    {
        armyComposition.clear();
        auto availGas           = Broodwar->self()->gas() - (Upgrading::getReservedGas() + Researching::getReservedGas() + Planning::getPlannedGas());
        const auto withoutAddon = {Terran_Goliath, Terran_Vulture, Terran_Wraith};

        const auto buildingAvailable = [&](auto &type) {
            auto needAddon = type.whatBuilds().first.canBuildAddon() && find(withoutAddon.begin(), withoutAddon.end(), type) == withoutAddon.end();

            auto building = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self, [&](auto &u) {
                return u->getType() == type.whatBuilds().first && u->isCompleted() && (!needAddon || u->unit()->getAddon()) && u->getRemainingTrainFrames() < 10;
            });
            return building;
        };

        if (inOpening) {
            if (terranUnitPump[Terran_SCV])
                armyComposition[Terran_SCV] = 1.00;

            const auto buildings = {Terran_Barracks, Terran_Factory, Terran_Starport};

            for (auto &building : buildings) {
                vector<UnitType> sortedByGas = {building.buildsWhat().begin(), building.buildsWhat().end()};
                sort(sortedByGas.begin(), sortedByGas.end(), [&](auto &lhs, auto &rhs) { return lhs.gasPrice() >= rhs.gasPrice(); });

                for (auto &type : sortedByGas) {
                    if (!terranUnitPump[type] || availGas < type.gasPrice() || !buildingAvailable(type))
                        continue;
                    armyComposition[type] = 1.00;
                    break;
                }
            }
        }

        if (!inOpening) {
            static vector<pair<UnitType, int>> priorityOrder;

            if (rampType == Terran_Factory) {
                priorityOrder = {
                    {Terran_SCV, 60},                                                                   // CC
                    {Terran_Vulture, 2},        {Terran_Goliath, 1}, {Terran_Siege_Tank_Tank_Mode, 2},  // Factory
                    {Terran_Vulture, 8},        {Terran_Goliath, 2}, {Terran_Siege_Tank_Tank_Mode, 4},  //
                    {Terran_Vulture, 16},       {Terran_Goliath, 4}, {Terran_Siege_Tank_Tank_Mode, 8},  //
                    {Terran_Vulture, 24},       {Terran_Goliath, 4}, {Terran_Siege_Tank_Tank_Mode, 16}, //
                    {Terran_Vulture, 32},       {Terran_Goliath, 6}, {Terran_Siege_Tank_Tank_Mode, 32}, //
                    {Terran_Science_Vessel, 3},                                                         // Starport
                };

                // Swap tank/goliath count vs carriers
                if (Players::getTotalCount(PlayerState::Enemy, Protoss_Carrier) > 0) {
                    for (auto &[type, cnt] : priorityOrder) {
                        if (type == Terran_Siege_Tank_Tank_Mode) {
                            type = Terran_Goliath;
                        }
                        else if (type == Terran_Goliath) {
                            type = Terran_Siege_Tank_Tank_Mode;
                        }
                    }
                }
            }

            if (rampType == Terran_Barracks) {
                priorityOrder = {
                    {Terran_SCV, 60},                              // CC
                    {Terran_Marine, 5},        {Terran_Medic, 2},  // Rax
                    {Terran_Marine, 20},       {Terran_Medic, 6},  //
                    {Terran_Marine, 40},       {Terran_Medic, 8},  //
                    {Terran_Marine, 80},       {Terran_Medic, 12}, //
                    {Terran_Science_Vessel, 6}                     // Starport
                };
            }

            for (auto &[type, count] : priorityOrder) {
                auto typeAvailable = (unlockReady(type) && vis(type) < count);
                if (type.isWorker() && Resources::isMineralSaturated() && Resources::isGasSaturated())
                    continue;
                if (!typeAvailable || availGas < type.gasPrice() || !buildingAvailable(type))
                    continue;
                armyComposition[type] = 1.00;
                break;
            }
        }
    }

    void unlocks()
    {
        // Unlocking units
        unlockedType.clear();
        for (auto &[type, per] : armyComposition) {
            if (per > 0.0)
                unlockedType.insert(type);
        }
    }
} // namespace McRave::BuildOrder::Terran