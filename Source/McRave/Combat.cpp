#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat {

    namespace {
        int lastRoleChange = 0;
        set<Position> defendPositions;
        vector<Position> lastSimPositions;
        multimap<double, Position> combatClusters;
        map<const BWEM::ChokePoint*, vector<WalkPosition>> concaveCache;
        multimap<double, UnitInfo&> combatUnitsByDistance;
        vector<TilePosition> occupiedTiles;

        bool holdChoke = false;;
        bool multipleUnitsBlocked = false;
        vector<BWEB::Station*> combatScoutOrder;

        BWEB::Path airClusterPath;
        weak_ptr<UnitInfo> airCommander;
        pair<double, Position> airCluster;
        pair<UnitCommandType, Position> airCommanderCommand;
        multimap<double, Position> groundCleanupPositions;
        multimap<double, Position> airCleanupPositions;

        constexpr tuple commands{ Command::misc, Command::special, Command::attack, Command::approach, Command::kite, Command::defend, Command::explore, Command::escort, Command::retreat, Command::move };

        bool lightUnitNeedsRegroup(UnitInfo& unit)
        {
            if (!unit.isLightAir())
                return false;
            return airCommander.lock() && unit.getPosition().getDistance(airCommander.lock()->getPosition()) > 64.0;
        }

        void getCleanupPosition(UnitInfo& unit)
        {
            // Finishing enemy off, find remaining bases we haven't scouted, if we haven't visited in 2 minutes
            if (Terrain::getEnemyStartingPosition().isValid()) {
                auto best = DBL_MAX;
                for (auto &area : mapBWEM.Areas()) {
                    for (auto &base : area.Bases()) {
                        if (area.AccessibleNeighbours().size() == 0
                            || Terrain::inTerritory(PlayerState::Self, base.Center()))
                            continue;

                        int time = Grids::lastVisibleFrame(base.Location());
                        if (time < best) {
                            best = time;
                            unit.setObjective(Position(base.Location()));
                        }
                    }
                }
            }

            // Need to scout in drone scouting order in ZvZ, ovie order in non ZvZ
            else {
                combatScoutOrder = Scouts::getScoutOrder(Zerg_Zergling);
                if (!combatScoutOrder.empty()) {
                    for (auto &station : combatScoutOrder) {
                        if (!Stations::isBaseExplored(station)) {
                            unit.setObjective(station->getBase()->Center());
                            break;
                        }
                    }
                }

                if (combatScoutOrder.size() > 2 && !Stations::isBaseExplored(*(combatScoutOrder.begin() + 1))) {
                    combatScoutOrder.erase(combatScoutOrder.begin());
                }
            }

            // Finish off positions that are old
            if (Util::getTime() > Time(8, 00)) {
                if (unit.isFlying() && !airCleanupPositions.empty()) {
                    unit.setObjective(airCleanupPositions.begin()->second);
                    airCleanupPositions.erase(airCleanupPositions.begin());
                }
                else if (!unit.isFlying() && !groundCleanupPositions.empty()) {
                    unit.setObjective(groundCleanupPositions.begin()->second);
                    groundCleanupPositions.erase(groundCleanupPositions.begin());
                }
            }
        }

        Position getPathPoint(UnitInfo& unit, Position start)
        {
            // Create a pathpoint
            auto pathPoint = start;
            auto usedTile = BWEB::Map::isUsed(TilePosition(start));
            if (!BWEB::Map::isWalkable(TilePosition(start), unit.getType()) || usedTile != UnitTypes::None) {
                auto dimensions = usedTile != UnitTypes::None ? usedTile.tileSize() : TilePosition(1, 1);
                auto closest = DBL_MAX;
                for (int x = TilePosition(start).x - dimensions.x; x < TilePosition(start).x + dimensions.x + 1; x++) {
                    for (int y = TilePosition(start).y - dimensions.y; y < TilePosition(start).y + dimensions.y + 1; y++) {
                        auto center = Position(TilePosition(x, y)) + Position(16, 16);
                        auto dist = center.getDistance(unit.getPosition());
                        if (dist < closest && BWEB::Map::isWalkable(TilePosition(x, y), unit.getType()) && BWEB::Map::isUsed(TilePosition(x, y)) == UnitTypes::None) {
                            closest = dist;
                            pathPoint = center;
                        }
                    }
                }
            }
            return pathPoint;
        }

        void updateObjective(UnitInfo& unit)
        {
            // If attacking and target is close, set as destination
            if (unit.getLocalState() == LocalState::Attack) {
                unit.setObjective(unit.getTarget().getPosition());

                if (unit.attemptingRunby())
                    unit.setObjective(unit.getEngagePosition());
                else if (unit.getInterceptPosition().isValid())
                    unit.setObjective(unit.getInterceptPosition());
                else if (unit.getSurroundPosition().isValid())
                    unit.setObjective(unit.getSurroundPosition());
                else
                    unit.setObjective(unit.getEngagePosition());

                // HACK: Performance improvement
                if (unit.getTargetPath().isReachable())
                    unit.setObjectivePath(unit.getTargetPath());
            }
            else if (unit.getGlobalState() == GlobalState::Retreat) {
                if (BuildOrder::isPlayPassive())
                    unit.setObjective(Terrain::getDefendPosition());
                else {
                    const auto &retreat = Stations::getClosestRetreatStation(unit);
                    retreat ? unit.setObjective(Stations::getDefendPosition(retreat)) : unit.setObjective(Position(BWEB::Map::getMainChoke()->Center()));
                }
            }
            else {

                if (unit.getGoal().isValid())
                    unit.setObjective(unit.getGoal());
                else if (lightUnitNeedsRegroup(unit))
                    unit.setObjective(airCommander.lock()->getPosition());
                else if (unit.attemptingHarass())
                    unit.setObjective(Terrain::getHarassPosition());
                else if (Terrain::getAttackPosition().isValid() && unit.canAttackGround())
                    unit.setObjective(Terrain::getAttackPosition());
                else if (unit.hasTarget()) {
                    unit.setObjective(unit.getTarget().getPosition());

                    // HACK: Performance improvement
                    unit.setObjectivePath(unit.getTargetPath());
                }
                else
                    getCleanupPosition(unit);
            }
        }

        void updateRetreat(UnitInfo& unit)
        {
            if (unit.getGlobalState() == GlobalState::Retreat && BuildOrder::isPlayPassive())
                unit.setRetreat(BWEB::Map::getMainPosition());
            else {
                const auto &retreat = Stations::getClosestRetreatStation(unit);
                retreat ? unit.setRetreat(retreat->getBase()->Center()) : unit.setRetreat(BWEB::Map::getMainPosition());
            }
        }

        void updateDestination(UnitInfo& unit)
        {
            if (unit.getDestination().isValid())
                return;

            if (unit.getFormation().isValid()) {
                unit.setDestination(unit.getFormation());
                unit.circle(Colors::Orange);
                return;
            }

            auto pathDestination = &unit.getObjectivePath();
            if (unit.getLocalState() == LocalState::Retreat)
                pathDestination = &unit.getRetreatPath();

            // If path is reachable, find a point n pixels away to set as new destination
            if (pathDestination->isReachable()) {
                auto newDestination = Util::findPointOnPath(*pathDestination, [&](Position p) {
                    return p.getDistance(unit.getPosition()) >= 160.0;
                });

                if (newDestination.isValid())
                    unit.setDestination(newDestination);
            }

            //Visuals::drawPath(unit.getObjectivePath());

            if (!unit.getDestination().isValid()) {
                if (unit.getGoal().isValid())
                    unit.setDestination(unit.getGoal());
                else if (unit.getLocalState() == LocalState::Retreat)
                    unit.setDestination(unit.getRetreat());
                else if (unit.getLocalState() == LocalState::Attack)
                    unit.setDestination(unit.getObjective());
                else if (unit.getGlobalState() == GlobalState::Retreat)
                    unit.setDestination(unit.getRetreat());
                else if (unit.getGlobalState() == GlobalState::Attack)
                    unit.setDestination(unit.getObjective());
            }
        }

        void updateDecision(UnitInfo& unit)
        {
            if (!unit.unit() || !unit.unit()->exists()                                                                                          // Prevent crashes            
                || unit.unit()->isLoaded()
                || unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())    // If the unit is locked down, maelstrommed, stassised, or not completed
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Special"),
                make_pair(2, "Attack"),
                make_pair(3, "Approach"),
                make_pair(4, "Kite"),
                make_pair(5, "Defend"),
                make_pair(6, "Explore"),
                make_pair(7, "Escort"),
                make_pair(8, "Retreat"),
                make_pair(9, "Move")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int height = unit.getType().height() / 2;
            int width = unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            auto startText = unit.getPosition() + Position(-4 * int(commandNames[i].length() / 2), height);
            Broodwar->drawTextMap(startText, "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateCleanup()
        {
            groundCleanupPositions.clear();
            airCleanupPositions.clear();

            if (Util::getTime() < Time(6, 00) || !Stations::getEnemyStations().empty())
                return;

            // Look at every TilePosition and sort by furthest oldest
            auto best = 0.0;
            for (int x = 0; x < Broodwar->mapWidth(); x++) {
                for (int y = 0; y < Broodwar->mapHeight(); y++) {
                    auto t = TilePosition(x, y);
                    auto p = Position(t) + Position(16, 16);

                    if (!Broodwar->isBuildable(t))
                        continue;

                    auto frameDiff = (Broodwar->getFrameCount() - Grids::lastVisibleFrame(t));
                    auto dist = p.getDistance(BWEB::Map::getMainPosition());

                    if (mapBWEM.GetArea(t) && mapBWEM.GetArea(t)->AccessibleFrom(BWEB::Map::getMainArea()))
                        groundCleanupPositions.emplace(make_pair(1.0 / (frameDiff * dist), p));
                    else
                        airCleanupPositions.emplace(make_pair(1.0 / (frameDiff * dist), p));
                }
            }
        }

        void checkHoldChoke()
        {
            auto defensiveAdvantage = (Players::ZvZ() && BuildOrder::getCurrentOpener() == Spy::getEnemyOpener()) || (Players::ZvZ() && BuildOrder::getCurrentOpener() == "12Pool" && Spy::getEnemyOpener() == "9Pool");

            // UMS Setting
            if (Broodwar->getGameType() == BWAPI::GameTypes::Use_Map_Settings) {
                holdChoke = true;
                return;
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss && Players::getSupply(PlayerState::Self, Races::None) > 40) {
                holdChoke = BuildOrder::takeNatural()
                    || vis(Protoss_Dragoon) > 0
                    || com(Protoss_Shield_Battery) > 0
                    || BuildOrder::isWallNat()
                    || (BuildOrder::isHideTech() && !Spy::enemyRush())
                    || Players::getSupply(PlayerState::Self, Races::None) > 60
                    || Players::vT();
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran && Players::getSupply(PlayerState::Self, Races::None) > 40)
                holdChoke = true;

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                holdChoke = (!Players::ZvZ() && (Spy::getEnemyBuild() != "2Gate" || !Spy::enemyProxy()))
                    || (defensiveAdvantage && !Spy::enemyPressure() && vis(Zerg_Sunken_Colony) == 0 && com(Zerg_Mutalisk) < 3 && Util::getTime() > Time(3, 30))
                    || (BuildOrder::takeNatural() && total(Zerg_Zergling) >= 10)
                    || Players::getSupply(PlayerState::Self, Races::None) > 60;
            }
        }


        void updateHarassPath(UnitInfo& unit)
        {
            if (!unit.isLightAir() || airCommander.expired())
                return;

            // Generate a flying path for harassing that obeys exploration and staying out of range of threats if possible
            auto canHarass = unit.getLocalState() != LocalState::Retreat && unit.getObjective() == Terrain::getHarassPosition() && unit.hasSimTarget() && unit.attemptingHarass();
            auto enemyPressure = unit.hasTarget() && Util::getTime() < Time(7, 00) && unit.getTarget().getPosition().getDistance(BWEB::Map::getMainPosition()) < unit.getTarget().getPosition().getDistance(Terrain::getEnemyStartingPosition());
            if (!unit.getGoal().isValid() && canHarass && !enemyPressure && unit == airCommander.lock()) {

                auto simDistCurrent = unit.getPosition().getApproxDistance(unit.getSimTarget().getPosition());
                auto simPosition = unit.getSimTarget().getPosition();

                const auto flyerAttack = [&](const TilePosition &t) {
                    const auto center = Position(t) + Position(16, 16);

                    auto d = center.getApproxDistance(simPosition);
                    for (auto &pos : lastSimPositions)
                        d = min(d, center.getApproxDistance(pos));

                    auto dist = unit.getSimState() == SimState::Win ? 1.0 : max(0.01, double(d) - min(simDistCurrent, int(unit.getRetreatRadius() + 64.0)));
                    auto vis = unit.getSimState() == SimState::Win ? 1.0 : clamp(double(Broodwar->getFrameCount() - Grids::lastVisibleFrame(t)) / 960.0, 0.5, 3.0);
                    return 1.00 / (vis * dist);
                };

                BWEB::Path newPath(unit.getPosition(), unit.getObjective(), unit.getType());
                newPath.generateAS(flyerAttack);
                unit.setObjectivePath(newPath);
            }

            // Generate a flying path for regrouping
            auto inCluster = unit.getPosition().getDistance(airCluster.second) < 64.0;
            if (!unit.getGoal().isValid() && !inCluster && airCluster.second.isValid() && !unit.globalRetreat()) {

                const auto flyerRegroup = [&](const TilePosition &t) {
                    const auto center = Position(t) + Position(16, 16);
                    auto threat = Grids::getEAirThreat(center);
                    return threat;
                };

                BWEB::Path newPath(unit.getPosition(), airCluster.second, unit.getType());
                newPath.generateAS(flyerRegroup);
                unit.setObjectivePath(newPath);
            }
        }

        void updateObjectivePath(UnitInfo& unit)
        {
            //Broodwar->drawLineMap(unit.getPosition(), unit.getObjective(), Colors::Green);
            // Generate a new path that obeys collision of terrain and buildings
            auto needPath = unit.getObjectivePath().getTiles().empty() || unit.getObjectivePath() != unit.getTargetPath();
            if (!unit.isFlying() && unit.getObjective().isValid() && needPath && unit.getObjectivePath().getTarget() != TilePosition(unit.getObjective())) {
                auto pathPoint = getPathPoint(unit, unit.getObjective());
                if (!mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)))) {
                    BWEB::Path newPath(unit.getPosition(), pathPoint, unit.getType());
                    newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
                    unit.setObjectivePath(newPath);
                }
            }
            //Visuals::drawPath(unit.getObjectivePath());
        }

        void updateRetreatPath(UnitInfo& unit)
        {
            // Generate a new path that obeys collision of terrain and buildings
            auto needPath = unit.getRetreatPath().getTiles().empty();
            if (!unit.isFlying() && needPath && unit.getRetreatPath().getTarget() != TilePosition(unit.getRetreat())) {
                auto pathPoint = getPathPoint(unit, unit.getRetreat());
                if (!mapBWEM.GetArea(TilePosition(unit.getPosition())) || !mapBWEM.GetArea(TilePosition(pathPoint)) || mapBWEM.GetArea(TilePosition(unit.getPosition()))->AccessibleFrom(mapBWEM.GetArea(TilePosition(pathPoint)))) {
                    BWEB::Path newPath(unit.getPosition(), pathPoint, unit.getType());
                    newPath.generateJPS([&](const TilePosition &t) { return newPath.unitWalkable(t); });
                    unit.setRetreatPath(newPath);
                }
            }
            //Visuals::drawPath(unit.getRetreatPath());
        }



        void updateAirCommander()
        {
            // Get an air commander if new one needed
            if (airCommander.expired() || airCommander.lock()->globalRetreat() || airCommander.lock()->localRetreat() || (airCluster.second.isValid() && airCommander.lock()->getPosition().getDistance(airCluster.second) > 64.0)) {
                for (auto &u : combatUnitsByDistance) {
                    auto &unit = u.second;
                    if (unit.isLightAir() && unit.getPosition().getDistance(airCluster.second) < 64.0) {
                        airCommander = unit.weak_from_this();
                        break;
                    }
                }
            }

            // If we have an air commander
            if (airCommander.lock()) {
                if (airCommander.lock()->hasSimTarget() && find(lastSimPositions.begin(), lastSimPositions.end(), airCommander.lock()->getSimTarget().getPosition()) == lastSimPositions.end()) {
                    if (lastSimPositions.size() >= 5)
                        lastSimPositions.pop_back();
                    lastSimPositions.insert(lastSimPositions.begin(), airCommander.lock()->getSimTarget().getPosition());
                }

                // Execute the air commanders commands
                Horizon::simulate(*airCommander.lock());
                updateObjective(*airCommander.lock());
                updateDestination(*airCommander.lock());
                updateHarassPath(*airCommander.lock());
                updateDecision(*airCommander.lock());

                // Setup air commander commands for other units to follow
                //airClusterPath = airCommander.lock()->getObjectivePath();
                airCommanderCommand = make_pair(airCommander.lock()->getCommandType(), airCommander.lock()->getCommandPosition());
            }
        }

        void updateRole(UnitInfo& unit)
        {
            // Can't change role to combat if not a worker or we did one this frame
            if (!unit.getType().isWorker()
                || lastRoleChange == Broodwar->getFrameCount()
                || unit.getBuildType() != None
                || unit.getGoal().isValid()
                || unit.getBuildPosition().isValid())
                return;

            // Only proactively pull the closest worker to our defend position
            auto closestWorker = Util::getClosestUnit(Terrain::getDefendPosition(), PlayerState::Self, [&](auto &u) {
                return u.getRole() == Role::Worker && !unit.getGoal().isValid() && (!unit.hasResource() || !unit.getResource().getType().isRefinery()) && !unit.getBuildPosition().isValid();
            });

            auto combatCount = Roles::getMyRoleCount(Role::Combat) - (unit.getRole() == Role::Combat ? 1 : 0);
            auto combatWorkersCount =  Roles::getMyRoleCount(Role::Combat) - com(Protoss_Zealot) - com(Protoss_Dragoon) - com(Zerg_Zergling) - (unit.getRole() == Role::Combat ? 1 : 0);

            const auto healthyWorker = [&] {

                // Don't pull low shield probes
                if (unit.getType() == Protoss_Probe && unit.getShields() <= 4)
                    return false;

                // Don't pull low health drones
                if (unit.getType() == Zerg_Drone && unit.getHealth() < 20)
                    return false;
                return true;
            };

            // Proactive pulls will result in the worker defending
            const auto proactivePullWorker = [&]() {

                // If this isn't the closest mineral worker to the defend position, don't pull it
                if (unit.getRole() == Role::Worker && unit.shared_from_this() != closestWorker)
                    return false;

                // Protoss
                if (Broodwar->self()->getRace() == Races::Protoss) {
                    int completedDefenders = com(Protoss_Photon_Cannon) + com(Protoss_Zealot);
                    int visibleDefenders = vis(Protoss_Photon_Cannon) + vis(Protoss_Zealot);

                    // If trying to hide tech, pull 1 probe with a Zealot
                    if (!BuildOrder::isRush() && BuildOrder::isHideTech() && combatCount < 2 && completedDefenders > 0)
                        return true;

                    // If trying to FFE, pull based on Cannon/Zealot numbers, or lack of scouting information
                    if (BuildOrder::getCurrentBuild() == "FFE") {
                        if (Spy::enemyRush() && Spy::getEnemyOpener() == "4Pool" && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 1)
                            return true;
                        if (Spy::enemyPressure() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 2)
                            return true;
                        if (Spy::enemyRush() && Spy::getEnemyOpener() == "9Pool" && Util::getTime() > Time(3, 15) && completedDefenders < 3)
                            return combatWorkersCount < 3;
                        if (!Terrain::getEnemyStartingPosition().isValid() && Spy::getEnemyBuild() == "Unknown" && combatCount < 2 && completedDefenders < 1 && visibleDefenders > 0)
                            return true;
                    }

                    // If trying to 2Gate at our natural, pull based on Zealot numbers
                    else if (BuildOrder::getCurrentBuild() == "2Gate" && BuildOrder::getCurrentOpener() == "Natural") {
                        if (Spy::enemyRush() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 1)
                            return true;
                        if (Spy::enemyPressure() && combatCount < 8 - (2 * completedDefenders) && visibleDefenders >= 2)
                            return true;
                    }

                    // If trying to 1GateCore and scouted 2Gate late, pull workers to block choke when we are ready
                    else if (BuildOrder::getCurrentBuild() == "1GateCore" && Spy::getEnemyBuild() == "2Gate" && BuildOrder::getCurrentTransition() != "Defensive" && holdChoke) {
                        if (Util::getTime() < Time(3, 30) && combatWorkersCount < 2)
                            return true;
                    }
                }

                // Terran

                // Zerg
                if (Broodwar->self()->getRace() == Races::Zerg) {
                    if (BuildOrder::getCurrentOpener() == "12Pool" && Spy::getEnemyOpener() == "9Pool" && total(Zerg_Zergling) < 16 && int(Stations::getMyStations().size()) >= 2)
                        return combatWorkersCount < 3;
                }
                return false;
            };

            // Reactive pulls will cause the worker to attack aggresively
            const auto reactivePullWorker = [&]() {

                auto proxyDangerousBuilding = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.isProxy() && u.getType().isBuilding() && u.canAttackGround();
                });
                auto proxyBuildingWorker = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u.getType().isWorker() && (u.isThreatening() || (proxyDangerousBuilding && u.getType().isWorker() && u.getPosition().getDistance(proxyDangerousBuilding->getPosition()) < 160.0));
                });

                // HACK: Don't pull workers reactively versus vultures
                if (Players::getVisibleCount(PlayerState::Enemy, Terran_Vulture) > 0)
                    return false;
                if (Spy::getEnemyBuild() == "2Gate" && Spy::enemyProxy())
                    return false;

                // If we have immediate threats
                if (Players::ZvT() && proxyDangerousBuilding && com(Zerg_Zergling) <= 2)
                    return combatWorkersCount < 6;
                if (Players::ZvP() && proxyDangerousBuilding && Spy::getEnemyBuild() == "CannonRush" && com(Zerg_Zergling) <= 2)
                    return combatWorkersCount < (4 * Players::getVisibleCount(PlayerState::Enemy, proxyDangerousBuilding->getType()));
                if (Spy::getWorkersNearUs() > 2 && com(Zerg_Zergling) < Spy::getWorkersNearUs())
                    return Spy::getWorkersNearUs() >= combatWorkersCount - 3;                
                if (BuildOrder::getCurrentOpener() == "12Hatch" && Spy::getEnemyOpener() == "8Rax" && com(Zerg_Zergling) < 2)
                    return combatWorkersCount <= com(Zerg_Drone) - 4;

                // If we're trying to make our expanding hatchery and the drone is being harassed
                if (vis(Zerg_Hatchery) == 1 && Util::getTime() < Time(3, 00) && BuildOrder::isOpener() && Units::getImmThreat() > 0.0f && Players::ZvP() && combatCount == 0)
                    return combatWorkersCount < 1;
                if (Players::ZvP() && Util::getTime() < Time(4, 00) && !Terrain::isShitMap() && int(Stations::getMyStations().size()) < 2 && BuildOrder::getBuildQueue()[Zerg_Hatchery] >= 2 && Players::getVisibleCount(PlayerState::Enemy, Protoss_Probe) > 0)
                    return combatWorkersCount < 1;

                // If we suspect a cannon rush is coming
                if (Players::ZvP() && Spy::enemyPossibleProxy() && Util::getTime() < Time(3, 00))
                    return combatWorkersCount < 1;
                return false;
            };

            // Check if workers should fight or work
            if (unit.getType().isWorker()) {
                auto react = reactivePullWorker();
                auto proact = proactivePullWorker();

                // Pull a worker if needed
                if (unit.getRole() == Role::Worker && !unit.unit()->isCarryingMinerals() && !unit.unit()->isCarryingGas() && healthyWorker() && (react || proact)) {
                    unit.setRole(Role::Combat);
                    unit.setBuildingType(None);
                    unit.setBuildPosition(TilePositions::Invalid);
                    lastRoleChange = Broodwar->getFrameCount();
                }

                // Return a worker if not needed
                else if (unit.getRole() == Role::Combat && ((!react && !proact) || !healthyWorker())) {
                    unit.setRole(Role::Worker);
                    lastRoleChange = Broodwar->getFrameCount();
                }

                // HACK: Check if this was a reactive pull, set worker to always engage
                if (unit.getRole() == Role::Combat) {
                    combatCount--;
                    combatWorkersCount--;
                    react = reactivePullWorker();
                    if (react && unit.hasTarget()) {
                        unit.setLocalState(LocalState::Attack);
                        unit.setGlobalState(GlobalState::Attack);
                    }
                }
            }
        }

        void updateClusters(UnitInfo& unit)
        {
            // Don't update clusters for fragile combat units
            if (unit.getType() == Protoss_High_Templar
                || unit.getType() == Protoss_Dark_Archon
                || unit.getType() == Protoss_Reaver
                || unit.getType() == Protoss_Interceptor
                || unit.getType() == Zerg_Defiler)
                return;

            // Figure out what type to make the center of our cluster around
            auto clusterAround = UnitTypes::None;
            if (Broodwar->self()->getRace() == Races::Protoss)
                clusterAround = vis(Protoss_Carrier) > 0 ? Protoss_Carrier : Protoss_Corsair;
            else if (Broodwar->self()->getRace() == Races::Zerg)
                clusterAround = vis(Zerg_Guardian) > 0 ? Zerg_Guardian : Zerg_Mutalisk;
            else if (Broodwar->self()->getRace() == Races::Terran)
                clusterAround = vis(Terran_Battlecruiser) > 0 ? Terran_Battlecruiser : Terran_Wraith;

            if (unit.isFlying() && unit.getType() == clusterAround) {
                if (Grids::getAAirCluster(unit.getWalkPosition()) > airCluster.first)
                    airCluster = make_pair(Grids::getAAirCluster(unit.getWalkPosition()), unit.getPosition());
            }
            else if (!unit.isFlying() && unit.getWalkPosition().isValid()) {
                const auto strength = Grids::getAGroundCluster(unit.getWalkPosition()) + Grids::getAAirCluster(unit.getWalkPosition());
                combatClusters.emplace(strength, unit.getPosition());
            }
        }

        void updateLocalState(UnitInfo& unit)
        {
            if (!unit.hasSimTarget() || !unit.hasTarget() || unit.getLocalState() != LocalState::None)
                return;

            const auto distSim = double(Util::boxDistance(unit.getType(), unit.getPosition(), unit.getSimTarget().getType(), unit.getSimTarget().getPosition()));
            const auto distTarget = double(Util::boxDistance(unit.getType(), unit.getPosition(), unit.getTarget().getType(), unit.getTarget().getPosition()));
            const auto insideRetreatRadius = distSim < unit.getRetreatRadius() && !unit.attemptingRunby();
            const auto insideEngageRadius = distTarget < unit.getEngageRadius() && unit.getGlobalState() == GlobalState::Attack;
            const auto atHome = Terrain::inTerritory(PlayerState::Self, unit.getTarget().getPosition()) && mapBWEM.GetArea(unit.getTilePosition()) == mapBWEM.GetArea(unit.getTarget().getTilePosition()) && !Players::ZvZ();
            const auto reAlign = (unit.getType() == Zerg_Mutalisk && unit.hasTarget() && unit.canStartAttack() && !unit.isWithinAngle(unit.getTarget()) && Util::boxDistance(unit.getType(), unit.getPosition(), unit.getTarget().getType(), unit.getTarget().getPosition()) <= 32.0);
            const auto winningState = (!atHome || !BuildOrder::isPlayPassive()) && unit.getSimState() == SimState::Win;

            // Regardless of any decision, determine if Unit is in danger and needs to retreat
            if ((Actions::isInDanger(unit, unit.getPosition()) && !unit.isTargetedBySuicide())
                || (Actions::isInDanger(unit, unit.getEngagePosition()) && insideEngageRadius && !unit.isTargetedBySuicide())
                || reAlign)
                unit.setLocalState(LocalState::Retreat);

            // Regardless of local decision, determine if Unit needs to attack or retreat
            else if (unit.globalEngage()) {
                unit.setLocalState(LocalState::Attack);
                unit.circle(Colors::Red);
            }
            else if (unit.globalRetreat())
                unit.setLocalState(LocalState::Retreat);

            // If within local decision range, determine if Unit needs to attack or retreat
            else if ((insideEngageRadius || atHome) && (unit.localEngage() || winningState))
                unit.setLocalState(LocalState::Attack);
            else if ((insideRetreatRadius || atHome) && (unit.localRetreat() || unit.getSimState() == SimState::Loss))
                unit.setLocalState(LocalState::Retreat);

            // Check if we shouldn't issue any commands
            else if (insideEngageRadius && !unit.isFlying())
                unit.setLocalState(LocalState::Hold);

            // Default state
            else
                unit.setLocalState(LocalState::None);
        }

        void updateGlobalState(UnitInfo& unit)
        {
            if (unit.getGlobalState() != GlobalState::None)
                return;

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if ((!BuildOrder::takeNatural() && Spy::enemyFastExpand())
                    || (Spy::enemyProxy() && !Spy::enemyRush())
                    || BuildOrder::isRush()
                    || unit.getType() == Protoss_Dark_Templar
                    || (Players::getVisibleCount(PlayerState::Enemy, Protoss_Dark_Templar) > 0 && com(Protoss_Observer) == 0 && Broodwar->getFrameCount() < 15000))
                    unit.setGlobalState(GlobalState::Attack);

                else if (unit.getType().isWorker()
                    || (Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
                    || (unit.getType() == Protoss_Corsair && !BuildOrder::firstReady() && Players::getStrength(PlayerState::Enemy).airToAir > 0.0)
                    || (unit.getType() == Protoss_Carrier && com(Protoss_Interceptor) < 16 && !Spy::enemyPressure()))
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }

            // Zerg
            else if (Broodwar->self()->getRace() == Races::Zerg) {
                if (BuildOrder::isRush()
                    || Broodwar->getGameType() == GameTypes::Use_Map_Settings)
                    unit.setGlobalState(GlobalState::Attack);
                else if ((Broodwar->getFrameCount() < 15000 && BuildOrder::isPlayPassive())
                    || (Players::ZvT() && Util::getTime() < Time(12, 00) && Util::getTime() > Time(3, 30) && unit.getType() == Zerg_Zergling && !Spy::enemyGreedy() && (Spy::getEnemyBuild() == "RaxFact" || Spy::enemyWalled() || Players::getCompleteCount(PlayerState::Enemy, Terran_Vulture) > 0))
                    || (Players::ZvZ() && Util::getTime() < Time(10, 00) && unit.getType() == Zerg_Zergling && Players::getCompleteCount(PlayerState::Enemy, Zerg_Zergling) > com(Zerg_Zergling))
                    || (Players::ZvZ() && Players::getCompleteCount(PlayerState::Enemy, Zerg_Drone) > 0 && !Terrain::getEnemyStartingPosition().isValid() && Util::getTime() < Time(2, 45))
                    || (BuildOrder::isProxy() && BuildOrder::isPlayPassive())
                    || (BuildOrder::isProxy() && unit.getType() == Zerg_Hydralisk)
                    || (unit.getType() == Zerg_Hydralisk && BuildOrder::getCompositionPercentage(Zerg_Lurker) >= 1.00)
                    || (unit.getType() == Zerg_Hydralisk && !unit.getGoal().isValid() && (!Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Grooved_Spines) || !Players::getPlayerInfo(Broodwar->self())->hasUpgrade(UpgradeTypes::Muscular_Augments)))
                    || (!Players::ZvZ() && unit.isLightAir() && com(Zerg_Mutalisk) < 5 && total(Zerg_Mutalisk) < 9))
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }

            // Terran
            else if (Broodwar->self()->getRace() == Races::Terran) {
                if (BuildOrder::isPlayPassive() || !BuildOrder::firstReady())
                    unit.setGlobalState(GlobalState::Retreat);
                else
                    unit.setGlobalState(GlobalState::Attack);
            }
        }

        void sortCombatUnits()
        {
            // 1. Sort combat units by distance to target
            combatUnitsByDistance.clear();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                auto &unit = *u;

                if (!unit.unit()->isCompleted()
                    || unit.getType() == Terran_Vulture_Spider_Mine
                    || unit.getType() == Protoss_Scarab
                    || unit.getType() == Protoss_Interceptor
                    || unit.getType().isSpell())
                    continue;

                // TODO: Move to Roles
                // 2. Check if we need a worker pull or return a worker
                if (unit.getType().isWorker())
                    updateRole(unit);

                if (unit.getRole() == Role::Combat) {
                    updateClusters(unit); // TODO: Move to Clusters
                    combatUnitsByDistance.emplace(unit.getPosition().getDistance(unit.getDestination()), unit);
                }
            }
        }

        void gogoCombat() {

            // Execute commands ordered by ascending distance
            for (auto &u : combatUnitsByDistance) {
                auto &unit = u.second;

                if (airCommander.lock() && unit == *airCommander.lock())
                    continue;

                // Light air close to the air cluster use the same command of the air commander
                if (airCommander.lock() && !unit.localRetreat() && !unit.globalRetreat() && unit.isLightAir() && !airCommander.lock()->isNearSuicide() && !unit.isNearSuicide() && unit.getPosition().getDistance(airCommander.lock()->getPosition()) <= 96.0) {
                    if (unit.hasTarget() && airCommanderCommand.first == UnitCommandTypes::Attack_Unit)
                        unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
                    else if (airCommanderCommand.first == UnitCommandTypes::Move && !unit.isTargetedBySplash())
                        unit.command(UnitCommandTypes::Move, airCommanderCommand.second);
                    else if (airCommanderCommand.first == UnitCommandTypes::Right_Click_Position && !unit.isTargetedBySplash())
                        unit.command(UnitCommandTypes::Right_Click_Position, airCommanderCommand.second);
                    else {
                        updateDestination(unit);
                        updateDecision(unit);
                        //Broodwar->drawLineMap(unit.getPosition(), unit.getObjective(), Colors::Green);
                        //Broodwar->drawLineMap(unit.getPosition(), unit.getRetreat(), Colors::Orange);
                        //Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Cyan);
                    }
                }

                // Combat unit decisions
                else if (unit.getRole() == Role::Combat) {
                    updateDestination(unit);
                    updateDecision(unit);
                    //Broodwar->drawLineMap(unit.getPosition(), unit.getObjective(), Colors::Green);
                    //Broodwar->drawLineMap(unit.getPosition(), unit.getRetreat(), Colors::Orange);
                    //Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Cyan);
                }
            }
        }

        void updateCombatUnits()
        {
            for (auto &[_, unit] : combatUnitsByDistance) {
                Horizon::simulate(unit);
                updateGlobalState(unit);
                updateLocalState(unit);

                updateObjective(unit);
                updateRetreat(unit);

                updateHarassPath(unit);

                updateObjectivePath(unit);
                updateRetreatPath(unit);
            }
        }

        void reset()
        {
            airCluster.first = 0.0;
            airCluster.second = Positions::Invalid;
            combatClusters.clear();
            combatUnitsByDistance.clear();
        }
    }

    void onFrame() {
        Visuals::startPerfTest();
        reset();
        sortCombatUnits();
        updateCombatUnits();
        Clusters::onFrame();
        Formations::onFrame();
        updateAirCommander();
        gogoCombat();
        checkHoldChoke();
        Visuals::endPerfTest("Combat");
    }

    bool defendChoke() { return holdChoke; }
    set<Position>& getDefendPositions() { return defendPositions; }
    multimap<double, Position>& getCombatClusters() { return combatClusters; }
    Position getAirClusterCenter() { return airCluster.second; }
    multimap<double, UnitInfo&> getCombatUnitsByDistance() { return combatUnitsByDistance; }
}