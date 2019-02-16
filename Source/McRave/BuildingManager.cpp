#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Buildings {

    namespace {

        int queuedMineral, queuedGas, nukesAvailable;
        int poweredSmall, poweredMedium, poweredLarge;
        int lairsMorphing, hivesMorphing;

        map <TilePosition, UnitType> buildingsQueued;
        TilePosition currentExpansion;

        void checkBest(UnitType building, TilePosition placement, Position desired, TilePosition& tileBest, double& distBest) {
            auto center = Position(placement) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
            auto current = center.getDistance(desired);
            if (current < distBest && isQueueable(building, placement) && isBuildable(building, placement)) {
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
            auto natOrMain = Terrain::getPlayerStartingTilePosition() == station->getBWEMBase()->Location() || BWEB::Map::getNaturalTile() == station->getBWEMBase()->Location();


            // Check closest stataion to see if one of their defense locations is best
            if (station) {
                for (auto &defense : station->getDefenseLocations()) {

                    // HACK: Pylon is separated because we want one unique best buildable position to check, rather than next best buildable position
                    if (building == Protoss_Pylon) {
                        double dist = Position(defense).getDistance(Position(here));
                        if (dist < distBest && (natOrMain || (defense.y <= station->getBWEMBase()->Location().y + 1 && defense.y >= station->getBWEMBase()->Location().y))) {
                            distBest = dist;
                            tileBest = defense;
                        }
                    }
                    else
                        checkBest(building, defense, here, tileBest, distBest);
                }
            }

            // Check closest Wall to see if one of their defense locations is best
            if (wall) {
                if (building == Protoss_Photon_Cannon || building == Zerg_Creep_Colony) {
                    for (auto &wall : BWEB::Walls::getWalls()) {
                        for (auto &tile : wall.getDefenses())
                            checkBest(building, tile, here, tileBest, distBest);
                    }
                }
            }

            return tileBest;
        }

        TilePosition closestProdLocation(UnitType building, Position here)
        {
            TilePosition tileBest = TilePositions::Invalid;
            double distBest = DBL_MAX;

            // TODO: Rename this or use the anonymous namespace one above
            const auto checkBestLocal = [&](Position blockCenter, set<TilePosition>& placements) {
                // Against rushes, hide our buildings away from our ramp unless defensive structure
                if ((Strategy::enemyRush() && Strategy::getEnemyBuild() != "2Gate" && building != Protoss_Shield_Battery && building != Protoss_Photon_Cannon)) {
                    distBest = 0.0;
                    double dist = blockCenter.getDistance((Position)BWEB::Map::getMainChoke()->Center());
                    if (dist > distBest) {
                        for (auto tile : placements) {
                            if (isQueueable(building, tile) && isBuildable(building, tile) && Terrain::isInAllyTerritory(tile))
                                distBest = dist, tileBest = tile;
                        }
                    }
                }

                // Else, closest to here
                else {
                    for (auto tile : placements) {
                        Position tileCenter = Position(tile) + Position(building.tileWidth() * 16, building.tileHeight() * 16);
                        double dist = tileCenter.getDistance(here);
                        if (dist < distBest && isQueueable(building, tile) && isBuildable(building, tile))
                            distBest = dist, tileBest = tile;
                    }
                }
                return tileBest.isValid();
            };

            // Refineries are only built on my own gas resources
            if (building.isRefinery()) {
                for (auto &g : Resources::getMyGas()) {
                    ResourceInfo &gas = *g;
                    double dist = gas.getPosition().getDistance(here);

                    if (isQueueable(building, gas.getTilePosition()) && isBuildable(building, gas.getTilePosition()) && gas.getResourceState() != ResourceState::None && dist < distBest) {
                        distBest = dist;
                        tileBest = gas.getTilePosition();
                    }
                }
                return tileBest;
            }

            // Arrange what set we need to check
            set<TilePosition> placements;
            for (auto &block : BWEB::Blocks::getBlocks()) {
                Position blockCenter = Position(block.getTilePosition()) + Position(block.width() * 16, block.height() * 16);

                if (Broodwar->self()->getRace() == Races::Protoss && building == Protoss_Pylon) {
                    bool power = true;
                    bool solo = true;

                    if ((block.getLargeTiles().empty() && poweredLarge == 0) || (block.getMediumTiles().empty() && poweredMedium == 0 && !BuildOrder::isProxy()))
                        power = false;
                    if (poweredLarge > poweredMedium && block.getMediumTiles().empty())
                        power = false;
                    if (poweredMedium >= poweredLarge && block.getLargeTiles().empty())
                        power = false;

                    // If we have no powered spots, can't build a solo spot
                    for (auto &small : block.getSmallTiles()) {
                        if (BWEB::Map::isUsed(small)
                            || (!Terrain::isInAllyTerritory(small) && !BuildOrder::isProxy() && !Terrain::isIslandMap()))
                            solo = false;
                    }

                    // HACK: 2nd pylon by choke if we're facing a 2 gate
                    if (Broodwar->self()->visibleUnitCount(Protoss_Pylon) == 1 && Strategy::getEnemyBuild() == "2Gate")
                        power = true;

                    if (!power || !solo)
                        continue;
                }

                // Don't let a Stargate get placed below any medium pieces, risks getting units stuck
                if (building == Protoss_Stargate && !block.getMediumTiles().empty())
                    continue;

                // Setup placements
                if (building.tileWidth() == 4)
                    placements = block.getLargeTiles();
                else if (building.tileWidth() == 3)
                    placements = block.getMediumTiles();
                else
                    placements = block.getSmallTiles();

                checkBestLocal(blockCenter, placements);
            }

            // Make sure we always place a pylon
            if (!tileBest.isValid() && Broodwar->self()->getRace() == Races::Protoss && building == Protoss_Pylon) {
                distBest = DBL_MAX;
                for (auto &block : BWEB::Blocks::getBlocks()) {
                    Position blockCenter = Position(block.getTilePosition()) + Position(block.width() * 16, block.height() * 16);

                    if (block.getLargeTiles().size() == 0 && Broodwar->self()->visibleUnitCount(Protoss_Pylon) == 0)
                        continue;
                    if (!Terrain::isInAllyTerritory(block.getTilePosition()))
                        continue;

                    if (!BuildOrder::isOpener() && !hasPoweredPositions() && Broodwar->self()->visibleUnitCount(Protoss_Pylon) > 10 && (poweredLarge > 0 || poweredMedium > 0)) {
                        if (poweredLarge == 0 && block.getLargeTiles().size() == 0)
                            continue;
                        if (poweredMedium == 0 && block.getMediumTiles().size() == 0)
                            continue;
                    }

                    placements = block.getSmallTiles();
                    checkBestLocal(blockCenter, placements);
                }
            }
            return tileBest;
        }

        TilePosition closestWallLocation(UnitType building, Position here)
        {
            auto tileBest = TilePositions::Invalid;
            auto distBest = DBL_MAX;
            const BWEB::Walls::Wall* wall = nullptr;
            set<TilePosition> placements;

            if (BuildOrder::isWallMain())
                wall = Terrain::getMainWall();
            else if (BuildOrder::isWallNat() || Broodwar->self()->getRace() == Races::Zerg)
                wall = Terrain::getNaturalWall();

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

        TilePosition closestExpoLocation()
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

                        // Shuttle check for island bases, check enemy owned bases
                        if ((!area.AccessibleFrom(BWEB::Map::getMainArea()) && Broodwar->self()->visibleUnitCount(shuttle) <= 0) || Terrain::isInEnemyTerritory(base.Location()))
                            continue;

                        // Get value of the expansion
                        double value = 0.0;
                        for (auto &mineral : base.Minerals())
                            value += double(mineral->Amount());
                        for (auto &gas : base.Geysers())
                            value += double(gas->Amount());
                        if (base.Geysers().size() == 0 && !Terrain::isIslandMap())
                            value = value / 2.5;

                        // Get distance of the expansion
                        double distance;
                        if (!area.AccessibleFrom(BWEB::Map::getMainArea()))
                            distance = log(base.Center().getDistance(BWEB::Map::getMainPosition()));
                        else if (Players::getPlayers().size() > 1 || !Terrain::getEnemyStartingPosition().isValid())
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

        TilePosition findResourceDepotLocation()
        {
            auto here = TilePositions::Invalid;
            auto depot = Broodwar->self()->getRace().getResourceDepot();

            if (Broodwar->self()->getRace() == Races::Zerg) {
                auto hatchCount = vis(Zerg_Hatchery) + vis(Zerg_Lair) + vis(Zerg_Hive);
                if (BWEB::Map::getNaturalChoke() && hatchCount >= 2) {
                    here = closestWallLocation(depot, Position(BWEB::Map::getNaturalChoke()->Center()));
                    if (here.isValid() && isBuildable(depot, here) && isQueueable(depot, here))
                        return here;
                }

                // Expand on 1 hatch, 3 hatch, 5 hatch.. etc                    
                auto expand = (1 + hatchCount) / 2 <= int(Stations::getMyStations().size());
                here = expand ? closestExpoLocation() : closestProdLocation(depot, BWEB::Map::getMainPosition());
            }
            else
                here = closestExpoLocation();
            return here;
        }

        TilePosition findPylonLocation()
        {
            auto here = TilePositions::Invalid;

            // Check if we are fast expanding
            if (BWEB::Map::getNaturalChoke() && !Strategy::enemyBust() && (BuildOrder::isWallNat() || BuildOrder::isWallMain())) {
                here = closestWallLocation(Protoss_Pylon, BWEB::Map::getMainPosition());
                if (here.isValid() && isBuildable(Protoss_Pylon, here))
                    return here;
            }

            // Check if we are being busted, add an extra pylon to the defenses
            if (Strategy::enemyBust() && Terrain::getNaturalWall()) {
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
                        if (dist < distBest) {
                            distBest = dist;
                            here = tile;
                        }
                    }
                }

                if (here.isValid() && isQueueable(Protoss_Pylon, here) && isBuildable(Protoss_Pylon, here))
                    return here;
            }

            // Check if any Nexus needs a Pylon for defense placement
            if (Broodwar->self()->completedUnitCount(Protoss_Pylon) >= (4 - (2 * Terrain::isIslandMap())) && Broodwar->self()->completedUnitCount(Protoss_Forge) >= 1) {
                for (auto &s : Stations::getMyStations()) {
                    auto &station = *s.second;
                    here = closestDefLocation(Protoss_Pylon, Position(station.getResourceCentroid()));
                    if (here.isValid() && isBuildable(Protoss_Pylon, here) && isQueueable(Protoss_Pylon, here))
                        return here;
                }
            }

            // Check if our main choke should get a Pylon
            if (Broodwar->self()->visibleUnitCount(Protoss_Pylon) == 1 + int(Players::vZ()) && !Terrain::isIslandMap()) {
                here = closestProdLocation(Protoss_Pylon, (Position)BWEB::Map::getMainChoke()->Center());
                if (here.isValid() && isBuildable(Protoss_Pylon, here) && isQueueable(Protoss_Pylon, here))
                    return here;
            }

            // Resort to finding a production location
            here = closestProdLocation(Protoss_Pylon, BWEB::Map::getMainPosition());
            return here;
        }

        TilePosition findDefenseLocation(UnitType building)
        {
            auto here = TilePositions::Invalid;

            // Battery placing near chokes
            if (building == Protoss_Shield_Battery) {
                if (BuildOrder::isWallNat())
                    here = closestProdLocation(building, (Position)BWEB::Map::getNaturalChoke()->Center());
                else
                    here = closestProdLocation(building, (Position)BWEB::Map::getMainChoke()->Center());

                if (here.isValid())
                    return here;
            }

            if (Broodwar->self()->completedUnitCount(Protoss_Forge) >= 1 && ((Broodwar->self()->visibleUnitCount(Protoss_Nexus) >= 3 + (Players::getNumberTerran() > 0 || Players::getNumberProtoss() > 0)) || (Terrain::isIslandMap() && Players::getNumberZerg() > 0))) {
                for (auto &station : Stations::getMyStations()) {
                    auto &s = *station.second;

                    if (Stations::needDefenses(s))
                        here = closestDefLocation(building, Position(s.getResourceCentroid()));

                    if (here.isValid())
                        break;
                }
            }

            else if (!here.isValid() && Terrain::getNaturalWall()) // Assumes we want to put it at the natural wall
                here = closestDefLocation(building, Terrain::getNaturalWall()->getCentroid());

            else if (building == Zerg_Creep_Colony)
                here = closestDefLocation(building, BWEB::Map::getNaturalPosition());



            return here;
        }

        TilePosition findWallLocation(UnitType building)
        {
            // TODO: Pass wall in as well to ensure position exists in wall
            auto chokeCenter = BuildOrder::isWallMain() ? Position(BWEB::Map::getMainChoke()->Center()) : Position(BWEB::Map::getNaturalChoke()->Center());
            return closestWallLocation(building, chokeCenter);
        }

        TilePosition findProxyLocation(UnitType building)
        {
            // TODO: Improve proxy location (add a BWEB block)
            return closestProdLocation(building, mapBWEM.Center());
        }

        TilePosition getBuildLocation(UnitType building)
        {
            auto placement = TilePositions::Invalid;

            auto isDefensiveBuilding = building == Protoss_Photon_Cannon
                || building == Protoss_Shield_Battery
                || building == Zerg_Creep_Colony
                || building == Terran_Missile_Turret
                || building == Terran_Bunker;

            auto isWallPiece = building == Protoss_Forge
                || building == Protoss_Gateway
                || building == Protoss_Pylon
                || building == Terran_Barracks
                || building == Terran_Supply_Depot
                || building == Zerg_Evolution_Chamber
                || building == Zerg_Hatchery
                || building == Zerg_Creep_Colony;

            auto canWall = (BuildOrder::isWallNat() && Terrain::getNaturalWall() && BWEB::Map::getNaturalChoke())
                || (BuildOrder::isWallMain() && Terrain::getMainWall() && BWEB::Map::getMainChoke());

            auto canProxy = BuildOrder::isOpener() && BuildOrder::isProxy();

            // Specific placements
            if (isDefensiveBuilding)
                return findDefenseLocation(building);

            if (!placement.isValid() && isWallPiece && canWall && (!Strategy::enemyBust() || !BuildOrder::isOpener()))
                placement = findWallLocation(building);
            if (!placement.isValid() && canProxy)
                placement = findProxyLocation(building);

            // General placements
            if (!placement.isValid() && building.isResourceDepot())
                placement = findResourceDepotLocation();
            if (!placement.isValid() && building == Protoss_Pylon)
                placement = findPylonLocation();

            // Default to finding a production location
            if (!placement.isValid() && !building.isResourceDepot())
                placement = closestProdLocation(building, BWEB::Map::getMainPosition());

            return placement;
        }

        void updateCommands(UnitInfo& building)
        {
            // Lair morphing
            if (building.getType() == Zerg_Hatchery && BuildOrder::buildCount(Zerg_Lair) > Broodwar->self()->visibleUnitCount(Zerg_Lair) + Broodwar->self()->visibleUnitCount(Zerg_Hive) + lairsMorphing + hivesMorphing) {
                building.unit()->morph(Zerg_Lair);
                lairsMorphing++;
            }

            // Hive morphing
            else if (building.getType() == Zerg_Lair && BuildOrder::buildCount(Zerg_Hive) > Broodwar->self()->visibleUnitCount(Zerg_Hive) + hivesMorphing) {
                building.unit()->morph(Zerg_Hive);
                hivesMorphing++;
            }

            // Greater Spire morphing
            else if (building.getType() == Zerg_Spire && BuildOrder::buildCount(Zerg_Greater_Spire) > Broodwar->self()->visibleUnitCount(Zerg_Greater_Spire))
                building.unit()->morph(Zerg_Greater_Spire);

            // Sunken morphing
            else if (building.getType() == Zerg_Creep_Colony)
                building.unit()->morph(Zerg_Sunken_Colony);

            // Terran building needs new scv
            else if (building.getType().getRace() == Races::Terran && !building.unit()->isCompleted() && !building.unit()->getBuildUnit()) {
                auto builder = Util::getClosestUnit(building.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getType().isWorker() && u.getBuildingType() == None;
                });

                if (builder)
                    builder->unit()->rightClick(building.unit());
            }

            // Nuke counting
            else if (building.getType() == Terran_Nuclear_Silo && building.unit()->hasNuke())
                nukesAvailable++;

            // Barracks
            if (building.getType() == Terran_Barracks) {

                // Wall lift
                bool wallCheck = (Terrain::getNaturalWall() && building.getPosition().getDistance(Terrain::getNaturalWall()->getCentroid()) < 256.0)
                    || (Terrain::getMainWall() && building.getPosition().getDistance(Terrain::getMainWall()->getCentroid()) < 256.0);

                if (wallCheck && !building.unit()->isFlying()) {
                    if (Units::getSupply() > 120 || BuildOrder::firstReady()) {
                        building.unit()->lift();
                        BWEB::Map::onUnitDestroy(building.unit());
                    }
                }

                // Find landing location as production building
                else if ((Units::getSupply() > 120 || BuildOrder::firstReady()) && building.unit()->isFlying()) {
                    auto here = closestProdLocation(building.getType(), BWEB::Map::getMainPosition());
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
                if (building.hasTarget() && building.getTarget().unit()->exists() && !Command::overlapsAllyDetection(building.getTarget().getPosition())) {
                    building.unit()->useTech(TechTypes::Scanner_Sweep, building.getTarget().getPosition());
                    Command::addAction(building.unit(), building.getTarget().getPosition(), TechTypes::Scanner_Sweep);
                }
            }

            // Cancelling Refinerys for our gas trick
            if (BuildOrder::isGasTrick() && building.getType().isRefinery() && !building.unit()->isCompleted() && BuildOrder::buildCount(building.getType()) < vis(building.getType())) {
                building.unit()->cancelMorph();
                BWEB::Map::removeUsed(building.getTilePosition(), 4, 2);
            }

            // Disabled, too buggy
            //// Cancelling buildings we don't want
            //if (building.getType().getRace() != Races::Zerg && BuildOrder::buildCount(building.getType()) < vis(building.getType())) {
            //    building.unit()->cancelConstruction();
            //}
        }

        void updateBuildings()
        {
            // Reset counters
            poweredSmall = 0; poweredMedium = 0; poweredLarge = 0;
            nukesAvailable = 0;
            lairsMorphing = 0, hivesMorphing = 0;

            // Add up how many powered available spots we have		
            for (auto &block : BWEB::Blocks::getBlocks()) {
                if (!Terrain::isInAllyTerritory(block.getTilePosition()))
                    continue;

                for (auto &tile : block.getSmallTiles()) {
                    if (isBuildable(Protoss_Photon_Cannon, tile) && isQueueable(Protoss_Photon_Cannon, tile))
                        poweredSmall++;
                }
                for (auto &tile : block.getMediumTiles()) {
                    if (isBuildable(Protoss_Forge, tile) && isQueueable(Protoss_Forge, tile))
                        poweredMedium++;
                }
                for (auto &tile : block.getLargeTiles()) {
                    if (isBuildable(Protoss_Gateway, tile) && isQueueable(Protoss_Gateway, tile))
                        poweredLarge++;
                }
            }

            // Update all my buildings
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;
                updateCommands(unit);
            }
        }

        void queueBuildings()
        {
            queuedMineral = 0;
            queuedGas = 0;
            buildingsQueued.clear();

            // 1) Add up how many buildings we have assigned to workers
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (unit.getBuildingType().isValid() && unit.getBuildPosition().isValid())
                    buildingsQueued[unit.getBuildPosition()] = unit.getBuildingType();
            }

            // 2) Add up how many more buildings of each type we need
            for (auto &i : BuildOrder::getItemQueue()) {
                UnitType building = i.first;
                BuildOrder::Item item = i.second;
                int queuedCount = 0;

                auto morphed = !building.whatBuilds().first.isWorker();
                auto addon = building.isAddon();

                if (addon || morphed || !building.isBuilding())
                    continue;

                // If the building morphed from another building type, add the visible amount of child type to the parent type
                // i.e. When we have a queued Hatchery, we need to check how many Lairs and Hives we have.
                int morphOffset = 0;
                if (building == Zerg_Creep_Colony)
                    morphOffset = Broodwar->self()->visibleUnitCount(Zerg_Sunken_Colony) + Broodwar->self()->visibleUnitCount(Zerg_Spore_Colony);
                if (building == Zerg_Hatchery)
                    morphOffset = Broodwar->self()->visibleUnitCount(Zerg_Lair) + Broodwar->self()->visibleUnitCount(Zerg_Hive);
                if (building == Zerg_Lair)
                    morphOffset = Broodwar->self()->visibleUnitCount(Zerg_Hive);

                // 3) Reserve building if our reserve count is higher than our visible count
                for (auto &queued : buildingsQueued) {
                    auto queuedType = queued.second;
                    if (queuedType == building) {
                        queuedCount++;

                        // If we want to reserve more than we have, reserve resources
                        if (item.getReserveCount() > Broodwar->self()->visibleUnitCount(building) + morphOffset) {
                            queuedMineral += building.mineralPrice();
                            queuedGas += building.gasPrice();
                        }
                    }
                }

                // 4) Queue building if our actual count is higher than our visible count
                if (item.getActualCount() > queuedCount + Broodwar->self()->visibleUnitCount(building) + morphOffset) {
                    auto here = getBuildLocation(building);

                    auto builder = Util::getClosestUnit(Position(here), PlayerState::Self, [&](auto &u) {
                        return u.getRole() == Role::Worker && (!u.hasResource() || u.getResource().getType().isMineralField()) && u.getBuildingType() == None;
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

                if (here == gas.getTilePosition() && gas.getResourceState() != ResourceState::None && gas.getType() != building)
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
                if (BWEB::Map::isUsed(t))
                    return false;
            }
        }

        // Addon room check
        if (building.canBuildAddon()) {
            if (BWEB::Map::isUsed(here + TilePosition(4, 1)))
                return false;
        }

        // Psi/Creep check
        if (building.requiresPsi() && !Pylons::hasPower(here, building))
            return false;

        if (building == Zerg_Hatchery) {
            auto builder = Util::getClosestUnit((Position)here, PlayerState::Self, [&](auto &u) {
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

    bool overlapsQueue(UnitType type, TilePosition here)
    {
        // HACK: We want to really make sure a unit doesn't block a building, so fudge it by a full tile
        int safetyOffset = int(!type.isBuilding());

        // Check if there's a building queued there already
        for (auto &queued : buildingsQueued) {
            TilePosition tile = queued.first;
            UnitType building = queued.second;

            for (int x = here.x - safetyOffset; x < here.x + safetyOffset; x++) {
                for (int y = here.y - safetyOffset; y < here.y + safetyOffset; y++) {
                    TilePosition t(x, y);
                    auto tx = tile.x * 32;
                    auto ty = tile.y * 32;
                    auto topLeft = Position(tx, ty);
                    auto botRight = Position(tx + building.tileWidth() * 32, ty + building.tileHeight() * 32);

                    if (Util::rectangleIntersect(topLeft, botRight, Position(t)))
                        return true;
                }
            }
        }
        return false;
    }

    bool hasPoweredPositions() { return (poweredLarge > 0 && poweredMedium > 0); }
    int getQueuedMineral() { return queuedMineral; }
    int getQueuedGas() { return queuedGas; }
    int getNukesAvailable() { return nukesAvailable; }
    TilePosition getCurrentExpansion() { return currentExpansion; }

    void onFrame()
    {
        updateBuildings();
        queueBuildings();
    }
}