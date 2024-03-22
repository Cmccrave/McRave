#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

#include "TerranBuildOrder.h"

namespace McRave::BuildOrder::Terran {

    namespace {
        bool againstRandom = false;
        bool needTurrets = false;

        void queueWallDefenses()
        {

        }

        void queueStationDefenses()
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
        }

        void queueSupply()
        {
            if (!inBookSupply) {
                if (vis(Terran_Supply_Depot) > 0) {
                    int count = min(22, s / 14) - (com(Terran_Command_Center) - 1);
                    buildQueue[Terran_Supply_Depot] = count;
                }
            }
        }

        void queueGeysers()
        {
            if (!inOpening) {
                gasDesired = ((Broodwar->self()->minerals() > 600 && Broodwar->self()->gas() < 200) || Resources::isMineralSaturated()) && com(Terran_SCV) >= 30;
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
                if (desiredDetection == Terran_Missile_Turret)
                    buildQueue[Terran_Engineering_Bay] = 1;
                else {
                    buildQueue[Terran_Academy] = 1;
                    buildQueue[Terran_Comsat_Station] = 2;
                }
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
                expandDesired = (focusUnit == None && Resources::isGasSaturated() && (Resources::isMineralSaturated() || com(Terran_Command_Center) >= 3) && (techSat || com(Terran_Command_Center) >= 3) && productionSat)
                    || (com(Terran_Command_Center) >= 2 && availableMinerals >= 800 && (Resources::isMineralSaturated() || Resources::isGasSaturated()))
                    || (Stations::getStations(PlayerState::Self).size() >= 4 && Stations::getMiningStationsCount() <= 2)
                    || (Stations::getStations(PlayerState::Self).size() >= 4 && Stations::getGasingStationsCount() <= 1);

                buildQueue[Terran_Command_Center] = min(2, vis(Terran_Command_Center) + expandDesired);
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
                    auto maxFacts = 8;
                    auto factsPerBase = 3;
                    productionSat = (vis(Terran_Factory) >= int(factsPerBase * vis(Terran_Command_Center)) || vis(Terran_Command_Center) >= maxFacts);
                    rampDesired = !productionSat && ((focusUnit == None && availableMinerals >= 150 && (techSat || com(Terran_Command_Center) >= 3)) || availableMinerals >= 300);

                    if (rampDesired) {
                        auto factCount = min({ maxFacts, int(round(com(Terran_Command_Center) * factsPerBase)), vis(Terran_Factory) + 1 });
                        buildQueue[Terran_Factory] = factCount;
                    }
                }

                // Bio play
                if (rampType == Terran_Barracks) {
                    auto maxRax = 10;
                    auto raxPerBase = 3;
                    productionSat = (vis(Terran_Barracks) >= int(raxPerBase * vis(Terran_Command_Center)) || vis(Terran_Command_Center) >= maxRax);
                    rampDesired = !productionSat && ((focusUnit == None && availableMinerals >= 150 && (techSat || com(Terran_Command_Center) >= 3)) || availableMinerals >= 300);

                    if (rampDesired) {
                        auto raxCount = min({ maxRax, int(round(com(Terran_Command_Center) * raxPerBase)), vis(Terran_Barracks) + 1 });
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

        void removeExcessGas()
        {

        }

        void queueUpgrades()
        {
            using namespace UpgradeTypes;
            if (inOpening)
                return;

            // Speed upgrades
            upgradeQueue[Ion_Thrusters] = true;

            // Range upgrades
            upgradeQueue[Charon_Boosters] = vis(Terran_Goliath) > 0;
            upgradeQueue[U_238_Shells] = Researching::haveOrResearching(TechTypes::Stim_Packs);

            // Mech unit upgrades
            auto upgradingMechAttack = (Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Plating)) || Broodwar->self()->isUpgrading(Terran_Vehicle_Weapons);
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
            auto upgradingBioAttack = (Broodwar->self()->getUpgradeLevel(Terran_Infantry_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Infantry_Armor)) || Broodwar->self()->isUpgrading(Terran_Infantry_Weapons);
            auto TvZBioAttack = Players::TvZ() && rampType == Terran_Barracks;
            auto TvTBioAttack = false;
            auto TvPBioAttack = false;

            if (vis(Terran_Engineering_Bay) > 0) {
                upgradeQueue[Terran_Infantry_Weapons] = TvZBioAttack || TvTBioAttack || TvPBioAttack;
                upgradeQueue[Terran_Infantry_Armor] = upgradingBioAttack;
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

            techQueue[Stim_Packs] = Players::TvZ();
            techQueue[Spider_Mines] = true;
            techQueue[Tank_Siege_Mode] = Researching::haveOrResearching(Spider_Mines);
            techQueue[Cloaking_Field] = isFocusUnit(Terran_Wraith);
            techQueue[Yamato_Gun] = isFocusUnit(Terran_Battlecruiser);
            techQueue[Personnel_Cloaking] = isFocusUnit(Terran_Ghost);
        }
    }

    void opener()
    {
        if (Players::getRaceCount(Races::Unknown, PlayerState::Enemy) > 0 && !Players::ZvFFA() && !Players::ZvTVB())
            againstRandom = true;

        // TODO: Team melee / Team FFA support
        if (Broodwar->getGameType() == GameTypes::Team_Free_For_All || Broodwar->getGameType() == GameTypes::Team_Melee) {
            TvA();
            return;
        }

        if (Players::TvZ())
            TvZ();
        else
            TvP();
    }

    void tech()
    {
        auto techVal = int(focusUnits.size()) + 1;
        techSat = (techVal > com(Terran_Command_Center));
        unitOrder ={ Terran_Vulture, Terran_Siege_Tank_Tank_Mode };

        if (techComplete())
            focusUnit = None;
        if (!inOpening && !getTech && !techSat && focusUnit == None)
            getTech = true;

        if (Players::TvZ())
            unitOrder ={ Terran_Medic, Terran_Science_Vessel, Terran_Siege_Tank_Tank_Mode };
        if (Players::TvP())
            unitOrder ={ Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Science_Vessel };
        if (Players::TvT())
            unitOrder ={ Terran_Vulture, Terran_Siege_Tank_Tank_Mode, Terran_Science_Vessel };

        getNewTech();
        getTechBuildings();
    }

    void situational()
    {
        // Queue up defenses
        queueWallDefenses();
        queueStationDefenses();

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
        // Clear composition before setting
        if (!inOpening)
            armyComposition.clear();

        if (Players::TvZ() && !inOpening) {

            if (isFocusUnit(Terran_Science_Vessel)) {
                armyComposition[Terran_Marine] = 0.80;
                armyComposition[Terran_Medic] = 0.20;
                armyComposition[Terran_Science_Vessel] = 1.00;
                armyComposition[Terran_SCV] = 1.00;
            }
            else if (isFocusUnit(Terran_Medic)) {
                armyComposition[Terran_Marine] = 0.80;
                armyComposition[Terran_Medic] = 0.20;
                armyComposition[Terran_SCV] = 1.00;
            }
        }

        if ((Players::TvT() || Players::TvP()) && !inOpening) {
            armyComposition[Terran_Vulture] =                   0.50;
            armyComposition[Terran_Siege_Tank_Tank_Mode] =      0.40;
            armyComposition[Terran_Goliath] =                   0.10;
            armyComposition[Terran_SCV] =                       1.00;
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

        // Unit limiting in opening book
        if (inOpening) {
            for (auto &type : unitLimits) {
                if (type.second > vis(type.first))
                    unlockedType.insert(type.first);
                else
                    unlockedType.erase(type.first);
            }
        }

        // UMS Unlocking
        if (Broodwar->getGameType() == GameTypes::Use_Map_Settings) {
            for (auto &type : BWAPI::UnitTypes::allUnitTypes()) {
                if (!type.isBuilding() && type.getRace() == Races::Terran && vis(type) >= 2) {
                    unlockedType.insert(type);
                    if (!type.isWorker())
                        focusUnits.insert(type);
                }
            }
        }
    }
}