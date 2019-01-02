#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Production {

    namespace {
        map <Unit, UnitType> idleProduction;
        map <Unit, TechType> idleTech;
        map <Unit, UpgradeType> idleUpgrade;
        int reservedMineral, reservedGas;
        int idleFrame = 0;

        bool isAffordable(UnitType unit)
        {
            auto mineralReserve = int(!BuildOrder::isTechUnit(unit)) * reservedMineral;
            auto gasReserve = int(!BuildOrder::isTechUnit(unit)) * reservedGas;
            auto mineralAffordable = (Broodwar->self()->minerals() >= unit.mineralPrice() + Buildings::getQueuedMineral() + mineralReserve) || unit.mineralPrice() == 0;
            auto gasAffordable = (Broodwar->self()->gas() >= unit.gasPrice() + Buildings::getQueuedGas() + gasReserve) || unit.gasPrice() == 0;
            auto supplyAffordable = Units::getSupply() + unit.supplyRequired() <= Broodwar->self()->supplyTotal();

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
            case UnitTypes::Enum::Protoss_Zealot:
                return true;
            case UnitTypes::Enum::Protoss_Dragoon:
                return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Cybernetics_Core) > 0;
            case UnitTypes::Enum::Protoss_Dark_Templar:
                return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives) > 0;
            case UnitTypes::Enum::Protoss_High_Templar:
                return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Templar_Archives) > 0;

                // Robo Units
            case UnitTypes::Enum::Protoss_Shuttle:
                return true;
            case UnitTypes::Enum::Protoss_Reaver:
                return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Robotics_Support_Bay) > 0;
            case UnitTypes::Enum::Protoss_Observer:
                return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Observatory) > 0;

                // Stargate Units
            case UnitTypes::Enum::Protoss_Corsair:
                return true;
            case UnitTypes::Enum::Protoss_Scout:
                return true;
            case UnitTypes::Enum::Protoss_Carrier:
                return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Fleet_Beacon) > 0;
            case UnitTypes::Enum::Protoss_Arbiter:
                return Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Arbiter_Tribunal) > 0;

                // Barracks Units
            case UnitTypes::Enum::Terran_Marine:
                return true;
            case UnitTypes::Enum::Terran_Firebat:
                return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Academy) > 0;
            case UnitTypes::Enum::Terran_Medic:
                return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Academy) > 0;
            case UnitTypes::Enum::Terran_Ghost:
                return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Covert_Ops) > 0;
            case UnitTypes::Enum::Terran_Nuclear_Missile:
                return Broodwar->self()->completedUnitCount(UnitTypes::Terran_Covert_Ops) > 0;

                // Factory Units
            case UnitTypes::Enum::Terran_Vulture:
                return true;
            case UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode:
                return building->getAddon() != nullptr ? true : false;
            case UnitTypes::Enum::Terran_Goliath:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Armory) > 0);

                // Starport Units
            case UnitTypes::Enum::Terran_Wraith:
                return true;
            case UnitTypes::Enum::Terran_Valkyrie:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Armory) > 0 && building->getAddon() != nullptr) ? true : false;
            case UnitTypes::Enum::Terran_Battlecruiser:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Physics_Lab) && building->getAddon() != nullptr) ? true : false;
            case UnitTypes::Enum::Terran_Science_Vessel:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Science_Facility) > 0 && building->getAddon() != nullptr) ? true : false;
            case UnitTypes::Enum::Terran_Dropship:
                return building->getAddon() != nullptr ? true : false;

                // Zerg Units
            case UnitTypes::Enum::Zerg_Drone:
                return true;
            case UnitTypes::Enum::Zerg_Zergling:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Spawning_Pool) > 0);
            case UnitTypes::Enum::Zerg_Hydralisk:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Hydralisk_Den) > 0);
            case UnitTypes::Enum::Zerg_Mutalisk:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Spire) > 0);
            case UnitTypes::Enum::Zerg_Scourge:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Spire) > 0);
            case UnitTypes::Enum::Zerg_Ultralisk:
                return (Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Ultralisk_Cavern) > 0);
            }
            return false;
        }

        bool isCreateable(UpgradeType upgrade)
        {
            if (upgrade == BuildOrder::getFirstUpgrade() && Broodwar->self()->getUpgradeLevel(upgrade) == 0 && !Broodwar->self()->isUpgrading(upgrade))
                return true;

            // Some hardcoded ones
            if (upgrade == UpgradeTypes::Adrenal_Glands)
                return Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Hive);

            for (auto &unit : upgrade.whatUses()) {
                if (BuildOrder::isUnitUnlocked(unit) && Broodwar->self()->getUpgradeLevel(upgrade) != upgrade.maxRepeats() && !Broodwar->self()->isUpgrading(upgrade))
                    return true;
            }
            return false;
        }

        bool isCreateable(TechType tech)
        {
            if (tech == BuildOrder::getFirstTech() && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech))
                return true;

            for (auto &unit : tech.whatUses()) {
                if (BuildOrder::isUnitUnlocked(unit) && !Broodwar->self()->hasResearched(tech) && !Broodwar->self()->isResearching(tech))
                    return true;
            }
            return false;
        }

        bool isSuitable(UnitType unit)
        {
            using namespace UnitTypes;

            if (unit.isWorker()) {
                if (Broodwar->self()->completedUnitCount(unit) < 90 && (!Resources::isMinSaturated() || !Resources::isGasSaturated()))
                    return true;
                else
                    return false;
            }

            if (unit.getRace() == Races::Zerg)
                return true;

            bool needReavers = false;
            bool needShuttles = false;

            // Determine whether we want reavers or shuttles;
            if (!Strategy::needDetection()) {
                if ((Terrain::isIslandMap() && vis(unit) < 2 * vis(UnitTypes::Protoss_Nexus))
                    || (vis(UnitTypes::Protoss_Reaver) > (vis(UnitTypes::Protoss_Shuttle) * 2))
                    || (Broodwar->mapFileName().find("Great Barrier") != string::npos && vis(UnitTypes::Protoss_Shuttle) < 1))
                    needShuttles = true;
                if (!Terrain::isIslandMap() || (vis(UnitTypes::Protoss_Reaver) <= (vis(UnitTypes::Protoss_Shuttle) * 2)))
                    needReavers = true;
            }

            // HACK: Want x reavers before a shuttle
            if (Players::vP() && vis(UnitTypes::Protoss_Reaver) < (2 + int(Strategy::getEnemyBuild() == "P4Gate")))
                needShuttles = false;

            //// No shuttles
            //needShuttles = false;

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
                return vis(unit) < 10 && (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) || Broodwar->self()->isResearching(TechTypes::Psionic_Storm));

                // Robo Units
            case Protoss_Shuttle:
                return needShuttles;
            case Protoss_Reaver:
                return needReavers;
            case Protoss_Observer:
                return vis(unit) < 1 + (Units::getSupply() / 100);

                // Stargate Units
            case Protoss_Corsair:
                return vis(unit) < (10 + (Terrain::isIslandMap() * 10));
            case Protoss_Scout:
                return true;
            case Protoss_Carrier:
                return true;
            case Protoss_Arbiter:
                return (vis(unit) < 10 && (Broodwar->self()->isUpgrading(UpgradeTypes::Khaydarin_Core) || Broodwar->self()->getUpgradeLevel(UpgradeTypes::Khaydarin_Core)));

                // Barracks Units
            case Terran_Marine:
                return true;
            case Terran_Firebat:
                return true;
            case Terran_Medic:
                return Broodwar->self()->completedUnitCount(unit) * 4 < Broodwar->self()->completedUnitCount(Terran_Marine);
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

            // If this is a specific unit upgrade, check if it's unlocked
            if (upgrade.whatUses().size() == 1) {
                for (auto &unit : upgrade.whatUses()) {
                    if (!BuildOrder::isUnitUnlocked(unit))
                        return false;
                }
            }

            // If this isn't the first tech/upgrade and we don't have our first tech/upgrade
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
                    return (vis(UnitTypes::Protoss_Assimilator) >= 4 && Broodwar->self()->hasResearched(TechTypes::Psionic_Storm) && Broodwar->self()->gas() >= 750);
                case Khaydarin_Core:
                    return true;

                    // Range upgrades
                case Singularity_Charge:
                    return vis(UnitTypes::Protoss_Dragoon) >= 1;

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
                    return vis(UnitTypes::Protoss_Shuttle) > 0;
                case Gravitic_Thrusters:
                    return vis(UnitTypes::Protoss_Scout) > 0;
                case Gravitic_Boosters:
                    return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
                case Leg_Enhancements:
                    return (vis(UnitTypes::Protoss_Nexus) >= 2);

                    // Ground unit upgrades
                case Protoss_Ground_Weapons:
                    return !Terrain::isIslandMap() && (Units::getSupply() > 120 || Players::getNumberZerg() > 0);
                case Protoss_Ground_Armor:
                    return !Terrain::isIslandMap() && (Broodwar->self()->getUpgradeLevel(Protoss_Ground_Weapons) > Broodwar->self()->getUpgradeLevel(Protoss_Ground_Armor) || Broodwar->self()->isUpgrading(Protoss_Ground_Weapons));
                case Protoss_Plasma_Shields:
                    return (Broodwar->self()->getUpgradeLevel(Protoss_Ground_Weapons) >= 2 && Broodwar->self()->getUpgradeLevel(Protoss_Ground_Armor) >= 2);

                    // Air unit upgrades
                case Protoss_Air_Weapons:
                    return (vis(UnitTypes::Protoss_Corsair) > 0 || vis(UnitTypes::Protoss_Scout) > 0 || (vis(Protoss_Stargate) > 0 && Players::vT()));
                case Protoss_Air_Armor:
                    return Broodwar->self()->getUpgradeLevel(Protoss_Air_Weapons) > Broodwar->self()->getUpgradeLevel(Protoss_Air_Armor);
                }
            }

            else if (Broodwar->self()->getRace() == Races::Terran) {
                switch (upgrade) {
                case Ion_Thrusters:
                    return true;
                case Charon_Boosters:
                    return Strategy::getUnitScore(UnitTypes::Terran_Goliath) > 1.00;
                case U_238_Shells:
                    return Broodwar->self()->hasResearched(TechTypes::Stim_Packs);
                case Terran_Infantry_Weapons:
                    return true;// (BuildOrder::isBioBuild());
                case Terran_Infantry_Armor:
                    return (Broodwar->self()->getUpgradeLevel(Terran_Infantry_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Infantry_Armor) || Broodwar->self()->isUpgrading(Terran_Infantry_Weapons));

                case Terran_Vehicle_Weapons:
                    return (Units::getGlobalAllyGroundStrength() > 20.0);
                case Terran_Vehicle_Plating:
                    return (Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Vehicle_Plating) || Broodwar->self()->isUpgrading(Terran_Vehicle_Weapons));
                case Terran_Ship_Weapons:
                    return (Units::getGlobalAllyAirStrength() > 20.0);
                case Terran_Ship_Plating:
                    return (Broodwar->self()->getUpgradeLevel(Terran_Ship_Weapons) > Broodwar->self()->getUpgradeLevel(Terran_Ship_Plating) || Broodwar->self()->isUpgrading(Terran_Ship_Weapons));
                }
            }

            else if (Broodwar->self()->getRace() == Races::Zerg) {
                switch (upgrade)
                {
                case Metabolic_Boost:
                    return true;
                case Grooved_Spines:
                    return true;
                case Muscular_Augments:
                    return Broodwar->self()->getUpgradeLevel(Grooved_Spines);
                case Pneumatized_Carapace:
                    return (Units::getSupply() > 160);
                case Anabolic_Synthesis:
                    return true;
                case Adrenal_Glands:
                    return true;

                    // Ground unit upgrades
                case Zerg_Melee_Attacks:
                    return (Units::getSupply() > 120);
                case Zerg_Missile_Attacks:
                    return false;
                case Zerg_Carapace:
                    return (Units::getSupply() > 120);

                    // Air unit upgrades
                case Zerg_Flyer_Attacks:
                    return (Units::getSupply() > 120);
                case Zerg_Flyer_Carapace:
                    return (Units::getSupply() > 120);
                }
            }
            return false;
        }

        bool isSuitable(TechType tech)
        {
            using namespace TechTypes;

            // Allow first upgrade
            if (tech == BuildOrder::getFirstTech() && !BuildOrder::firstReady())
                return true;

            // If this is a specific unit tech, check if it's unlocked
            if (tech.whatUses().size() == 1) {
                for (auto &unit : tech.whatUses()) {
                    if (!BuildOrder::isUnitUnlocked(unit))
                        return false;
                }
            }

            // If this isn't the first tech/upgrade and we don't have our first tech/upgrade
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
                    return (Broodwar->self()->minerals() > 1500 && Broodwar->self()->gas() > 1000);
                case Disruption_Web:
                    return (vis(UnitTypes::Protoss_Corsair) >= 10);
                }
            }

            else if (Broodwar->self()->getRace() == Races::Terran) {
                switch (tech) {
                case Stim_Packs:
                    return true;// BuildOrder::isBioBuild();
                case Spider_Mines:
                    return Broodwar->self()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) > 0 || Broodwar->self()->isUpgrading(UpgradeTypes::Ion_Thrusters);
                case Tank_Siege_Mode:
                    return Broodwar->self()->hasResearched(TechTypes::Spider_Mines) || Broodwar->self()->isResearching(TechTypes::Spider_Mines) || vis(UnitTypes::Terran_Siege_Tank_Tank_Mode) > 0;
                case Cloaking_Field:
                    return vis(UnitTypes::Terran_Wraith) >= 2;
                case Yamato_Gun:
                    return vis(UnitTypes::Terran_Battlecruiser) >= 0;
                case Personnel_Cloaking:
                    return vis(UnitTypes::Terran_Ghost) >= 2;
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

        void produce(UnitInfo& building)
        {
            int offset = 16;
            double best = 0.0;
            UnitType bestType = UnitTypes::None;
            for (auto &unit : building.getType().buildsWhat()) {

                double mineral = unit.mineralPrice() > 0 ? max(0.0, min(1.0, double(Broodwar->self()->minerals() - reservedMineral - Buildings::getQueuedMineral()) / (double)unit.mineralPrice())) : 1.0;
                double gas = unit.gasPrice() > 0 ? max(0.0, min(1.0, double(Broodwar->self()->gas() - reservedGas - Buildings::getQueuedGas()) / (double)unit.gasPrice())) : 1.0;
                double score = max(0.01, Strategy::getUnitScore(unit));
                double value = score * mineral * gas;

                if (unit.isAddon() && BuildOrder::getItemQueue().find(unit) != BuildOrder::getItemQueue().end() && BuildOrder::getItemQueue().at(unit).getActualCount() > vis(unit)) {
                    building.unit()->buildAddon(unit);
                    break;
                }

                // If we teched to DTs, try to create as many as possible
                if (unit == UnitTypes::Protoss_Dark_Templar && BuildOrder::getTechList().size() == 1 && isCreateable(building.unit(), unit) && isSuitable(unit)) {
                    best = DBL_MAX;
                    bestType = unit;
                }
                else if (unit == BuildOrder::getTechUnit() && isCreateable(building.unit(), unit) && isSuitable(unit) && vis(unit) == 0 && isAffordable(unit)) {
                    best = DBL_MAX;
                    bestType = unit;
                }
                else if (unit == UnitTypes::Protoss_Observer && isCreateable(building.unit(), unit) && isSuitable(unit) && vis(unit) < Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Nexus)) {
                    best = DBL_MAX;
                    bestType = unit;
                }
                else if (value >= best && isCreateable(building.unit(), unit) && isSuitable(unit) && (isAffordable(bestType) || Broodwar->getFrameCount() < 8000)) {
                    best = value;
                    bestType = unit;
                }
            }

            if (bestType != UnitTypes::None) {

                // If we can afford it, train it
                if (isAffordable(bestType)) {
                    building.unit()->train(bestType);
                    building.setRemainingTrainFrame(bestType.buildTime());
                    idleProduction.erase(building.unit());
                    return; // Only produce 1 unit per frame to prevent overspending
                }

                if (bestType == UnitTypes::Protoss_Dark_Templar && !isAffordable(UnitTypes::Protoss_Dark_Templar) && Players::vP() && Broodwar->self()->minerals() > 300)
                    bestType = UnitTypes::Protoss_Zealot;

                // Else if this is a tech unit, add it to idle production
                else if (BuildOrder::getTechUnit() == bestType || BuildOrder::getTechList().find(bestType) != BuildOrder::getTechList().end()) {
                    if (Units::getSupply() < 380)
                        idleFrame = Broodwar->getFrameCount();

                    idleProduction[building.unit()] = bestType;
                    reservedMineral += bestType.mineralPrice();
                    reservedGas += bestType.gasPrice();
                }
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
            reservedMineral = 0, reservedGas = 0;

            for (auto &b : idleProduction) {
                reservedMineral += b.second.mineralPrice();
                reservedGas += b.second.gasPrice();
            }

            for (auto &t : idleTech) {
                reservedMineral += t.second.mineralPrice();
                reservedGas += t.second.gasPrice();
            }

            for (auto &u : idleUpgrade) {
                reservedMineral += u.second.mineralPrice();
                reservedGas += u.second.gasPrice();
            }
            return;
        }

        void updateProduction()
        {
            for (auto &b : Units::getMyUnits()) {
                auto &building = b.second;

                if (!building.unit() || building.getRole() != Role::Producing || (!building.unit()->isCompleted() && !building.getType().isResourceDepot() && building.getType().getRace() != Races::Zerg) || Broodwar->getFrameCount() % Broodwar->getRemainingLatencyFrames() != 0)
                    continue;

                bool latencyIdle = building.getRemainingTrainFrames() < Broodwar->getRemainingLatencyFrames();

                if (latencyIdle && !building.getType().isResourceDepot()) {
                    idleProduction.erase(building.unit());
                    idleUpgrade.erase(building.unit());
                    idleTech.erase(building.unit());

                    produce(building);
                    research(building);
                    upgrade(building);
                }

                // CC/Nexus
                else if (building.getType().isResourceDepot() && latencyIdle) {
                    for (auto &unit : building.getType().buildsWhat()) {
                        if (unit.isAddon() && !building.unit()->getAddon() && BuildOrder::getItemQueue().find(unit) != BuildOrder::getItemQueue().end() && BuildOrder::getItemQueue().at(unit).getActualCount() > vis(unit)) {
                            building.unit()->buildAddon(unit);
                            continue;
                        }
                        if (!BuildOrder::isWorkerCut() && unit.isWorker() && Broodwar->self()->completedUnitCount(unit) < 75 && isAffordable(unit) && (!Resources::isGasSaturated() || !Resources::isMinSaturated())) {
                            building.unit()->train(unit);
                            building.setRemainingTrainFrame(unit.buildTime());
                        }
                    }
                };
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

    int getReservedMineral() { return reservedMineral; }
    int getReservedGas() { return reservedGas; }
    bool hasIdleProduction() { return Broodwar->getFrameCount() == idleFrame; }
    
    //auto needOverlords = Units::getMyTypeCount(UnitTypes::Zerg_Overlord) <= min(22, (int)floor((Units::getSupply() / max(14, 16 - Units::getMyTypeCount(UnitTypes::Zerg_Overlord)))));

    //for (auto &larva : building.unit()->getLarva()) {

    //    if (needOverlords)
    //        larva->morph(UnitTypes::Zerg_Overlord);
    //    else {
    //        double best = 0.0;
    //        UnitType typeBest;

    //        for (auto &type : Strategy::getUnitScores()) {
    //            UnitType unit = type.first;
    //            double mineral = unit.mineralPrice() > 0 ? max(0.0, min(1.0, double(Broodwar->self()->minerals() - reservedMineral - Buildings::getQueuedMineral()) / (double)unit.mineralPrice())) : 1.0;
    //            double gas = unit.gasPrice() > 0 ? max(0.0, min(1.0, double(Broodwar->self()->gas() - reservedGas - Buildings::getQueuedGas()) / (double)unit.gasPrice())) : 1.0;
    //            double score = max(0.01, Strategy::getUnitScore(unit));
    //            double value = score * mineral * gas;

    //            if (BuildOrder::isUnitUnlocked(type.first) && value > best && isCreateable(building.unit(), type.first) && (isAffordable(type.first) || type.first == Strategy::getHighestUnitScore()) && isSuitable(type.first)) {
    //                best = value;
    //                typeBest = type.first;
    //            }
    //        }

    //        if (typeBest != UnitTypes::None) {
    //            if (isAffordable(typeBest)) {
    //                larva->morph(typeBest);
    //                return;	// Only produce 1 unit per frame to allow for scores to be re-calculated
    //            }
    //            else if (BuildOrder::isTechUnit(typeBest)) {
    //                idleProduction[building.unit()] = typeBest;
    //            }
    //        }
    //    }
    //}
}