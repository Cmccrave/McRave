#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Producing {

    namespace {
        map <Unit, UnitType> idleProduction;
        int reservedMineral, reservedGas;
        int lastProduceFrame = -999;
        int availMin, availGas;

        string scoreThisFrame;
        string nodeName = "[Production]: ";

        void reset()
        {
            idleProduction.clear();
            reservedMineral = 0;
            reservedGas = 0;
            scoreThisFrame = "";
        }

        bool isAffordable(UnitType unit)
        {
            if (unit == Zerg_Overlord)
                return Broodwar->self()->minerals() >= unit.mineralPrice() + Planning::getPlannedMineral();

            auto availSupply            = Players::getSupply(PlayerState::Self, unit.getRace());

            auto mineralAffordable      = unit.mineralPrice() == 0 || (availMin >= unit.mineralPrice());
            auto gasAffordable          = unit.gasPrice() == 0 || (availGas >= unit.gasPrice());
            auto supplyAffordable       = unit.supplyRequired() == 0 || (availSupply + unit.supplyRequired() <= Broodwar->self()->supplyTotal());

            return mineralAffordable && gasAffordable && supplyAffordable;
        }

        bool isCreateable(Unit building, UnitType unit)
        {
            // Whether something is creatable is determined mostly by ordering it up in the build order, this checks edge cases
            if (unit == Terran_Siege_Tank_Tank_Mode || unit == Terran_Valkyrie || unit == Terran_Battlecruiser || unit == Terran_Science_Vessel || unit == Terran_Dropship)
                return building->getAddon() != nullptr;
            return true;
        }

        bool isSuitable(UnitType unit)
        {
            // 
            if (!BuildOrder::isUnitUnlocked(unit))
                return false;
            return true;
        }

        bool addon(UnitInfo& building, UnitType type)
        {
            for (auto &unit : building.getType().buildsWhat()) {
                if (unit.isAddon() && BuildOrder::buildCount(unit) > vis(unit)) {
                    building.unit()->buildAddon(unit);
                    return true;
                }
            }
            return false;
        }

        bool produce(UnitInfo& building, UnitType type)
        {
            if (type == None)
                return false;

            // If we can afford it, train it
            if (isAffordable(type)) {
                building.unit()->train(type);
                building.setRemainingTrainFrame(type.buildTime());
                idleProduction.erase(building.unit());
                lastProduceFrame = Broodwar->getFrameCount();
                Util::debug(nodeName + scoreThisFrame);
                return true;
            }

            // Else if this is a tech unit, add it to idle production
            else if (BuildOrder::isFocusUnit(type) && (Workers::getMineralWorkers() > 0 || Broodwar->self()->minerals() >= type.mineralPrice()) && (Workers::getGasWorkers() > 0 || Broodwar->self()->gas() >= type.gasPrice())) {
                idleProduction[building.unit()] = type;
                availMin -= type.mineralPrice();
                availGas -= type.gasPrice();
            }

            // Else store a zero value idle
            else
                idleProduction[building.unit()] = None;

            return false;
        }

        void updateReservedResources()
        {
            // We're allowed to spend up to our limits before buildings, upgrades and research
            availMin = Broodwar->self()->minerals() - Researching::getReservedMineral() - Upgrading::getReservedMineral() - Planning::getPlannedMineral();
            availGas = Broodwar->self()->gas() - Researching::getReservedGas() - Upgrading::getReservedGas() - Planning::getPlannedGas();
        }

        void updateLarva()
        {
            // Find the best UnitType
            auto best = 0.0;
            UnitType bestType = None;

            // Rules for choosing a valid larva
            auto validLarva = [&](UnitInfo &larva, double saturation, const BWEB::Station * station) {
                if (!larva.unit()
                    || larva.getType() != Zerg_Larva
                    || larva.getRole() != Role::Production
                    || (larva.isProxy() && bestType != Zerg_Hydralisk && BuildOrder::getCurrentTransition().find("Lurker") != string::npos)
                    || (!larva.isProxy() && BuildOrder::isProxy() && bestType == Zerg_Hydralisk)
                    || !larva.unit()->getHatchery()
                    || !larva.unit()->isCompleted()
                    || lastProduceFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4
                    || (Planning::overlapsPlan(larva, larva.getPosition()) && Util::getTime() > Time(4, 00)))
                    return false;

                // Strategic checks
                if (Players::ZvP() && Stations::ownedBy(Terrain::getMyNatural()) == PlayerState::Self) {
                    auto time = (Spy::getEnemyBuild() != "FFE") ? Time(4, 00) : Time(6, 00);
                    if (bestType == Zerg_Overlord && !station->isNatural() && Util::getTime() > time && Players::getTotalCount(PlayerState::Self, Zerg_Mutalisk) == 0 && Players::getTotalCount(PlayerState::Self, Zerg_Scourge) == 0)
                        return false;
                }

                auto closestStation = Stations::getClosestStationAir(larva.getPosition(), PlayerState::Self);
                return station == closestStation;
            };

            // Find the best type to train right now
            for (auto &type : Zerg_Larva.buildsWhat()) {
                if (!isCreateable(nullptr, type)
                    || !isSuitable(type))
                    continue;

                const auto value = scoreUnit(type);
                scoreThisFrame += "{" + string(type.c_str()) + ", " + to_string(value) + "}, ";

                if (value >= best) {
                    best = value;
                    bestType = type;
                }
            }

            // Find a station that we should morph at based on the UnitType we want.
            auto produced = false;
            multimap<double, const BWEB::Station *> stations;
            for (auto &[val, station] : Stations::getStationsBySaturation()) {
                auto saturation = bestType.isWorker() ? val : 1.0 / val;
                auto larvaCount = (int)count_if(Units::getUnits(PlayerState::Self).begin(), Units::getUnits(PlayerState::Self).end(), [&](auto &u) {
                    auto &unit = *u;
                    return unit.getType() == Zerg_Larva && unit.unit()->getHatchery() && unit.unit()->getHatchery()->getTilePosition() == station->getBase()->Location();
                });
                auto totalLarva = Players::getTotalCount(PlayerState::Self, Zerg_Larva);

                // Try to just use the larvas immediately, queue first
                if (larvaCount >= 3)
                    saturation = 0.01;

                // Try not to queue drones at high saturation
                if (larvaCount > 1 && totalLarva <= 4 && !BuildOrder::isOpener() && bestType.isWorker() && saturation >= 1.6)
                    continue;

                stations.emplace(saturation * (double)larvaCount, station);
            }

            for (auto &[val, station] : stations) {
                for (auto &u : Units::getUnits(PlayerState::Self)) {
                    UnitInfo &larva = *u;
                    if (larva.getType() == Zerg_Larva && !larvaTrickRequired(larva)) {
                        if (validLarva(larva, val, station)) {
                            produced = produce(larva, bestType);
                        }
                        else if (larvaTrickOptional(larva))
                            continue;
                        if (produced)
                            return;
                    }
                }
            }
        }

        void updateProduction()
        {
            UnitType bestType = None;

            // TODO: Pass the addon type we want instead of it implicitly looking
            const auto commands ={ addon, produce };
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &building = *u;

                if (!building.unit()
                    || building.getRole() != Role::Production
                    || !building.unit()->isCompleted()
                    || building.getRemainingTrainFrames() >= Broodwar->getLatencyFrames() + 1
                    || lastProduceFrame >= Broodwar->getFrameCount() - Broodwar->getLatencyFrames() - 4
                    || Upgrading::upgradedThisFrame()
                    || Researching::researchedThisFrame()
                    || Producing::producedThisFrame()
                    || building.getType() == Zerg_Larva)
                    continue;

                // Look through each UnitType this can train
                auto best = 0.0;
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

                // Iterate commmands and break if we execute one
                for (auto &command : commands) {
                    if (command(building, bestType))
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
        // If we need an Overlord, it's highest priority
        if (type == Zerg_Overlord) {
            if (BuildOrder::buildCount(Zerg_Overlord) > vis(Zerg_Overlord))
                return 500.0;
            return -1.0;
        }

        // Check if we are saving larva but not for this type
        if (BuildOrder::getUnitReservation(type) == 0 && (BuildOrder::getUnitReservation(Zerg_Scourge) > 0 || BuildOrder::getUnitReservation(Zerg_Mutalisk) > 0 || BuildOrder::getUnitReservation(Zerg_Hydralisk) > 0)) {
            auto larvaMinCost = (Zerg_Mutalisk.mineralPrice() * BuildOrder::getUnitReservation(Zerg_Mutalisk))
                + (Zerg_Hydralisk.mineralPrice() * BuildOrder::getUnitReservation(Zerg_Hydralisk))
                + (Zerg_Scourge.mineralPrice() * BuildOrder::getUnitReservation(Zerg_Scourge));

            auto larvaGasCost = (Zerg_Mutalisk.gasPrice() * BuildOrder::getUnitReservation(Zerg_Mutalisk))
                + (Zerg_Hydralisk.gasPrice() * BuildOrder::getUnitReservation(Zerg_Hydralisk))
                + (Zerg_Scourge.gasPrice() * BuildOrder::getUnitReservation(Zerg_Scourge));

            auto larvaRequirements = BuildOrder::getUnitReservation(Zerg_Mutalisk) + BuildOrder::getUnitReservation(Zerg_Hydralisk) + BuildOrder::getUnitReservation(Zerg_Scourge);

            if ((type != Zerg_Overlord && vis(Zerg_Larva) <= larvaRequirements)
                || (type.mineralPrice() > 0 && Broodwar->self()->minerals() - type.mineralPrice() < larvaMinCost)
                || (type.gasPrice() > 0 && Broodwar->self()->gas() - type.gasPrice() < larvaGasCost))
                return -1.0;
        }

        // Figure out how many we're trying to make
        auto trainedCount = vis(type);
        for (auto &[_, idleType] : idleProduction) {
            if (idleType == type)
                trainedCount++;
        }

        // Get the cost of making this type
        auto typeMineralCost = Math::realisticMineralCost(type);
        auto typeGasCost = Math::realisticGasCost(type);
        for (auto &secondType : type.buildsWhat()) {
            if (!secondType.isBuilding()) {
                trainedCount += vis(secondType);
                typeMineralCost += (int)round(BuildOrder::getCompositionPercentage(secondType) * BuildOrder::getCompositionPercentage(type) * Math::realisticMineralCost(secondType));
                typeGasCost += (int)round(BuildOrder::getCompositionPercentage(secondType) * BuildOrder::getCompositionPercentage(type) * Math::realisticGasCost(secondType));
            }
        }

        auto minCostScore = (Broodwar->self()->minerals() == 0 || typeMineralCost == 0) ? 1.0 : max(1.0, double(availMin * 2 - typeMineralCost - reservedMineral)) / double(Broodwar->self()->minerals());
        auto gasCostScore = (Broodwar->self()->gas() == 0 || typeGasCost == 0) ? 1.0 : max(1.0, double(availGas * 2 - typeGasCost - reservedGas)) / double(Broodwar->self()->gas());

        // Can't make them if we aren't mining and can't afford
        if ((Workers::getGasWorkers() == 0 && typeGasCost > 0 && Broodwar->self()->gas() < typeGasCost)
            || (Workers::getMineralWorkers() == 0 && typeMineralCost > 0 && Broodwar->self()->minerals() < typeMineralCost))
            return -1.0;

        // If we can't even afford 50% of the gas cost, then we shouldn't bother
        if ((typeGasCost > 0 && double(Broodwar->self()->gas()) / double(typeGasCost) < 0.5)
            || (typeGasCost > 0 && availGas < 0))
            return -1.0;

        const auto percentage = BuildOrder::getCompositionPercentage(type);
        const auto resourceScore = gasCostScore * minCostScore;
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