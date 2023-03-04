#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Upgrading {

    namespace {
        map <Unit, UpgradeType> idleUpgrade;
        int reservedMineral, reservedGas;
        int lastUpgradeFrame = -999;
    
        void reset()
        {
            reservedMineral = 0;
            reservedGas = 0;
        }

        bool isAffordable(UpgradeType upgrade)
        {
            auto mineralCost        = upgrade.mineralPrice() + (upgrade.mineralPriceFactor() * Broodwar->self()->getUpgradeLevel(upgrade));
            auto gasCost            = upgrade.gasPrice() + (upgrade.gasPriceFactor() * Broodwar->self()->getUpgradeLevel(upgrade));

            return Broodwar->self()->minerals() >= mineralCost && Broodwar->self()->gas() >= gasCost;
        }

        bool isCreateable(Unit building, UpgradeType upgrade)
        {
            auto geyserType = Broodwar->self()->getRace().getRefinery();
            if (upgrade.gasPrice() > 0 && com(geyserType) == 0)
                return false;

            // First upgrade check
            if (upgrade == BuildOrder::getFirstUpgrade() && Broodwar->self()->getUpgradeLevel(upgrade) == 0 && !Broodwar->self()->isUpgrading(upgrade))
                return true;

            // Upgrades that require a building
            if (upgrade == UpgradeTypes::Adrenal_Glands && Broodwar->self()->getUpgradeLevel(upgrade) == 0 && !Broodwar->self()->isUpgrading(upgrade))
                return com(Zerg_Hive);

            // Armor/Weapon upgrades
            if (upgrade.maxRepeats() >= 3) {
                if (upgrade.getRace() == Races::Zerg && ((Broodwar->self()->getUpgradeLevel(upgrade) == 1 && com(Zerg_Lair) == 0 && com(Zerg_Hive) == 0) || (Broodwar->self()->getUpgradeLevel(upgrade) == 2 && com(Zerg_Hive) == 0)))
                    return false;
                if (upgrade.getRace() == Races::Protoss && Broodwar->self()->getUpgradeLevel(upgrade) >= 1 && com(Protoss_Templar_Archives) == 0)
                    return false;
                if (upgrade.getRace() == Races::Terran && Broodwar->self()->getUpgradeLevel(upgrade) >= 1 && com(Terran_Science_Facility) == 0)
                    return false;
            }

            for (auto &unit : upgrade.whatUses()) {
                if (BuildOrder::isUnitUnlocked(unit) && Broodwar->self()->getUpgradeLevel(upgrade) != upgrade.maxRepeats() && !Broodwar->self()->isUpgrading(upgrade))
                    return true;
            }
            return false;
        }

        bool isSuitable(UpgradeType upgrade)
        {
            using namespace UpgradeTypes;

            // Allow first upgrade
            if (upgrade == BuildOrder::getFirstUpgrade() && !BuildOrder::firstReady())
                return true;

            // Don't upgrade anything in opener if nothing is chosen
            if (BuildOrder::getFirstUpgrade() == UpgradeTypes::None && BuildOrder::isOpener())
                return false;

            // If this is a specific unit upgrade, check if it's unlocked
            if (upgrade.whatUses().size() == 1) {
                for (auto &unit : upgrade.whatUses()) {
                    if (!BuildOrder::isUnitUnlocked(unit))
                        return false;
                }
            }

            // If this isn't the first upgrade and we don't have our first tech/upgrade
            if (upgrade != BuildOrder::getFirstUpgrade()) {
                if (BuildOrder::getFirstUpgrade() != UpgradeTypes::None && Broodwar->self()->getUpgradeLevel(BuildOrder::getFirstUpgrade()) <= 0 && !Broodwar->self()->isUpgrading(BuildOrder::getFirstUpgrade()))
                    return false;
                if (BuildOrder::getFirstTech() != TechTypes::None && !Broodwar->self()->hasResearched(BuildOrder::getFirstTech()) && !Broodwar->self()->isResearching(BuildOrder::getFirstTech()))
                    return false;
            }

            // If we're playing Protoss, check Protoss upgrades
            if (Broodwar->self()->getRace() == Races::Protoss) {
                switch (upgrade) {

                    // Energy upgrades
                case Khaydarin_Amulet:
                    return (vis(Protoss_Assimilator) >= 4 && Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) && Broodwar->self()->gas() >= 750);
                case Khaydarin_Core:
                    return true;

                    // Range upgrades
                case Singularity_Charge:
                    return vis(Protoss_Dragoon) >= 1;

                    // Sight upgrades
                case Apial_Sensors:
                    return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
                case Sensor_Array:
                    return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);

                    // Capacity upgrades
                case Carrier_Capacity:
                    return vis(Protoss_Carrier) >= 2;
                case Reaver_Capacity:
                    return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
                case Scarab_Damage:
                    return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);

                    // Speed upgrades
                case Gravitic_Drive:
                    return vis(Protoss_Shuttle) > 0 && (vis(Protoss_High_Templar) > 0 || com(Protoss_Reaver) >= 2);
                case Gravitic_Thrusters:
                    return vis(Protoss_Scout) > 0;
                case Gravitic_Boosters:
                    return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
                case Leg_Enhancements:
                    return (vis(Protoss_Nexus) >= 3) || Players::PvZ();

                    // Ground unit upgrades
                case Protoss_Ground_Weapons:
                    return !Terrain::isIslandMap() && (Players::getSupply(PlayerState::Self, Races::Protoss) > 300 || Players::vZ());
                case Protoss_Ground_Armor:
                    return !Terrain::isIslandMap() && (Broodwar->self()->getUpgradeLevel(Protoss_Ground_Weapons) > Broodwar->self()->getUpgradeLevel(Protoss_Ground_Armor) || Broodwar->self()->isUpgrading(Protoss_Ground_Weapons));
                case Protoss_Plasma_Shields:
                    return haveOrUpgrading(Protoss_Ground_Weapons, 3) && haveOrUpgrading(Protoss_Ground_Armor, 3);

                    // Air unit upgrades
                case Protoss_Air_Weapons:
                    return (vis(Protoss_Corsair) > 0 || vis(Protoss_Scout) > 0 || (vis(Protoss_Stargate) > 0 && BuildOrder::isTechUnit(Protoss_Carrier) && Players::vT()));
                case Protoss_Air_Armor:
                    return Broodwar->self()->getUpgradeLevel(Protoss_Air_Weapons) > Broodwar->self()->getUpgradeLevel(Protoss_Air_Armor);
                }
            }

            else if (Broodwar->self()->getRace() == Races::Terran) {
                switch (upgrade) {

                    // Speed upgrades
                case Ion_Thrusters:
                    return true;

                    // Range upgrades
                case Charon_Boosters:
                    return vis(Terran_Goliath) > 0;
                case U_238_Shells:
                    return Broodwar->self()->hasResearched(TechTypes::Stim_Packs);

                    // Bio upgrades
                case Terran_Infantry_Weapons:
                    return true;
                case Terran_Infantry_Armor:
                    return (Broodwar->self()->getUpgradeLevel(Terran_Infantry_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Infantry_Armor) || Broodwar->self()->isUpgrading(Terran_Infantry_Weapons));

                    // Mech upgrades
                case Terran_Vehicle_Weapons:
                    return (Players::getStrength(PlayerState::Self).groundToGround > 20.0);
                case Terran_Vehicle_Plating:
                    return (Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Plating) || Broodwar->self()->isUpgrading(Terran_Vehicle_Weapons));
                case Terran_Ship_Weapons:
                    return (Players::getStrength(PlayerState::Self).airToAir > 20.0);
                case Terran_Ship_Plating:
                    return (Broodwar->self()->getUpgradeLevel(Terran_Ship_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Ship_Plating) || Broodwar->self()->isUpgrading(Terran_Ship_Weapons));
                }
            }

            else if (Broodwar->self()->getRace() == Races::Zerg) {
                switch (upgrade)
                {
                    // Speed upgrades
                case Metabolic_Boost:
                    return vis(Zerg_Zergling) >= 20 || total(Zerg_Hive) > 0 || total(Zerg_Defiler_Mound) > 0;
                case Muscular_Augments:
                    return (BuildOrder::isTechUnit(Zerg_Hydralisk) && Players::ZvP()) || Broodwar->self()->getUpgradeLevel(Grooved_Spines) > 0;
                case Pneumatized_Carapace:
                    return (Players::ZvT() && Spy::getEnemyTransition() == "2PortWraith")
                        || (Players::ZvP() && Players::getStrength(PlayerState::Enemy).airToAir > 0 && Players::getSupply(PlayerState::Self, Races::Zerg) >= 160)
                        || (Spy::enemyInvis() && (BuildOrder::isTechUnit(Zerg_Hydralisk) || BuildOrder::isTechUnit(Zerg_Ultralisk)))
                        || (Players::getSupply(PlayerState::Self, Races::Zerg) >= 200);
                case Anabolic_Synthesis:
                    return Players::getTotalCount(PlayerState::Enemy, Terran_Marine) < 20 || Broodwar->self()->getUpgradeLevel(Chitinous_Plating) > 0;

                    // Range upgrades
                case Grooved_Spines:
                    return (BuildOrder::isTechUnit(Zerg_Hydralisk) && !Players::ZvP()) || Broodwar->self()->getUpgradeLevel(Muscular_Augments) > 0;

                    // Other upgrades
                case Chitinous_Plating:
                    return Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 20 || Broodwar->self()->getUpgradeLevel(Anabolic_Synthesis) > 0;
                case Adrenal_Glands:
                    return Broodwar->self()->getUpgradeLevel(Metabolic_Boost) > 0;

                    // Ground unit upgrades
                case Zerg_Melee_Attacks:
                    return (BuildOrder::getCompositionPercentage(Zerg_Zergling) > 0.0 || BuildOrder::getCompositionPercentage(Zerg_Ultralisk) > 0.0) && Players::getSupply(PlayerState::Self, Races::Zerg) > 160 && (Broodwar->self()->getUpgradeLevel(Zerg_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Melee_Attacks) || Broodwar->self()->isUpgrading(Zerg_Carapace));
                case Zerg_Missile_Attacks:
                    return (BuildOrder::getCompositionPercentage(Zerg_Hydralisk) > 0.0 || BuildOrder::getCompositionPercentage(Zerg_Lurker) > 0.0) && (Broodwar->self()->getUpgradeLevel(Zerg_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Missile_Attacks) || Broodwar->self()->isUpgrading(Zerg_Carapace));
                case Zerg_Carapace:
                    return (BuildOrder::getTechUnit() == Zerg_Ultralisk
                        || BuildOrder::getCompositionPercentage(Zerg_Hydralisk) > 0.0
                        || BuildOrder::getCompositionPercentage(Zerg_Zergling) > 0.0
                        || BuildOrder::getCompositionPercentage(Zerg_Ultralisk) > 0.0) && Players::getSupply(PlayerState::Self, Races::Zerg) > 100;

                    // Air unit upgrades
                case Zerg_Flyer_Attacks:
                    if (Players::ZvP() && Players::getVisibleCount(PlayerState::Enemy, Protoss_Corsair) < 2)
                        return false;
                    if (Players::ZvZ() && int(Stations::getStations(PlayerState::Self).size()) < 2)
                        return false;
                    return BuildOrder::getCompositionPercentage(Zerg_Mutalisk) > 0.0 && (Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Carapace) > Broodwar->self()->getUpgradeLevel(Zerg_Flyer_Attacks) + 1 || Broodwar->self()->isUpgrading(Zerg_Flyer_Carapace));
                case Zerg_Flyer_Carapace:
                    if (Players::ZvZ() && int(Stations::getStations(PlayerState::Self).size()) < 2)
                        return false;
                    return BuildOrder::getCompositionPercentage(Zerg_Mutalisk) > 0.0 && total(Zerg_Mutalisk) >= 12 && Stations::getStations(PlayerState::Self).size() >= 3;
                }
            }
            return false;
        }

        bool upgrade(UnitInfo& building)
        {
            // Clear idle checks
            auto idleItr = idleUpgrade.find(building.unit());
            if (idleItr != idleUpgrade.end()) {
                reservedMineral -= idleItr->second.mineralPrice();
                reservedGas -= idleItr->second.gasPrice();
                idleUpgrade.erase(building.unit());
            }

            int offset = 0;
            for (auto &upgrade : building.getType().upgradesWhat()) {
                if (isCreateable(building.unit(), upgrade) && isSuitable(upgrade)) {
                    if (isAffordable(upgrade)) {
                        building.setRemainingTrainFrame(upgrade.upgradeTime(Broodwar->self()->getUpgradeLevel(upgrade) + 1));
                        building.unit()->upgrade(upgrade);
                        lastUpgradeFrame = Broodwar->getFrameCount();
                        return true;
                    }
                    else if ((Workers::getMineralWorkers() > 0 || Broodwar->self()->minerals() >= upgrade.mineralPrice()) && (Workers::getGasWorkers() > 0 || Broodwar->self()->gas() >= upgrade.gasPrice())) {
                        idleUpgrade[building.unit()] = upgrade;
                        reservedMineral += upgrade.mineralPrice();
                        reservedGas += upgrade.gasPrice();
                    }
                    break;
                }
            }
            return false;
        }

        void updateReservedResources()
        {
            // Reserved resources for idle upgrades
            for (auto &[unit, upgrade] : idleUpgrade) {
                if (unit && unit->exists()) {
                    reservedMineral += upgrade.mineralPrice();
                    reservedGas += upgrade.gasPrice();
                }
            }
        }

        void updateUpgrading()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &building = *u;

                if (!building.unit()
                    || building.getRole() != Role::Production
                    || !building.unit()->isCompleted()
                    || building.getRemainingTrainFrames() >= Broodwar->getLatencyFrames()
                    || Upgrading::upgradedThisFrame()
                    || Researching::researchedThisFrame()
                    || Producing::producedThisFrame()
                    || building.getType() == Zerg_Larva)
                    continue;

                upgrade(building);
            }
        }
    }

    void onFrame() 
    {
        Visuals::startPerfTest();
        reset();
        updateReservedResources();
        updateUpgrading();
        Visuals::endPerfTest("Upgrading");
    }

    bool upgradedThisFrame() {
        return lastUpgradeFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4;
    }

    bool haveOrUpgrading(UpgradeType upgrade, int level) {
        return ((Broodwar->self()->isUpgrading(upgrade) && Broodwar->self()->getUpgradeLevel(upgrade) == level - 1) || Broodwar->self()->getUpgradeLevel(upgrade) >= level);
    }

    int getReservedMineral() { return reservedMineral; }
    int getReservedGas() { return reservedGas; }
    bool hasIdleUpgrades() { return !idleUpgrade.empty(); }
}