#include "McRave.h"
#include "UnitInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Buildings {

    namespace {

        int queuedMineral, queuedGas, nukesAvailable;
        int poweredSmall, poweredMedium, poweredLarge;
        int lairsMorphing, hivesMorphing;

        map <TilePosition, UnitType> buildingsQueued;
        TilePosition currentExpansion;

        TilePosition findDefLocation(UnitType building, Position here)
        {
            set<TilePosition> placements;
            TilePosition tileBest = TilePositions::Invalid;
            double distBest = DBL_MAX;
            auto station = BWEB::Stations::getClosestStation(TilePosition(here));

            if (!station)
                return TilePositions::Invalid;

            const auto natOrMain = [&]() {
                if (Terrain().getPlayerStartingTilePosition() == station->BWEMBase()->Location() || BWEB::Map::getNaturalTile() == station->BWEMBase()->Location())
                    return true;
                return false;
            };

            // Check all Stations to see if one of their defense locations is best		
            for (auto &defense : station->DefenseLocations()) {

                // Pylon is separated because we want one unique best buildable position to check, rather than next best buildable position
                if (building == UnitTypes::Protoss_Pylon) {
                    double dist = Position(defense).getDistance(Position(here));
                    if (dist < distBest && (natOrMain() || (defense.y <= station->BWEMBase()->Location().y + 1 && defense.y >= station->BWEMBase()->Location().y)))
                        distBest = dist, tileBest = defense;
                }
                else {
                    double dist = Position(defense).getDistance(here);
                    if (dist < distBest && isQueueable(building, defense) && isBuildable(building, defense))
                        distBest = dist, tileBest = defense;
                }
            }

            // If this is a Cannon, also check the Walls defense locations
            if (building == UnitTypes::Protoss_Photon_Cannon || building == UnitTypes::Zerg_Creep_Colony) {
                for (auto &wall : BWEB::Walls::getWalls()) {
                    for (auto &tile : wall.getDefenses()) {
                        Position defenseCenter = Position(tile) + Position(32, 32);
                        double dist = defenseCenter.getDistance(here);
                        if (dist < distBest && isQueueable(building, tile) && isBuildable(building, tile))
                            distBest = dist, tileBest = tile;
                    }
                }
            }

            return tileBest;
        }

        TilePosition findProdLocation(UnitType building, Position here)
        {
            TilePosition tileBest = TilePositions::Invalid;
            double distBest = DBL_MAX;

            const auto checkBest = [&](Position blockCenter, set<TilePosition>& placements) {
                // Against rushes, hide our buildings away from our ramp unless defensive structure
                if ((Strategy::enemyRush() && Strategy::getEnemyBuild() != "P2Gate" && building != UnitTypes::Protoss_Shield_Battery && building != UnitTypes::Protoss_Photon_Cannon)) {
                    distBest = 0.0;
                    double dist = blockCenter.getDistance((Position)BWEB::Map::getMainChoke()->Center());
                    if (dist > distBest) {
                        for (auto tile : placements) {
                            if (isQueueable(building, tile) && isBuildable(building, tile) && Terrain().isInAllyTerritory(tile))
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
                    ResourceInfo &gas = g.second;
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
                Position blockCenter = Position(block.Location()) + Position(block.width() * 16, block.height() * 16);

                if (Broodwar->self()->getRace() == Races::Protoss && building == UnitTypes::Protoss_Pylon) {
                    bool power = true;
                    bool solo = true;

                    if ((block.LargeTiles().empty() && poweredLarge == 0) || (block.MediumTiles().empty() && poweredMedium == 0 && !BuildOrder::isProxy()))
                        power = false;
                    if (poweredLarge > poweredMedium && block.MediumTiles().empty())
                        power = false;
                    if (poweredMedium >= poweredLarge && block.LargeTiles().empty())
                        power = false;

                    // If we have no powered spots, can't build a solo spot
                    for (auto &small : block.SmallTiles()) {
                        if (BWEB::Map::isUsed(small)
                            || (!Terrain().isInAllyTerritory(small) && !BuildOrder::isProxy() && !Terrain().isIslandMap()))
                            solo = false;
                    }

                    // HACK: 2nd pylon by choke if we're facing a 2 gate
                    if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) == 1 && Strategy::getEnemyBuild() == "P2Gate")
                        power = true;

                    if (!power || !solo)
                        continue;
                }

                // Setup placements
                if (building.tileWidth() == 4)
                    placements = block.LargeTiles();
                else if (building.tileWidth() == 3)
                    placements = block.MediumTiles();
                else
                    placements = block.SmallTiles();

                checkBest(blockCenter, placements);
            }

            // Make sure we always place a pylon
            if (!tileBest.isValid() && Broodwar->self()->getRace() == Races::Protoss && building == UnitTypes::Protoss_Pylon) {
                distBest = DBL_MAX;
                for (auto &block : BWEB::Blocks::getBlocks()) {
                    Position blockCenter = Position(block.Location()) + Position(block.width() * 16, block.height() * 16);

                    if (block.LargeTiles().size() == 0 && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) == 0)
                        continue;
                    if (!Terrain().isInAllyTerritory(block.Location()))
                        continue;

                    if (!BuildOrder::isOpener() && !hasPoweredPositions() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) > 10 && (poweredLarge > 0 || poweredMedium > 0)) {
                        if (poweredLarge == 0 && block.LargeTiles().size() == 0)
                            continue;
                        if (poweredMedium == 0 && block.MediumTiles().size() == 0)
                            continue;
                    }

                    placements = block.SmallTiles();
                    checkBest(blockCenter, placements);
                }
            }
            return tileBest;
        }

        TilePosition findWallLocation(UnitType building, Position here)
        {
            auto tileBest = TilePositions::Invalid;
            auto distBest = DBL_MAX;
            const BWEB::Walls::Wall* wall = nullptr;
            set<TilePosition> placements;

            // Zerg can place at any wall close to this position
            if (Broodwar->self()->getRace() == Races::Zerg) {
                for (auto &w : BWEB::Walls::getWalls()) {
                    auto dist = w.getCentroid().getDistance(here);
                    if (dist < distBest) {
                        wall = &w;
                        distBest = dist;
                    }
                }
            }

            // Protoss and Terran only wall at main/natural for now
            else {
                if (BuildOrder::isWallMain())
                    wall = Terrain().getMainWall();
                else if (BuildOrder::isWallNat())
                    wall = Terrain().getNaturalWall();
            }

            if (!wall)
                return tileBest;

            if (building.tileWidth() == 4)
                placements = wall->largeTiles();
            else if (building.tileWidth() == 3)
                placements = wall->mediumTiles();
            else if (building.tileWidth() == 2)
                placements = wall->smallTiles();

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

        TilePosition findExpoLocation()
        {
            // If we are expanding, it must be on an expansion area
            UnitType baseType = Broodwar->self()->getRace().getResourceDepot();
            double best = 0.0;
            TilePosition tileBest = TilePositions::Invalid;

            // Fast expands must be as close to home and have a gas geyser
            if (MyStations().getMyStations().size() == 1 && isBuildable(baseType, BWEB::Map::getNaturalTile()) && isQueueable(baseType, BWEB::Map::getNaturalTile()))
                tileBest = BWEB::Map::getNaturalTile();

            // Other expansions must be as close to home but as far away from the opponent
            else {
                for (auto &area : mapBWEM.Areas()) {
                    for (auto &base : area.Bases()) {
                        UnitType shuttle = Broodwar->self()->getRace().getTransport();

                        // Shuttle check for island bases, check enemy owned bases
                        if ((!area.AccessibleFrom(BWEB::Map::getMainArea()) && Broodwar->self()->visibleUnitCount(shuttle) <= 0) || Terrain().isInEnemyTerritory(base.Location()))
                            continue;

                        // Get value of the expansion
                        double value = 0.0;
                        for (auto &mineral : base.Minerals())
                            value += double(mineral->Amount());
                        for (auto &gas : base.Geysers())
                            value += double(gas->Amount());
                        if (base.Geysers().size() == 0 && !Terrain().isIslandMap())
                            value = value / 2.5;

                        // Get distance of the expansion
                        double distance;
                        if (!area.AccessibleFrom(BWEB::Map::getMainArea()))
                            distance = log(base.Center().getDistance(BWEB::Map::getMainPosition()));
                        else if (Players::getPlayers().size() > 1 || !Terrain().getEnemyStartingPosition().isValid())
                            distance = BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), base.Center());
                        else
                            distance = BWEB::Map::getGroundDistance(BWEB::Map::getMainPosition(), base.Center()) / (BWEB::Map::getGroundDistance(Terrain().getEnemyStartingPosition(), base.Center()));

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

        TilePosition getBuildLocation(UnitType building)
        {
            auto here = TilePositions::Invalid;

            // HACK: Versus busts, add an extra pylon to the defenses
            if (building == UnitTypes::Protoss_Pylon && (Strategy::getEnemyBuild() == "Z2HatchHydra" || Strategy::getEnemyBuild() == "Z3HatchHydra") && Terrain().getNaturalWall()) {
                int cnt = 0;
                TilePosition sum(0, 0);
                TilePosition center;
                for (auto &defense : Terrain().getNaturalWall()->getDefenses()) {
                    if (!Pylons::hasPower(defense, UnitTypes::Protoss_Photon_Cannon)) {
                        sum += defense;
                        cnt++;
                    }
                }

                if (cnt != 0) {
                    center = sum / cnt;
                    Position c(center);
                    double distBest = DBL_MAX;

                    // Find unique closest tile to center of defenses
                    for (auto &tile : Terrain().getNaturalWall()->getDefenses()) {
                        Position defenseCenter = Position(tile) + Position(32, 32);
                        double dist = defenseCenter.getDistance(c);
                        if (dist < distBest)
                            distBest = dist, here = tile;
                    }
                }

                if (here.isValid() && isQueueable(building, here) && isBuildable(building, here))
                    return here;
            }

            // HACK: choose a wall position as Zerg because fuck it
            if (Broodwar->self()->getRace() == Races::Zerg) {
                for (auto &area : Terrain().getAllyTerritory()) {
                    auto wall = BWEB::Walls::getWall(area);
                    if (wall) {
                        here = findWallLocation(building, wall->getCentroid());
                        if (here.isValid()) {
                            Broodwar << "Valid spot found" << endl;
                            return here;
                        }
                    }
                }
            }

            // If this is a Resource Depot, Zerg can place it as a production building or expansion, Protoss and Terran only expand
            if (building.isResourceDepot()) {
                if (Broodwar->self()->getRace() == Races::Zerg && BWEB::Map::getNaturalChoke() && BuildOrder::isOpener() && BuildOrder::isWallNat() && Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery) >= 2) {
                    here = findWallLocation(building, Position(BWEB::Map::getNaturalChoke()->Center()));

                    if (here.isValid() && isBuildable(building, here) && isQueueable(building, here))
                        return here;
                }
                if (Broodwar->self()->getRace() == Races::Zerg) {
                    int test = MyStations().getMyStations().size();

                    if (test > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hatchery))
                        here = findProdLocation(building, BWEB::Map::getMainPosition());
                    else
                        here = findExpoLocation();
                }
                else
                    here = findExpoLocation();

                return here;
            }

            // If this is a Pylon, check if any Nexus needs a Pylon
            if (building == UnitTypes::Protoss_Pylon && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Pylon) >= (4 - (2 * Terrain().isIslandMap())) && Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge) >= 1) {
                for (auto &station : MyStations().getMyStations()) {
                    auto &s = *station.second;
                    here = findDefLocation(building, Position(s.ResourceCentroid()));
                    if (here.isValid() && isBuildable(building, here) && isQueueable(building, here))
                        return here;
                }
            }

            // Second pylon place near our main choke
            if (building == UnitTypes::Protoss_Pylon && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Pylon) == 1 && !Terrain().isIslandMap()) {
                here = findProdLocation(building, (Position)BWEB::Map::getMainChoke()->Center());
                if (here.isValid() && isBuildable(building, here) && isQueueable(building, here))
                    return here;
            }

            // If this is a defensive building
            if (building == UnitTypes::Protoss_Photon_Cannon || building == UnitTypes::Zerg_Creep_Colony || building == UnitTypes::Terran_Missile_Turret) {

                if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Forge) >= 1 && ((Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus) >= 3 + (Players::getNumberTerran() > 0 || Players::getNumberProtoss() > 0)) || (Terrain().isIslandMap() && Players::getNumberZerg() > 0))) {
                    for (auto &station : MyStations().getMyStations()) {
                        auto &s = *station.second;

                        if (MyStations().needDefenses(s))
                            here = findDefLocation(building, Position(s.ResourceCentroid()));

                        if (here.isValid())
                            break;
                    }
                }

                else if (!here.isValid() && Terrain().getNaturalWall()) // Assumes we want to put it at the natural wall
                    here = findDefLocation(building, Terrain().getNaturalWall()->getCentroid());

                else if (building == UnitTypes::Zerg_Creep_Colony)
                    here = findDefLocation(building, BWEB::Map::getNaturalPosition());

                return here;
            }

            // If we are fast expanding
            auto isWallPiece = building == UnitTypes::Protoss_Forge || building == UnitTypes::Protoss_Gateway || building == UnitTypes::Protoss_Pylon || building == UnitTypes::Terran_Barracks || building == UnitTypes::Terran_Supply_Depot;
            if (BWEB::Map::getNaturalChoke() && isWallPiece && !Strategy::enemyBust() && (BuildOrder::isWallNat() || BuildOrder::isWallMain())) {
                here = findWallLocation(building, BWEB::Map::getMainPosition());
                if (here.isValid() && isBuildable(building, here))
                    return here;
            }

            // If we are proxying
            if (BuildOrder::isOpener() && BuildOrder::isProxy()) {
                here = findProdLocation(building, mapBWEM.Center());
                if (here.isValid())
                    return here;
            }

            // Battery placing near chokes
            if (building == UnitTypes::Protoss_Shield_Battery) {
                if (BuildOrder::isWallNat())
                    here = findProdLocation(building, (Position)BWEB::Map::getNaturalChoke()->Center());
                else
                    here = findProdLocation(building, (Position)BWEB::Map::getMainChoke()->Center());

                if (here.isValid())
                    return here;
            }

            // Default to finding a production location
            here = findProdLocation(building, BWEB::Map::getMainPosition());
            if (here.isValid())
                return here;
            return TilePositions::Invalid;
        }

        void updateCommands(UnitInfo& building)
        {
            // Lair morphing
            if (building.getType() == UnitTypes::Zerg_Hatchery && BuildOrder::buildCount(UnitTypes::Zerg_Lair) > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive) + lairsMorphing + hivesMorphing) {
                building.unit()->morph(UnitTypes::Zerg_Lair);
                lairsMorphing++;
            }

            // Hive morphing
            else if (building.getType() == UnitTypes::Zerg_Lair && BuildOrder::buildCount(UnitTypes::Zerg_Hive) > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive) + hivesMorphing) {
                building.unit()->morph(UnitTypes::Zerg_Hive);
                hivesMorphing++;
            }

            // Greater Spire morphing
            else if (building.getType() == UnitTypes::Zerg_Spire && BuildOrder::buildCount(UnitTypes::Zerg_Greater_Spire) > Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Greater_Spire))
                building.unit()->morph(UnitTypes::Zerg_Greater_Spire);

            // Sunken morphing
            else if (building.getType() == UnitTypes::Zerg_Creep_Colony)
                building.unit()->morph(UnitTypes::Zerg_Sunken_Colony);

            // Terran building needs new scv
            else if (building.getType().getRace() == Races::Terran && !building.unit()->isCompleted() && !building.unit()->getBuildUnit()) {
                auto builder = Util::getClosestBuilder(building.getPosition());
                if (builder)
                    builder->unit()->rightClick(building.unit());
            }

            // Nuke counting
            else if (building.getType() == UnitTypes::Terran_Nuclear_Silo && building.unit()->hasNuke())
                nukesAvailable++;

            // Barracks
            if (building.getType() == UnitTypes::Terran_Barracks) {

                // Wall lift
                bool wallCheck = (Terrain().getNaturalWall() && building.getPosition().getDistance(Terrain().getNaturalWall()->getCentroid()) < 256.0)
                    || (Terrain().getMainWall() && building.getPosition().getDistance(Terrain().getMainWall()->getCentroid()) < 256.0);

                if (wallCheck && !building.unit()->isFlying()) {
                    if (Units::getSupply() > 120 || BuildOrder::firstReady()) {
                        building.unit()->lift();
                        BWEB::Map::onUnitDestroy(building.unit());
                    }
                }

                // Find landing location as production building
                else if ((Units::getSupply() > 120 || BuildOrder::firstReady()) && building.unit()->isFlying()) {
                    auto here = findProdLocation(building.getType(), BWEB::Map::getMainPosition());
                    auto center = (Position)here + Position(building.getType().tileWidth() * 16, building.getType().tileHeight() * 16);

                    if (building.unit()->getLastCommand().getType() != UnitCommandTypes::Land || building.unit()->getLastCommand().getTargetTilePosition() != here)
                        building.unit()->land(here);
                }

                // Add used tiles back to grid
                else if (!building.unit()->isFlying() && building.unit()->getLastCommand().getType() == UnitCommandTypes::Land)
                    BWEB::Map::onUnitDiscover(building.unit());
            }

            // Comsat scans - Move to special manager
            if (building.getType() == UnitTypes::Terran_Comsat_Station) {
                if (building.hasTarget() && building.getTarget().unit()->exists() && !Command::overlapsAllyDetection(building.getTarget().getPosition())) {
                    building.unit()->useTech(TechTypes::Scanner_Sweep, building.getTarget().getPosition());
                    Command::addCommand(building.unit(), building.getTarget().getPosition(), TechTypes::Scanner_Sweep);
                }
            }
        }

        void updateBuildings()
        {
            // TEST: Find cannon rush spots
            bool testCannonRush = false;
            if (testCannonRush) {
                for (auto &station : BWEB::Stations::getStations()) {
                    TilePosition start = (TilePosition)station.BWEMBase()->Center();

                    for (int x = start.x - 8; x < start.x + 8; x++) {
                        for (int y = start.y - 8; y < start.y + 8; y++) {
                            TilePosition t(x, y);
                            Position p = Position(t) + Position(16, 16);
                            if (!t.isValid())
                                continue;

                            if (mapBWEM.GetTile(t).MinAltitude() > 100 || !isBuildable(UnitTypes::Protoss_Pylon, t) || p.getDistance(station.ResourceCentroid()) > 200.0)
                                continue;

                            Broodwar->drawBoxMap(p - Position(8, 8), p + Position(8, 8), Colors::Green);
                        }
                    }
                }
            }

            // Reset counters
            poweredSmall = 0; poweredMedium = 0; poweredLarge = 0;
            nukesAvailable = 0;
            lairsMorphing = 0, hivesMorphing = 0;

            // Add up how many powered available spots we have		
            for (auto &block : BWEB::Blocks::getBlocks()) {
                if (!Terrain().isInAllyTerritory(block.Location()))
                    continue;

                for (auto &tile : block.SmallTiles()) {
                    if (isBuildable(UnitTypes::Protoss_Photon_Cannon, tile) && isQueueable(UnitTypes::Protoss_Photon_Cannon, tile))
                        poweredSmall++;
                }
                for (auto &tile : block.MediumTiles()) {
                    if (isBuildable(UnitTypes::Protoss_Forge, tile) && isQueueable(UnitTypes::Protoss_Forge, tile))
                        poweredMedium++;
                }
                for (auto &tile : block.LargeTiles()) {
                    if (isBuildable(UnitTypes::Protoss_Gateway, tile) && isQueueable(UnitTypes::Protoss_Gateway, tile))
                        poweredLarge++;
                }
            }

            // Update all my buildings
            for (auto &b : Units::getMyUnits()) {
                auto &building = b.second;
                updateCommands(building);
            }
        }

        void queueBuildings()
        {
            queuedMineral = 0;
            queuedGas = 0;
            buildingsQueued.clear();

            // 1) Add up how many buildings we have assigned to workers
            for (auto &u : Units::getMyUnits()) {
                auto &unit = u.second;

                if (unit.getBuildingType().isValid() && unit.getBuildPosition().isValid())
                    buildingsQueued[unit.getBuildPosition()] = unit.getBuildingType();
            }

            // 2) Add up how many more buildings of each type we need
            for (auto &item : BuildOrder::getItemQueue()) {
                UnitType building = item.first;
                BuildOrder::Item i = item.second;
                int queuedCount = 0;

                auto morphed = !building.whatBuilds().first.isWorker(); //building == UnitTypes::Zerg_Lair || building == UnitTypes::Zerg_Hive || building == UnitTypes::Zerg_Greater_Spire || building == UnitTypes::Zerg_Sunken_Colony || building == UnitTypes::Zerg_Spore_Colony;
                auto addon = building.isAddon();

                if (addon || morphed)
                    continue;

                // If the building morphed from another building type, add the visible amount of child type to the parent type
                // i.e. When we have a queued Hatchery, we need to check how many Lairs and Hives we have.
                int offset = 0;
                if (building == UnitTypes::Zerg_Creep_Colony)
                    offset = Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Sunken_Colony) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Spore_Colony);
                if (building == UnitTypes::Zerg_Hatchery)
                    offset = Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Lair) + Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive);
                if (building == UnitTypes::Zerg_Lair)
                    offset = Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Hive);

                // 3) Reserve building if our reserve count is higher than our visible count
                for (auto &queued : buildingsQueued) {
                    auto queuedType = queued.second;
                    if (queuedType == building) {
                        queuedCount++;

                        // If we want to reserve more than we have, reserve resources
                        if (i.getReserveCount() > Broodwar->self()->visibleUnitCount(building) + offset) {
                            queuedMineral += building.mineralPrice();
                            queuedGas += building.gasPrice();
                        }
                    }
                }

                // 4) Queue building if our actual count is higher than our visible count
                if (i.getActualCount() > queuedCount + Broodwar->self()->visibleUnitCount(building) + offset) {
                    auto here = getBuildLocation(building);
                    auto builder = Util::getClosestBuilder(Position(here));

                    if (here.isValid() && builder) {
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
                ResourceInfo &gas = g.second;

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
            if (BWEB::Map::isUsed(TilePosition(here) + TilePosition(4, 1)))
                return false;
        }

        // Psi/Creep check
        if (building.requiresPsi() && !Pylons::hasPower(here, building))
            return false;

        //// HACK: Had to find a way to let hatcheries be prevented from being queued by a BWEB block too close to resources
        //if (building == UnitTypes::Zerg_Hatchery && !Broodwar->canBuildHere(here, building))
        //	return false;
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
                    auto topLeft = Position(tile.x * 32, tile.y * 32);
                    auto botRight = Position(tile.x * 32 + building.tileWidth(), tile.y * 32 + building.tileHeight());

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