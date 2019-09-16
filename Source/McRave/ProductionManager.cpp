#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Production {

    namespace {
        map <Unit, UnitType> idleProduction;
        map <Unit, TechType> idleTech;
        map <Unit, UpgradeType> idleUpgrade;
        map <UnitType, int> trainedThisFrame;
        int reservedMineral, reservedGas;
        int lastTrainFrame = 0;

        bool haveOrUpgrading(UpgradeType upgrade, int level) {
            return ((Broodwar->self()->isUpgrading(upgrade) && Broodwar->self()->getUpgradeLevel(upgrade) == level - 1) || Broodwar->self()->getUpgradeLevel(upgrade) >= level);
        }

        bool isAffordable(UnitType unit)
        {
            auto mineralReserve = int(!BuildOrder::isTechUnit(unit)) * reservedMineral;
            auto gasReserve = int(!BuildOrder::isTechUnit(unit)) * reservedGas;
            auto mineralAffordable = (Broodwar->self()->minerals() >= unit.mineralPrice() + Buildings::getQueuedMineral() + mineralReserve) || unit.mineralPrice() == 0;
            auto gasAffordable = (Broodwar->self()->gas() >= unit.gasPrice() + Buildings::getQueuedGas() + gasReserve) || unit.gasPrice() == 0;
            auto supplyAffordable = Players::getSupply(PlayerState::Self) + unit.supplyRequired() <= Broodwar->self()->supplyTotal();

            return mineralAffordable && gasAffordable && supplyAffordable;
        }

        bool isAffordable(TechType tech)
        {
            return Broodwar->self()->minerals() >= tech.mineralPrice() && Broodwar->self()->gas() >= tech.gasPrice();
        }

        bool isAffordable(UpgradeType upgrade)
        {
            return Broodwar->self()->minerals() >= upgrade.mineralPrice() && Broodwar->self()->gas() >= upgrade.gasPrice();
        }

        bool isCreateable(Unit building, UnitType unit)
        {
            if (!BuildOrder::isUnitUnlocked(unit))
                return false;

            switch (unit)
            {
                // Gateway Units
            case Enum::Protoss_Zealot:
                return true;
            case Enum::Protoss_Dragoon:
                return com(Protoss_Cybernetics_Core) > 0;
            case Enum::Protoss_Dark_Templar:
                return com(Protoss_Templar_Archives) > 0;
            case Enum::Protoss_High_Templar:
                return com(Protoss_Templar_Archives) > 0;

                // Robo Units
            case Enum::Protoss_Shuttle:
                return true;
            case Enum::Protoss_Reaver:
                return com(Protoss_Robotics_Support_Bay) > 0;
            case Enum::Protoss_Observer:
                return com(Protoss_Observatory) > 0;

                // Stargate Units
            case Enum::Protoss_Corsair:
                return true;
            case Enum::Protoss_Scout:
                return true;
            case Enum::Protoss_Carrier:
                return com(Protoss_Fleet_Beacon) > 0;
            case Enum::Protoss_Arbiter:
                return com(Protoss_Arbiter_Tribunal) > 0;

                // Barracks Units
            case Enum::Terran_Marine:
                return true;
            case Enum::Terran_Firebat:
                return com(Terran_Academy) > 0;
            case Enum::Terran_Medic:
                return com(Terran_Academy) > 0;
            case Enum::Terran_Ghost:
                return com(Terran_Covert_Ops) > 0;
            case Enum::Terran_Nuclear_Missile:
                return com(Terran_Covert_Ops) > 0;

                // Factory Units
            case Enum::Terran_Vulture:
                return true;
            case Enum::Terran_Siege_Tank_Tank_Mode:
                return building->getAddon() != nullptr ? true : false;
            case Enum::Terran_Goliath:
                return (com(Terran_Armory) > 0);

                // Starport Units
            case Enum::Terran_Wraith:
                return true;
            case Enum::Terran_Valkyrie:
                return (com(Terran_Armory) > 0 && building->getAddon() != nullptr) ? true : false;
            case Enum::Terran_Battlecruiser:
                return (com(Terran_Physics_Lab) && building->getAddon() != nullptr) ? true : false;
            case Enum::Terran_Science_Vessel:
                return (com(Terran_Science_Facility) > 0 && building->getAddon() != nullptr) ? true : false;
            case Enum::Terran_Dropship:
                return building->getAddon() != nullptr ? true : false;

                // Zerg Units
            case Enum::Zerg_Drone:
                return true;
            case Enum::Zerg_Zergling:
                return (com(Zerg_Spawning_Pool) > 0);
            case Enum::Zerg_Hydralisk:
                return (com(Zerg_Hydralisk_Den) > 0);
            case Enum::Zerg_Mutalisk:
                return (com(Zerg_Spire) > 0);
            case Enum::Zerg_Scourge:
                return (com(Zerg_Spire) > 0);
            case Enum::Zerg_Ultralisk:
                return (com(Zerg_Ultralisk_Cavern) > 0);
            }
            return false;
        }

        bool isCreateable(UpgradeType upgrade)
        {
            auto geyserType = Broodwar->self()->getRace().getRefinery();
            if (upgrade.gasPrice() > 0 && com(geyserType) == 0)
                return false;

            // First upgrade check
            if (upgrade == BuildOrder::getFirstUpgrade() && Broodwar->self()->getUpgradeLevel(upgrade) == 0 && !Broodwar->self()->isUpgrading(upgrade))
                return true;

            // Upgrades that require a building
            if (upgrade == UpgradeTypes::Adrenal_Glands)
                return com(Zerg_Hive);

            for (auto &unit : upgrade.whatUses()) {
                if (BuildOrder::isUnitUnlocked(unit) && Broodwar->self()->getUpgradeLevel(upgrade) != upgrade.maxRepeats() && !Broodwar->self()->isUpgrading(upgrade))
                    return true;
            }
            return false;
        }

        bool isCreateable(TechType tech)
        {
            // First tech check
            if (tech == BuildOrder::getFirstTech() && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech))
                return true;

            // Tech that require a building
            if (tech == TechTypes::Lurker_Aspect)
                return com(Zerg_Lair);

            for (auto &unit : tech.whatUses()) {
                if (BuildOrder::isUnitUnlocked(unit) && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech))
                    return true;
            }
            return false;
        }

        bool isSuitable(UnitType unit)
        {
            if (unit.isWorker()) {
                if (com(unit) < 90 && (!Resources::isMinSaturated() || !Resources::isGasSaturated()))
                    return true;
                else
                    return false;
            }

            if (unit.getRace() == Races::Zerg)
                return true;

            bool needReavers = false;
            bool needShuttles = false;

            // Determine whether we want reavers or shuttles
            if (!Strategy::needDetection()) {
                if ((Terrain::isIslandMap() && vis(unit) < 2 * vis(Protoss_Nexus))
                    || vis(Protoss_Reaver) > vis(Protoss_Shuttle) * 2
                    || vis(Protoss_High_Templar) > vis(Protoss_Shuttle) * 4)
                    needShuttles = true;
                if (!Terrain::isIslandMap() || (vis(Protoss_Reaver) <= (vis(Protoss_Shuttle) * 2)))
                    needReavers = true;
            }

            // Determine our templar caps
            auto htCap = min(2 * (Players::getSupply(PlayerState::Self) / 100), Players::vP ? 4 : 8);


            if (Players::vP() && com(Protoss_Reaver) + com(Protoss_High_Templar) <= 2) {
                needShuttles = false;
                needReavers = true;
            }

            switch (unit)
            {
                // Gateway Units
            case Protoss_Zealot:
                return true;
            case Protoss_Dragoon:
                return true;
            case Protoss_Dark_Templar:
                return vis(unit) < 4;
            case Protoss_High_Templar:
                return (vis(unit) < 2 + (Players::getSupply(PlayerState::Self) / 200) && vis(unit) <= com(unit) + 2) || Production::scoreUnit(UnitTypes::Protoss_Archon) > Production::scoreUnit(UnitTypes::Protoss_High_Templar);

                // Robo Units
            case Protoss_Shuttle:
                return needShuttles;
            case Protoss_Reaver:
                return needReavers;
            case Protoss_Observer:
                return BuildOrder::isTechUnit(Protoss_Reaver) && Broodwar->getFrameCount() <= 13000 ? vis(unit) < 1 : vis(unit) < 1 + (Players::getSupply(PlayerState::Self) / 100);

                // Stargate Units
            case Protoss_Corsair:
                return vis(unit) < (10 + (Terrain::isIslandMap() * 10));
            case Protoss_Scout:
                return true;
            case Protoss_Carrier:
                return true;
            case Protoss_Arbiter:
                return (vis(unit) < 8 && (Broodwar->self()->isUpgrading(UpgradeTypes::Khaydarin_Core) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core)));

                // Barracks Units
            case Terran_Marine:
                return true;
            case Terran_Firebat:
                return true;
            case Terran_Medic:
                return com(unit) * 4 < com(Terran_Marine);
            case Terran_Ghost:
                return BuildOrder::getCurrentBuild() == "TNukeMemes";
            case Terran_Nuclear_Missile:
                return BuildOrder::getCurrentBuild() == "TNukeMemes";

                // Factory Units
            case Terran_Vulture:
                return true;
            case Terran_Siege_Tank_Tank_Mode:
                return true;
            case Terran_Goliath:
                return true;

                // Starport Units
            case Terran_Wraith:
                return BuildOrder::getCurrentBuild() == "T2PortWraith";
            case Terran_Valkyrie:
                return BuildOrder::getCurrentBuild() == "T2PortWraith";
            case Terran_Battlecruiser:
                return true;
            case Terran_Science_Vessel:
                return vis(unit) < 6;
            case Terran_Dropship:
                return vis(unit) <= 0;
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
                    return (vis(Protoss_Nexus) >= 3);

                    // Ground unit upgrades
                case Protoss_Ground_Weapons:
                    return !Terrain::isIslandMap() && (Players::getSupply(PlayerState::Self) > 300 || Players::vZ());
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
                    return Strategy::getUnitScore(Terran_Goliath) > 1.00;
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
                    return true;
                case Muscular_Augments:
                    return Broodwar->self()->getUpgradeLevel(Grooved_Spines);
                case Pneumatized_Carapace:
                    return !BuildOrder::isOpener();
                case Anabolic_Synthesis:
                    return true;

                    // Range upgrades
                case Grooved_Spines:
                    return true;

                    // Other upgrades
                case Chitinous_Plating:
                    return true;
                case Adrenal_Glands:
                    return true;

                    // Ground unit upgrades
                case Zerg_Melee_Attacks:
                    return (Players::getSupply(PlayerState::Self) > 120);
                case Zerg_Missile_Attacks:
                    return vis(Zerg_Hydralisk) >= 8 || vis(Zerg_Lurker) >= 4;
                case Zerg_Carapace:
                    return (Players::getSupply(PlayerState::Self) > 120);

                    // Air unit upgrades
                case Zerg_Flyer_Attacks:
                    return (Players::getSupply(PlayerState::Self) > 120);
                case Zerg_Flyer_Carapace:
                    return (Players::getSupply(PlayerState::Self) > 120);
                }
            }
            return false;
        }

        bool isSuitable(TechType tech)
        {
            using namespace TechTypes;

            // Allow first tech
            if (tech == BuildOrder::getFirstTech() && !BuildOrder::firstReady())
                return true;

            // If this is a specific unit tech, check if it's unlocked
            if (tech.whatUses().size() == 1) {
                for (auto &unit : tech.whatUses()) {
                    if (!BuildOrder::isUnitUnlocked(unit))
                        return false;
                }
            }

            // If this isn't the first tech and we don't have our first tech/upgrade
            if (tech != BuildOrder::getFirstTech()) {
                if (BuildOrder::getFirstUpgrade() != UpgradeTypes::None && Broodwar->self()->getUpgradeLevel(BuildOrder::getFirstUpgrade()) <= 0 && !Broodwar->self()->isUpgrading(BuildOrder::getFirstUpgrade()))
                    return false;
                if (BuildOrder::getFirstTech() != TechTypes::None && !Broodwar->self()->hasResearched(BuildOrder::getFirstTech()) && !Broodwar->self()->isResearching(BuildOrder::getFirstTech()))
                    return false;
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
                    return true;// BuildOrder::isBioBuild();
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
                switch (tech) {
                case Lurker_Aspect:
                    return true;
                }
            }
            return false;
        }

        void addon(UnitInfo& building)
        {
            for (auto &unit : building.getType().buildsWhat()) {
                if (unit.isAddon() && BuildOrder::buildCount(unit) > vis(unit))
                    building.unit()->buildAddon(unit);
            }
        }

        void produce(UnitInfo& building)
        {
            auto offset = 16;
            auto best = 0.0;
            auto bestType = None;

            if (building.getType() == Zerg_Larva && BuildOrder::buildCount(Zerg_Overlord) > vis(Zerg_Overlord) + trainedThisFrame[Zerg_Overlord]) {
                building.unit()->morph(Zerg_Overlord);
                trainedThisFrame[Zerg_Overlord]++;
                return;
            }

            if (building.getType() == Zerg_Larva && Buildings::overlapsQueue(building, building.getTilePosition())) {
                building.unit()->stop();
                return;
            }

            for (auto &type : building.getType().buildsWhat()) {

                if (!isCreateable(building.unit(), type) || !isSuitable(type))
                    continue;

                const auto value = scoreUnit(type);

                // If we teched to DTs, try to create as many as possible
                if (type == Protoss_Dark_Templar && Broodwar->getFrameCount() < 12000 && vis(Protoss_Dark_Templar) < 2) {
                    best = DBL_MAX;
                    bestType = type;
                }
                else if (type == BuildOrder::getTechUnit() && vis(type) == 0 && isAffordable(type)) {
                    best = DBL_MAX;
                    bestType = type;
                }
                else if (type == Protoss_Observer && vis(type) < com(Protoss_Nexus)) {
                    best = DBL_MAX;
                    bestType = type;
                }
                else if (value >= best) {
                    best = value;
                    bestType = type;
                }
            }

            if (bestType != None) {

                // If we can afford it, train it
                if (isAffordable(bestType)) {
                    building.unit()->train(bestType);
                    building.setRemainingTrainFrame(bestType.buildTime());
                    idleProduction.erase(building.unit());
                    lastTrainFrame = Broodwar->getFrameCount();
                }

                // Else if this is a tech unit, add it to idle production
                else if (BuildOrder::isTechUnit(bestType)) {
                    idleProduction[building.unit()] = bestType;
                    reservedMineral += bestType.mineralPrice();
                    reservedGas += bestType.gasPrice();
                }

                // Else store a zero value idle
                else
                    idleProduction[building.unit()] = None;
            }
        }

        void research(UnitInfo& building)
        {
            for (auto &research : building.getType().researchesWhat()) {
                if (isCreateable(research) && isSuitable(research)) {
                    if (isAffordable(research))
                        building.unit()->research(research), idleTech.erase(building.unit());
                    else
                        idleTech[building.unit()] = research;
                    reservedMineral += research.mineralPrice();
                    reservedGas += research.gasPrice();
                }
            }
        }

        void upgrade(UnitInfo& building)
        {
            for (auto &upgrade : building.getType().upgradesWhat()) {
                if (isCreateable(upgrade) && isSuitable(upgrade)) {
                    if (isAffordable(upgrade))
                        building.unit()->upgrade(upgrade), idleUpgrade.erase(building.unit());
                    else
                        idleUpgrade[building.unit()] = upgrade;
                    reservedMineral += upgrade.mineralPrice();
                    reservedGas += upgrade.gasPrice();
                }
            }
        }

        void updateReservedResources()
        {
            // Reserved minerals for idle buildings, tech and upgrades
            reservedMineral = 0;
            reservedGas = 0;

            for (auto &[_, type] : idleProduction) {
                if (BuildOrder::isTechUnit(type)) {
                    reservedMineral += type.mineralPrice();
                    reservedGas += type.gasPrice();
                }
            }

            for (auto &[_, tech] : idleTech) {
                reservedMineral += tech.mineralPrice();
                reservedGas += tech.gasPrice();
            }

            for (auto &[_, upgrade] : idleUpgrade) {
                reservedMineral += upgrade.mineralPrice();
                reservedGas += upgrade.gasPrice();
            }
        }

        void updateProduction()
        {
            trainedThisFrame.clear();

            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &building = *u;

                if (!building.unit()
                    || building.getRole() != Role::Production
                    || !building.unit()->isCompleted()
                    || building.getRemainingTrainFrames() >= Broodwar->getLatencyFrames()
                    || lastTrainFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames())
                    continue;

                // TODO: Combine into one - iterate all commands and return when true
                if (!building.getType().isResourceDepot() || building.getType().getRace() == Races::Zerg) {
                    idleProduction.erase(building.unit());
                    idleUpgrade.erase(building.unit());
                    idleTech.erase(building.unit());

                    addon(building);
                    produce(building);
                    research(building);
                    upgrade(building);
                }

                else {
                    for (auto &unit : building.getType().buildsWhat()) {
                        if (unit.isAddon() && !building.unit()->getAddon() && BuildOrder::buildCount(unit) > vis(unit)) {
                            building.unit()->buildAddon(unit);
                            continue;
                        }
                        auto makeExtra = vis(Protoss_Probe) <= 28 && Broodwar->self()->minerals() >= 200;
                        if (!BuildOrder::isWorkerCut() && unit.isWorker() && com(unit) < 75 && isAffordable(unit) && (!Resources::isGasSaturated() || !Resources::isMinSaturated() || makeExtra)) {
                            building.unit()->train(unit);
                            building.setRemainingTrainFrame(unit.buildTime());
                            lastTrainFrame = Broodwar->getFrameCount();
                        }
                    }
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateReservedResources();
        updateProduction();
        Visuals::endPerfTest("Production");
    }

    double scoreUnit(UnitType type)
    {
        auto typeMineralCost = Math::realisticMineralCost(type);
        auto typeGasCost = Math::realisticGasCost(type);

        auto mineralCost = Broodwar->self()->minerals() == 0 || typeMineralCost == 0 ? 1.0 : double(Broodwar->self()->minerals() - typeMineralCost - (!BuildOrder::isTechUnit(type) * reservedMineral) - Buildings::getQueuedMineral()) / double(Broodwar->self()->minerals());
        auto gasCost = Broodwar->self()->gas() == 0 || typeGasCost == 0 ? 1.0 : double(Broodwar->self()->gas() - typeGasCost - (!BuildOrder::isTechUnit(type) * reservedGas) - Buildings::getQueuedGas()) / double(Broodwar->self()->gas());

        // HACK: Prevent them going negative
        mineralCost = max(0.1, mineralCost);
        gasCost = max(0.1, gasCost);

        const auto resourceScore = clamp(gasCost * mineralCost, 0.01, 1.0);
        const auto strategyScore = clamp(Strategy::getUnitScore(type) / double(max(1, vis(type))), 0.01, 1.0);

        return resourceScore * strategyScore;
    }

    int getReservedMineral() { return reservedMineral; }
    int getReservedGas() { return reservedGas; }
    bool hasIdleProduction() { return int(idleProduction.size()) > 0; }
}