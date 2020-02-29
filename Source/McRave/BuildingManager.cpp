#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Buildings {

    namespace {

        int queuedMineral, queuedGas;
        int poweredSmall, poweredMedium, poweredLarge;
        int lairsMorphing, hivesMorphing;

        map <TilePosition, UnitType> buildingsQueued;
        vector<BWEB::Block> unpoweredBlocks;
        map<BWEB::Piece, int> availablePieces;
        TilePosition currentExpansion;

        bool isDefensiveType(UnitType building)
        {
            return building == Protoss_Photon_Cannon
                || building == Protoss_Shield_Battery
                || building == Zerg_Creep_Colony
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


        void checkClosest(UnitType building, TilePosition placement, Position desired, TilePosition& tileBest, double& distBest) {
            auto center = Position(placement) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
            auto current = center.getDistance(desired);
            if (current < distBest && isQueueable(building, placement) && isBuildable(building, placement)) {
                distBest = current;
                tileBest = placement;
            }
        }

        void checkFurthest(UnitType building, TilePosition placement, Position desired, TilePosition& tileBest, double& distBest) {
            auto center = Position(placement) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
            auto current = center.getDistance(desired);
            if (current > distBest && isQueueable(building, placement) && isBuildable(building, placement)) {
                distBest = current;
                tileBest = placement;
            }
        }


        TilePosition closestDefLocation(UnitType building, Position here)
        {
            auto tileBest = TilePositions::Invalid;
            auto distBest = DBL_MAX;
            auto station = BWEB::Stations::getClosestStation(TilePosition(here));
            auto wall = BWEB::Walls::getClosestWall(TilePosition(here));
            auto natOrMain = BWEB::Map::getMainTile() == station->getBWEMBase()->Location() || BWEB::Map::getNaturalTile() == station->getBWEMBase()->Location();
            auto attacks = building == Protoss_Photon_Cannon || building == Terran_Missile_Turret || building == Zerg_Creep_Colony;

            auto stationColonies = 0;
            auto wallColonies = 0;

            // Current Creep Colony counts
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (station) {
                    for (auto &defense : station->getDefenseLocations())
                        stationColonies += int(BWEB::Map::isUsed(defense) == Zerg_Creep_Colony);
                }
                if (wall) {
                    for (auto &defense : wall->getDefenses())
                        wallColonies += int(BWEB::Map::isUsed(defense) == Zerg_Creep_Colony);
                }
            }

            // Check closest stataion to see if one of their defense locations is best
            if (station && Stations::ownedBy(station) == PlayerState::Self && (building == Protoss_Pylon || Stations::needGroundDefenses(*station) > stationColonies || Stations::needAirDefenses(*station) > stationColonies)) {
                for (auto &defense : station->getDefenseLocations()) {

                    // Pylon is separated because we want one unique best buildable position to check, rather than next best buildable position
                    if (building == Protoss_Pylon) {
                        double dist = Position(defense).getDistance(Position(here));
                        if (dist < distBest && (natOrMain || (defense.y <= station->getBWEMBase()->Location().y + 1 && defense.y >= station->getBWEMBase()->Location().y))) {
                            distBest = dist;
                            tileBest = defense;
                        }
                    }
                    else
                        checkClosest(building, defense, here, tileBest, distBest);
                }

                // Set back to invalid if not buildable/queuable (specifically checks for pylon unique location)
                if (!isBuildable(building, tileBest) || !isQueueable(building, tileBest))
                    tileBest = TilePositions::Invalid;
            }

            // Check closest Wall to see if one of their defense locations is best
            if (wall && building != Protoss_Pylon && (!attacks || Terrain::needAirDefenses(*Terrain::getNaturalWall()) > wallColonies || Terrain::needGroundDefenses(*Terrain::getNaturalWall()) > wallColonies)) {
                for (auto &wall : BWEB::Walls::getWalls()) {
                    for (auto &tile : wall.getDefenses()) {

                        if (!Terrain::isInAllyTerritory(tile) && !attacks)
                            continue;

                        Broodwar->drawCircleMap(Terrain::getNaturalWall()->getCentroid(), 8, Colors::Yellow, false);

                        if (vis(building) > 1) {
                            auto closestGeo = BWEB::Map::getClosestChokeTile(wall.getChokePoint(), Position(tile) + Position(32, 32));
                            checkClosest(building, tile, closestGeo, tileBest, distBest);
                        }
                        else
                            checkClosest(building, tile, here, tileBest, distBest);
                    }
                }
            }

            // Defensive buildings placed at regular blocks
            if (building == Protoss_Shield_Battery || building == Protoss_Pylon || (building == Protoss_Photon_Cannon && Strategy::needDetection())) {
                set<TilePosition> placements;
                for (auto &block : BWEB::Blocks::getBlocks()) {

                    if (!Terrain::isInAllyTerritory(block.getTilePosition()))
                        continue;

                    if (building == Protoss_Pylon)
                        placements = block.getSmallTiles();
                    else if (block.isDefensive())
                        placements = block.getMediumTiles();

                    for (auto &tile : placements)
                        checkClosest(building, tile, here, tileBest, distBest);
                }
                if (tileBest.isValid())
                    return tileBest;
            }

            return tileBest;
        }

        TilePosition closestWallLocation(UnitType building, Position here)
        {
            auto tileBest = TilePositions::Invalid;
            auto distBest = DBL_MAX;
            set<TilePosition> placements;

            auto wall = BWEB::Walls::getClosestWall(TilePosition(here));

            if (!wall)
                return tileBest;

            if (building.tileWidth() == 4)
                placements = wall->getLargeTiles();
            else if (building.tileWidth() == 3)
                placements = wall->getMediumTiles();
            else if (building.tileWidth() == 2)
                placements = wall->getSmallTiles();

            // Iterate tiles and check for best
            for (auto tile : placements) {
                double dist = Position(tile).getDistance(here);
                if (dist < distBest && isBuildable(building, tile) && isQueueable(building, tile)) {
                    tileBest = tile;
                    distBest = dist;
                }
            }
            return tileBest;
        }

        TilePosition closestExpoLocation(UnitType building, Position here)
        {
            // If we are expanding, it must be on an expansion area
            UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
            double best = 0.0;
            TilePosition tileBest = TilePositions::Invalid;

            // Fast expands must be as close to home and have a gas geyser
            if (Stations::getMyStations().size() == 1 && isBuildable(baseType, BWEB::Map::getNaturalTile()) && isQueueable(baseType, BWEB::Map::getNaturalTile()))
                tileBest = BWEB::Map::getNaturalTile();

            // Other expansions must be as close to home but as far away from the opponent
            else {
                for (auto &area : mapBWEM.Areas()) {
                    for (auto &base : area.Bases()) {
                        UnitType shuttle = Broodwar->self()->getRace().getTransport();

                        // Shuttle check for island bases, check enemy owned bases - DISABLED
                        if ((!area.AccessibleFrom(BWEB::Map::getMainArea()) /*&& vis(shuttle) <= 0*/) || Terrain::isInEnemyTerritory(base.Location()))
                            continue;

                        // Get production potential
                        int largePieces = 0;
                        for (auto &block : BWEB::Blocks::getBlocks()) {
                            if (mapBWEM.GetArea(block.getTilePosition()) != base.GetArea())
                                continue;
                            for (auto &large : block.getLargeTiles())
                                largePieces++;
                        }

                        // Get value of the expansion
                        double value = 0.0;
                        for (auto &mineral : base.Minerals())
                            value += double(mineral->Amount());
                        for (auto &gas : base.Geysers())
                            value += double(gas->Amount());
                        if (base.Geysers().size() == 0 && !Terrain::isIslandMap() && int(Stations::getMyStations().size()) < 3)
                            value = value / 1.5;

                        if (availablePieces[BWEB::Piece::Large] < 3 && largePieces < availablePieces[BWEB::Piece::Large])
                            value = value / 1.5;

                        // Get distance of the expansion
                        double distance;
                        if (!area.AccessibleFrom(BWEB::Map::getMainArea()))
                            distance = log(base.Center().getDistance(BWEB::Map::getMainPosition()));
                        else if (Players::getPlayers().size() > 3 || !Terrain::getEnemyStartingPosition().isValid())
                            distance = BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), base.Center());
                        else
                            distance = BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), base.Center()) / (BWEB::Map::getGroundDistance(Terrain::getEnemyStartingPosition(), base.Center()));

                        if (isBuildable(baseType, base.Location()) && isQueueable(baseType, base.Location()) && value / distance > best) {
                            best = value / distance;
                            tileBest = base.Location();
                        }
                    }
                }
            }
            currentExpansion = tileBest;
            return tileBest;
        }

        TilePosition furthestLocation(UnitType building, Position here)
        {
            auto tileBest = TilePositions::Invalid;
            auto distBest = 0.0;

            // Arrange what set we need to check
            set<TilePosition> placements;
            for (auto &block : BWEB::Blocks::getBlocks()) {
                Position blockCenter = Position(block.getTilePosition()) + Position(block.width() * 16, block.height() * 16);
                if (!Terrain::isInAllyTerritory(block.getTilePosition()))
                    continue;

                if (int(block.getMediumTiles().size()) < 2)
                    continue;

                // Setup placements
                if (building.tileWidth() == 4)
                    placements = block.getLargeTiles();
                else if (building.tileWidth() == 3)
                    placements = block.getMediumTiles();
                else
                    placements = block.getSmallTiles();

                for (auto &tile : placements) 
                    checkFurthest(building, tile, here, tileBest, distBest);                
            }
            return tileBest;
        }

        TilePosition closestLocation(UnitType building, Position here)
        {
            TilePosition tileBest = TilePositions::Invalid;
            double distBest = DBL_MAX;

            // Refineries are only built on my own gas resources
            if (building.isRefinery()) {
                for (auto &g : Resources::getMyGas()) {
                    auto &gas = *g;
                    auto dist = gas.getPosition().getDistance(here);

                    if (Stations::ownedBy(gas.getStation()) != PlayerState::Self
                        || !isQueueable(building, gas.getTilePosition())
                        || !isBuildable(building, gas.getTilePosition())
                        || dist >= distBest)
                        continue;

                    distBest = dist;
                    tileBest = gas.getTilePosition();
                }
                return tileBest;
            }

            // Arrange what set we need to check
            set<TilePosition> placements;
            for (auto &block : BWEB::Blocks::getBlocks()) {
                Position blockCenter = Position(block.getTilePosition()) + Position(block.width() * 16, block.height() * 16);
                if (!Terrain::isInAllyTerritory(block.getTilePosition()) && !BuildOrder::isProxy())
                    continue;

                // --- Move to PylonLocator ---
                if (Broodwar->self()->getRace() == Races::Protoss && building == Protoss_Pylon && vis(Protoss_Pylon) != 1) {
                    bool power = true;
                    bool solo = true;

                    // Check if we are fulfilling power requirements
                    if ((block.getLargeTiles().empty() && poweredLarge == 0) || (block.getMediumTiles().empty() && poweredMedium == 0 && !BuildOrder::isProxy()))
                        power = false;
                    if (poweredLarge > poweredMedium && block.getMediumTiles().empty())
                        power = false;
                    if (poweredMedium > poweredLarge && block.getLargeTiles().empty())
                        power = false;

                    // If we have no powered spots, can't build a solo spot
                    for (auto &small : block.getSmallTiles()) {
                        if (BWEB::Map::isUsed(small) != None
                            || (!Terrain::isInAllyTerritory(small) && !BuildOrder::isProxy() && !Terrain::isIslandMap()))
                            solo = false;
                    }

                    if (!power || !solo)
                        continue;
                }

                // --- Move to TechLocator ---
                // Don't let a Citadel get placed below any larges pieces, risks getting units stuck
                if (building == Protoss_Citadel_of_Adun && !block.getLargeTiles().empty())
                    continue;

                // Likewise, force a Core to be in a start block so we can eventually make a Citadel
                if (building == Protoss_Cybernetics_Core && block.getLargeTiles().empty() && Util::getTime() < Time(5, 0))
                    continue;

                // Get valid placements
                if (building.tileWidth() == 4)
                    placements = block.getLargeTiles();
                else if (building.tileWidth() == 3)
                    placements = block.getMediumTiles();
                else
                    placements = block.getSmallTiles();

                // Itrate placements checking for best
                for (auto &tile : placements)
                    checkClosest(building, tile, here, tileBest, distBest);
            }

            // --- Move to PylonLocator ---
            // Make sure we always place a pylon if we need large/medium spots or need supply
            if (!tileBest.isValid() && building == Protoss_Pylon) {
                distBest = DBL_MAX;
                for (auto &block : BWEB::Blocks::getBlocks()) {
                    Position blockCenter = Position(block.getTilePosition()) + Position(block.width() * 16, block.height() * 16);

                    if (block.getLargeTiles().size() == 0 && vis(Protoss_Pylon) == 0)
                        continue;
                    if (!Terrain::isInAllyTerritory(block.getTilePosition()))
                        continue;

                    if (!BuildOrder::isOpener() && (!hasPoweredPositions() || vis(Protoss_Pylon) < 20) && (poweredLarge > 0 || poweredMedium > 0)) {
                        if (poweredLarge == 0 && block.getLargeTiles().size() == 0)
                            continue;
                        if (poweredMedium == 0 && block.getMediumTiles().size() == 0)
                            continue;
                    }

                    placements = block.getSmallTiles();

                    for (auto &tile : placements)
                        checkClosest(building, tile, here, tileBest, distBest);
                }
            }
            return tileBest;
        }


        bool findResourceDepotLocation(UnitType building, TilePosition& placement)
        {
            if (!building.isResourceDepot())
                return false;

            if (Broodwar->self()->getRace() == Races::Zerg) {
                auto hatchCount = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);

                // Expand vs macro hatch
                auto expand = (hatchCount > 1 && BuildOrder::shouldExpand()) || (hatchCount <= (1 + BuildOrder::isFastExpand() - Players::ZvZ()));
                placement = expand ? closestExpoLocation(building, BWEB::Map::getMainPosition()) : closestLocation(building, BWEB::Map::getMainPosition());
            }
            else
                placement = closestExpoLocation(building, BWEB::Map::getMainPosition());
            return placement.isValid();
        }

        bool findProductionLocation(UnitType building, TilePosition& placement)
        {
            if (!isProductionType(building))
                return false;

            // If against a Zerg rush
            if (Strategy::enemyRush() && Players::vZ())
                placement = furthestLocation(building, (Position)BWEB::Map::getMainChoke()->Center());
            else
                placement = closestLocation(building, Position(BWEB::Map::getMainChoke()->Center()));
            return placement.isValid();
        }

        bool findTechLocation(UnitType building, TilePosition& placement)
        {
            if (!isTechType(building))
                return false;

            // Hide tech if needed or against a Zerg rush
            if ((BuildOrder::isHideTech() && (building == Protoss_Citadel_of_Adun || building == Protoss_Templar_Archives))
                || (Strategy::enemyRush() && Players::vZ()))
                placement = furthestLocation(building, (Position)BWEB::Map::getMainChoke()->Center());
            else
                placement = closestLocation(building, BWEB::Map::getMainPosition());
            return placement.isValid();
        }

        bool findDefenseLocation(UnitType building, TilePosition& placement)
        {
            if (!isDefensiveType(building))
                return false;

            auto chokeCenter = BuildOrder::isWallMain() ? Position(BWEB::Map::getMainChoke()->Center()) : Position(BWEB::Map::getNaturalChoke()->Center());

            if (Broodwar->self()->getRace() == Races::Protoss && vis(Protoss_Photon_Cannon) <= 1)
                chokeCenter = (Position(BWEB::Map::getMainChoke()->Center()) + Position(BWEB::Map::getNaturalChoke()->Center())) / 2;

            // Battery placing near chokes
            if (building == Protoss_Shield_Battery) {
                placement = closestDefLocation(building, chokeCenter);
                return placement.isValid();
            }

            // Defense placements near stations
            for (auto &station : Stations::getMyStations()) {
                auto &s = *station.second;

                int colonies = 0;
                for (auto& tile : s.getDefenseLocations()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Stations::needGroundDefenses(s) > colonies || Stations::needAirDefenses(s) > colonies)
                    placement = closestDefLocation(building, Position(s.getResourceCentroid()));

                if (placement.isValid())
                    return true;
            }

            // Defense placements near walls
            if (Terrain::getNaturalWall()) {
                int colonies = 0;
                for (auto& tile : Terrain::getNaturalWall()->getDefenses()) {
                    if (BWEB::Map::isUsed(tile) == Zerg_Creep_Colony)
                        colonies++;
                }

                if (Terrain::needAirDefenses(*Terrain::getNaturalWall()) > colonies || Terrain::needGroundDefenses(*Terrain::getNaturalWall()) > colonies)
                    placement = closestDefLocation(building, chokeCenter);
            }
            return placement.isValid();
        }

        bool findWallLocation(UnitType building, TilePosition& placement)
        {
            if (building != Protoss_Forge
                && building != Protoss_Gateway
                && building != Protoss_Pylon
                && building != Terran_Barracks
                && building != Terran_Supply_Depot
                && building != Zerg_Evolution_Chamber
                && building != Zerg_Hatchery
                && building != Zerg_Creep_Colony)
                return false;

            if ((!BuildOrder::isWallNat() && !BuildOrder::isWallMain()) || (Strategy::enemyBust() && BuildOrder::isOpener()))
                return false;

            // As Zerg, we have to place natural hatch before wall
            if (building == Zerg_Hatchery && buildingsQueued.find(BWEB::Map::getNaturalTile()) == buildingsQueued.end() && BWEB::Map::isUsed(BWEB::Map::getNaturalTile()) == UnitTypes::None)
                return false;

            // TODO: Currently only works for main/natural walls
            if (Broodwar->self()->getRace() == Races::Zerg)
                placement = closestWallLocation(building, BWEB::Map::getNaturalPosition());
            else {
                auto chokeCenter = BuildOrder::isWallMain() ? Position(BWEB::Map::getMainChoke()->Center()) : Position(BWEB::Map::getNaturalChoke()->Center());
                placement = closestWallLocation(building, chokeCenter);
            }
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

            // Check if this our second Pylon and we're hiding tech
            if (building == Protoss_Pylon && vis(Protoss_Pylon) == 2) {
                placement = furthestLocation(building, (Position)BWEB::Map::getMainChoke()->Center());
                if (placement.isValid())
                    return true;
            }

            // Check if this is our first Pylon versus Protoss
            if (vis(Protoss_Pylon) == 0 && Players::vP() && !BuildOrder::isWallNat() && !BuildOrder::isWallMain()) {
                placement = closestLocation(Protoss_Pylon, (Position)BWEB::Map::getMainChoke()->Center());
                if (placement.isValid())
                    return true;
            }

            // Check if we are fast expanding
            if (BWEB::Map::getNaturalChoke() && !Strategy::enemyBust() && (BuildOrder::isWallNat() || BuildOrder::isWallMain())) {
                placement = closestWallLocation(Protoss_Pylon, BWEB::Map::getMainPosition());
                if (placement.isValid())
                    return true;
            }

            // Check if any buildings lost power
            if (!unpoweredBlocks.empty()) {
                auto distBest = DBL_MAX;
                for (auto &block : unpoweredBlocks) {
                    for (auto &tile : block.getSmallTiles())
                        checkClosest(Protoss_Pylon, tile, Position(block.getTilePosition()), placement, distBest);
                }

                if (placement.isValid())
                    return true;
            }

            // Check if our main choke should get a Pylon for a Shield Battery
            if (vis(Protoss_Pylon) == 1 && !BuildOrder::isFastExpand() && (!Strategy::enemyRush() || !Players::vZ())) {
                placement = closestLocation(Protoss_Pylon, (Position)BWEB::Map::getMainChoke()->Center());
                if (placement.isValid())
                    return true;
            }

            // Check if we are being busted, add an extra pylon to the defenses
            if (Strategy::enemyBust() && Terrain::getNaturalWall() && BuildOrder::isWallNat()) {
                int cnt = 0;
                TilePosition sum(0, 0);
                TilePosition center(0, 0);
                for (auto &defense : Terrain::getNaturalWall()->getDefenses()) {
                    if (!Pylons::hasPower(defense, Protoss_Photon_Cannon)) {
                        sum += defense;
                        cnt++;
                    }
                }

                if (cnt != 0) {
                    center = sum / cnt;
                    Position c(center);
                    double distBest = DBL_MAX;

                    // Find unique closest tile to center of defenses
                    for (auto &tile : Terrain::getNaturalWall()->getDefenses()) {
                        Position defenseCenter = Position(tile) + Position(32, 32);
                        double dist = defenseCenter.getDistance(c);
                        if (dist < distBest && isQueueable(Protoss_Pylon, tile) && isBuildable(Protoss_Pylon, tile)) {
                            distBest = dist;
                            placement = tile;
                        }
                    }
                }

                if (placement.isValid())
                    return true;
            }

            // Check if any Nexus needs a Pylon for defense placement due to muta rush
            if ((Strategy::getEnemyBuild() == "2HatchMuta" || Strategy::getEnemyBuild() == "3HatchMuta")) {
                for (auto &s : Stations::getMyStations()) {
                    auto &station = *s.second;
                    placement = closestDefLocation(Protoss_Pylon, Position(station.getResourceCentroid()));
                    if (placement.isValid())
                        return true;
                }
            }

            // Check if we need powered spaces
            if (poweredLarge == 0 || poweredMedium == 0) {
                placement = closestLocation(Protoss_Pylon, BWEB::Map::getMainPosition());
                if (placement.isValid())
                    return true;
            }

            // Check if any Nexus needs a Pylon for defense placement
            if (com(Protoss_Pylon) >= (Players::vT() ? 5 : 3)) {
                for (auto &s : Stations::getMyStations()) {
                    auto &station = *s.second;
                    placement = closestDefLocation(Protoss_Pylon, Position(station.getResourceCentroid()));
                    if (placement.isValid())
                        return true;
                }
            }

            // Check if we can block an enemy expansion
            if (Util::getTime() > Time(8, 0) && int(Stations::getMyStations().size()) >= 2) {
                placement = Terrain::getEnemyExpand();
                if (placement.isValid() && isBuildable(Protoss_Pylon, placement) && isQueueable(Protoss_Pylon, placement))
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

            // Wall
            if (findWallLocation(building, placement))
                return placement;

            // Defensive
            if (findDefenseLocation(building, placement))
                return placement;

            // Proxy
            if (findProxyLocation(building, placement))
                return placement;

            // Resource depot
            if (findResourceDepotLocation(building, placement))
                return placement;

            // Pylon
            if (findPylonLocation(building, placement))
                return placement;

            // Production
            if (findProductionLocation(building, placement))
                return placement;

            // Tech
            if (findTechLocation(building, placement))
                return placement;

            // Refinery
            if (findRefineryLocation(building, placement))
                return placement;

            // HACK: Try to get a placement if we are being horror gated
            if (Strategy::enemyProxy() && Util::getTime() < Time(5, 0) && !isDefensiveType(building) && !building.isResourceDepot())
                placement = Broodwar->getBuildLocation(building, BWEB::Map::getMainTile(), 16);

            return placement;
        }

        void updateCommands(UnitInfo& building)
        {
            // Lair morphing
            if (building.getTilePosition() == BWEB::Map::getMainTile() && building.getType() == Zerg_Hatchery && BuildOrder::buildCount(Zerg_Lair) > vis(Zerg_Lair) + vis(Zerg_Hive) + lairsMorphing + hivesMorphing) {
                building.unit()->morph(Zerg_Lair);
                lairsMorphing++;
            }

            // Hive morphing
            else if (building.getTilePosition() == BWEB::Map::getMainTile() && building.getType() == Zerg_Lair && BuildOrder::buildCount(Zerg_Hive) > vis(Zerg_Hive) + hivesMorphing) {
                building.unit()->morph(Zerg_Hive);
                hivesMorphing++;
            }

            // Greater Spire morphing
            else if (building.getType() == Zerg_Spire && BuildOrder::buildCount(Zerg_Greater_Spire) > vis(Zerg_Greater_Spire))
                building.unit()->morph(Zerg_Greater_Spire);

            // Sunken / Spore morphing
            else if (building.getType() == Zerg_Creep_Colony) {
                auto morphType = UnitTypes::None;
                auto station = BWEB::Stations::getClosestStation(building.getTilePosition());
                auto wall = BWEB::Walls::getClosestWall(building.getTilePosition());

                auto closestAttacker = Util::getClosestUnit(building.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return !u.getType().isFlyer() && u.getGroundDamage() > 0.0 && u.getSpeed() > 0.0;
                });

                // If this is a Station defense
                if (station && station->getDefenseLocations().find(building.getTilePosition()) != station->getDefenseLocations().end()) {
                    if (Stations::needAirDefenses(*station) > 0)
                        morphType = Zerg_Spore_Colony;
                    if (Stations::needGroundDefenses(*station) > 0)
                        morphType = Zerg_Sunken_Colony;
                    building.unit()->morph(morphType);
                }

                // If this is a Wall defense
                if (wall && wall->getDefenses().find(building.getTilePosition()) != wall->getDefenses().end()) {

                    // Morph Spore if needed
                    if (Terrain::needAirDefenses(*wall) > 0)
                        morphType = Zerg_Spore_Colony;

                    // Morph Sunken if needed
                    if (Terrain::needGroundDefenses(*wall) > 0) {
                        if (Strategy::enemyPressure())
                            morphType = Zerg_Sunken_Colony;

                        if (closestAttacker) {
                            auto timeToEngage = closestAttacker->getPosition().getDistance(building.getPosition()) / closestAttacker->getSpeed();
                            if (timeToEngage <= Zerg_Sunken_Colony.buildTime())
                                morphType = Zerg_Sunken_Colony;                            
                        }
                    }
                    building.unit()->morph(morphType);
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

                // Wall lift
                bool wallCheck = (Terrain::getNaturalWall() && building.getPosition().getDistance(Terrain::getNaturalWall()->getCentroid()) < 256.0)
                    || (Terrain::getMainWall() && building.getPosition().getDistance(Terrain::getMainWall()->getCentroid()) < 256.0);

                if (wallCheck && !building.unit()->isFlying()) {
                    if (Players::getSupply(PlayerState::Self) > 120 || BuildOrder::firstReady()) {
                        building.unit()->lift();
                        BWEB::Map::onUnitDestroy(building.unit());
                    }
                }

                // Find landing location as production building
                else if ((Players::getSupply(PlayerState::Self) > 120 || BuildOrder::firstReady()) && building.unit()->isFlying()) {
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

            // Cancelling buildings that are being warped in but being attacked
            if (!building.unit()->isCompleted()) {
                auto bulletDamage = 0;
                for (auto &unit : building.getTargetedBy()) {
                    if (unit.lock())
                        bulletDamage+= int(unit.lock()->getGroundDamage());
                }

                if (bulletDamage > building.getHealth() + building.getShields()) {
                    building.unit()->cancelConstruction();
                    Broodwar << "Cancelled Photon, incoming damage at " << bulletDamage << " while health at " << building.getHealth() + building.getShields() << endl;
                }
            }
        }

        void updateInformation(UnitInfo& building)
        {
            // If a building is unpowered, get a pylon placement ready
            if (building.getType().requiresPsi() && !Pylons::hasPower(building.getTilePosition(), building.getType())) {
                auto block = BWEB::Blocks::getClosestBlock(building.getTilePosition());
                if (block)
                    unpoweredBlocks.push_back(*block);
            }
        }

        void updateBuildings()
        {
            // Reset counters
            poweredSmall = 0; poweredMedium = 0; poweredLarge = 0;
            lairsMorphing = 0, hivesMorphing = 0;
            availablePieces.clear();
            unpoweredBlocks.clear();

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

            // Add up how many powered available spots we have        
            for (auto &block : BWEB::Blocks::getBlocks()) {
                for (auto &tile : block.getSmallTiles()) {
                    if (isBuildable(smallType, tile) && isQueueable(smallType, tile))
                        poweredSmall++;
                    else if (BWEB::Map::isUsed(tile) == None && Terrain::isInAllyTerritory(tile))
                        availablePieces[BWEB::Piece::Small]++;
                }
                for (auto &tile : block.getMediumTiles()) {
                    if (isBuildable(mediumType, tile) && isQueueable(mediumType, tile))
                        poweredMedium++;
                    else if (BWEB::Map::isUsed(tile) == None && Terrain::isInAllyTerritory(tile))
                        availablePieces[BWEB::Piece::Medium]++;
                }
                for (auto &tile : block.getLargeTiles()) {
                    if (isBuildable(largeType, tile) && isQueueable(largeType, tile))
                        poweredLarge++;
                    else if (BWEB::Map::isUsed(tile) == None && Terrain::isInAllyTerritory(tile))
                        availablePieces[BWEB::Piece::Large]++;
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

        void queueBuildings()
        {
            queuedMineral = 0;
            queuedGas = 0;
            buildingsQueued.clear();

            // Add up how many buildings we have assigned to workers
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getBuildType().isValid() && unit.getBuildPosition().isValid()) {

                    // Keep queued building after 4 minutes
                    if (Util::getTime() > Time(4, 0))
                        buildingsQueued[unit.getBuildPosition()] = unit.getBuildType();

                    // Dequeue to continue to verify building location is correct
                    else {
                        unit.setBuildingType(UnitTypes::None);
                        unit.setBuildPosition(TilePositions::Invalid);
                    }
                }
            }

            // Add up how many more buildings of each type we need
            for (auto &[building, count] : BuildOrder::getBuildQueue()) {
                int queuedCount = 0;

                auto morphed = !building.whatBuilds().first.isWorker();
                auto addon = building.isAddon();

                if (addon || morphed || !building.isBuilding())
                    continue;

                // If the building morphed from another building type, add the visible amount of child type to the parent type
                int morphOffset = 0;
                if (building == Zerg_Creep_Colony)
                    morphOffset = vis(Zerg_Sunken_Colony) + vis(Zerg_Spore_Colony);
                if (building == Zerg_Hatchery)
                    morphOffset = vis(Zerg_Lair) + vis(Zerg_Hive);
                if (building == Zerg_Lair)
                    morphOffset = vis(Zerg_Hive);

                // Reserve building if our reserve count is higher than our visible count
                for (auto &[_, queuedType] : buildingsQueued) {
                    if (queuedType == building) {
                        queuedCount++;

                        // If we want to reserve more than we have, reserve resources, starts at 8 supply currently
                        if (Players::getSupply(PlayerState::Self) >= 16 && count > vis(building) + morphOffset) {
                            queuedMineral += building.mineralPrice();
                            queuedGas += building.gasPrice();
                        }
                    }
                }

                // Queue building if our actual count is higher than our visible count
                if (count > queuedCount + vis(building) + morphOffset) {
                    auto here = getBuildLocation(building);

                    auto &builder = Util::getClosestUnit(Position(here), PlayerState::Self, [&](auto &u) {
                        return u.getRole() == Role::Worker && !u.isStuck() && (!u.hasResource() || u.getResource().getType().isMineralField()) && u.getBuildType() == None;
                    });

                    if (here.isValid() && builder && Workers::shouldMoveToBuild(*builder, here, building)) {
                        builder->setBuildingType(building);
                        builder->setBuildPosition(here);
                        buildingsQueued[here] = building;
                    }
                }
            }
        }
    }

    bool isQueueable(UnitType building, TilePosition buildTilePosition)
    {
        // Check if there's a building queued there already
        for (auto &queued : buildingsQueued) {
            if (queued.first == buildTilePosition)
                return false;
        }
        return true;
    }

    bool isBuildable(UnitType building, TilePosition here)
    {
        // Refinery only on Geysers
        if (building.isRefinery()) {
            for (auto &g : Resources::getMyGas()) {
                ResourceInfo &gas = *g;

                if (here == gas.getTilePosition() && gas.getType() != building)
                    return true;
            }
            return false;
        }

        // Used tile check
        for (int x = here.x; x < here.x + building.tileWidth(); x++) {
            for (int y = here.y; y < here.y + building.tileHeight(); y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    return false;

                if (building.getRace() == Races::Zerg && building.requiresCreep() && !Broodwar->hasCreep(t))
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

        // Psi/Creep check
        if (building.requiresPsi() && !Pylons::hasPower(here, building))
            return false;

        if (building == Zerg_Hatchery) {
            auto &builder = Util::getClosestUnit((Position)here, PlayerState::Self, [&](auto &u) {
                return u.getType().isWorker();
            });
            if (builder) {
                if (!Broodwar->canBuildHere(here, building, builder->unit()))
                    return false;
            }
            else
                return false;
        }
        return true;
    }

    bool overlapsQueue(UnitInfo& unit, Position here)
    {
        // Check if there's a building queued there already
        for (auto &[tile, building] : buildingsQueued) {

            if (Broodwar->self()->minerals() < building.mineralPrice()
                || Broodwar->self()->gas() < building.gasPrice()
                || unit.getBuildPosition() == tile)
                continue;

            // Bordering box of the queued building
            const auto buildingTopLeft = Position(tile);
            const auto buildingBotRight = buildingTopLeft + Position(building.tileSize());

            // Bordering box of the unit
            const auto unitTopLeft = here + Position(-unit.getType().width() / 2, -unit.getType().height() / 2);
            const auto unitTopRight = here + Position(unit.getType().width() / 2, -unit.getType().height() / 2);
            const auto unitBotLeft = here + Position(-unit.getType().width() / 2, unit.getType().height() / 2);
            const auto unitBotRight = here + Position(unit.getType().width() / 2, unit.getType().height() / 2);

            return (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitTopLeft))
                || (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitTopRight))
                || (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitBotLeft))
                || (Util::rectangleIntersect(buildingTopLeft, buildingBotRight, unitBotRight));
            
        }
        return false;
    }

    bool hasPoweredPositions() { return (poweredLarge > 0 && poweredMedium > 0); }
    int getQueuedMineral() { return queuedMineral; }
    int getQueuedGas() { return queuedGas; }
    TilePosition getCurrentExpansion() { return currentExpansion; }

    void onFrame()
    {
        updateBuildings();
        queueBuildings();
    }

    void onStart()
    {
        // Initialize Blocks
        BWEB::Blocks::findBlocks();
    }
}