#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Producing {

    namespace {
        map <Unit, UnitType> idleProduction;
        map <UnitType, int> trainedThisFrame;
        int reservedMineral, reservedGas;
        int lastProduceFrame = -999;

        void reset()
        {
            trainedThisFrame.clear();
            reservedMineral = 0;
            reservedGas = 0;
        }

        bool isAffordable(UnitType unit)
        {
            if (unit == Zerg_Overlord)
                return Broodwar->self()->minerals() >= unit.mineralPrice() + Planning::getPlannedMineral();

            auto selfMineral            = Broodwar->self()->minerals();
            auto selfGas                = Broodwar->self()->gas();
            auto selfSupply             = Players::getSupply(PlayerState::Self, unit.getRace());

            for (auto &[type, cnt] : trainedThisFrame) {
                selfMineral            -= type.mineralPrice() * cnt;
                selfGas                -= type.gasPrice() * cnt;
                selfSupply             += type.supplyRequired() * cnt;
            }

            auto mineralReserve         = (!BuildOrder::isTechUnit(unit) || Researching::hasIdleResearch() || Upgrading::hasIdleUpgrades()) ? reservedMineral + Researching::getReservedMineral() + Upgrading::getReservedMineral() : 0;
            auto gasReserve             = (!BuildOrder::isTechUnit(unit) || Researching::hasIdleResearch() || Upgrading::hasIdleUpgrades()) ? reservedGas + Researching::getReservedGas() + Upgrading::getReservedGas() : 0;
            auto mineralAffordable      = unit.mineralPrice() == 0 || (selfMineral >= unit.mineralPrice() + Planning::getPlannedMineral() + mineralReserve);
            auto gasAffordable          = unit.gasPrice() == 0 || (selfGas >= unit.gasPrice() + Planning::getPlannedGas() + gasReserve);
            auto supplyAffordable       = unit.supplyRequired() == 0 || (selfSupply + unit.supplyRequired() <= Broodwar->self()->supplyTotal());

            return mineralAffordable && gasAffordable && supplyAffordable;
        }

        bool isCreateable(Unit building, UnitType unit)
        {
            if (!BuildOrder::isUnitUnlocked(unit)
                || BuildOrder::getCompositionPercentage(unit) <= 0.0)
                return false;

            switch (unit)
            {
            case Enum::Protoss_Probe:
                return true;

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

            case Enum::Terran_SCV:
                return true;

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
                return com(Zerg_Spawning_Pool) > 0;
            case Enum::Zerg_Hydralisk:
                return com(Zerg_Hydralisk_Den) > 0;
            case Enum::Zerg_Mutalisk:
                return com(Zerg_Spire) > 0 || com(Zerg_Greater_Spire) > 0;
            case Enum::Zerg_Scourge:
                return com(Zerg_Spire) > 0 || com(Zerg_Greater_Spire) > 0;
            case Enum::Zerg_Defiler:
                return com(Zerg_Defiler_Mound) > 0;
            case Enum::Zerg_Ultralisk:
                return com(Zerg_Ultralisk_Cavern) > 0;
            }
            return false;
        }       

        bool isSuitable(UnitType unit)
        {
            if (unit.isWorker())
                return vis(unit) < 70 && (!Resources::isMineralSaturated() || !Resources::isGasSaturated());

            if (unit.getRace() == Races::Zerg) {
                if (unit == Zerg_Defiler)
                    return vis(unit) < 4;
                return true;
            }

            // Determine whether we want reavers or shuttles
            bool needReavers = false;
            bool needShuttles = false;
            if (!Spy::enemyInvis() && BuildOrder::isTechUnit(Protoss_Reaver)) {
                if ((Terrain::isIslandMap() && vis(unit) < 2 * vis(Protoss_Nexus))
                    || vis(Protoss_Shuttle) == 0
                    || vis(Protoss_Reaver) > vis(Protoss_Shuttle) * 2
                    || vis(Protoss_High_Templar) > vis(Protoss_Shuttle) * 4)
                    needShuttles = true;
                if (!Terrain::isIslandMap() || (vis(Protoss_Reaver) <= (vis(Protoss_Shuttle) * 2)))
                    needReavers = true;
            }

            // Determine our templar caps
            auto htCap = min(2 * (Players::getSupply(PlayerState::Self, Races::Protoss) / 100), Players::vP() ? 4 : 8);

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
                return (vis(unit) < 2 + (Players::getSupply(PlayerState::Self, Races::Protoss) / 200) && vis(unit) <= com(unit) + 2) || scoreUnit(UnitTypes::Protoss_Archon) > scoreUnit(UnitTypes::Protoss_High_Templar);

                // Robo Units
            case Protoss_Shuttle:
                return needShuttles;
            case Protoss_Reaver:
                return needReavers;
            case Protoss_Observer:
                return (Util::getTime() > Time(10, 0) || Spy::enemyInvis()) ? vis(unit) < 1 + (Players::getSupply(PlayerState::Self, Races::Protoss) / 100) : vis(unit) < 1;

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
                return vis(unit) < 1 && vis(Terran_Nuclear_Missile) > 0;
            case Terran_Nuclear_Missile:
                return true;

                // Factory Units
            case Terran_Vulture:
                return true;
            case Terran_Siege_Tank_Tank_Mode:
                return true;
            case Terran_Goliath:
                return true;

                // Starport Units
            case Terran_Wraith:
                return false;
            case Terran_Valkyrie:
                return false;
            case Terran_Battlecruiser:
                return true;
            case Terran_Science_Vessel:
                return vis(unit) < 6;
            case Terran_Dropship:
                return vis(unit) <= 0;
            }
            return false;
        }

        bool addon(UnitInfo& building)
        {
            for (auto &unit : building.getType().buildsWhat()) {
                if (unit.isAddon() && BuildOrder::buildCount(unit) > vis(unit)) {
                    building.unit()->buildAddon(unit);
                    return true;
                }
            }
            return false;
        }

        bool produce(UnitInfo& building)
        {
            auto offset = 16;
            auto best = 0.0;
            auto bestType = None;

            // Clear idle checks
            auto idleItr = idleProduction.find(building.unit());
            if (idleItr != idleProduction.end()) {
                reservedMineral -= idleItr->second.mineralPrice();
                reservedGas -= idleItr->second.gasPrice();
                idleProduction.erase(building.unit());
            }

            // Choose an Overlord if we need one
            if (building.getType() == Zerg_Larva && BuildOrder::buildCount(Zerg_Overlord) > vis(Zerg_Overlord) + trainedThisFrame[Zerg_Overlord]) {
                if (isAffordable(Zerg_Overlord)) {
                    building.unit()->morph(Zerg_Overlord);
                    building.setRemainingTrainFrame(bestType.buildTime());
                    trainedThisFrame[Zerg_Overlord]++;
                    lastProduceFrame = Broodwar->getFrameCount();
                }
                return true;
            }

            // Look through each UnitType this can train
            for (auto &type : building.getType().buildsWhat()) {

                if (!isCreateable(building.unit(), type)
                    || !isSuitable(type))
                    continue;

                const auto value = scoreUnit(type);
                if (value > best) {
                    best = value;
                    bestType = type;
                }
            }

            if (bestType != None) {

                // If we can afford it, train it
                if (isAffordable(bestType)) {
                    trainedThisFrame[bestType]++;
                    building.unit()->train(bestType);
                    building.setRemainingTrainFrame(bestType.buildTime());
                    idleProduction.erase(building.unit());
                    lastProduceFrame = Broodwar->getFrameCount();
                    return true;
                }

                // Else if this is a tech unit, add it to idle production
                else if (BuildOrder::isTechUnit(bestType) && Broodwar->self()->getRace() != Races::Zerg && (Workers::getMineralWorkers() > 0 || Broodwar->self()->minerals() >= bestType.mineralPrice()) && (Workers::getGasWorkers() > 0 || Broodwar->self()->gas() >= bestType.gasPrice())) {
                    trainedThisFrame[bestType]++;
                    idleProduction[building.unit()] = bestType;
                    reservedMineral += bestType.mineralPrice();
                    reservedGas += bestType.gasPrice();
                }

                // Else store a zero value idle
                else
                    idleProduction[building.unit()] = None;
            }
            return false;
        }

        void updateReservedResources()
        {
            // Reserved resources for idle production
            for (auto &[unit, type] : idleProduction) {
                if (BuildOrder::isTechUnit(type) && unit->exists()) {
                    reservedMineral += type.mineralPrice();
                    reservedGas += type.gasPrice();
                }
            }
        }

        void updateLarva()
        {
            for (int i = 0; i < vis(Zerg_Larva); i++) {

                // Find the best UnitType
                auto best = 0.0;
                auto bestType = None;

                // Choose an Overlord if we need one
                if (BuildOrder::buildCount(Zerg_Overlord) > vis(Zerg_Overlord) + trainedThisFrame[Zerg_Overlord])
                    bestType = Zerg_Overlord;
                else {
                    for (auto &type : Zerg_Larva.buildsWhat()) {
                        if (!isCreateable(nullptr, type)
                            || !isSuitable(type))
                            continue;

                        const auto value = scoreUnit(type);
                        if (value >= best) {
                            best = value;
                            bestType = type;
                        }
                    }
                }

                auto validLarva = [&](UnitInfo &larva, double saturation, BWEB::Station * station) {
                    if (!larva.unit()
                        || larva.getType() != Zerg_Larva
                        || larva.getRole() != Role::Production
                        || (bestType == Zerg_Overlord && !station->isNatural() && Players::ZvP() && Util::getTime() > Time(4, 00) && Util::getTime() < Time(7, 00) && Spy::getEnemyTransition() == "Corsair")
                        || ((bestType == Zerg_Overlord || bestType.isWorker()) && larva.isProxy())
                        || (larva.isProxy() && bestType != Zerg_Hydralisk && BuildOrder::getCurrentTransition().find("Lurker") != string::npos)
                        || (!larva.isProxy() && BuildOrder::isProxy() && bestType == Zerg_Hydralisk)
                        || !larva.unit()->getHatchery()
                        || !larva.unit()->isCompleted()
                        || larva.getRemainingTrainFrames() >= Broodwar->getLatencyFrames()
                        || lastProduceFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4
                        || (Planning::overlapsPlan(larva, larva.getPosition()) && Util::getTime() > Time(4, 00)))
                        return false;

                    auto closestStation = Stations::getClosestStationAir(larva.getPosition(), PlayerState::Self);
                    return station == closestStation;
                };

                // Find a station that we should morph at based on the UnitType we want.
                auto produced = false;
                multimap<double, BWEB::Station*> stations;
                for (auto &[val, station] : Stations::getStationsBySaturation()) {
                    auto saturation = bestType.isWorker() ? val : 1.0 / val;
                    auto larvaCount = count_if(Units::getUnits(PlayerState::Self).begin(), Units::getUnits(PlayerState::Self).end(), [&](auto &u) {
                        auto &unit = *u;
                        return unit.getType() == Zerg_Larva && unit.unit()->getHatchery() && unit.unit()->getHatchery()->getTilePosition() == station->getBase()->Location();
                    });
                    if (larvaCount >= 3 && bestType != Zerg_Drone)
                        saturation = 1.0 / 24.0;

                    stations.emplace(saturation * larvaCount, station);
                }

                for (auto &[val, station] : stations) {
                    for (auto &u : Units::getUnits(PlayerState::Self)) {
                        UnitInfo &larva = *u;
                        if (!larvaTrickRequired(larva)) {
                            if (validLarva(larva, val, station)) {
                                produce(larva);
                                produced = true;
                            }
                            else if (larvaTrickOptional(larva))
                                continue;
                            if (produced)
                                return;
                        }
                    }
                }
            }
        }

        void updateProduction()
        {
            const auto commands ={ addon, produce };
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &building = *u;

                if (!building.unit()
                    || building.getRole() != Role::Production
                    || !building.unit()->isCompleted()
                    || building.getRemainingTrainFrames() >= Broodwar->getLatencyFrames()
                    || lastProduceFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4
                    || Upgrading::upgradedThisFrame()
                    || Researching::researchedThisFrame()
                    || Producing::producedThisFrame()
                    || building.getType() == Zerg_Larva)
                    continue;

                // Iterate commmands and break if we execute one
                for (auto &command : commands) {
                    if (command(building))
                        break;
                }
            }
        }
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        reset();
        updateReservedResources();
        updateLarva();
        updateProduction();
        Visuals::endPerfTest("Production");
    }

    bool producedThisFrame() {
        return lastProduceFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4;
    }

    double scoreUnit(UnitType type)
    {
        if (BuildOrder::getTechUnit() == type && Broodwar->self()->getRace() != Races::Zerg)
            return DBL_MAX;

        // Check if we are saving larva but not for this type
        if (BuildOrder::getUnitReservation(type) == 0 && (BuildOrder::getUnitReservation(Zerg_Mutalisk) > 0 || BuildOrder::getUnitReservation(Zerg_Hydralisk) > 0)) {
            auto larvaMinCost = (BuildOrder::getTechUnit().mineralPrice() * BuildOrder::getUnitReservation(Zerg_Mutalisk))
                + (Zerg_Hydralisk.mineralPrice() * BuildOrder::getUnitReservation(Zerg_Hydralisk))
                + (Zerg_Scourge.mineralPrice() * BuildOrder::getUnitReservation(Zerg_Scourge));

            auto larvaGasCost = (Zerg_Mutalisk.gasPrice() * BuildOrder::getUnitReservation(Zerg_Mutalisk))
                + (Zerg_Hydralisk.gasPrice() * BuildOrder::getUnitReservation(Zerg_Hydralisk))
                + (Zerg_Scourge.gasPrice() * BuildOrder::getUnitReservation(Zerg_Scourge));

            auto larvaRequirements = BuildOrder::getUnitReservation(Zerg_Mutalisk) + BuildOrder::getUnitReservation(Zerg_Hydralisk) + (BuildOrder::getUnitReservation(Zerg_Scourge) / 2);

            if ((type != Zerg_Overlord && vis(Zerg_Larva) <= larvaRequirements)
                || Broodwar->self()->minerals() - type.mineralPrice() < larvaMinCost
                || Broodwar->self()->gas() - type.gasPrice() < larvaGasCost)
                return 0.0;
        }

        auto percentage = BuildOrder::getCompositionPercentage(type);
        auto trainedCount = vis(type) + trainedThisFrame[type];

        auto typeMineralCost = Math::realisticMineralCost(type);
        auto typeGasCost = Math::realisticGasCost(type);

        for (auto &idleType : idleProduction) {
            if (type == idleType.second)
                trainedCount++;
        }

        for (auto &secondType : type.buildsWhat()) {
            if (!secondType.isBuilding()) {
                trainedCount += vis(secondType) + trainedThisFrame[secondType];
                typeMineralCost += (int)round(BuildOrder::getCompositionPercentage(secondType) * BuildOrder::getCompositionPercentage(type) * Math::realisticMineralCost(secondType));
                typeGasCost += (int)round(BuildOrder::getCompositionPercentage(secondType) * BuildOrder::getCompositionPercentage(type) * Math::realisticGasCost(secondType));
            }
        }

        auto mineralCost = (Broodwar->self()->minerals() == 0 || typeMineralCost == 0) ? 1.0 : max(1.0, double((Broodwar->self()->minerals() * 2) - typeMineralCost - reservedMineral)) / double(Broodwar->self()->minerals());
        auto gasCost = (Broodwar->self()->gas() == 0 || typeGasCost == 0) ? 1.0 : max(1.0, double((Broodwar->self()->gas() * 2) - typeGasCost - reservedGas)) / double(Broodwar->self()->gas());

        // Can't make them if we aren't mining and can't afford
        if ((Workers::getGasWorkers() == 0 && typeGasCost > 0 && Broodwar->self()->gas() < typeGasCost)
            || (Workers::getMineralWorkers() == 0 && typeMineralCost > 0 && Broodwar->self()->minerals() < typeMineralCost))
            return 0.0;

        const auto resourceScore = gasCost * mineralCost;
        const auto strategyScore = 100.0 * percentage / double(max(1, trainedCount));
        return resourceScore + strategyScore;
    }

    bool larvaTrickRequired(UnitInfo& larva) {
        if (larva.getType() == Zerg_Larva && larva.unit()->getHatchery() && larva.getRemainingTrainFrames() <= 0) {
            auto closestStation = Stations::getClosestStationAir(larva.unit()->getHatchery()->getPosition(), PlayerState::Self);
            if (!closestStation)
                return false;

            auto mustMoveToLeft = Planning::overlapsPlan(larva, larva.getPosition());
            auto canMove = (larva.getPosition().y - 16.0 > larva.unit()->getHatchery()->getPosition().y || larva.getPosition().x + 24 > larva.unit()->getHatchery()->getPosition().x);
            if (canMove && mustMoveToLeft) {
                if (larva.unit()->getLastCommand().getType() != UnitCommandTypes::Stop)
                    larva.unit()->stop();
                return true;
            }
        }
        return false;
    };

    bool larvaTrickOptional(UnitInfo& larva) {
        if (larva.getType() == Zerg_Larva && larva.unit()->getHatchery() && larva.getRemainingTrainFrames() <= 0) {
            auto closestStation = Stations::getClosestStationAir(larva.unit()->getHatchery()->getPosition(), PlayerState::Self);
            if (!closestStation)
                return false;

            auto tryMoveToLeft = closestStation->getResourceCentroid().y + 64.0 < closestStation->getBase()->Center().y || closestStation->getResourceCentroid().x < closestStation->getBase()->Center().x;
            auto canMove = (larva.getPosition().y - 16.0 > larva.unit()->getHatchery()->getPosition().y || larva.getPosition().x + 24 > larva.unit()->getHatchery()->getPosition().x);
            if (canMove && tryMoveToLeft) {
                if (larva.unit()->getLastCommand().getType() != UnitCommandTypes::Stop)
                    larva.unit()->stop();
                return true;
            }
        }
        return false;
    };

    int getReservedMineral() { return reservedMineral; }
    int getReservedGas() { return reservedGas; }
    bool hasIdleProduction() { return int(idleProduction.size()) > 0; }
}