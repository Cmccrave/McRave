#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Researching {

    namespace {
        map <Unit, TechType> idleResearch;
        int reservedMineral, reservedGas;
        int lastResearchFrame = -999;

        void reset()
        {
            reservedMineral = 0;
            reservedGas = 0;
        }

        bool isAffordable(TechType tech)
        {
            auto mineralCost        = tech.mineralPrice();
            auto gasCost            = tech.gasPrice();

            // Plan buildings and upgrades before we research
            auto mineralReserve     = Planning::getPlannedMineral() + Upgrading::getReservedMineral();
            auto gasReserve         = Planning::getPlannedGas() + Upgrading::getReservedGas();

            return Broodwar->self()->minerals() >= (mineralCost + mineralReserve) && Broodwar->self()->gas() >= (gasCost + gasReserve);
        }

        bool isCreateable(Unit building, TechType tech)
        {
            // First tech check
            if (tech == BuildOrder::getFirstFocusTech() && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech))
                return true;

            // Avoid researching Burrow with a Lair/Hive
            if (tech == TechTypes::Burrowing && building->getType() != Zerg_Hatchery)
                return false;

            // Tech that require a building
            if (tech == TechTypes::Lurker_Aspect && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech))
                return (com(Zerg_Lair) > 0 || com(Zerg_Hive) > 0) && BuildOrder::isUnitUnlocked(Zerg_Lurker);

            for (auto &unit : tech.whatUses()) {
                if (BuildOrder::isUnitUnlocked(unit) && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech))
                    return true;
            }
            return false;
        }

        bool isSuitable(TechType tech)
        {
            using namespace TechTypes;

            // If we have an upgrade order, follow it
            auto incompleteQueue = false;
            for (auto &[t, cnt] : BuildOrder::getTechQueue()) {
                if (!haveOrResearching(t)) {
                    incompleteQueue = true;
                    if (tech == t)
                        return true;
                }
            }
            if (incompleteQueue)
                return false;

            // If this is a specific unit tech, check if it's unlocked
            if (tech != BuildOrder::getFirstFocusTech() && tech.whatUses().size() == 1) {
                for (auto &unit : tech.whatUses()) {
                    if (!BuildOrder::isUnitUnlocked(unit))
                        return false;
                }
            }

            if (Broodwar->self()->getRace() == Races::Protoss) {
                switch (tech) {
                case Psionic_Storm:
                    return true;
                case Stasis_Field:
                    return Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core) > 0;
                case Recall:
                    return (Broodwar->self()->hasResearched(TechTypes::Stasis_Field) && Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
                case Disruption_Web:
                    return (vis(Protoss_Corsair) >= 10);
                }
            }

            else if (Broodwar->self()->getRace() == Races::Terran) {
                switch (tech) {
                case Stim_Packs:
                    return true;
                case Spider_Mines:
                    return Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) > 0 || Broodwar->self()->isUpgrading(UpgradeTypes::Ion_Thrusters);
                case Tank_Siege_Mode:
                    return Broodwar->self()->hasResearched(TechTypes::Spider_Mines) || Broodwar->self()->isResearching(TechTypes::Spider_Mines) || vis(Terran_Siege_Tank_Tank_Mode) > 0;
                case Cloaking_Field:
                    return vis(Terran_Wraith) >= 2;
                case Yamato_Gun:
                    return vis(Terran_Battlecruiser) >= 0;
                case Personnel_Cloaking:
                    return vis(Terran_Ghost) >= 2;
                }
            }

            else if (Broodwar->self()->getRace() == Races::Zerg) {
                return false;
            }
            return false;
        }

        bool research(UnitInfo& building)
        {
            // Clear idle checks
            auto idleItr = idleResearch.find(building.unit());
            if (idleItr != idleResearch.end()) {
                reservedMineral -= idleItr->second.mineralPrice();
                reservedGas -= idleItr->second.gasPrice();
                idleResearch.erase(building.unit());
            }

            for (auto &research : building.getType().researchesWhat()) {
                if (isCreateable(building.unit(), research) && isSuitable(research)) {
                    if (isAffordable(research)) {
                        building.setRemainingTrainFrame(research.researchTime());
                        building.unit()->research(research);
                        lastResearchFrame = Broodwar->getFrameCount();
                        return true;
                    }
                    else if ((Workers::getMineralWorkers() > 0 || Broodwar->self()->minerals() >= research.mineralPrice()) && (Workers::getGasWorkers() > 0 || Broodwar->self()->gas() >= research.gasPrice())) {
                        idleResearch[building.unit()] = research;
                        reservedMineral += research.mineralPrice();
                        reservedGas += research.gasPrice();
                    }
                    break;
                }
            }
            return false;
        }

        void updateReservedResources()
        {
            // Reserved resources for idle research
            for (auto &[unit, tech] : idleResearch) {
                if (unit && unit->exists()) {
                    reservedMineral += tech.mineralPrice();
                    reservedGas += tech.gasPrice();
                }
            }
        }

        void updateResearching()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &building = *u;

                if (!building.unit()
                    || building.getRole() != Role::Production
                    || !building.isCompleted()
                    || building.getRemainingTrainFrames() >= Broodwar->getLatencyFrames()
                    || Upgrading::upgradedThisFrame()
                    || Researching::researchedThisFrame()
                    || Producing::producedThisFrame()
                    || building.getType() == Zerg_Larva)
                    continue;

                research(building);
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        reset();
        updateReservedResources();
        updateResearching();
        Visuals::endPerfTest("Upgrading");
    }

    bool researchedThisFrame() {
        return lastResearchFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4;
    }

    bool haveOrResearching(TechType tech) {
        return Broodwar->self()->isResearching(tech) || Broodwar->self()->hasResearched(tech);
    }

    int getReservedMineral() { return reservedMineral; }
    int getReservedGas() { return reservedGas; }
    bool hasIdleResearch() { return !idleResearch.empty(); }
}