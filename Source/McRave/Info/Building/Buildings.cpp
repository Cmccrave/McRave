#include "Main/McRave.h"
#include "Main/Events.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Buildings {
    namespace {
        map <UnitType, int> morphedThisFrame;
        set<Position> larvaPositions;
        set<TilePosition> unpoweredPositions;

        bool willDieToAttacks(UnitInfo& building) {
            auto possibleDamage = 0;
            for (auto &a : building.getUnitsTargetingThis()) {
                if (auto attacker = a.lock()) {
                    if (attacker->isWithinRange(building))
                        possibleDamage+= int(attacker->getGroundDamage());
                }
            }

            return possibleDamage > building.getHealth() + building.getShields();
        };

        void updateInformation(UnitInfo& building)
        {
            // If a building is unpowered, get a pylon placement ready
            if (building.getType().requiresPsi() && !Pylons::hasPowerSoon(building.getTilePosition(), building.getType()))
                unpoweredPositions.insert(building.getTilePosition());

            if (Util::getTime() > Time(6, 00)) {
                auto range = int(max({ building.getGroundRange(), building.getAirRange(), double(building.getType().sightRange()) }));
                Zones::addZone(building.getPosition(), ZoneType::Defend, 1, range);
            }
        }

        void updateLarvaEncroachment(UnitInfo& building)
        {
            // Updates a list of larva positions that can block planning
            if (building.getType() == Zerg_Larva && !Producing::larvaTrickRequired(building)) {
                for (auto &[_, pos] : building.getPositionHistory())
                    larvaPositions.insert(pos);
            }
            if (building.getType() == Zerg_Egg || building.getType() == Zerg_Lurker_Egg)
                larvaPositions.insert(building.getPosition());
        }

        void cancel(UnitInfo& building)
        {
            auto isStation = BWEB::Stations::getClosestStation(building.getTilePosition())->getBase()->Location() == building.getTilePosition();

            // Cancelling refineries for our gas trick
            if (BuildOrder::isGasTrick() && building.getType().isRefinery() && !building.unit()->isCompleted() && BuildOrder::buildCount(building.getType()) < vis(building.getType())) {
                BWEB::Map::removeUsed(building.getTilePosition(), 4, 2);
                building.unit()->cancelMorph();
            }

            // Cancelling refineries we don't want
            if (building.getType().isRefinery() && Spy::enemyRush() && vis(building.getType()) == 1 && building.getType().getRace() != Races::Zerg && BuildOrder::buildCount(building.getType()) < vis(building.getType())) {
                building.unit()->cancelConstruction();
            }

            // Cancelling buildings that are being built/morphed but will be dead
            if (!building.unit()->isCompleted() && willDieToAttacks(building) && building.getType() != Zerg_Sunken_Colony && building.getType() != Zerg_Spore_Colony) {
                Events::onUnitCancelBecauseBWAPISucks(building);
                building.unit()->cancelConstruction();
            }


            auto naturalHatch = building.getType() == Zerg_Hatchery && isStation && Terrain::getMyNatural() && building.getTilePosition() == Terrain::getMyNatural()->getBase()->Location();
            auto cancelTiming = !BuildOrder::takeNatural() && Util::getTime() < Time(4, 00);
            auto cancelLow = (building.frameCompletesWhen() - Broodwar->getFrameCount() < 24 || building.getHealth() < 50);

            // Cancelling hatcheries if we're being proxy 2gated and took a 3rd
            if (naturalHatch && cancelTiming && cancelLow && vis(Zerg_Hatchery) >= 3 && Spy::getEnemyOpener() == "Proxy9/9") {
                Events::onUnitCancelBecauseBWAPISucks(building);
                building.unit()->cancelConstruction();
            }

            // Cancelling hatchery if against early pool
            auto earlyPool = Spy::getEnemyOpener() == "4Pool" || Spy::getEnemyOpener() == "7Pool";
            if (naturalHatch && cancelTiming && earlyPool) {
                Events::onUnitCancelBecauseBWAPISucks(building);
                building.unit()->cancelConstruction();
            }

            // Cancelling colonies we don't need now
            if (building.getType() == Zerg_Creep_Colony) {

            }
        }

        void morph(UnitInfo& building)
        {
            auto needLarvaSpending = vis(Zerg_Larva) > 3 && Broodwar->self()->supplyUsed() < Broodwar->self()->supplyTotal() && BuildOrder::getUnitReservation(Zerg_Mutalisk) == 0 && Util::getTime() < Time(4, 30) && com(Zerg_Sunken_Colony) > 2;
            auto morphType = UnitTypes::None;
            auto station = BWEB::Stations::getClosestStation(building.getTilePosition());
            auto wall = BWEB::Walls::getClosestWall(building.getPosition());
            auto plannedType = Planning::whatPlannedHere(building.getTilePosition());

            // Lair morphing
            if (building.getType() == Zerg_Hatchery && station && station->isMain() && !willDieToAttacks(building) && BuildOrder::buildCount(Zerg_Lair) > vis(Zerg_Lair) + vis(Zerg_Hive) + morphedThisFrame[Zerg_Lair] + morphedThisFrame[Zerg_Hive]) {
                if ((Util::getTime() >= Time(2, 31) && BuildOrder::getCurrentTransition().find("1Hatch") != string::npos)
                    || (Util::getTime() >= Time(3, 02) && BuildOrder::getCurrentTransition().find("2Hatch") != string::npos)
                    || Util::getTime() >= Time(3, 31)
                    || Players::ZvZ())
                    morphType = Zerg_Lair;
            }

            // Hive morphing
            else if (building.getType() == Zerg_Lair && !willDieToAttacks(building) && BuildOrder::buildCount(Zerg_Hive) > vis(Zerg_Hive) + morphedThisFrame[Zerg_Hive])
                morphType = Zerg_Hive;

            // Greater Spire morphing
            else if (building.getType() == Zerg_Spire && !willDieToAttacks(building) && BuildOrder::buildCount(Zerg_Greater_Spire) > vis(Zerg_Greater_Spire) + morphedThisFrame[Zerg_Greater_Spire])
                morphType = Zerg_Greater_Spire;

            // Sunken / Spore morphing
            else if (building.getType() == Zerg_Creep_Colony && !willDieToAttacks(building) && !needLarvaSpending) {
                auto wallDefense = wall && wall->getDefenses().find(building.getTilePosition()) != wall->getDefenses().end();
                auto stationDefense = station && (station->getDefenses().find(building.getTilePosition()) != station->getDefenses().end() || building.getTilePosition() == station->getPocketDefense());

                // If we planned already
                if (plannedType == Zerg_Sunken_Colony)
                    morphType = Zerg_Sunken_Colony;
                else if (plannedType == Zerg_Spore_Colony)
                    morphType = Zerg_Spore_Colony;

                // If this is a Station defense
                else if (stationDefense && Stations::needGroundDefenses(station) > morphedThisFrame[Zerg_Sunken_Colony] && com(Zerg_Spawning_Pool) > 0)
                    morphType = Zerg_Sunken_Colony;
                else if (stationDefense && Stations::needAirDefenses(station) > morphedThisFrame[Zerg_Spore_Colony] && com(Zerg_Evolution_Chamber) > 0)
                    morphType = Zerg_Spore_Colony;

                // If this is a Wall defense
                else if (wallDefense && Walls::needAirDefenses(*wall) > morphedThisFrame[Zerg_Spore_Colony] && plannedType == Zerg_Spore_Colony && com(Zerg_Evolution_Chamber) > 0)
                    morphType = Zerg_Spore_Colony;
                else if (wallDefense && Walls::needGroundDefenses(*wall) > morphedThisFrame[Zerg_Sunken_Colony] && plannedType == Zerg_Sunken_Colony && com(Zerg_Spawning_Pool) > 0)
                    morphType = Zerg_Sunken_Colony;
            }

            // Look for the closest possible non worker enemy
            if (plannedType == Zerg_Sunken_Colony && Spy::getEnemyBuild() != "CannonRush" && Spy::getEnemyBuild() != "WorkerRush") {
                auto closestThreat = Util::getClosestUnit(building.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    if (!u->getType().isWorker() && !u->getType().isBuilding() && !u->isFlying()) {
                        if (!Terrain::getEnemyMain())
                            return true;
                        const auto visDiff = Broodwar->getFrameCount() - u->getLastVisibleFrame();

                        // Check if we know they weren't at home and are missing on the map for 30 seconds
                        if (!Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), u->getPosition()))
                            return Time(u->frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, 30);
                        return false;
                    }
                    return false;
                });
                if (!closestThreat)
                    return;
            }

            // Morph
            if (morphType.isValid() && building.isCompleted()) {
                building.unit()->morph(morphType);
                morphedThisFrame[morphType]++;
            }
        }

        void reBuild(UnitInfo& building)
        {
            // Terran building needs new scv
            if (building.getType().getRace() == Races::Terran && !building.unit()->isCompleted() && !building.unit()->getBuildUnit()) {
                auto builder = Util::getClosestUnit(building.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u->getType().isWorker() && u->getBuildType() == None;
                });

                if (builder)
                    builder->unit()->rightClick(building.unit());
            }
        }

        void updateCommands(UnitInfo& building)
        {
            morph(building);
            cancel(building);
            reBuild(building);
        }
    }

    void onFrame()
    {
        // Reset counters
        larvaPositions.clear();
        morphedThisFrame.clear();
        unpoweredPositions.clear();

        // Update all my buildings
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            auto &unit = *u;

            if (unit.getRole() == Role::Production || unit.getRole() == Role::Defender) {
                updateLarvaEncroachment(unit);
                updateInformation(unit);
                updateCommands(unit);
            }
        }
    }

    set<TilePosition>& getUnpoweredPositions() { return unpoweredPositions; }
    set<Position>& getLarvaPositions() { return larvaPositions; }
}