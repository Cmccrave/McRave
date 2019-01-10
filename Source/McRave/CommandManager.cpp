#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Command {

    namespace {

        void updateCommands()
        {
            // Clear cache
            myCommands.clear();
            enemyCommands.clear();

            // Store bullets as enemy units if they can affect us too
            for (auto &b : Broodwar->getBullets()) {
                if (b && b->exists()) {
                    if (b->getType() == BulletTypes::Psionic_Storm)
                        addCommand(nullptr, b->getPosition(), TechTypes::Psionic_Storm, true);

                    if (b->getType() == BulletTypes::EMP_Missile)
                        addCommand(nullptr, b->getTargetPosition(), TechTypes::EMP_Shockwave, true);
                }
            }

            // Store enemy detection and assume casting orders
            for (auto &u : Units::getEnemyUnits()) {
                UnitInfo& unit = u.second;

                if (!unit.unit() || (unit.unit()->exists() && (unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())))
                    continue;

                if (unit.getType().isDetector())
                    addCommand(unit.unit(), unit.getPosition(), unit.getType(), true);

                if (unit.unit()->exists()) {
                    Order enemyOrder = unit.unit()->getOrder();
                    Position enemyTarget = unit.unit()->getOrderTargetPosition();

                    if (enemyOrder == Orders::CastEMPShockwave)
                        addCommand(unit.unit(), enemyTarget, TechTypes::EMP_Shockwave, true);
                    if (enemyOrder == Orders::CastPsionicStorm)
                        addCommand(unit.unit(), enemyTarget, TechTypes::Psionic_Storm, true);
                }

                if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine)
                    addCommand(unit.unit(), unit.getPosition(), TechTypes::Spider_Mines, true);
            }

            // Store nuke dots
            for (auto &dot : Broodwar->getNukeDots())
                addCommand(nullptr, dot, TechTypes::Nuclear_Strike, true);
        }
    }

    bool misc(UnitInfo& unit)
    {
        // Unstick a unit
        if (unit.isStuck() && unit.unit()->isMoving()) {
            unit.unit()->stop();
            return true;
        }

        // DT hold position vs spider mines
        else if (unit.getType() == UnitTypes::Protoss_Dark_Templar && unit.hasTarget() && unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine && unit.getTarget().hasTarget() && unit.getTarget().getTarget().unit() == unit.unit() && !unit.getTarget().isBurrowed()) {
            if (unit.unit()->getLastCommand().getType() != UnitCommandTypes::Hold_Position)
                unit.unit()->holdPosition();
            return true;
        }

        // If unit has a transport, load into it if we need to
        else if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
            if (unit.getType() == UnitTypes::Protoss_Reaver && unit.unit()->isTraining() && unit.unit()->getScarabCount() != MAX_SCARAB) {
                unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
                return true;
            }
            else if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75) {
                unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
                return true;
            }
        }

        // Units targeted by splash need to move away from the army
        // TODO: Maybe move this to their respective functions
        else if (unit.hasTarget() && Units::getSplashTargets().find(unit.unit()) != Units::getSplashTargets().end()) {
            if (unit.hasTransport())
                unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
            else if (unit.hasTarget() && unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames() && unit.getTarget().unit()->exists())
                unit.command(UnitCommandTypes::Attack_Unit, &unit.getTarget());
            else
                unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition());
            return true;
        }
        return false;
    }

    bool attack(UnitInfo& unit)
    {
        // If we have no target or it doesn't exist, we can't attack
        if (!unit.hasTarget() || !unit.getTarget().unit()->exists())
            return false;

        // Can the unit execute an attack command
        const auto canAttack = [&]() {
            auto cooldown = unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() : unit.unit()->getGroundWeaponCooldown();
            return cooldown < Broodwar->getRemainingLatencyFrames();
        };

        // Should the unit execute an attack command
        const auto shouldAttack = [&]() {
            if (unit.getRole() == Role::Combat)
                return unit.getLocalState() == LocalState::Attack;

            if (unit.getRole() == Role::Scout)
                return Grids::getEGroundThreat(unit.getEngagePosition()) <= 0.0;
        };

        // Special Case: Carriers
        if (shouldAttack() && unit.getType() == UnitTypes::Protoss_Carrier) {
            auto leashRange = 320;
            for (auto &interceptor : unit.unit()->getInterceptors()) {
                if (interceptor->getOrder() == Orders::InterceptorReturn && interceptor->isCompleted()) {
                    unit.command(UnitCommandTypes::Attack_Unit, &unit.getTarget());
                    return true;
                }
            }
            if (unit.getPosition().getApproxDistance(unit.getTarget().getPosition()) >= leashRange) {
                unit.command(UnitCommandTypes::Attack_Unit, &unit.getTarget());
                unit.circleBlue();
                return true;
            }
        }

        // If unit should and can be attacking
        else if (shouldAttack() && canAttack()) {
            // Flyers don't want to decel when out of range, so we move to the target then attack when in range
            if ((unit.getType().isFlyer() || unit.getType().isWorker()) && unit.hasTarget() && !Util::unitInRange(unit))
                unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition());
            else
                unit.command(UnitCommandTypes::Attack_Unit, &unit.getTarget());
            return true;
        }
        return false;
    }

    bool approach(UnitInfo& unit)
    {
        // If we don't have a target or the targets position is invalid, we can't approach it
        if (!unit.hasTarget() || !unit.getTarget().getPosition().isValid())
            return false;

        // Can the unit approach its target
        const auto canApproach = [&]() {
            auto canAttack = unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() < Broodwar->getRemainingLatencyFrames() : unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames();
            if (unit.getSpeed() <= 0.0 || canAttack)
                return false;
            return true;
        };

        // Should the unit approach its target
        const auto shouldApproach = [&]() {
            auto unitRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

            if (unit.getRole() == Role::Combat) {

                // Capital ships want to stay at max range due to their slow nature
                if (unit.isCapitalShip() || unit.getTarget().isSuicidal())
                    return false;

                // Light air wants to approach air targets or anything that cant attack air
                if (unit.isLightAir()) {
                    if (unit.getTarget().getType().isFlyer() || unit.getTarget().getAirRange() == 0.0)
                        return true;
                    return false;
                }

                // HACK: Dragoons shouldn't approach Vultures before range upgrade
                if (unit.getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
                    return false;

                // If we have a decisive win
                if (unit.getSimValue() >= 5.0 && !unit.getTarget().getType().isWorker())
                    return true;
            }

            // If this units range is lower and target isn't a building
            if (unitRange < enemyRange && !unit.getTarget().getType().isBuilding())
                return true;
            return false;
        };

        if (shouldApproach() && canApproach()) {
            unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition());
            return true;
        }
        return false;
    }

    bool move(UnitInfo& unit)
    {
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            // Manual conversion until BWAPI::Point is fixed
            auto p = Position((w.x * 8) + 4, (w.y * 8) + 4);
            double distance = (unit.getType().isFlyer() || Terrain::isIslandMap()) ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination());
            double threat = Util::getHighestThreat(w, unit);
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = grouping / (threat * distance);
            return score;
        };

        auto canMove = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        auto shouldMove = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.getLocalState() == LocalState::Retreat)
                    return false;
            }
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Worker || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        // 1) If unit has a transport, move to it or load into it
        if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
            unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
            return true;
        }

        // 2) If unit can move and should move
        if (canMove() && shouldMove()) {

            // Find the best position to move to
            auto bestPosition = findViablePosition(unit, scoreFunction);

            if (bestPosition.isValid()) {

                // Create a path from this unit and from best position to determine if bestPosition is truly closer
                BWEB::PathFinding::Path unitPath;
                BWEB::PathFinding::Path bPath;

                unitPath.createUnitPath(unit.getPosition(), unit.getDestination());
                bPath.createUnitPath(bestPosition, unit.getDestination());

                Visuals::displayPath(unitPath.getTiles());

                // If it is closer, move to it
                if (bPath.getDistance() < unitPath.getDistance()) {
                    unit.command(UnitCommandTypes::Move, bestPosition);
                    return true;
                }
            }

            // If it wasn't closer or didn't find one, move to our destination
            unit.command(UnitCommandTypes::Move, unit.getDestination());
            return true;
        }
        return false;
    }

    bool kite(UnitInfo& unit)
    {
        // If we don't have a target, we can't kite it
        if (!unit.hasTarget())
            return false;

        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            // Manual conversion until BWAPI::Point is fixed
            auto p = Position((w.x * 8) + 4, (w.y * 8) + 4);
            double distance = unit.hasTarget() ? 1.0 / (p.getDistance(unit.getTarget().getPosition())) : p.getDistance(BWEB::Map::getMainPosition());
            double threat = Util::getHighestThreat(w, unit);
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = grouping / (threat * distance);
            return score;
        };

        const auto canKite = [&]() {
            auto canAttack = unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() < Broodwar->getRemainingLatencyFrames() : unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames();
            if (unit.getSpeed() <= 0.0 || canAttack)
                return false;
            return true;
        };

        const auto shouldKite = [&]() {
            auto allyRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());
            
            if (unit.getRole() == Role::Combat) {

                if (unit.getTarget().getType().isBuilding() && !unit.getType().isFlyer())
                    return false;

                // Capital ships want to stay at max range due to their slow nature
                if (unit.isCapitalShip() || unit.getTarget().isSuicidal())
                    return true;

                // Light air shouldn't kite flyers or units that can't attack air
                if (unit.isLightAir()) {
                    if (unit.getTarget().getType().isFlyer() || unit.getTarget().getAirRange() == 0.0)
                        return false;
                    return true;
                }

                // HACK: Dragoons should always kite Vultures before range upgrade
                if (unit.getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
                    return true;

                if (unit.getType() == UnitTypes::Protoss_Reaver
                    || unit.getType() == UnitTypes::Terran_Vulture
                    || (unit.unit()->isUnderAttack() && allyRange >= enemyRange)
                    || unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine)
                    return true;
            }

            // If unit isn't attacking at maximum range
            if (enemyRange <= allyRange && unit.unit()->getDistance(unit.getTarget().getPosition()) <= allyRange - enemyRange)
                return true;
            return false;
        };

        // Special Case: Carriers
        if (unit.getType() == UnitTypes::Protoss_Carrier) {
            auto leashRange = 320;
            for (auto &interceptor : unit.unit()->getInterceptors()) {
                if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->isCompleted())
                    return false;
            }
            if (unit.getPosition().getApproxDistance(unit.getTarget().getPosition()) >= leashRange)
                return false;
        }

        if (shouldKite() && canKite()) {

            // If we found a valid position, move to it
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(UnitCommandTypes::Move, bestPosition);
                return true;
            }
        }
        return false;
    }

    bool defend(UnitInfo& unit)
    {
        bool defendingExpansion = unit.getDestination().isValid() && !Terrain::isInEnemyTerritory((TilePosition)unit.getDestination());
        bool closeToDefend = Terrain::getDefendPosition().getDistance(unit.getPosition()) < 640.0 || Terrain::isInAllyTerritory(unit.getTilePosition());

        if (!closeToDefend || unit.getLocalState() != LocalState::Retreat)
            return false;

        // Probe Cannon surround
        if (unit.getType().isWorker() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) {
            auto cannon = Util::getClosestUnit(mapBWEM.Center(), PlayerState::Self, [&](auto &u) {
                return u.getType() == UnitTypes::Protoss_Photon_Cannon;
            });

            auto distBest = DBL_MAX;
            auto walkBest = WalkPositions::Invalid;
            auto start = cannon->getWalkPosition();
            for (int x = start.x - 2; x < start.x + 10; x++) {
                for (int y = start.y - 2; y < start.y + 10; y++) {
                    WalkPosition w(x, y);
                    double dist = Position(w).getDistance(mapBWEM.Center());
                    if (dist < distBest && Util::isWalkable(unit.getWalkPosition(), w, unit.getType())) {
                        distBest = dist;
                        walkBest = w;
                    }
                }
            }

            if (walkBest.isValid() && unit.unit()->getLastCommand().getTargetPosition() != Position(walkBest)) {
                unit.command(UnitCommandTypes::Move, Position(walkBest));
                return true;
            }
        }

        // HACK: Flyers defend above a base
        // TODO: Choose a base instead of closest to enemy, sometimes fly over a base I dont own
        if (unit.getType().isFlyer()) {
            if (Terrain::getEnemyStartingPosition().isValid() && BWEB::Map::getMainPosition().getDistance(Terrain::getEnemyStartingPosition()) < BWEB::Map::getNaturalPosition().getDistance(Terrain::getEnemyStartingPosition()))
                unit.command(UnitCommandTypes::Move, BWEB::Map::getMainPosition());
            else
                unit.command(UnitCommandTypes::Move, BWEB::Map::getNaturalPosition());
        }
        return false;
    }

    bool hunt(UnitInfo& unit)
    {
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {

            // Manual conversion until BWAPI::Point is fixed
            auto p = Position((w.x * 8) + 4, (w.y * 8) + 4);

            // Check if this is too far from transports - TODO: Move to transports
            for (auto &u : unit.getAssignedCargo()) {
                if (!u->unit()->isLoaded() && u->getPosition().getDistance(p) > 32.0)
                    return 0.0;
            }

            double threat = Util::getHighestThreat(w, unit);
            double distance = (unit.getType().isFlyer() ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination()));
            double visited = log(min(500.0, double(Broodwar->getFrameCount() - Grids::lastVisitedFrame(w))));
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = grouping * visited / distance;

            if (threat == MIN_THREAT || (unit.unit()->isCloaked() && !overlapsEnemyDetection(p)))
                return score;
            return 0.0;
        };

        // Hunting is only valid with workers, flyers or dark templars
        auto shouldHunt = unit.getType().isWorker() || unit.getType().isFlyer() /*|| unit.getType() == UnitTypes::Protoss_Dark_Templar*/;
        if (!shouldHunt)
            return false;

        // HACK: If unit has no destination, give target as a destination
        if (!unit.getDestination().isValid()) {
            if (unit.hasTarget())
                unit.setDestination(unit.getTarget().getPosition());
            else if (Terrain::getAttackPosition().isValid())
                unit.setDestination(Terrain::getAttackPosition());
        }

        // If we found a valid position
        auto bestPosition = findViablePosition(unit, scoreFunction);

        // Check if we can get free attacks
        if (unit.hasTarget() && Util::getHighestThreat(WalkPosition(unit.getEngagePosition()), unit) == MIN_THREAT && Util::unitInRange(unit)) {
            attack(unit);
            return true;
        }
        // Move to the position
        else if (bestPosition.isValid() && bestPosition != unit.getDestination()) {
            unit.command(UnitCommandTypes::Move, bestPosition);
            return true;
        }
        return false;
    }

    bool retreat(UnitInfo& unit)
    {
        // Low distance, low threat, high clustering
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            // Manual conversion until BWAPI::Point is fixed
            auto p = Position((w.x * 8) + 4, (w.y * 8) + 4);
            double distance = ((unit.getType().isFlyer() || Terrain::isIslandMap()) ? p.getDistance(BWEB::Map::getMainPosition()) : Grids::getDistanceHome(w));
            double threat = Util::getHighestThreat(w, unit);
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = grouping / (threat * distance);
            return score;
        };

        // Retreating is only valid when local state is retreating
        auto shouldRetreat = unit.getLocalState() == LocalState::Retreat || unit.getRole() == Role::Scout;
        if (!shouldRetreat)
            return false;

        // If we found a valid position, move to it
        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid()) {
            unit.command(UnitCommandTypes::Move, bestPosition);
            return true;
        }
        return false;
    }

    bool escort(UnitInfo& unit)
    {
        // Low distance, low threat
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            // Manual conversion until BWAPI::Point is fixed
            auto p = Position((w.x * 8) + 4, (w.y * 8) + 4);
            double threat = Util::getHighestThreat(w, unit);
            double distance = 1.0 + (unit.getType().isFlyer() ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination()));
            double score = 1.0 / (threat * distance);
            return score;
        };

        // Escorting
        auto shouldEscort = false;
        if (!shouldEscort)
            return false;

        // If we found a valid position, move to it
        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid()) {
            unit.command(UnitCommandTypes::Move, bestPosition);
            return true;
        }
        return false;
    }

    bool overlapsCommands(Unit unit, TechType tech, Position here, int radius)
    {
        auto checkTopLeft = here + Position(-radius / 2, -radius / 2);
        auto checkTopRight = here + Position(radius / 2, -radius / 2);
        auto checkBotLeft = here + Position(-radius / 2, radius / 2);
        auto checkBotRight = here + Position(radius / 2, radius / 2);

        // TechType checks use a rectangular check
        for (auto &command : myCommands) {
            auto topLeft = command.pos - Position(radius, radius);
            auto botRight = command.pos + Position(radius, radius);

            if (command.unit != unit && command.tech == tech &&
                (Util::rectangleIntersect(topLeft, botRight, checkTopLeft)
                    || Util::rectangleIntersect(topLeft, botRight, checkTopRight)
                    || Util::rectangleIntersect(topLeft, botRight, checkBotLeft)
                    || Util::rectangleIntersect(topLeft, botRight, checkBotRight)))
                return true;
        }
        return false;
    }

    bool overlapsCommands(Unit unit, UnitType type, Position here, int radius)
    {
        // UnitType checks use a radial check
        for (auto &command : myCommands) {
            if (command.unit != unit && command.type == type && command.pos.getDistance(here) <= radius * 2)
                return true;
        }
        return false;
    }

    bool overlapsAllyDetection(Position here)
    {
        // Detection checks use a radial check
        for (auto &command : myCommands) {
            if (command.type == UnitTypes::Spell_Scanner_Sweep) {
                double range = 420.0;
                if (command.pos.getDistance(here) < range)
                    return true;
            }
            else if (command.type.isDetector()) {
                double range = command.type.isBuilding() ? 256.0 : 360.0;
                if (command.pos.getDistance(here) < range)
                    return true;
            }
        }
        return false;
    }

    bool overlapsEnemyDetection(Position here)
    {
        // Detection checks use a radial check
        for (auto &command : enemyCommands) {
            if (command.type == UnitTypes::Spell_Scanner_Sweep) {
                double range = 420.0;
                double ff = 32;
                if (command.pos.getDistance(here) < range + ff)
                    return true;
            }

            else if (command.type.isDetector()) {
                double range = command.type.isBuilding() ? 256.0 : 360.0;
                double ff = 32;
                if (command.pos.getDistance(here) < range + ff)
                    return true;
            }
        }
        return false;
    }

    bool isInDanger(UnitInfo& unit, Position here)
    {
        int halfWidth = int(ceil(unit.getType().width() / 2));
        int halfHeight = int(ceil(unit.getType().height() / 2));

        if (here == Positions::Invalid)
            here = unit.getPosition();

        auto uTopLeft = here + Position(-halfWidth, -halfHeight);
        auto uTopRight = here + Position(halfWidth, -halfHeight);
        auto uBotLeft = here + Position(-halfWidth, halfHeight);
        auto uBotRight = here + Position(halfWidth, halfHeight);

        const auto checkCorners = [&](Position cTopLeft, Position cBotRight) {
            if (Util::rectangleIntersect(cTopLeft, cBotRight, uTopLeft)
                || Util::rectangleIntersect(cTopLeft, cBotRight, uTopRight)
                || Util::rectangleIntersect(cTopLeft, cBotRight, uBotLeft)
                || Util::rectangleIntersect(cTopLeft, cBotRight, uBotRight))
                return true;
            return false;
        };

        // Check that we're not in danger of Storm, DWEB, EMP
        for (auto &command : myCommands) {
            if (command.tech == TechTypes::Psionic_Storm
                || command.tech == TechTypes::Disruption_Web
                || command.tech == TechTypes::EMP_Shockwave
                || command.tech == TechTypes::Spider_Mines) {
                auto cTopLeft = command.pos - Position(48, 48);
                auto cBotRight = command.pos + Position(48, 48);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }

            if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
                auto cTopLeft = command.pos - Position(320, 320);
                auto cBotRight = command.pos + Position(320, 320);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }
        }

        for (auto &command : enemyCommands) {
            if (command.tech == TechTypes::Psionic_Storm
                || command.tech == TechTypes::Disruption_Web
                || command.tech == TechTypes::EMP_Shockwave
                || command.tech == TechTypes::Spider_Mines) {
                auto cTopLeft = command.pos - Position(48, 48);
                auto cBotRight = command.pos + Position(48, 48);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }
            if (command.tech == TechTypes::Nuclear_Strike && here.getDistance(command.pos) <= 640) {
                auto cTopLeft = command.pos - Position(320, 320);
                auto cBotRight = command.pos + Position(320, 320);

                if (checkCorners(cTopLeft, cBotRight))
                    return true;
            }
        }
        return false;
    }

    Position findViablePosition(UnitInfo& unit, function<double(WalkPosition)> score)
    {
        // Radius for checking tiles
        auto start = unit.getWalkPosition();
        auto radius = 8 + int(unit.getSpeed());
        auto walkHeight = int(unit.getType().height() / 8.0);
        auto walkWidth = int(unit.getType().width() / 8.0);
        auto mapWidth = Broodwar->mapWidth() * 4;
        auto mapHeight = Broodwar->mapHeight() * 4;
        auto offset = unit.getType().isFlyer() ? 12 : 0; // offset for flyers to try not to get stuck on map edges
        auto center = WalkPosition(unit.getPosition());

        // Boundaries
        auto left = max(start.x - radius, offset);
        auto right = min(start.x + radius + walkWidth, mapWidth - offset);
        auto top = max(start.y - radius, offset);
        auto bot = min(start.y + radius + walkHeight, mapHeight - offset);


        // Check what directions are okay
        //auto topY = start.y - 1;
        //if (topY > 0 && topY < mapHeight) {
        //    topOk = true;
        //    for (int x = start.x - 1; x <= start.x + walkWidth; x++) {
        //        Position p = Position(WalkPosition(x, topY));
        //        Broodwar->drawBoxMap(p, p + Position(8, 8), Colors::Black);
        //        if (Grids::getCollision(WalkPosition(x, topY)) > 0 || !Broodwar->isWalkable(WalkPosition(x, topY)))
        //            topOk = false;
        //    }
        //}
        //auto botY = start.y + walkHeight;
        //if (botY > 0 && botY < mapHeight) {
        //    botOk = true;
        //    for (int x = start.x - 1; x <= start.x + walkWidth; x++) {
        //        Position p = Position(WalkPosition(x, botY));
        //        Broodwar->drawBoxMap(p, p + Position(8, 8), Colors::Black);
        //        if (Grids::getCollision(WalkPosition(x, botY)) > 0 || !Broodwar->isWalkable(WalkPosition(x, botY)))
        //            botOk = false;
        //    }
        //}
        //auto leftX = start.x - 1;
        //if (leftX > 0 && leftX < mapWidth) {
        //    leftOk = true;
        //    for (int y = start.y - 1; y <= start.y + walkHeight; y++) {
        //        Position p = Position(WalkPosition(leftX, y));
        //        Broodwar->drawBoxMap(p, p + Position(8, 8), Colors::Black);
        //        if (Grids::getCollision(WalkPosition(leftX, y)) > 0 || !Broodwar->isWalkable(WalkPosition(leftX, y)))
        //            leftOk = false;
        //    }
        //}
        //auto rightX = start.x + walkWidth;
        //if (rightX > 0 && rightX < mapWidth) {
        //    rightOk = true;
        //    for (int y = start.y - 1; y <= start.y + walkHeight; y++) {
        //        Position p = Position(WalkPosition(rightX, y));
        //        Broodwar->drawBoxMap(p, p + Position(8, 8), Colors::Black);
        //        if (Grids::getCollision(WalkPosition(rightX, y)) > 0 || !Broodwar->isWalkable(WalkPosition(rightX, y)))
        //            leftOk = false;
        //    }
        //}

        auto topWalkPosition = center - WalkPosition(0, walkHeight * 2);
        auto topOk = topWalkPosition.isValid() && Grids::getCollision(topWalkPosition) == 0 && Broodwar->isWalkable(topWalkPosition);

        auto botWalkPosition = center + WalkPosition(0, walkHeight * 2);
        auto botOk = botWalkPosition.isValid() && Grids::getCollision(botWalkPosition) == 0 && Broodwar->isWalkable(botWalkPosition);

        auto leftWalkPosition = center - WalkPosition(walkWidth * 2, 0);
        auto leftOk = leftWalkPosition.isValid() && Grids::getCollision(leftWalkPosition) == 0 && Broodwar->isWalkable(leftWalkPosition);

        auto rightWalkPosition = center + WalkPosition(walkWidth * 2, 0);
        auto rightOk = rightWalkPosition.isValid() && Grids::getCollision(rightWalkPosition) == 0 && Broodwar->isWalkable(rightWalkPosition);


        auto topLeftWalkPosition = center - WalkPosition(walkWidth * 2, walkHeight * 2);
        auto topLeftOk = topLeftWalkPosition.isValid() && Grids::getCollision(topLeftWalkPosition) == 0 && Broodwar->isWalkable(topLeftWalkPosition);

        auto topRightWalkPosition = center + WalkPosition(walkWidth * 2, -walkHeight * 2);
        auto topRightOk = topRightWalkPosition.isValid() && Grids::getCollision(topRightWalkPosition) == 0 && Broodwar->isWalkable(topRightWalkPosition);

        auto botLeftWalkPosition = center - WalkPosition(walkWidth * 2, -walkHeight * 2);
        auto botLeftOk = botLeftWalkPosition.isValid() && Grids::getCollision(botLeftWalkPosition) == 0 && Broodwar->isWalkable(botLeftWalkPosition);

        auto botRightWalkPosition = center + WalkPosition(walkWidth * 2, walkHeight * 2);
        auto botRightOk = botRightWalkPosition.isValid() && Grids::getCollision(botRightWalkPosition) == 0 && Broodwar->isWalkable(botRightWalkPosition);


        //Broodwar->drawCircleMap(Position(topWalkPosition), 4, topOk ? Colors::Green : Colors::Red);
        //Broodwar->drawCircleMap(Position(botWalkPosition), 4, botOk ? Colors::Green : Colors::Red);
        //Broodwar->drawCircleMap(Position(leftWalkPosition), 4, leftOk ? Colors::Green : Colors::Red);
        //Broodwar->drawCircleMap(Position(rightWalkPosition), 4, rightOk ? Colors::Green : Colors::Red);

        //Broodwar->drawCircleMap(Position(topLeftWalkPosition), 4, topLeftOk ? Colors::Green : Colors::Red);
        //Broodwar->drawCircleMap(Position(topRightWalkPosition), 4, topRightOk ? Colors::Green : Colors::Red);
        //Broodwar->drawCircleMap(Position(botLeftWalkPosition), 4, botLeftOk ? Colors::Green : Colors::Red);
        //Broodwar->drawCircleMap(Position(botRightWalkPosition), 4, botRightOk ? Colors::Green : Colors::Red);

        const auto directionsOkay = [&](WalkPosition here) {
            if (unit.getType().isFlyer())
                return true;

            auto yDiff = start.y - here.y;
            auto xDiff = start.x - here.x;

            // If this Position is a move horizontally
            if (abs(xDiff) > walkWidth) {

                // If this Position is a move vertically
                if (abs(yDiff) > walkHeight) {
                    if (xDiff > 0 && yDiff > 0 && !topLeftOk)
                        return false;
                    if (xDiff < 0 && yDiff > 0 && !topRightOk)
                        return false;
                    if (xDiff > 0 && yDiff < 0 && !botLeftOk)
                        return false;
                    if (xDiff < 0 && yDiff < 0 && !botRightOk)
                        return false;
                }
                else if (xDiff > 0 && !leftOk)
                    return false;
                else if (xDiff < 0 && !rightOk)
                    return false;
            }
            else if (yDiff > 0 && !topOk)
                return false;
            else if (yDiff < 0 && !botOk)
                return false;
            return true;
        };

        const auto viablePosition = [&](WalkPosition here, Position p) {
            // If not a flyer and position blocks a building, has collision or a splash threat
            if (!unit.getType().isFlyer() &&
                (Buildings::overlapsQueue(unit.getType(), unit.getTilePosition()) || Grids::getESplash(here) > 0))
                return false;

            // If too close of a command, is in danger or isn't walkable
            if (p.getDistance(unit.getPosition()) > radius * 8
                || !Broodwar->isWalkable(here)
                || isInDanger(unit, p)
                || !Util::isWalkable(unit.getWalkPosition(), here, unit.getType()))
                return false;
            return true;
        };

        auto bestPosition = Positions::Invalid;
        auto best = 0.0;
        for (int x = left; x < right; x++) {
            for (int y = top; y < bot; y++) {
                auto w = WalkPosition(x, y);
                auto p = Position((x * 8) + 4, (y * 8) + 4);

                //if (directionsOkay(w))
                    //Broodwar->drawBoxMap(p, p + Position(8, 8), Colors::Black);

                auto current = score(w);
                if (current > best && directionsOkay(w) && viablePosition(w, p)) {
                    bestPosition = p;
                    best = current;
                }
            }
        }
        return bestPosition;
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateCommands();
        Visuals::endPerfTest("Commands");
    }

    vector <CommandType>& getMyCommands() { return myCommands; }
    vector <CommandType>& getEnemyCommands() { return enemyCommands; }
}
