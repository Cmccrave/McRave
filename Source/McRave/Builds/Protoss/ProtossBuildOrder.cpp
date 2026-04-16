#include "ProtossBuildOrder.h"

#include "Info/Player/Players.h"
#include "Info/Resource/Resources.h"
#include "Macro/Planning/Planning.h"
#include "Macro/Researching/Researching.h"
#include "Macro/Upgrading/Upgrading.h"
#include "Main/Common.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Map/Walls/Walls.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder::Protoss {
    namespace {
        bool againstRandom = false;

        void queueDefenses()
        {
            // Adding Wall Defenses
            if (Walls::getNaturalWall()) {
                if (vis(Protoss_Forge) > 0 && (Walls::needAirDefenses(*Walls::getNaturalWall()) > 0 || Walls::needGroundDefenses(*Walls::getNaturalWall()) > 0))
                    buildQueue[Protoss_Photon_Cannon] = vis(Protoss_Photon_Cannon) + 1;
            }

            // Adding Station Defenses
            if (int(Stations::getStations(PlayerState::Self).size()) >= 2) {
                for (auto &station : Stations::getStations(PlayerState::Self)) {
                    if (vis(Protoss_Forge) > 0 && (Stations::needGroundDefenses(station) > 0 || Stations::needAirDefenses(station) > 0))
                        buildQueue[Protoss_Photon_Cannon] = vis(Protoss_Photon_Cannon) + 1;
                }
            }
        }

        void queueSupply()
        {
            if (!inBookSupply) {
                int count                 = min(22, s / 14) - (com(Protoss_Nexus) - 1);
                buildQueue[Protoss_Pylon] = count;

                for (auto &station : Stations::getStations(PlayerState::Self)) {
                    if (vis(Protoss_Pylon) > 10 && Stations::needPower(station)) {
                        buildQueue[Protoss_Pylon]++;
                        break;
                    }
                }
            }
        }

        void queueGeysers()
        {
            if (!inOpening) {
                auto minProbesAllGas = 48;
                auto minProbesPerGas = 24 + (4 * vis(Protoss_Assimilator));
                auto takeAllGeysers  = com(Protoss_Probe) >= minProbesAllGas;
                auto allowNewGeyser  = com(Protoss_Probe) >= minProbesPerGas && (Resources::isMineralSaturated() || productionSat || takeAllGeysers);
                auto needGeyser      = gasLimit > vis(Protoss_Assimilator) * 3;
                gasDesired           = allowNewGeyser && needGeyser;

                buildQueue[Protoss_Assimilator] = min(vis(Protoss_Assimilator) + gasDesired, Resources::getGasCount());
            }
        }

        void queueUpgradeStructures()
        {
            auto forgeDetectionBuilds = {P_4Gate, P_DT};
            if (Spy::enemyInvis() && Util::contains(forgeDetectionBuilds, currentTransition)) {
                buildQueue[Protoss_Forge]         = 1;
                buildQueue[Protoss_Photon_Cannon] = com(Protoss_Forge) * 2;
            }

            if (inOpening)
                return;

            // Adding upgrade buildings
            if (com(Protoss_Assimilator) >= 3) {
                auto forgeCount = com(Protoss_Assimilator) >= 4 ? 2 - (int)Terrain::isIslandMap() : 1;
                auto coreCount  = com(Protoss_Assimilator) >= 4 ? 1 + (int)Terrain::isIslandMap() : 1;

                buildQueue[Protoss_Cybernetics_Core] = 1 + (int)Terrain::isIslandMap();
                buildQueue[Protoss_Forge]            = 2 - (int)Terrain::isIslandMap();
            }

            // Add robo tech if we have the opposite one
            if (com(Protoss_Robotics_Facility) > 0 && (com(Protoss_Observatory) > 0 || com(Protoss_Robotics_Support_Bay) > 0)) {
                buildQueue[Protoss_Observatory]          = total(Protoss_Reaver) > 0;
                buildQueue[Protoss_Robotics_Support_Bay] = total(Protoss_Observer) > 0;
            }

            // Important to have a forge vs Z
            if (com(Protoss_Nexus) >= 2 && Players::vZ())
                buildQueue[Protoss_Forge] = 1;

            // Ensure we build a core outside our opening book
            if (com(Protoss_Gateway) >= 2)
                buildQueue[Protoss_Cybernetics_Core] = 1;

            // Corsair/Scout upgrades
            if (s >= 300 && (isFocusUnit(Protoss_Scout) || isFocusUnit(Protoss_Corsair)))
                buildQueue[Protoss_Fleet_Beacon] = 1;
        }

        void queueExpansions()
        {
            // Against FFE add a Nexus
            if (Spy::getEnemyBuild() == P_FFE && Broodwar->getFrameCount() < 15000) {
                auto cannonCount = Players::getVisibleCount(PlayerState::Enemy, Protoss_Photon_Cannon);
                wantNatural      = true;

                if (cannonCount < 6) {
                    buildQueue[Protoss_Nexus]       = 2;
                    buildQueue[Protoss_Assimilator] = (vis(Protoss_Nexus) >= 2) + (s >= 120);
                    gasLimit                        = vis(Protoss_Nexus) != buildCount(Protoss_Nexus) ? 0 : INT_MAX;
                }
                else {
                    buildQueue[Protoss_Nexus]       = 3;
                    buildQueue[Protoss_Assimilator] = (vis(Protoss_Nexus) >= 2) + (s >= 120);
                    gasLimit                        = vis(Protoss_Nexus) != buildCount(Protoss_Nexus) ? 0 : INT_MAX;
                }
            }

            // If we're not in our opener
            if (!inOpening) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto incompleteGate    = vis(Protoss_Gateway) - com(Protoss_Gateway);
                const auto waitForMinerals   = 300 + (100 * incompleteGate);
                const auto resourceSat       = availableMinerals >= waitForMinerals && Resources::isMineralSaturated() && Resources::isGasSaturated();
                const auto excessResources   = availableMinerals >= waitForMinerals * 2;

                auto selfCount  = Stations::getStations(PlayerState::Self).size();
                auto enemyCount = Stations::getStations(PlayerState::Enemy).size();

                expandDesired = (resourceSat && techSat && productionSat && Stations::getMiningStationsCount() <= 5) //
                                || (excessResources && productionSat)                                                //
                                || (selfCount >= 4 && Stations::getMiningStationsCount() <= 2)                       //
                                || (selfCount >= 4 && Stations::getGasingStationsCount() <= 1)                       //
                                || (Stations::getMiningStationsCount() < 2 && Util::getTime() > Time(12, 00));

                buildQueue[Protoss_Nexus] = com(Protoss_Nexus) + expandDesired;
            }
        }

        void queueProduction()
        {
            // If we're not in our opener
            if (!inOpening) {
                const auto availableMinerals = Broodwar->self()->minerals() - BuildOrder::getMinQueued();
                const auto incompleteGate    = vis(Protoss_Gateway) - com(Protoss_Gateway);
                const auto waitForMinerals   = 150 + (75 * incompleteGate);
                auto maxGates                = Players::vT() ? 10 : 8;
                auto gatesPerBase            = 3.0;

                productionSat = (vis(Protoss_Gateway) >= int(gatesPerBase * Stations::getGasingStationsCount())) || vis(Protoss_Gateway) >= maxGates || Planning::isUnplannable(Protoss_Gateway) ||
                                Planning::isUnplannable(Protoss_Stargate);

                // Adding production
                const auto resourceSat     = (availableMinerals >= waitForMinerals && Resources::isMineralSaturated() && Resources::isGasSaturated());
                const auto excessResources = (availableMinerals >= waitForMinerals * 2);

                if (!productionSat) {
                    rampDesired                  = resourceSat || excessResources;
                    auto gateCount               = min({maxGates, int(round(Stations::getGasingStationsCount() * gatesPerBase)), vis(Protoss_Gateway) + rampDesired});
                    auto stargateCount           = min({4, int(isFocusUnit(Protoss_Carrier) || isFocusUnit(Protoss_Scout)) * Stations::getGasingStationsCount(), vis(Protoss_Stargate) + rampDesired});
                    buildQueue[Protoss_Gateway]  = gateCount;
                    buildQueue[Protoss_Stargate] = stargateCount;
                }
            }
        }

        void calculateGasLimit()
        {
            // Gas limits
            if ((buildCount(Protoss_Assimilator) == 0 && com(Protoss_Probe) <= 12) || com(Protoss_Probe) <= 8)
                gasLimit = 0;
            else if (com(Protoss_Probe) < 20)
                gasLimit = min(gasLimit, com(Protoss_Probe) / 4);
            else if (!inOpening && com(Protoss_Probe) >= 20)
                gasLimit = gasMax();
        }

        void removeExcessGas() {}

        void queueUpgrades()
        {
            using namespace UpgradeTypes;
            if (inOpening)
                return;

            // Speed upgrades
            upgradeQueue[Gravitic_Drive]     = vis(Protoss_Shuttle) > 0 && (vis(Protoss_High_Templar) > 0 || com(Protoss_Reaver) >= 2);
            upgradeQueue[Gravitic_Thrusters] = BuildOrder::isFocusUnit(Protoss_Scout);
            upgradeQueue[Gravitic_Boosters]  = (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
            upgradeQueue[Leg_Enhancements]   = (vis(Protoss_Nexus) >= 3) || Players::PvZ();

            // Range upgrades
            upgradeQueue[Singularity_Charge] = vis(Protoss_Dragoon) >= 1;

            // Energy upgrades
            upgradeQueue[Khaydarin_Amulet] = (vis(Protoss_Assimilator) >= 4 && Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) && Broodwar->self()->gas() >= 750);
            upgradeQueue[Khaydarin_Core]   = true;

            // Sight upgrades
            upgradeQueue[Apial_Sensors] = (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
            upgradeQueue[Sensor_Array]  = (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);

            // Capacity upgrades
            upgradeQueue[Carrier_Capacity] = vis(Protoss_Carrier) >= 2;
            upgradeQueue[Reaver_Capacity]  = (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
            upgradeQueue[Scarab_Damage]    = (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);

            // Ground unit upgrades
            auto upgradingGrdWeapon = (Broodwar->self()->getUpgradeLevel(Protoss_Ground_Weapons) > Broodwar->self()->getUpgradeLevel(Protoss_Ground_Armor)) ||
                                      Broodwar->self()->isUpgrading(Protoss_Ground_Weapons);
            upgradeQueue[Protoss_Ground_Weapons] = int(Stations::getStations(PlayerState::Self).size()) >= 2;
            upgradeQueue[Protoss_Ground_Armor]   = upgradingGrdWeapon;
            upgradeQueue[Protoss_Plasma_Shields] = Upgrading::haveOrUpgrading(Protoss_Ground_Weapons, 3) && Upgrading::haveOrUpgrading(Protoss_Ground_Armor, 3);

            // Want 3x upgrades by default
            upgradeQueue[Protoss_Ground_Weapons] *= 3;
            upgradeQueue[Protoss_Ground_Armor] *= 3;
            upgradeQueue[Protoss_Plasma_Shields] *= 3;

            // Air unit upgrades
            auto upgradingAirAttack = (Broodwar->self()->getUpgradeLevel(Protoss_Air_Weapons) > Broodwar->self()->getUpgradeLevel(Protoss_Air_Armor)) ||
                                      Broodwar->self()->isUpgrading(Protoss_Air_Weapons);
            auto PvZAirAttack = Players::PvZ() && Players::getTotalCount(PlayerState::Enemy, Protoss_Corsair) > 0;
            auto PvTAirAttack = Players::PvT() && Players::getTotalCount(PlayerState::Enemy, Protoss_Carrier) > 0;
            auto PvPAirAttack = false;

            if (vis(Protoss_Stargate) > 0) {
                upgradeQueue[Protoss_Air_Weapons] = PvZAirAttack || PvTAirAttack || PvPAirAttack;
                upgradeQueue[Protoss_Air_Armor]   = upgradingAirAttack;
            }

            // Want 3x upgrades by default
            upgradeQueue[Protoss_Air_Weapons] *= 3;
            upgradeQueue[Protoss_Air_Armor] *= 3;
        }

        void queueResearch()
        {
            using namespace TechTypes;
            if (inOpening)
                return;

            techQueue[Psionic_Storm]  = isFocusUnit(Protoss_High_Templar);
            techQueue[Stasis_Field]   = Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core) > 0;
            techQueue[Disruption_Web] = vis(Protoss_Corsair) >= 10;
        }
    } // namespace

    bool goonRange() { return Upgrading::haveOrUpgrading(UpgradeTypes::Singularity_Charge, 1); }

    void opener()
    {
        if (Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0 && !Players::PvFFA() && !Players::PvTVB())
            againstRandom = true;

        protossUnitPump.clear();
        if (Players::PvT())
            PvT();
        else if (Players::PvP())
            PvP();
        else if (Players::PvZ() || againstRandom)
            PvZ();
        else if (Players::PvFFA() || Players::PvTVB())
            PvFFA();
    }

    void tech()
    {
        getTech = false;
        if (inOpening)
            return;

        auto techOffset = 0;

        // PvP
        if (Players::PvP()) {
            techOffset = 1 + (currentTransition == P_4Gate);
            if (focusUnit == Protoss_Dark_Templar)
                unitOrder = {Protoss_High_Templar, Protoss_Observer};
            else
                unitOrder = {Protoss_Reaver, Protoss_High_Templar};
        }

        // PvZ
        if (Players::PvZ()) {
            techOffset = 1 + (currentTransition == P_4Gate);
            if (focusUnit == Protoss_Reaver)
                unitOrder = {Protoss_Corsair, Protoss_High_Templar};
            else if (focusUnit == Protoss_Corsair)
                unitOrder = {Protoss_High_Templar, Protoss_Reaver};
            else if (focusUnit == Protoss_High_Templar)
                unitOrder = {Protoss_Corsair, Protoss_Reaver};
            else if (focusUnit == Protoss_Scout)
                unitOrder = {Protoss_Scout};
            else
                unitOrder = {Protoss_Corsair, Protoss_Observer, Protoss_High_Templar};
        }

        // PvT
        if (Players::PvT()) {
            techOffset = 1 + (currentTransition == P_4Gate);
            if (focusUnit == Protoss_Dark_Templar)
                unitOrder = {Protoss_Arbiter, Protoss_Observer, Protoss_High_Templar};
            else if (focusUnit == Protoss_Carrier)
                unitOrder = {Protoss_Observer, Protoss_High_Templar};
            else
                unitOrder = {Protoss_Observer, Protoss_Arbiter, Protoss_High_Templar};
        }

        // PvFFA
        if (Players::PvFFA()) {
            unitOrder = {Protoss_Observer, Protoss_Reaver, Protoss_Carrier};
        }

        // Ensure anything we already made is added into the list
        for (auto unit : unitOrder) {
            if (unlockReady(unit))
                focusUnits.insert(unit);
        }

        // Adding tech
        const auto endOfTech   = !unitOrder.empty() && isFocusUnit(unitOrder.back());
        const auto techVal     = int(focusUnits.size()) + techOffset + mineralThird;
        const auto readyToTech = (vis(Protoss_Assimilator) > 0 || int(Stations::getStations(PlayerState::Self).size()) >= 3 || focusUnits.empty()) && vis(Protoss_Probe) >= 20;
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
        // PvP
        // Remove zealot if no speed
        //

        // PvZ
        // Remove dragoons if no range
        //

        // PvT
        // Remove zealot if no speed
        //

        armyComposition.clear();
        auto availGas = Broodwar->self()->gas() - (Upgrading::getReservedGas() + Researching::getReservedGas() + Planning::getPlannedGas());

        const auto buildingAvailable = [&](auto &type) {
            auto building = Util::getClosestUnit(Terrain::getMainPosition(), PlayerState::Self,
                                                 [&](auto &u) { return u->getType() == type.whatBuilds().first && u->isCompleted() && u->unit()->isPowered() && u->getRemainingTrainFrames() < 10; });
            return building;
        };

        if (inOpening) {
            if (protossUnitPump[Protoss_Probe])
                armyComposition[Protoss_Probe] = 1.00;

            const auto buildings = {Protoss_Gateway, Protoss_Robotics_Facility, Protoss_Stargate};
            for (auto &building : buildings) {
                vector<UnitType> sortedByGas = {building.buildsWhat().begin(), building.buildsWhat().end()};
                sort(sortedByGas.begin(), sortedByGas.end(), [&](auto &lhs, auto &rhs) { return lhs.gasPrice() >= rhs.gasPrice(); });

                for (auto &type : sortedByGas) {
                    if (!protossUnitPump[type] || !unlockReady(type) || availGas < type.gasPrice() || !buildingAvailable(type))
                        continue;

                    armyComposition[type] = 1.00;
                    break;
                }
            }
        }

        if (!inOpening) {
            static vector<pair<UnitType, int>> priorityOrder;
            static vector<pair<UnitType, int>> nexusOrder;
            static vector<pair<UnitType, int>> gateOrder;
            static vector<pair<UnitType, int>> roboOrder;
            static vector<pair<UnitType, int>> stargateOrder;

            nexusOrder = {{Protoss_Probe, 60}};

            // PvP
            if (Players::PvP() || Players::PvTVB() || Players::PvFFA()) {
                priorityOrder = {
                    {Protoss_Probe, 60},                                                                              // Nexus
                    {Protoss_Observer, 1},     {Protoss_Reaver, 1},       {Protoss_Shuttle, 1},                       // Robo
                    {Protoss_Observer, 2},     {Protoss_Reaver, 4},       {Protoss_Shuttle, 2},                       //
                    {Protoss_Observer, 3},     {Protoss_Reaver, 8},       {Protoss_Shuttle, 4},                       //
                    {Protoss_Dark_Templar, 1}, {Protoss_High_Templar, 1}, {Protoss_Dragoon, 24}, {Protoss_Zealot, 4}, // Gateway
                    {Protoss_High_Templar, 2}, {Protoss_Dragoon, 36},     {Protoss_Zealot, 12},                       //
                    {Protoss_High_Templar, 4}, {Protoss_Dragoon, 64},                                                 //
                };
            }

            // PvT
            if (Players::PvT()) {
                priorityOrder = {
                    {Protoss_Probe, 60},                                                                              // Nexus
                    {Protoss_Carrier, 12},     {Protoss_Arbiter, 4},                                                  // Stargate
                    {Protoss_Observer, 1},     {Protoss_Reaver, 1},       {Protoss_Shuttle, 1},                       // Robo
                    {Protoss_Observer, 2},     {Protoss_Reaver, 4},       {Protoss_Shuttle, 2},                       //
                    {Protoss_Observer, 3},     {Protoss_Reaver, 8},       {Protoss_Shuttle, 4},                       //
                    {Protoss_Dark_Templar, 1}, {Protoss_High_Templar, 1}, {Protoss_Dragoon, 12}, {Protoss_Zealot, 6}, // Gateway
                    {Protoss_High_Templar, 2}, {Protoss_Dragoon, 24},     {Protoss_Zealot, 12},                       //
                    {Protoss_High_Templar, 4}, {Protoss_Dragoon, 36},     {Protoss_Zealot, 18},                       //
                    {Protoss_Dragoon, 64}                                                                             //
                };
            }

            // PvZ
            if (Players::PvZ()) {
                priorityOrder = {
                    {Protoss_Probe, 60},                                                                               // Nexus
                    {Protoss_Corsair, 12},                                                                             // Stargate
                    {Protoss_Observer, 1},     {Protoss_Reaver, 1},       {Protoss_Shuttle, 1},                        // Robo
                    {Protoss_Observer, 2},     {Protoss_Reaver, 4},       {Protoss_Shuttle, 2},                        //
                    {Protoss_Observer, 3},     {Protoss_Reaver, 8},       {Protoss_Shuttle, 4},                        //
                    {Protoss_Dark_Templar, 1}, {Protoss_High_Templar, 1}, {Protoss_Dragoon, 12}, {Protoss_Zealot, 12}, // Gateway
                    {Protoss_High_Templar, 3}, {Protoss_Dragoon, 24},     {Protoss_Zealot, 24},                        //
                    {Protoss_High_Templar, 6}, {Protoss_Dragoon, 36},     {Protoss_Zealot, 36},                        //
                    {Protoss_Zealot, 64}                                                                               //
                };
            }

            // Remove Shuttles if no HT/Reaver
            if (vis(Protoss_High_Templar) == 0 && vis(Protoss_Reaver) == 0) {
                for (auto &[type, cnt] : priorityOrder) {
                    if (type == Protoss_Shuttle)
                        cnt = 0;
                }
            }

            // Remove HT if no storm
            if (!Researching::haveOrResearching(TechTypes::Psionic_Storm)) {
                for (auto &[type, cnt] : priorityOrder) {
                    if (type == Protoss_High_Templar)
                        cnt = 0;
                }
            }

            // Remove Dragoon if no range
            if (!Upgrading::haveOrUpgrading(UpgradeTypes::Singularity_Charge, 1)) {
                for (auto &[type, cnt] : priorityOrder) {
                    if (type == Protoss_Dragoon)
                        cnt = 0;
                }
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
} // namespace McRave::BuildOrder::Protoss