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

            // Plan buildings before we upgrade
            auto mineralReserve     = Planning::getPlannedMineral();
            auto gasReserve         = Planning::getPlannedGas();

            return Broodwar->self()->minerals() >= (mineralCost + mineralReserve) && Broodwar->self()->gas() >= (gasCost + gasReserve);
        }

        bool isCreateable(Unit building, UpgradeType upgrade)
        {
            auto geyserType = Broodwar->self()->getRace().getRefinery();
            if (upgrade.gasPrice() > 0 && com(geyserType) == 0)
                return false;

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

            // If we aren't upgrading it and aren't maxed, we can upgrade it
            if (Broodwar->self()->getUpgradeLevel(upgrade) != upgrade.maxRepeats() && !Broodwar->self()->isUpgrading(upgrade))
                return true;
            return false;
        }

        bool isSuitable(UpgradeType upgrade)
        {
            // If we have an upgrade order, follow it
            for (auto &[u, cnt] : BuildOrder::getUpgradeQueue()) {
                if (!haveOrUpgrading(u, cnt)) {
                    if (u == upgrade && cnt > 0)
                        return true;
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
                        building.setRemainingTrainFrame(upgrade.upgradeTime(Broodwar->self()->getUpgradeLevel(upgrade) + 1) + Broodwar->getLatencyFrames() + 1);
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

                Broodwar->drawTextMap(building.getPosition(), "%d", building.getRemainingTrainFrames());

                if (!building.unit()
                    || building.getRole() != Role::Production
                    || !building.isCompleted()
                    || building.getRemainingTrainFrames() > 0
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