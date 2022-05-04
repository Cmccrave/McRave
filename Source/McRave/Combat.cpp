#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Combat {

    namespace {
        bool ignoreSim = false;
        double minWinPercent = 0.6;
        double maxWinPercent = 1.2;
        double minThreshold = 0.0;
        double maxThreshold = 0.0;
        int lastRoleChange = 0;
        set<Position> defendPositions;
        vector<Position> lastSimPositions;
        multimap<double, UnitInfo&> combatUnitsByDistance;
        vector<TilePosition> occupiedTiles;

        bool holdChoke = false;;
        vector<BWEB::Station*> combatScoutOrder;
        multimap<double, Position> groundCleanupPositions;
        multimap<double, Position> airCleanupPositions;

        constexpr tuple commands{ Command::misc, Command::special, Command::attack, Command::approach, Command::kite, Command::defend, Command::explore, Command::escort, Command::retreat, Command::move };

        bool lightUnitNeedsRegroup(UnitInfo& unit)
        {
            if (!unit.isLightAir())
                return false;
            return unit.hasCommander() && unit.getPosition().getDistance(unit.getCommander().getPosition()) > 64.0;
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

        void updateSimulation(UnitInfo& unit)
        {
            if (unit.getEngDist() == DBL_MAX || !unit.hasTarget()) {
                unit.setSimState(SimState::Loss);
                unit.setSimValue(0.0);
                return;
            }

            // If we have excessive resources, ignore our simulation and engage
            if (!ignoreSim && Broodwar->self()->minerals() >= 2000 && Broodwar->self()->gas() >= 2000 && Players::getSupply(PlayerState::Self, Races::None) >= 380)
                ignoreSim = true;
            if (ignoreSim && (Broodwar->self()->minerals() <= 500 || Broodwar->self()->gas() <= 500 || Players::getSupply(PlayerState::Self, Races::None) <= 240))
                ignoreSim = false;

            if (ignoreSim) {
                unit.setSimState(SimState::Win);
                unit.setSimValue(10.0);
                return;
            }

            auto baseCountSwing = Util::getTime() > Time(5, 00) ? max(0.0, (double(Stations::getMyStations().size()) - double(Stations::getEnemyStations().size())) / 20) : 0.0;
            auto baseDistSwing = Util::getTime() > Time(5, 00) ? unit.getEngagePosition().getDistance(Terrain::getEnemyStartingPosition()) / (10 * BWEB::Map::getMainPosition().getDistance(Terrain::getEnemyStartingPosition())) : 0.0;
            auto belowGrdtoGrdLimit = false;
            auto belowGrdtoAirLimit = false;
            auto belowAirtoAirLimit = false;
            auto belowAirtoGrdLimit = false;

            // P
            if (Players::PvP()) {
                minWinPercent = 0.80;
                maxWinPercent = 1.20;
            }
            if (Players::PvZ()) {
                minWinPercent = 0.6;
                maxWinPercent = 1.2;
            }
            if (Players::PvT()) {
                minWinPercent = 0.6 - baseCountSwing;
                maxWinPercent = 1.0 - baseCountSwing;
            }

            // Z
            if (Players::ZvP()) {
                minWinPercent = 0.90;
                maxWinPercent = 1.60;
            }
            if (Players::ZvZ()) {
                minWinPercent = 0.8;
                maxWinPercent = 1.4;
            }
            if (Players::ZvT()) {
                minWinPercent = 0.8 - baseCountSwing + baseDistSwing;
                maxWinPercent = 1.2 - baseCountSwing + baseDistSwing;
            }

            minThreshold = minWinPercent - baseCountSwing + baseDistSwing;
            maxThreshold = maxWinPercent - baseCountSwing + baseDistSwing;

            Horizon::simulate(unit);

            auto engagedAlreadyOffset = unit.getSimState() == SimState::Win ? 0.2 : 0.0;

            // Check if any allied unit is below the limit to synhcronize sim values
            for (auto &a : Units::getUnits(PlayerState::Self)) {
                UnitInfo &self = *a;

                if (self.hasTarget() && unit.hasTarget() && self.getTarget() == unit.getTarget() && self.getSimValue() <= minThreshold && self.getSimValue() != 0.0) {
                    self.isFlying() ?
                        (self.getTarget().isFlying() ? belowAirtoAirLimit = true : belowAirtoGrdLimit = true) :
                        (self.getTarget().isFlying() ? belowGrdtoAirLimit = true : belowGrdtoGrdLimit = true);
                }
            }
            for (auto &a : Units::getUnits(PlayerState::Ally)) {
                UnitInfo &ally = *a;

                if (ally.hasTarget() && unit.hasTarget() && ally.getTarget() == unit.getTarget() && ally.getSimValue() <= minThreshold && ally.getSimValue() != 0.0) {
                    ally.isFlying() ?
                        (ally.getTarget().isFlying() ? belowAirtoAirLimit = true : belowAirtoGrdLimit = true) :
                        (ally.getTarget().isFlying() ? belowGrdtoAirLimit = true : belowGrdtoGrdLimit = true);
                }
            }

            // If above/below thresholds, it's a sim win/loss
            if (unit.getSimValue() >= maxThreshold - engagedAlreadyOffset)
                unit.setSimState(SimState::Win);
            else if (unit.getSimValue() < minThreshold - engagedAlreadyOffset || (unit.getSimState() == SimState::None && unit.getSimValue() < maxThreshold))
                unit.setSimState(SimState::Loss);

            // Check for hardcoded directional losses
            if (unit.hasTarget() && unit.getSimValue() < maxThreshold - engagedAlreadyOffset) {
                if (unit.isFlying()) {
                    if (unit.getTarget().isFlying() && belowAirtoAirLimit)
                        unit.setSimState(SimState::Loss);
                    else if (!unit.getTarget().isFlying() && belowAirtoGrdLimit)
                        unit.setSimState(SimState::Loss);
                }
                else {
                    if (unit.getTarget().isFlying() && belowGrdtoAirLimit)
                        unit.setSimState(SimState::Loss);
                    else if (!unit.getTarget().isFlying() && belowGrdtoGrdLimit)
                        unit.setSimState(SimState::Loss);
                }
            }

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
                else if ((unit.isLightAir() || unit.getType() == Zerg_Scourge) && ((Units::getImmThreat() > 25.0 && Stations::getMyStations().size() >= 3 && Stations::getMyStations().size() > Stations::getEnemyStations().size()) || (Players::ZvZ() && Units::getImmThreat() > 5.0))) {
                    auto &attacker = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                        return u->isThreatening() && !u->isHidden();
                    });
                    if (attacker)
                        unit.setDestination(attacker->getPosition());
                }
                else if (unit.attemptingHarass())
                    unit.setObjective(Terrain::getHarassPosition());
                else if (unit.hasTarget()) {
                    unit.setObjective(unit.getTarget().getPosition());

                    // HACK: Performance improvement
                    unit.setObjectivePath(unit.getTargetPath());
                }
                else if (Terrain::getAttackPosition().isValid() && unit.canAttackGround())
                    unit.setObjective(Terrain::getAttackPosition());
                else
                    getCleanupPosition(unit);
            }
        }

        void updateRetreat(UnitInfo& unit)
        {
            if (unit.getGlobalState() == GlobalState::Retreat)
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

            if (unit.getFormation().isValid() && unit.getLocalState() == LocalState::Retreat) {
                unit.setDestination(unit.getFormation());
                return;
            }
            else if (lightUnitNeedsRegroup(unit))
                unit.setDestination(unit.getCommander().getPosition());

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

            // Resort destination to something
            if (!unit.getDestination().isValid()) {
                if (unit.getGoal().isValid())
                    unit.setDestination(unit.getGoal());
                else if (unit.getLocalState() == LocalState::Retreat)
                    unit.setDestination(unit.getRetreat());
                else
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
            if (!unit.isLightAir())
                return;

            // Generate a flying path for harassing that obeys exploration and staying out of range of threats if possible
            auto canHarass = unit.getLocalState() != LocalState::Retreat && unit.getObjective() == Terrain::getHarassPosition() && unit.hasSimTarget() && unit.attemptingHarass();
            auto enemyPressure = unit.hasTarget() && Util::getTime() < Time(7, 00) && unit.getTarget().getPosition().getDistance(BWEB::Map::getMainPosition()) < unit.getTarget().getPosition().getDistance(Terrain::getEnemyStartingPosition());
            if (!unit.getGoal().isValid() && canHarass && !enemyPressure) {

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
        }

        void updateObjectivePath(UnitInfo& unit)
        {
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
            // Generate a flying path for regrouping
            if (!unit.isLightAir() && lightUnitNeedsRegroup(unit) && !unit.getGoal().isValid() && !unit.globalRetreat()) {

                const auto flyerRegroup = [&](const TilePosition &t) {
                    const auto center = Position(t) + Position(16, 16);
                    auto threat = Grids::getEAirThreat(center);
                    return threat;
                };

                BWEB::Path newPath(unit.getPosition(), unit.getObjective(), unit.getType());
                newPath.generateAS(flyerRegroup);
                unit.setObjectivePath(newPath);
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



        void updateCommanders()
        {
            // Execute commands for commanders first
            for (auto &cluster : Clusters::getClusters()) {
                auto &commander = cluster.commander.lock();
                if (!commander || cluster.commandShare == CommandShare::None)
                    continue;

                // For pathing purposes, we store light air commander sim positions
                if (commander->hasSimTarget() && find(lastSimPositions.begin(), lastSimPositions.end(), commander->getSimTarget().getPosition()) == lastSimPositions.end()) {
                    if (lastSimPositions.size() >= 5)
                        lastSimPositions.pop_back();
                    lastSimPositions.insert(lastSimPositions.begin(), commander->getSimTarget().getPosition());
                }

                // Air commanders have a harass path as their objective
                updateSimulation(*commander);
                updateObjective(*commander);
                updateHarassPath(*commander);
                updateDestination(*commander);
                updateDecision(*commander);
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
                return u->getRole() == Role::Worker && !unit.getGoal().isValid() && (!unit.hasResource() || !unit.getResource().getType().isRefinery()) && !unit.getBuildPosition().isValid();
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
                    return u->isProxy() && u->getType().isBuilding() && u->canAttackGround();
                });
                auto proxyBuildingWorker = Util::getClosestUnit(BWEB::Map::getMainPosition(), PlayerState::Enemy, [&](auto &u) {
                    return u->getType().isWorker() && (u->isThreatening() || (proxyDangerousBuilding && u->getType().isWorker() && u->getPosition().getDistance(proxyDangerousBuilding->getPosition()) < 160.0));
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
                || (unit.isNearSuicide() && unit.isFlying())
                || reAlign)
                unit.setLocalState(LocalState::Retreat);

            // Regardless of local decision, determine if Unit needs to attack or retreat
            else if (unit.globalEngage()) {
                unit.setLocalState(LocalState::Attack);
                unit.circle(Colors::Green);
            }
            else if (unit.globalRetreat()) {
                unit.circle(Colors::Red);
                unit.setLocalState(LocalState::Retreat);
            }

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

                if (unit.getRole() == Role::Combat)
                    combatUnitsByDistance.emplace(unit.getPosition().getDistance(unit.getDestination()), unit);
            }
        }

        void updateCommands()
        {
            for (auto &cluster : Clusters::getClusters()) {
                for (auto &u : cluster.units) {
                    auto &unit = *u.lock();
                    if (cluster.commandShare == CommandShare::Exact && !unit.localRetreat() && !unit.globalRetreat() && !lightUnitNeedsRegroup(unit)) {
                        auto commander = cluster.commander.lock();
                        if (commander->getCommandType() == UnitCommandTypes::Attack_Unit && unit.hasTarget())
                            unit.command(commander->getCommandType(), unit.getTarget());
                        else if (commander->getCommandType() == UnitCommandTypes::Move && !unit.isTargetedBySplash())
                            unit.command(commander->getCommandType(), commander->getCommandPosition());
                        else if (commander->getCommandType() == UnitCommandTypes::Right_Click_Position && !unit.isTargetedBySplash())
                            unit.command(UnitCommandTypes::Right_Click_Position, commander->getCommandPosition());
                        else {
                            updateDestination(unit);
                            updateDecision(unit);
                        }
                    }
                    else {
                        updateDestination(unit);
                        updateDecision(unit);
                        Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Cyan);
                    }
                }
            }
        }

        void updateCombatUnits()
        {
            for (auto &[_, unit] : combatUnitsByDistance) {
                updateSimulation(unit);
                updateGlobalState(unit);
                updateLocalState(unit);
                updateObjective(unit);
                updateRetreat(unit);
                updateObjectivePath(unit);
                updateRetreatPath(unit);
            }
        }

        void reset()
        {
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
        updateCommanders();
        updateCommands();
        checkHoldChoke();
        Visuals::endPerfTest("Combat");
    }

    void onStart() {

    }

    bool defendChoke() { return holdChoke; }
    set<Position>& getDefendPositions() { return defendPositions; }
    multimap<double, UnitInfo&> getCombatUnitsByDistance() { return combatUnitsByDistance; }
}