#include "McRave.h"
#include "EventManager.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Buildings {
    namespace {

        int queuedMineral, queuedGas;
        int poweredSmall, poweredMedium, poweredLarge;

        map <UnitType, int> morphedThisFrame;
        map <TilePosition, UnitType> buildingsPlanned;
        set<TilePosition> plannedGround, plannedAir;
        vector<BWEB::Block> unpoweredBlocks;
        set<TilePosition> validDefenses;
        set<Position> larvaEncroaches;
        bool expansionPlanned = false;

        struct ExpandPlan {
            vector<BWEB::Station*> bestOrder;
            map<BWEB::Station *, map<BWEB::Station*, BWEB::Path>> pathNetwork;
            BWEB::Station* lastExpansion = nullptr;
            BWEB::Station* currentExpansion = nullptr;
            BWEB::Station* nextExpansion = nullptr;
            bool expansionPlanned = false;
            bool blockersExists = false;
            map<BWEB::Station*, vector<weak_ptr<UnitInfo>>> blockingNeutrals;
            vector<BWEB::Station*> dangerousStations, islandStations;
        };
        ExpandPlan myExpandPlan;

        bool overlapsLarvaHistory(UnitType building, TilePosition here)
        {
            auto center = Position(here) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
            for (auto &pos : larvaEncroaches) {
                if (!pos.isValid())
                    continue;
                if (Util::boxDistance(Zerg_Larva, pos, building, center) <= 0)
                    return true;
                if (Util::boxDistance(Zerg_Egg, pos, building, center) <= 0)
                    return true;
            }
            return false;
        }

        bool isDefensiveType(UnitType building)
        {
            return building == Protoss_Photon_Cannon
                || building == Protoss_Shield_Battery
                || building == Zerg_Creep_Colony
                || building == Zerg_Sunken_Colony
                || building == Zerg_Spore_Colony
                || building == Zerg_Spire
                || building == Terran_Missile_Turret
                || building == Terran_Bunker;
        }

        bool isProductionType(UnitType building)
        {
            return building == Protoss_Gateway
                || building == Protoss_Robotics_Facility
                || building == Protoss_Stargate
                || building == Terran_Barracks
                || building == Terran_Factory
                || building == Terran_Starport
                || building == Zerg_Hatchery;
        }

        bool isTechType(UnitType building)
        {
            return building == Protoss_Forge
                || building == Protoss_Cybernetics_Core
                || building == Protoss_Robotics_Support_Bay
                || building == Protoss_Observatory
                || building == Protoss_Citadel_of_Adun
                || building == Protoss_Templar_Archives
                || building == Protoss_Fleet_Beacon
                || building == Protoss_Arbiter_Tribunal
                || building == Terran_Supply_Depot
                || building == Terran_Engineering_Bay
                || building == Terran_Academy
                || building == Terran_Armory
                || building == Terran_Science_Facility
                || building == Zerg_Spawning_Pool
                || building == Zerg_Evolution_Chamber
                || building == Zerg_Hydralisk_Den
                || building == Zerg_Spire
                || building == Zerg_Queens_Nest
                || building == Zerg_Defiler_Mound
                || building == Zerg_Ultralisk_Cavern;
        }

        bool isWallType(UnitType building)
        {
            return building == Protoss_Forge
                || building == Protoss_Gateway
                || building == Protoss_Pylon
                || building == Terran_Barracks
                || building == Terran_Supply_Depot
                || building == Zerg_Hydralisk_Den
                || building == Zerg_Evolution_Chamber
                || building == Zerg_Hatchery;
        }

        bool isPathable(UnitType building, TilePosition here) {
            const auto closestChoke = Util::getClosestChokepoint(Position(here));
            if (!closestChoke || isDefensiveType(building))
                return true;

            const auto insideBlock = [&](TilePosition t) {
                return Util::rectangleIntersect(Position(here), Position(here) + Position(building.tileSize()), Position(t));
            };

            for (int x = here.x - 1; x < here.x + building.tileWidth() + 1; x++) {
                for (int y = here.y - 1; y < here.y + building.tileHeight() + 1; y++) {
                    TilePosition tile = TilePosition(x, y);
                    if (!BWEB::Map::isWalkable(tile, Zerg_Ultralisk) || BWEB::Map::isUsed(tile) != UnitTypes::None)
                        continue;
                    if (insideBlock(tile))
                        continue;

                    BWEB::Path path(Position(tile), Position(closestChoke->Center()), Zerg_Ultralisk, false, true);
                    path.generateJPS([&](auto t) { return path.unitWalkable(t) && !insideBlock(t); });
                    if (!path.isReachable())
                        return false;
                }
            }
            return true;
        }

        bool isPlannable(UnitType building, TilePosition here)
        {
            // Check if we have specifically marked this position invalid
            if (isDefensiveType(building) && validDefenses.find(here) == validDefenses.end())
                return false;

            // Check if there's a building queued there already
            for (auto &queued : buildingsPlanned) {
                if (queued.first == here)
                    return false;
            }
            return true;
        }

        bool isBuildable(UnitType building, TilePosition here)
        {
            auto center = Position(here) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
            auto creepFully = true;

            if (building.requiresCreep()) {
                for (int x = here.x; x < here.x + building.tileWidth(); x++) {
                    for (int y = here.y; y < here.y + building.tileHeight(); y++) {
                        TilePosition t(x, y);
                        if (!Broodwar->hasCreep(t))
                            creepFully = false;
                    }
                }
            }

            // See if it's being blocked
            auto closestEnemy = Util::getClosestUnit(center, PlayerState::Enemy, [&](auto &u) {
                return !u.isFlying();
            });
            if (closestEnemy && Util::boxDistance(closestEnemy->getType(), closestEnemy->getPosition(), building, center) < 32.0)
                return false;            

            // Check to see if we expect creep to be here soon
            auto creepSoon = false;
            auto distCheck = 110.0;
            if (!creepFully && building.requiresCreep()) {
                auto closestStation = Stations::getClosestStationGround(PlayerState::Self, Position(here));
                if (closestStation) {

                    if (Players::ZvP() && Strategy::getEnemyOpener() != "10/17" && (here.y > closestStation->getBase()->Location().y + 1 || here.y < closestStation->getBase()->Location().y))
                        return false;

                    // Found that > 5 tiles away causes us to wait forever
                    if (center.getDistance(closestStation->getBase()->Center()) > distCheck)
                        return false;
                    creepSoon = true;

                    // Don't build vertically
                    if (Strategy::enemyRush() && (here.y > closestStation->getBase()->Location().y + 4 || here.y < closestStation->getBase()->Location().y - 2))
                        return false;
                }
            }

            // Refinery only on Geysers
            if (building.isRefinery()) {
                for (auto &g : Resources::getMyGas()) {
                    ResourceInfo &gas = *g;

                    if (here == gas.getTilePosition() && gas.getType() != building)
                        return true;
                }
                return false;
            }

            // Used tile / creep check
            for (int x = here.x; x < here.x + building.tileWidth(); x++) {
                for (int y = here.y; y < here.y + building.tileHeight(); y++) {
                    TilePosition t(x, y);
                    if (!t.isValid())
                        return false;

                    if (building.getRace() == Races::Zerg && !creepSoon && building.requiresCreep() && !Broodwar->hasCreep(t))
                        return false;
                    if (BWEB::Map::isUsed(t) != None)
                        return false;
                }
            }

            // Addon room check
            if (building.canBuildAddon()) {
                if (BWEB::Map::isUsed(here + TilePosition(4, 1)) != None)
                    return false;
            }

            // Psi check
            if (building.requiresPsi() && !Pylons::hasPowerSoon(here, building))
                return false;
            return true;
        }

        BWEB::Block* getClosestBlock(Position here, function<bool(BWEB::Block*)> pred) {
            auto distBest = DBL_MAX;
            BWEB::Block * bestBlock = nullptr;
            for (auto &block : BWEB::Blocks::getBlocks()) {
                auto dist = block.getCenter().getDistance(here);
                if (dist < distBest && pred(&block)) {
                    distBest = dist;
                    bestBlock = &block;
                }
            }
            return bestBlock;
        }

        TilePosition returnFurthest(UnitType building, set<TilePosition> placements, Position desired, bool purelyFurthest = false) {
            auto tileBest = TilePositions::Invalid;
            auto distBest = 0.0;
            for (auto &placement : placements) {
                auto center = Position(placement) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
                auto current = center.getDistance(desired);
                if (current > distBest && isPlannable(building, placement) && isBuildable(building, placement) && isPathable(building, placement)) {
                    distBest = current;
                    tileBest = placement;
                }
            }
            return tileBest;
        }

        TilePosition returnClosest(UnitType building, set<TilePosition> placements, Position desired, bool purelyClosest = false) {
            auto tileBest = TilePositions::Invalid;
            auto distBest = DBL_MAX;

            for (auto &placement : placements) {
                //Visuals::drawBox(Position(placement) + Position(2,2), Position(placement + building.tileSize()) - Position(2, 2), Colors::Grey);
                auto center = Position(placement) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
                auto current = center.getDistance(desired);
                if (!purelyClosest && !Pylons::hasPowerNow(placement, building))
                    current = current * 4.0;
                //Visuals::drawBox(Position(placement) + Position(2, 2), Position(placement + building.tileSize()) - Position(2, 2), Colors::Grey);

                if (overlapsLarvaHistory(building, placement)) {
                    if (Util::getTime() < Time(6, 00) || isTechType(building))
                        continue;
                    current = current * 4.0;
                }

                if (current < distBest && isPlannable(building, placement) && (isBuildable(building, placement) || purelyClosest) && isPathable(building, placement)) {
                    distBest = current;
                    tileBest = placement;
                }
            }
            return tileBest;
        }

        TilePosition furthestLocation(UnitType building, Position here)
        {
            // Arrange what set we need to check
            set<TilePosition> placements;
            for (auto &block : BWEB::Blocks::getBlocks()) {
                Position blockCenter = Position(block.getTilePosition()) + Position(block.width() * 16, block.height() * 16);
                if (!Terrain::isInAllyTerritory(block.getTilePosition()))
                    continue;

                // Not sure what this is for
                if (Broodwar->self()->getRace() != Races::Zerg && int(block.getMediumTiles().size()) < 2)
                    continue;

                // Setup placements
                if (building.tileWidth() == 4)
                    placements.insert(block.getLargeTiles().begin(), block.getLargeTiles().end());
                else if (building.tileWidth() == 3)
                    placements.insert(block.getMediumTiles().begin(), block.getMediumTiles().end());
                else
                    placements.insert(block.getSmallTiles().begin(), block.getSmallTiles().end());
            }
            return returnFurthest(building, placements, here);
        }

        TilePosition closestLocation(UnitType building, Position here)
        {
            TilePosition tileBest = TilePositions::Invalid;
            double distBest = DBL_MAX;

            // Refineries are only built on my own gas resources
            if (building.isRefinery()) {
                for (auto &g : Resources::getMyGas()) {
                    auto &gas = *g;
                    auto dist = BWEB::Map::getGroundDistance(gas.getPosition(), here);

                    if (Stations::ownedBy(gas.getStation()) != PlayerState::Self
                        || !isPlannable(building, gas.getTilePosition())
                        || !isBuildable(building, gas.getTilePosition())
                        || dist >= distBest)
                        continue;

                    distBest = dist;
                    tileBest = gas.getTilePosition();
                }
                return tileBest;
            }

            // Check if any secondary locations are available here
            auto closestStation = Stations::getClosestStationAir(PlayerState::Self, here);
            if (closestStation && building == Zerg_Hatchery) {
                for (auto &location : closestStation->getSecondaryLocations()) {
                    if (isBuildable(building, location) && isPlannable(building, location) && isPathable(building, location)) {
                        tileBest = location;
                        return tileBest;
                    }
                }
            }

            // Generate a list of Blocks sorted by closest to here
            auto &list = BWEB::Blocks::getBlocks();
            multimap<double, BWEB::Block*> listByDist;
            for (auto &block : list) {
                if (!closestStation
                    || (!closestStation->isMain() && Broodwar->self()->getRace() != Races::Zerg)
                    || (!Terrain::isInAllyTerritory(block.getTilePosition()) && !BuildOrder::isProxy())
                    || (!Terrain::isIslandMap() && Broodwar->getGroundHeight(TilePosition(closestStation->getBase()->Center())) != Broodwar->getGroundHeight(TilePosition(block.getCenter())))
                    || (closestStation->isNatural() && closestStation->getChokepoint() && Position(closestStation->getChokepoint()->Center()).getDistance(block.getCenter()) < Position(closestStation->getChokepoint()->Center()).getDistance(closestStation->getBase()->Center()))
                    || (closestStation->getBase()->GetArea() != mapBWEM.GetArea(TilePosition(block.getCenter()))))
                    continue;
                if (!block.getPlacements(building).empty())
                    listByDist.emplace(make_pair(block.getCenter().getDistance(here), &block));
            }

            // Iterate sorted list and find a suitable block
            for (auto &[_, block] : listByDist) {
                tileBest = returnClosest(building, block->getPlacements(building), here);
                if (tileBest.isValid())
                    return tileBest;
            }
            return tileBest;
        }

        bool findResourceDepotLocation(UnitType building, TilePosition& placement)
        {
            const auto baseType = Broodwar->self()->getRace().getResourceDepot();
            const auto hatchCount = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);

            // If we are expanding, it must be on an expansion area and be when build order requests one
            const auto expand = Broodwar->self()->getRace() != Races::Zerg
                || (!BuildOrder::isOpener() && BuildOrder::shouldExpand())
                || (BuildOrder::isOpener() && int(Stations::getMyStations().size()) <= 1 && BuildOrder::takeNatural())
                || (BuildOrder::isOpener() && int(Stations::getMyStations().size()) <= 2 && BuildOrder::takeThird());

            if (!building.isResourceDepot()
                || !expand
                || expansionPlanned
                || BuildOrder::isProxy())
                return false;

            // First expansion is always the Natural
            if (Stations::getMyStations().size() == 1 && !Terrain::isShitMap() && isBuildable(baseType, BWEB::Map::getNaturalTile()) && isPlannable(baseType, BWEB::Map::getNaturalTile()) && isPathable(building, BWEB::Map::getNaturalTile())) {
                placement = BWEB::Map::getNaturalTile();
                myExpandPlan.currentExpansion = Terrain::getMyNatural();
                expansionPlanned = true;
                return true;
            }

            if (Stations::getMyStations().size() == 1 && Terrain::isShitMap() && Terrain::getEnemyStartingPosition().isValid()) {
                if (BWEB::Map::getMainTile() == TilePosition(8, 7)) {
                    if (Terrain::getEnemyStartingTilePosition() == TilePosition(43, 118))
                        placement = TilePosition(45, 19);
                    else
                        placement = TilePosition(8, 41);
                }
                if (BWEB::Map::getMainTile() == TilePosition(43, 118)) {
                    if (Terrain::getEnemyStartingTilePosition() == TilePosition(8, 7))
                        placement = TilePosition(77, 118);
                    else
                        placement = TilePosition(8, 113);
                }
                if (BWEB::Map::getMainTile() == TilePosition(117, 51)) {
                    if (Terrain::getEnemyStartingTilePosition() == TilePosition(8, 7))
                        placement = TilePosition(91, 67);
                    else
                        placement = TilePosition(108, 13);

                }

                expansionPlanned = true;
                return true;
            }

            // Consult expansion plan for next expansion
            if (myExpandPlan.currentExpansion) {
                placement = myExpandPlan.currentExpansion->getBase()->Location();
                expansionPlanned = true;
                return true;
            }
            return false;
        }

        bool findProductionLocation(UnitType building, TilePosition& placement)
        {
            if (!isProductionType(building))
                return false;

            // Return if valid
            if (placement.isValid())
                return true;

            // Figure out where we need to place a production building
            for (auto &[_, station] : Stations::getStationsBySaturation()) {
                auto desiredCenter = (Broodwar->self()->getRace() == Races::Zerg || !station->getChokepoint()) ? station->getBase()->Center() : Position(station->getChokepoint()->Center());
                placement = closestLocation(building, desiredCenter);
                if (placement.isValid())
                    return true;
            }
            return false;
        }

        bool findTechLocation(UnitType building, TilePosition& placement)
        {
            // Don't place if not tech building
            if (!isTechType(building))
                return false;

            // Hide tech if needed or against a rush
            if ((BuildOrder::isHideTech() && (building == Protoss_Citadel_of_Adun || building == Protoss_Templar_Archives))
                || (Strategy::enemyRush() && (Players::PvZ() || Players::TvZ())))
                placement = furthestLocation(building, (Position)BWEB::Map::getMainChoke()->Center());

            // Try to place the tech building inside a main base
            for (auto &station : Stations::getMyStations()) {
                if (station->isMain()) {
                    auto desiredCenter = Players::ZvZ() ? station->getResourceCentroid() : station->getBase()->Center();
                    placement = closestLocation(building, desiredCenter);
                    if (placement.isValid())
                        return true;
                }
            }

            // Try to place the tech building anywhere
            for (auto &station : Stations::getMyStations()) {
                if (!station->isMain()) {
                    placement = closestLocation(building, station->getBase()->Center());
                    if (placement.isValid())
                        return true;
                }
            }

            return placement.isValid();
        }

        bool findDefenseLocation(UnitType building, TilePosition& placement)
        {
            // Don't place if not defensive building
            if (!isDefensiveType(building))
                return false;

            // Place spire as close to the Lair in case we're hiding it
            if (building == Zerg_Spire) {
                auto closestLair = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getType() == Zerg_Lair;
                });

                if (closestLair) {
                    auto closestStation = Stations::getClosestStationAir(PlayerState::Self, closestLair->getPosition());
                    if (closestStation) {
                        placement = returnFurthest(building, closestStation->getDefenses(), Position(BWEB::Map::getMainChoke()->Center()));
                        if (placement.isValid())
                            return true;
                    }
                }

                placement = returnFurthest(building, Terrain::getMyMain()->getDefenses(), Position(BWEB::Map::getMainChoke()->Center()));
                if (placement.isValid())
                    return true;
            }

            // Defense placements near stations
            for (auto &station : Stations::getMyStations()) {

                int colonies = 0;
                for (auto& tile : station->getDefenses()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony || buildingsPlanned.find(tile) != buildingsPlanned.end())
                        colonies++;
                }

                // If we need ground defenses
                if (Stations::needGroundDefenses(station) > colonies) {
                    if (station->isMain() && station->getChokepoint()->Center()) {
                        Position desiredCenter = Position(station->getChokepoint()->Center());
                        placement = returnClosest(building, station->getDefenses(), desiredCenter);
                        if (placement.isValid())
                            return true;
                    }
                    else {

                        // Sort Chokepoints by most vulnerable (closest to middle of map)
                        map<double, Position> chokesByDist;
                        for (auto &choke : station->getBase()->GetArea()->ChokePoints())
                            chokesByDist.emplace(Position(choke->Center()).getDistance(mapBWEM.Center()), Position(choke->Center()));

                        // Place a uniquely best position near each chokepoint if possible
                        for (auto &[_, choke] : chokesByDist) {
                            placement = returnClosest(building, station->getDefenses(), choke, true);
                            if (placement.isValid() && isBuildable(building, placement) && isPathable(building, placement))
                                return true;
                            placement = TilePositions::Invalid;
                        }

                        // Otherwise resort to closest to resource centroid
                        placement = returnClosest(building, station->getDefenses(), station->getResourceCentroid());
                        if (placement.isValid() && isBuildable(building, placement) && isPathable(building, placement))
                            return true;
                        placement = TilePositions::Invalid;
                    }
                }
                if (Stations::needAirDefenses(station) > colonies) {
                    placement = returnClosest(building, station->getDefenses(), Position(station->getResourceCentroid()));
                    if (placement.isValid())
                        return true;
                }
            }

            // Defense placements near walls
            for (auto &[_, wall] : BWEB::Walls::getWalls()) {
                auto desiredCenter = Position(wall.getChokePoint()->Center());
                auto closestMain = BWEB::Stations::getClosestMainStation(TilePosition(wall.getChokePoint()->Center()));

                if (wall.getGroundDefenseCount() < 2) {
                    if (closestMain && closestMain->getChokepoint())
                        desiredCenter = Position(closestMain->getChokepoint()->Center());
                    else if (wall.getStation())
                        desiredCenter = Position(wall.getStation()->getBase()->Center());
                }
                else if (wall.getStation()) {
                    desiredCenter = Position(wall.getStation()->getResourceCentroid());
                }

                int colonies = 0;
                for (auto& tile : wall.getDefenses()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony || buildingsPlanned.find(tile) != buildingsPlanned.end())
                        colonies++;
                }

                // If this wall needs defenses
                if (Walls::needGroundDefenses(wall) > colonies) {

                    // Against Zealot builds try to place far back
                    if (Strategy::getEnemyOpener() == "9/9") {
                        for (int i = 4; i > 1; i--) {
                            placement = returnClosest(building, wall.getDefenses(i), desiredCenter);
                            if (placement.isValid()) {
                                plannedGround.insert(placement);
                                return true;
                            }
                        }
                    }

                    // Try to always place in front rows first
                    auto firstRow = wall.getGroundDefenseCount() == 0 && !Players::ZvT() ? 2 : 1;
                    for (int i = firstRow; i <= 4; i++) {
                        placement = returnClosest(building, wall.getDefenses(i), desiredCenter);
                        if (placement.isValid()) {
                            plannedGround.insert(placement);
                            return true;
                        }
                    }

                    // Resort to just placing a defense in the wall
                    placement = returnClosest(building, wall.getDefenses(0), desiredCenter);
                    if (placement.isValid()) {
                        plannedGround.insert(placement);
                        return true;
                    }
                }

                if (Walls::needAirDefenses(wall) > colonies) {

                    // Try to always place in middle rows first
                    for (int i = 2; i <= 3; i++) {
                        placement = returnClosest(building, wall.getDefenses(i), desiredCenter);
                        if (placement.isValid()) {
                            plannedAir.insert(placement);
                            return true;
                        }
                    }

                    // Resort to just placing a defense in the wall
                    placement = returnClosest(building, wall.getDefenses(0), desiredCenter);
                    if (placement.isValid()) {
                        plannedAir.insert(placement);
                        return true;
                    }
                }
            }
            return false;
        }

        bool findWallLocation(UnitType building, TilePosition& placement)
        {
            // Don't place if not wall building
            if (!isWallType(building))
                return false;

            // Don't wall if not needed
            if ((!BuildOrder::isWallNat() && !BuildOrder::isWallMain()) || (Strategy::enemyBust() && BuildOrder::isOpener()))
                return false;

            // As Zerg, we have to place natural hatch before wall
            if (building == Zerg_Hatchery && !Terrain::isShitMap() && isPlannable(building, BWEB::Map::getNaturalTile()) && BWEB::Map::isUsed(BWEB::Map::getNaturalTile()) == UnitTypes::None)
                return false;

            // Find a wall location
            set<TilePosition> placements;
            if (!Terrain::isShitMap()) {
                for (auto &[_, wall] : BWEB::Walls::getWalls()) {

                    if (!Terrain::isInAllyTerritory(wall.getArea()))
                        continue;

                    // Setup placements
                    if (building.tileWidth() == 4)
                        placements.insert(wall.getLargeTiles().begin(), wall.getLargeTiles().end());
                    else if (building.tileWidth() == 3)
                        placements.insert(wall.getMediumTiles().begin(), wall.getMediumTiles().end());
                    else
                        placements.insert(wall.getSmallTiles().begin(), wall.getSmallTiles().end());
                }
            }
            else if (Walls::getNaturalWall()) {

                // Setup placements
                if (building.tileWidth() == 4)
                    placements.insert(Walls::getNaturalWall()->getLargeTiles().begin(), Walls::getNaturalWall()->getLargeTiles().end());
                else if (building.tileWidth() == 3)
                    placements.insert(Walls::getNaturalWall()->getMediumTiles().begin(), Walls::getNaturalWall()->getMediumTiles().end());
                else
                    placements.insert(Walls::getNaturalWall()->getSmallTiles().begin(), Walls::getNaturalWall()->getSmallTiles().end());
            }

            // Get closest placement
            auto desired = !Stations::getStationsBySaturation().empty() ? Stations::getStationsBySaturation().begin()->second->getBase()->Center() : BWEB::Map::getMainPosition();
            placement = returnClosest(building, placements, BWEB::Map::getMainPosition());
            return placement.isValid();
        }

        bool findProxyLocation(UnitType building, TilePosition& placement)
        {
            if (!BuildOrder::isOpener() || !BuildOrder::isProxy())
                return false;

            // Find our proxy block
            for (auto &block : BWEB::Blocks::getBlocks()) {
                auto blockCenter = Position(block.getTilePosition() + TilePosition(block.width(), block.height()));
                if (block.isProxy()) {
                    placement = closestLocation(building, blockCenter);
                    return placement.isValid();
                }
            }

            // Otherwise try to find something close to the center and hopefully don't mess up - TODO: Don't mess up better
            placement = closestLocation(building, mapBWEM.Center());
            return placement.isValid();
        }

        bool findBatteryLocation(TilePosition& placement)
        {
            return false;
        }

        bool findPylonLocation(UnitType building, TilePosition& placement)
        {
            if (building != Protoss_Pylon)
                return false;

            // Pylon is separated because we want one unique best buildable position to check, rather than next best buildable position
            const auto stationNeedsPylon = [&](BWEB::Station * station) {
                auto distBest = DBL_MAX;
                auto tileBest = TilePositions::Invalid;
                auto natOrMain = station->isMain() || station->isNatural();

                // Check all defense locations for this station
                for (auto &defense : station->getDefenses()) {
                    double dist = Position(defense).getDistance(Position(station->getResourceCentroid()));
                    if (dist < distBest && (natOrMain || (defense.y <= station->getBase()->Location().y + 1 && defense.y >= station->getBase()->Location().y))) {
                        distBest = dist;
                        tileBest = defense;
                    }
                }
                if (isBuildable(building, tileBest) && isPlannable(building, tileBest) && isPathable(building, tileBest))
                    return tileBest;
                return TilePositions::Invalid;
            };

            // Check if any Nexus needs a Pylon for defense placement
            if (com(Protoss_Pylon) >= (Players::vT() ? 5 : 3) || Strategy::getEnemyTransition() == "2HatchMuta" || Strategy::getEnemyTransition() == "3HatchMuta") {
                for (auto &station : Stations::getMyStations()) {
                    placement = stationNeedsPylon(station);
                    if (placement.isValid())
                        return true;
                }
            }

            // Check if this our second Pylon and we're hiding tech
            if (building == Protoss_Pylon && vis(Protoss_Pylon) == 2 && BuildOrder::isHideTech()) {
                placement = furthestLocation(building, (Position)BWEB::Map::getMainChoke()->Center());
                if (placement.isValid())
                    return true;
            }

            // Check if any buildings lost power
            if (!unpoweredBlocks.empty()) {
                for (auto &block : unpoweredBlocks)
                    placement = returnClosest(Protoss_Pylon, block.getSmallTiles(), Position(block.getTilePosition()));

                if (placement.isValid())
                    return true;
            }

            // Check if our main choke should get a Pylon for a Shield Battery
            if (vis(Protoss_Pylon) == 1 && !BuildOrder::takeNatural() && (!Strategy::enemyRush() || !Players::vZ())) {
                placement = closestLocation(Protoss_Pylon, (Position)BWEB::Map::getMainChoke()->Center());
                if (placement.isValid())
                    return true;
            }

            // Check if we are being busted, add an extra pylon to the defenses
            if (Strategy::enemyBust() && Walls::getNaturalWall() && BuildOrder::isWallNat()) {
                int cnt = 0;
                TilePosition sum(0, 0);
                TilePosition center(0, 0);
                for (auto &defense : Walls::getNaturalWall()->getDefenses()) {
                    if (!Pylons::hasPowerNow(defense, Protoss_Photon_Cannon)) {
                        sum += defense;
                        cnt++;
                    }
                }

                if (cnt != 0) {
                    center = sum / cnt;
                    Position c(center);
                    double distBest = DBL_MAX;

                    // Find unique closest tile to center of defenses
                    for (auto &tile : Walls::getNaturalWall()->getDefenses()) {
                        Position defenseCenter = Position(tile) + Position(32, 32);
                        double dist = defenseCenter.getDistance(c);
                        if (dist < distBest && isPlannable(Protoss_Pylon, tile) && isBuildable(Protoss_Pylon, tile) && isPathable(building, tile)) {
                            distBest = dist;
                            placement = tile;
                        }
                    }
                }

                if (placement.isValid())
                    return true;
            }

            // Resort to finding a production location
            placement = closestLocation(Protoss_Pylon, BWEB::Map::getMainPosition());
            return placement.isValid();
        }

        bool findRefineryLocation(UnitType building, TilePosition& placement)
        {
            if (!building.isRefinery())
                return false;

            placement = closestLocation(building, BWEB::Map::getMainPosition());
            return placement.isValid();
        }

        TilePosition getBuildLocation(UnitType building)
        {
            auto placement = TilePositions::Invalid;
            const auto finders ={ findWallLocation, findDefenseLocation, findProxyLocation, findResourceDepotLocation, findPylonLocation, findProductionLocation, findTechLocation, findRefineryLocation };
            for (auto &finder : finders) {
                if (finder(building, placement))
                    return placement;
            }

            // HACK: Try to get a placement if we are being horror gated
            if (Strategy::enemyProxy() && Util::getTime() < Time(5, 0) && !isDefensiveType(building) && !building.isResourceDepot())
                placement = Broodwar->getBuildLocation(building, BWEB::Map::getMainTile(), 16);

            return placement;
        }

        void updateCommands(UnitInfo& building)
        {
            const auto willDieToAttacks = [&]() {
                auto possibleDamage = 0;
                for (auto &attacker : building.getTargetedBy()) {
                    if (attacker.lock() && attacker.lock()->isWithinRange(building))
                        possibleDamage+= int(attacker.lock()->getGroundDamage());
                }

                return possibleDamage > building.getHealth() + building.getShields();
            };
            auto needLarvaSpending = vis(Zerg_Larva) > 3 && Broodwar->self()->supplyUsed() < Broodwar->self()->supplyTotal() && BuildOrder::getUnitLimit(Zerg_Larva) == 0 && Util::getTime() < Time(4, 30) && com(Zerg_Sunken_Colony) > 2;

            // Lair morphing
            if (building.getType() == Zerg_Hatchery && !willDieToAttacks() && BuildOrder::buildCount(Zerg_Lair) > vis(Zerg_Lair) + vis(Zerg_Hive) + morphedThisFrame[Zerg_Lair] + morphedThisFrame[Zerg_Hive]) {
                auto morphTile = BWEB::Map::getMainTile();
                const auto closestScout = Util::getClosestUnitGround(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getType().isWorker();
                });
                if (closestScout && int(Stations::getMyStations().size()) >= 2 && mapBWEM.GetArea(closestScout->getTilePosition()) == BWEB::Map::getMainArea())
                    morphTile = BWEB::Map::getNaturalTile();

                if (building.getTilePosition() == morphTile) {
                    building.unit()->morph(Zerg_Lair);
                    morphedThisFrame[Zerg_Lair]++;
                }
            }

            // Hive morphing
            else if (building.getType() == Zerg_Lair && !willDieToAttacks() && BuildOrder::buildCount(Zerg_Hive) > vis(Zerg_Hive) + morphedThisFrame[Zerg_Hive]) {
                building.unit()->morph(Zerg_Hive);
                morphedThisFrame[Zerg_Hive]++;
            }

            // Greater Spire morphing
            else if (building.getType() == Zerg_Spire && !willDieToAttacks() && BuildOrder::buildCount(Zerg_Greater_Spire) > vis(Zerg_Greater_Spire) + morphedThisFrame[Zerg_Greater_Spire]) {
                building.unit()->morph(Zerg_Greater_Spire);
                morphedThisFrame[Zerg_Greater_Spire]++;
            }

            // Sunken / Spore morphing
            else if (building.getType() == Zerg_Creep_Colony && !willDieToAttacks() && !needLarvaSpending) {
                auto morphType = UnitTypes::None;
                auto station = BWEB::Stations::getClosestStation(building.getTilePosition());
                auto wall = BWEB::Walls::getClosestWall(building.getTilePosition());

                auto closestAttacker = Util::getClosestUnit(building.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return !u.getType().isFlyer() && !u.getType().isWorker() && u.getGroundDamage() > 0.0 && u.getSpeed() > 0.0;
                });

                auto wallDefense = wall && wall->getDefenses().find(building.getTilePosition()) != wall->getDefenses().end();
                auto stationDefense = station && station->getDefenses().find(building.getTilePosition()) != station->getDefenses().end();

                // If we planned already
                if (plannedGround.find(building.getTilePosition()) != plannedGround.end())
                    morphType = Zerg_Sunken_Colony;
                else if (plannedAir.find(building.getTilePosition()) != plannedAir.end())
                    morphType = Zerg_Spore_Colony;

                // If this is a Station defense
                else if (stationDefense && Stations::needGroundDefenses(station) > morphedThisFrame[Zerg_Sunken_Colony] && com(Zerg_Spawning_Pool) > 0)
                    morphType = Zerg_Sunken_Colony;
                else if (stationDefense && Stations::needAirDefenses(station) > morphedThisFrame[Zerg_Spore_Colony] && com(Zerg_Evolution_Chamber) > 0)
                    morphType = Zerg_Spore_Colony;

                // If this is a Wall defense
                else if (wallDefense && Walls::needAirDefenses(*wall) > morphedThisFrame[Zerg_Spore_Colony] && plannedAir.find(building.getTilePosition()) != plannedAir.end() && com(Zerg_Evolution_Chamber) > 0)
                    morphType = Zerg_Spore_Colony;
                else if (wallDefense && Walls::needGroundDefenses(*wall) > morphedThisFrame[Zerg_Sunken_Colony] && plannedGround.find(building.getTilePosition()) != plannedGround.end() && com(Zerg_Spawning_Pool) > 0) {

                    // Morph if we need defenses now
                    if (BuildOrder::makeDefensesNow() || Util::getTime() > Time(5, 00))
                        morphType = Zerg_Sunken_Colony;

                    // Check timing for when an attacker will arrive
                    else if (closestAttacker) {
                        auto timeToEngage = closestAttacker->getPosition().getDistance(building.getPosition()) / closestAttacker->getSpeed();

                        auto visDiff = Broodwar->getFrameCount() - closestAttacker->getLastVisibleFrame();
                        if (timeToEngage - visDiff <= Zerg_Sunken_Colony.buildTime() + 96 || closestAttacker->getType() == Terran_Vulture)
                            morphType = Zerg_Sunken_Colony;
                    }
                }

                if (morphType.isValid() && building.isCompleted()) {
                    building.unit()->morph(morphType);
                    morphedThisFrame[morphType]++;
                    queuedMineral+=morphType.mineralPrice();
                }
            }

            // Terran building needs new scv
            else if (building.getType().getRace() == Races::Terran && !building.unit()->isCompleted() && !building.unit()->getBuildUnit()) {
                auto &builder = Util::getClosestUnit(building.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getType().isWorker() && u.getBuildType() == None;
                });

                if (builder)
                    builder->unit()->rightClick(building.unit());
            }

            // Barracks
            if (building.getType() == Terran_Barracks) {
                return;// screw lifting

                // Wall lift
                bool wallCheck = (Walls::getNaturalWall() && building.getPosition().getDistance(Walls::getNaturalWall()->getCentroid()) < 256.0)
                    || (Walls::getMainWall() && building.getPosition().getDistance(Walls::getMainWall()->getCentroid()) < 256.0);

                if (wallCheck && !building.unit()->isFlying()) {
                    if (Players::getSupply(PlayerState::Self, Races::None) > 120 || BuildOrder::firstReady()) {
                        building.unit()->lift();
                        BWEB::Map::onUnitDestroy(building.unit());
                    }
                }

                // Find landing location as production building
                else if ((Players::getSupply(PlayerState::Self, Races::None) > 120 || BuildOrder::firstReady()) && building.unit()->isFlying()) {
                    auto here = closestLocation(building.getType(), BWEB::Map::getMainPosition());
                    auto center = (Position)here + Position(building.getType().tileWidth() * 16, building.getType().tileHeight() * 16);

                    if (building.unit()->getLastCommand().getType() != UnitCommandTypes::Land || building.unit()->getLastCommand().getTargetTilePosition() != here)
                        building.unit()->land(here);
                }

                // Add used tiles back to grid
                else if (!building.unit()->isFlying() && building.unit()->getLastCommand().getType() == UnitCommandTypes::Land)
                    BWEB::Map::onUnitDiscover(building.unit());
            }

            // Comsat scans - Move to special manager
            if (building.getType() == Terran_Comsat_Station) {
                if (building.hasTarget() && building.getTarget().unit()->exists() && !Actions::overlapsDetection(building.unit(), building.getTarget().getPosition(), PlayerState::Enemy)) {
                    building.unit()->useTech(TechTypes::Scanner_Sweep, building.getTarget().getPosition());
                    Actions::addAction(building.unit(), building.getTarget().getPosition(), TechTypes::Scanner_Sweep, PlayerState::Self);
                }
            }

            // Cancelling Refinerys for our gas trick
            if (BuildOrder::isGasTrick() && building.getType().isRefinery() && !building.unit()->isCompleted() && BuildOrder::buildCount(building.getType()) < vis(building.getType())) {
                building.unit()->cancelMorph();
                BWEB::Map::removeUsed(building.getTilePosition(), 4, 2);
            }

            // Cancelling refineries we don't want
            if (building.getType().isRefinery() && Strategy::enemyRush() && vis(building.getType()) == 1 && building.getType().getRace() != Races::Zerg && BuildOrder::buildCount(building.getType()) < vis(building.getType())) {
                building.unit()->cancelConstruction();
            }

            // Cancelling buildings that are being built/morphed but will be dead
            if (!building.unit()->isCompleted() && willDieToAttacks() && building.getType() != Zerg_Sunken_Colony && building.getType() != Zerg_Spore_Colony) {
                building.unit()->cancelConstruction();
                Events::onUnitCancelBecauseBWAPISucks(building);
            }

            // Cancelling hatcheries if we're being proxy 2gated
            if (building.getType() == Zerg_Hatchery && building.unit()->getRemainingBuildTime() < 120 && building.getTilePosition() == Terrain::getMyNatural()->getBase()->Location() && BuildOrder::isOpener() && Strategy::getEnemyBuild() == "2Gate" && Strategy::enemyProxy()) {
                building.unit()->cancelConstruction();
                Events::onUnitCancelBecauseBWAPISucks(building);
            }

            // Cancelling lairs if we're being proxy 2gated
            if (building.getType() == Zerg_Lair && BuildOrder::isOpener() && Strategy::getEnemyBuild() == "2Gate" && Strategy::getEnemyOpener() == "Proxy" && BuildOrder::getCurrentTransition().find("Muta") == string::npos) {
                building.unit()->cancelConstruction();
            }
        }

        void updateInformation(UnitInfo& building)
        {
            // If a building is unpowered, get a pylon placement ready
            if (building.getType().requiresPsi() && !Pylons::hasPowerSoon(building.getTilePosition(), building.getType())) {
                auto block = BWEB::Blocks::getClosestBlock(building.getTilePosition());
                if (block)
                    unpoweredBlocks.push_back(*block);
            }

            // If this is a defensive building and is blocking, mark it for suicide
            if (building.getRole() == Role::Defender && validDefenses.find(building.getTilePosition()) == validDefenses.end()) {

                for (auto &[_, wall] : BWEB::Walls::getWalls()) {
                    for (auto &defTile : wall.getDefenses()) {
                        if (defTile == building.getTilePosition())
                            building.setMarkForDeath(true);
                    }
                }
                for (auto &station : BWEB::Stations::getStations()) {
                    for (auto &defTile : station.getDefenses()) {
                        if (defTile == building.getTilePosition())
                            building.setMarkForDeath(true);
                    }
                }
            }
        }

        void updateLarvaEncroachment()
        {
            larvaEncroaches.clear();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (!unit.unit())
                    continue;

                if (unit.getType() == Zerg_Larva && !Production::larvaTrickRequired(unit)) {
                    for (auto &[_, pos] : unit.getPositionHistory())
                        larvaEncroaches.insert(pos);
                }
                if (unit.getType() == Zerg_Egg || unit.getType() == Zerg_Lurker_Egg)
                    larvaEncroaches.insert(unit.getPosition());
            }
        }

        void updateBuildings()
        {
            // Reset counters
            poweredSmall = 0; poweredMedium = 0; poweredLarge = 0;
            morphedThisFrame.clear();
            unpoweredBlocks.clear();
            validDefenses.clear();

            auto largeType = None;
            auto mediumType = None;
            auto smallType = None;

            // Protoss buildings
            if (Broodwar->self()->getRace() == Races::Protoss) {
                largeType = Protoss_Gateway;
                mediumType = Protoss_Forge;
                smallType = Protoss_Photon_Cannon;
            }
            // Terran buildings
            if (Broodwar->self()->getRace() == Races::Terran) {
                largeType = Terran_Factory;
                mediumType = Terran_Supply_Depot;
                smallType = Terran_Missile_Turret;
            }
            // Zerg buildings
            if (Broodwar->self()->getRace() == Races::Zerg) {
                largeType = Zerg_Hatchery;
                mediumType = Zerg_Evolution_Chamber;
                smallType = Zerg_Spire;
            }

            const auto checkDefense = [&](TilePosition tile, TilePosition pathTile) {
                if (tile == pathTile
                    || tile + TilePosition(0, 1) == pathTile
                    || tile + TilePosition(1, 0) == pathTile
                    || tile + TilePosition(1, 1) == pathTile)
                    validDefenses.erase(tile);
                return;
            };

            // Create valid defenses based on needing to allow a path or not
            for (auto &[_, wall] : BWEB::Walls::getWalls()) {
                for (auto &defTile : wall.getDefenses())
                    validDefenses.insert(defTile);
            }
            for (auto &station : BWEB::Stations::getStations()) {
                for (auto &defTile : station.getDefenses()) {
                    validDefenses.insert(defTile);
                }
            }

            // Erase positions that are blockers
            if (Util::getTime() > Time(12, 00)) {
                if (total(Protoss_Dragoon) > 0
                    || total(Zerg_Ultralisk) > 0
                    || total(Zerg_Lurker) > 0
                    || (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Grooved_Spines) > 0 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Muscular_Augments) > 0)) {
                    for (auto &[_, wall] : BWEB::Walls::getWalls()) {
                        for (auto &pathTile : wall.getPath().getTiles()) {
                            for (auto &defTile : wall.getDefenses())
                                checkDefense(defTile, pathTile);

                            if (wall.getStation()) {
                                for (auto &defTile : wall.getStation()->getDefenses())
                                    checkDefense(defTile, pathTile);
                            }
                        }
                    }
                }
            }

            // Update all my buildings
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getRole() == Role::Production || unit.getRole() == Role::Defender) {
                    updateInformation(unit);
                    updateCommands(unit);
                }
            }
        }

        void updateExpandPlan()
        {
            if (Broodwar->getFrameCount() < 5
                || !Terrain::getEnemyNatural())
                return;

            // Create a map of blocking neutrals per station
            myExpandPlan.blockingNeutrals.clear();
            for (auto &[station, path] : myExpandPlan.pathNetwork[Terrain::getMyMain()]) {
                Visuals::drawPath(path);
                Util::testPointOnPath(path, [&](Position &p) {
                    auto type = BWEB::Map::isUsed(TilePosition(p));
                    if (type != UnitTypes::None) {
                        const auto closestNeutral = Util::getClosestUnit(p, PlayerState::Neutral, [&](auto &u) {
                            return u.getType() == type;
                        });
                        if (closestNeutral && closestNeutral->getPosition().getDistance(p) < 96.0) {
                            myExpandPlan.blockingNeutrals[station].push_back(closestNeutral);
                            return true;
                        }
                    }
                    return false;
                });
            }

            // Establish an initial parent station as our natural (most of our units rally through this)
            auto parentStation = Terrain::getMyNatural();
            auto enemyStation = Terrain::getEnemyNatural();

            // Check if any base goes through enemy territory currently
            myExpandPlan.dangerousStations.clear();
            for (auto &station : BWEB::Stations::getStations()) {
                auto& path = myExpandPlan.pathNetwork[Terrain::getMyMain()][&station];

                //Visuals::drawPath(path);

                if (!path.getTiles().empty()) {
                    auto danger = Util::findPointOnPath(path, [&](auto &t) {
                        return Terrain::isInEnemyTerritory(TilePosition(t));
                    });

                    if (danger.isValid())
                        myExpandPlan.dangerousStations.push_back(&station);
                }
            }

            // Check if any base is an island separated by resources
            for (auto &station : BWEB::Stations::getStations()) {
                for (auto &b : myExpandPlan.blockingNeutrals[&station]) {
                    if (auto blocker = b.lock()) {
                        if (blocker->getType().isMineralField() || blocker->getType().isRefinery())
                            myExpandPlan.islandStations.push_back(&station);
                    }
                }
            }

            // Score each station
            auto allowedFirstMineralBase = (Players::vT() || Players::ZvZ() || Players::ZvP()) ? 4 : 3;
            myExpandPlan.bestOrder.clear();

            if (Terrain::getMyMain())
                myExpandPlan.bestOrder.push_back(Terrain::getMyMain());
            if (Terrain::getMyNatural())
                myExpandPlan.bestOrder.push_back(Terrain::getMyNatural());
            for (int i = 0; i < int(BWEB::Stations::getStations().size()); i++) {
                auto costBest = DBL_MAX;
                BWEB::Station * stationBest = nullptr;

                for (auto &station : BWEB::Stations::getStations()) {
                    auto grdParent = log(myExpandPlan.pathNetwork[parentStation][&station].getDistance());
                    auto grdHome = myExpandPlan.pathNetwork[Terrain::getMyMain()][&station].getDistance();
                    auto airParent = station.getBase()->Center().getDistance(parentStation->getBase()->Center());
                    auto airHome = station.getBase()->Center().getDistance(Position(BWEB::Map::getNaturalChoke()->Center()));
                    auto airCenter = station.getBase()->Center().getDistance(mapBWEM.Center());
                    auto grdEnemy = myExpandPlan.pathNetwork[enemyStation][&station].getDistance();
                    auto airEnemy = station.getBase()->Center().getDistance(enemyStation->getBase()->Center());

                    if (station.getBase()->GetArea() == mapBWEM.GetArea(TilePosition(mapBWEM.Center())) && myExpandPlan.bestOrder.size() < 4)
                        continue;
                    if (find_if(myExpandPlan.bestOrder.begin(), myExpandPlan.bestOrder.end(), [&](auto &s) { return s == &station; }) != myExpandPlan.bestOrder.end())
                        continue;
                    if (station == Terrain::getMyMain()
                        || station == Terrain::getMyNatural()
                        || (Terrain::getEnemyMain() && station == Terrain::getEnemyMain())
                        || (Terrain::getEnemyNatural() && station == Terrain::getEnemyNatural()))
                        continue;
                    if (station.getBase()->Geysers().empty() && int(myExpandPlan.bestOrder.size()) < allowedFirstMineralBase)
                        continue;
                    if (mapBWEM.GetPath(BWEB::Map::getMainPosition(), station.getBase()->Center()).empty() && myExpandPlan.bestOrder.size() < 3)
                        continue;
                    if (Terrain::isInEnemyTerritory(station.getBase()->GetArea()))
                        continue;
                    if (find_if(myExpandPlan.islandStations.begin(), myExpandPlan.islandStations.end(), [&](auto &s) { return s == &station; }) != myExpandPlan.islandStations.end())
                        continue;
                    if (find_if(myExpandPlan.dangerousStations.begin(), myExpandPlan.dangerousStations.end(), [&](auto &s) { return s == &station; }) != myExpandPlan.dangerousStations.end()) {
                        grdEnemy = sqrt(1.0 + grdEnemy);
                        airEnemy = sqrt(1.0 + airEnemy);
                    }
                    if (!myExpandPlan.blockingNeutrals[&station].empty()) {
                        grdHome *= sqrt(1.0 + double(myExpandPlan.blockingNeutrals[&station].size()));
                        airHome *= sqrt(1.0 + double(myExpandPlan.blockingNeutrals[&station].size()));
                    }

                    auto dist = (grdParent * grdHome * airParent * airHome) / (grdEnemy * airEnemy * airCenter);
                    auto blockerCost = 0.0;
                    for (auto &blocker : myExpandPlan.blockingNeutrals[&station])
                        blockerCost += double(blocker.lock()->getHealth()) / 1000.0;

                    if (blockerCost > 0)
                        dist = blockerCost;

                    if (dist < costBest) {
                        costBest = dist;
                        stationBest = &station;
                    }
                }

                if (stationBest) {
                    myExpandPlan.bestOrder.push_back(stationBest);
                    parentStation = stationBest;
                }
            }

            // Determine what our current/next expansion is
            auto baseType = Broodwar->self()->getRace().getResourceDepot();
            for (auto &station : myExpandPlan.bestOrder) {
                if (isBuildable(baseType, station->getBase()->Location())) {
                    myExpandPlan.currentExpansion = station;
                    break;
                }
            }
            for (auto &station : myExpandPlan.bestOrder) {
                if (station != myExpandPlan.currentExpansion && isBuildable(baseType, station->getBase()->Location())) {
                    myExpandPlan.nextExpansion = station;
                    break;
                }
            }

            // Check if a blocker exists
            myExpandPlan.blockersExists = false;
            if (myExpandPlan.currentExpansion && Util::getTime() > Time(8, 00)) {

                // Blockers exist if our current expansion has blockers, not our next one
                myExpandPlan.blockersExists = !myExpandPlan.blockingNeutrals[myExpandPlan.currentExpansion].empty();

                // Mark the blockers for the next expansion for death, so we can kill it before we need an expansion
                for (auto &n : myExpandPlan.blockingNeutrals[myExpandPlan.currentExpansion]) {
                    if (auto neutral = n.lock())
                        neutral->setMarkForDeath(true);
                }
            }
        }

        void updatePlan()
        {
            // Wipe clean all queued buildings, store a cache of old buildings
            map<shared_ptr<UnitInfo>, TilePosition> oldBuilders;
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                if (unit.getBuildPosition().isValid()) {
                    oldBuilders[u] = unit.getBuildPosition();
                    unit.setBuildingType(UnitTypes::None);
                    unit.setBuildPosition(TilePositions::Invalid);
                }
            }
            buildingsPlanned.clear();

            // Add up how many more buildings of each type we need
            for (auto &[building, count] : BuildOrder::getBuildQueue()) {
                int queuedCount = 0;

                auto morphed = !building.whatBuilds().first.isWorker();
                auto addon = building.isAddon();

                if (building.isAddon()
                    || !building.isBuilding())
                    continue;

                // If the building morphed from another building type, add the visible amount of child type to the parent type
                int morphOffset = 0;
                if (building == Zerg_Creep_Colony)
                    morphOffset = vis(Zerg_Sunken_Colony) + vis(Zerg_Spore_Colony);
                if (building == Zerg_Hatchery)
                    morphOffset = vis(Zerg_Lair) + vis(Zerg_Hive);
                if (building == Zerg_Lair)
                    morphOffset = vis(Zerg_Hive);

                // Queue the cost of any morphs or building
                if (count > vis(building) + morphOffset && (Workers::getMineralWorkers() > 0 || building.mineralPrice() == 0 || Broodwar->self()->minerals() >= building.mineralPrice()) && (Workers::getGasWorkers() > 0 || building.gasPrice() == 0 || Broodwar->self()->gas() >= building.gasPrice())) {
                    queuedMineral += building.mineralPrice() * (count - vis(building) - morphOffset);
                    queuedGas += building.gasPrice() * (count - vis(building) - morphOffset);
                }

                if (morphed)
                    continue;

                // Queue building if our actual count is higher than our visible count
                while (count > queuedCount + vis(building) + morphOffset) {
                    queuedCount++;
                    auto here = getBuildLocation(building);
                    auto center = Position(here) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
                    if (!here.isValid())
                        continue;

                    //Visuals::drawBox(Position(here) + Position(4, 4), Position(here + building.tileSize()) - Position(4, 4), Colors::White);

                    auto &builder = Util::getClosestUnitGround(center, PlayerState::Self, [&](auto &u) {
                        if (u.getType().getRace() != building.getRace()
                            || u.getBuildType() != None
                            || u.unit()->getOrder() == Orders::ConstructingBuilding
                            || !Workers::canAssignToBuild(u))
                            return false;
                        return true;
                    });

                    // Use old builder if we're not early game, as long as it's not stuck or was stuck recently
                    if (builder && Util::getTime() > Time(3, 00)) {
                        for (auto &[oldBuilder, oldHere] : oldBuilders) {
                            if (oldHere == here && oldBuilder && Workers::canAssignToBuild(*oldBuilder))
                                builder = oldBuilder;
                        }
                    }

                    if (here.isValid() && builder && Workers::shouldMoveToBuild(*builder, here, building)) {
                        //Visuals::drawLine(builder->getPosition(), center, Colors::White);
                        builder->setBuildingType(building);
                        builder->setBuildPosition(here);
                        buildingsPlanned[here] = building;
                    }
                }
            }
        }
    }

    bool overlapsPlan(UnitInfo& unit, Position here)
    {
        // Check if there's a building queued there already
        for (auto &[tile, building] : buildingsPlanned) {

            if (Broodwar->self()->minerals() < building.mineralPrice() * 0.8
                || Broodwar->self()->gas() < building.gasPrice() * 0.8
                || unit.getBuildPosition() == tile)
                continue;

            auto center = Position(tile) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
            if (Util::boxDistance(unit.getType(), here, building, center) <= 4)
                return true;
            if (unit.getType() == Zerg_Larva && Util::boxDistance(Zerg_Egg, here, building, center) <= 4)
                return true;
        }
        return false;
    }

    bool overlapsUnit(UnitInfo& unit, TilePosition here, UnitType building)
    {
        // Bordering box of the queued building
        const auto buildingTopLeft = Position(here);
        const auto buildingBotRight = buildingTopLeft + Position(building.tileSize());

        // Bordering box of the unit
        const auto unitTopLeft = unit.getPosition() + Position(-unit.getType().width() / 2, -unit.getType().height() / 2);
        const auto unitTopRight = unit.getPosition() + Position(unit.getType().width() / 2, -unit.getType().height() / 2);
        const auto unitBotLeft = unit.getPosition() + Position(-unit.getType().width() / 2, unit.getType().height() / 2);
        const auto unitBotRight = unit.getPosition() + Position(unit.getType().width() / 2, unit.getType().height() / 2);

        return (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitTopLeft))
            || (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitTopRight))
            || (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitBotLeft))
            || (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitBotRight));
    }

    bool hasPoweredPositions() { return (poweredLarge > 0 && poweredMedium > 0); }
    int getQueuedMineral() { return queuedMineral; }
    int getQueuedGas() { return queuedGas; }
    BWEB::Station * getCurrentExpansion() { return myExpandPlan.currentExpansion; }
    BWEB::Station * getLastExpansion() { return myExpandPlan.lastExpansion; }
    bool expansionBlockersExists() { return myExpandPlan.blockersExists; }

    void onFrame()
    {
        expansionPlanned = false;
        queuedMineral = 0;
        queuedGas = 0;

        updateLarvaEncroachment();
        updateBuildings();
        updateExpandPlan();
        updatePlan();
    }

    void onStart()
    {
        // Initialize Blocks
        BWEB::Blocks::findBlocks();
        BWEB::Pathfinding::clearCacheFully();

        // Create a path to each station that only obeys terrain
        for (auto &station : BWEB::Stations::getStations()) {
            for (auto &otherStation : BWEB::Stations::getStations()) {
                if (station == otherStation)
                    continue;

                BWEB::Path unitPath(station.getBase()->Center(), otherStation.getBase()->Center(), Protoss_Dragoon, true, false);
                unitPath.generateJPS([&](auto &t) { return unitPath.unitWalkable(t); });

                if (unitPath.isReachable())
                    myExpandPlan.pathNetwork[&station][&otherStation] = unitPath;
                else {
                    BWEB::Path terrainPath(station.getBase()->Center(), otherStation.getBase()->Center(), Protoss_Dragoon, true, false);
                    terrainPath.generateJPS([&](auto &t) { return terrainPath.terrainWalkable(t); });
                    myExpandPlan.pathNetwork[&station][&otherStation] = terrainPath;
                }
            }
        }
    }
}