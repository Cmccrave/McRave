#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Command {

    namespace {

        void updateCommands()
        {
            // Clear cache
            myActions.clear();
            enemyActions.clear();

            // Store bullets as enemy units if they can affect us too
            for (auto &b : Broodwar->getBullets()) {
                if (b && b->exists()) {
                    if (b->getType() == BulletTypes::Psionic_Storm)
                        addAction(nullptr, b->getPosition(), TechTypes::Psionic_Storm, true);

                    if (b->getType() == BulletTypes::EMP_Missile)
                        addAction(nullptr, b->getTargetPosition(), TechTypes::EMP_Shockwave, true);
                }
            }

            // Store enemy detection and assume casting orders
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;

                if (!unit.unit() || (unit.unit()->exists() && (unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())))
                    continue;

                if (unit.getType().isDetector())
                    addAction(unit.unit(), unit.getPosition(), unit.getType(), true);

                if (unit.unit()->exists()) {
                    Order enemyOrder = unit.unit()->getOrder();
                    Position enemyTarget = unit.unit()->getOrderTargetPosition();

                    if (enemyOrder == Orders::CastEMPShockwave)
                        addAction(unit.unit(), enemyTarget, TechTypes::EMP_Shockwave, true);
                    if (enemyOrder == Orders::CastPsionicStorm)
                        addAction(unit.unit(), enemyTarget, TechTypes::Psionic_Storm, true);
                }

                if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine)
                    addAction(unit.unit(), unit.getPosition(), TechTypes::Spider_Mines, true);
            }

            // Store nuke dots
            for (auto &dot : Broodwar->getNukeDots())
                addAction(nullptr, dot, TechTypes::Nuclear_Strike, true);
        }
    }

    bool misc(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
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
                unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
                return true;
            }
            else if (unit.getType() == UnitTypes::Protoss_High_Templar && unit.getEnergy() < 75) {
                unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
                return true;
            }
        }

        // Units targeted by splash need to move away from the army
        // TODO: Maybe move this to their respective functions
        else if (unit.hasTarget() && Units::getSplashTargets().find(unit.unit()) != Units::getSplashTargets().end()) {
            if (unit.hasTransport())
                unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
            else if (unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames() && unit.getTarget().unit()->exists())
                unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
            else
                unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition(), true);
            return true;
        }
        return false;
    }

    bool attack(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
        // If we have no target or it doesn't exist, we can't attack
        if (!unit.hasTarget() || !unit.getTarget().unit()->exists())
            return false;

        // Can the unit execute an attack command
        const auto canAttack = [&]() {
            if (unit.getType() == UnitTypes::Zerg_Lurker && !unit.isBurrowed())
                return false;

            auto cooldown = unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() : unit.unit()->getGroundWeaponCooldown();
            return cooldown < Broodwar->getRemainingLatencyFrames();
        };

        // Should the unit execute an attack command
        const auto shouldAttack = [&]() {
            if (unit.getRole() == Role::Combat) {

                // Check if we can get free attacks
                if (Util::getHighestThreat(WalkPosition(unit.getEngagePosition()), unit) == MIN_THREAT && Util::unitInRange(unit))
                    return true;
                return unit.getLocalState() == LocalState::Attack;
            }

            if (unit.getRole() == Role::Scout)
                return Grids::getEGroundThreat(unit.getEngagePosition()) <= 0.0;
            return false;
        };

        // Special Case: Carriers
        if (shouldAttack() && unit.getType() == UnitTypes::Protoss_Carrier) {
            auto leashRange = 320;
            for (auto &interceptor : unit.unit()->getInterceptors()) {
                if (interceptor->getOrder() == Orders::InterceptorReturn && interceptor->isCompleted()) {
                    unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
                    return true;
                }
            }
            if (unit.getPosition().getApproxDistance(unit.getTarget().getPosition()) >= leashRange) {
                unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
                unit.circleBlue();
                return true;
            }
        }

        // If unit should and can be attacking
        else if (shouldAttack() && canAttack()) {
            // Flyers don't want to decel when out of range, so we move to the target then attack when in range
            if ((unit.getType().isFlyer() || unit.getType().isWorker()) && unit.hasTarget() && !Util::unitInRange(unit)) {
                unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition(), true);
                unit.circleGreen();
            }
            else
                unit.command(UnitCommandTypes::Attack_Unit, unit.getTarget());
            return true;
        }
        return false;
    }

    bool approach(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
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
            unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition(), false);
            return true;
        }
        return false;
    }

    bool move(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            // Manual conversion until BWAPI::Point is fixed
            auto p = Position((w.x * 8) + 4, (w.y * 8) + 4);
            double distance = (unit.getType().isFlyer() || Terrain::isIslandMap()) ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination());
            double threat = exp(Util::getHighestThreat(w, unit));
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
                if (Util::unitInRange(unit) && !unit.getTarget().isHidden() && unit.getType() != UnitTypes::Zerg_Lurker)
                    return false;
                if (unit.getLocalState() == LocalState::Attack)
                    return true;
            }
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Worker || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        // 1) If unit has a transport, move to it or load into it
        if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
            unit.command(UnitCommandTypes::Right_Click_Unit, unit.getTransport());
            return true;
        }

        // 2) If unit can move and should move
        if (canMove() && shouldMove()) {

            // Make a path
            BWEB::PathFinding::Path unitPath;
            unitPath.createUnitPath(unit.getPosition(), unit.getDestination());
            Visuals::displayPath(unitPath.getTiles());

            // Move to the first point that is at least 5 tiles away if possible
            if (!unitPath.getTiles().empty() && unitPath.isReachable()) {
                for (auto &tile : unitPath.getTiles()) {
                    Position p = Position(tile) + Position(16, 16);
                    if (p.getDistance(unit.getPosition()) >= 256.0) {
                        unit.command(UnitCommandTypes::Move, p, true);
                        return true;
                    }
                }               
            }

            // Find the best position to move to
            if (unit.getType().isFlyer()) {
                auto bestPosition = findViablePosition(unit, scoreFunction);
                if (bestPosition.isValid()) {
                    unit.command(UnitCommandTypes::Move, bestPosition, true);
                    return true;
                }
            }

            // If it wasn't closer or didn't find one, move to our destination
            unit.command(UnitCommandTypes::Move, unit.getDestination(), false);
            return true;
        }
        return false;
    }

    bool kite(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
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

            if (unit.hasResource()) {
                unit.unit()->gather(unit.getResource().unit());
                return true;
            }
            else {
                // If we found a valid position, move to it
                auto bestPosition = findViablePosition(unit, scoreFunction);
                if (bestPosition.isValid()) {
                    unit.command(UnitCommandTypes::Move, bestPosition, true);
                    return true;
                }
            }
        }
        return false;
    }

    bool defend(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
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
                    if (dist < distBest && Util::isWalkable(unit, w)) {
                        distBest = dist;
                        walkBest = w;
                    }
                }
            }

            if (walkBest.isValid() && unit.unit()->getLastCommand().getTargetPosition() != Position(walkBest)) {
                unit.command(UnitCommandTypes::Move, Position(walkBest), false);
                return true;
            }
        }

        // HACK: Flyers defend above a base
        // TODO: Choose a base instead of closest to enemy, sometimes fly over a base I dont own
        if (unit.getType().isFlyer()) {
            if (Terrain::getEnemyStartingPosition().isValid() && BWEB::Map::getMainPosition().getDistance(Terrain::getEnemyStartingPosition()) < BWEB::Map::getNaturalPosition().getDistance(Terrain::getEnemyStartingPosition()))
                unit.command(UnitCommandTypes::Move, BWEB::Map::getMainPosition(), true);
            else
                unit.command(UnitCommandTypes::Move, BWEB::Map::getNaturalPosition(), true);
        }
        else {
            // Estimate a melee radius
            auto meleeRadius = Terrain::isDefendNatural() && Terrain::getNaturalWall() ? 32 : min(160, Units::getNumberMelee() * 16);

            // Estimate a ranged radius
                // At least: behind the melee arc or at least this units range
                // At most: the number of ranged units we have * half a tile + this units ground range
            auto rangedRadius = min(max(meleeRadius + 32, (int)unit.getGroundRange()), (Units::getNumberRanged() * 16) + (int)unit.getGroundRange());

            // Find a concave position at the desired radius
            auto radius = unit.getGroundRange() > 32.0 ? rangedRadius : meleeRadius;
            auto defendArea = Terrain::isDefendNatural() ? BWEB::Map::getNaturalArea() : BWEB::Map::getMainArea();
            auto bestPosition = Util::getConcavePosition(unit, radius, defendArea, Terrain::getDefendPosition());
            unit.command(UnitCommandTypes::Move, bestPosition, false);
            addAction(unit.unit(), bestPosition, unit.getType());
            return true;
        }
        return false;
    }

    bool hunt(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
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

            if (threat == MIN_THREAT
                || (unit.unit()->isCloaked() && !overlapsEnemyDetection(p)))
                return score;
            return 0.0;
        };

        const auto canHunt = [&]() {
            if (unit.getDestination().isValid() && unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        const auto shouldHunt = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.isLightAir())
                    return true;
            }
            if (unit.getRole() == Role::Transport)
                return true;
            if (unit.getRole() == Role::Scout && Terrain::getEnemyStartingPosition().isValid())
                return true;
            return false;
        };

        if (shouldHunt() && canHunt()) {
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid() && bestPosition != unit.getDestination()) {
                unit.command(UnitCommandTypes::Move, bestPosition, true);
                return true;
            }
        }
        return false;
    }

    bool retreat(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
        // Low distance, low threat, high clustering
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            // Manual conversion until BWAPI::Point is fixed
            auto p = Position((w.x * 8) + 4, (w.y * 8) + 4);
            double distance = ((unit.getType().isFlyer() || Terrain::isIslandMap()) ? log(p.getDistance(BWEB::Map::getMainPosition())) : Grids::getDistanceHome(w));
            double threat = Util::getHighestThreat(w, unit);
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = grouping / (threat * distance);
            return score;
        };

        const auto canRetreat = [&]() {
            if (unit.getSpeed() > 0.0)
                return true;
            return false;
        };

        const auto shouldRetreat = [&]() {
            if (unit.getRole() == Role::Combat) {
                if (unit.getLocalState() == LocalState::Retreat)
                    return true;
            }
            if (unit.getRole() == Role::Scout || unit.getRole() == Role::Transport)
                return true;
            return false;
        };

        if (canRetreat() && shouldRetreat()) {
            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {
                unit.command(UnitCommandTypes::Move, bestPosition, true);
                return true;
            }
        }
        return false;
    }

    bool escort(const shared_ptr<UnitInfo>& u)
    {
        auto &unit = *u;
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
            unit.command(UnitCommandTypes::Move, bestPosition, true);
            return true;
        }
        return false;
    }

    bool overlapsActions(Unit unit, TechType tech, Position here, int radius)
    {
        auto checkTopLeft = here + Position(-radius / 2, -radius / 2);
        auto checkTopRight = here + Position(radius / 2, -radius / 2);
        auto checkBotLeft = here + Position(-radius / 2, radius / 2);
        auto checkBotRight = here + Position(radius / 2, radius / 2);

        // TechType checks use a rectangular check
        for (auto &command : myActions) {
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

    bool overlapsActions(Unit unit, UnitType type, Position here, int radius)
    {
        // UnitType checks use a radial check
        for (auto &command : myActions) {
            if (command.unit != unit && command.type == type && command.pos.getDistance(here) <= radius * 2)
                return true;
        }
        return false;
    }

    bool overlapsAllyDetection(Position here)
    {
        // Detection checks use a radial check
        for (auto &command : myActions) {
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
        for (auto &command : enemyActions) {
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
        for (auto &command : myActions) {
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

        for (auto &command : enemyActions) {
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
        auto radius = 8;
        auto walkHeight = int(unit.getType().height() / 8.0);
        auto walkWidth = int(unit.getType().width() / 8.0);
        auto mapWidth = Broodwar->mapWidth() * 4;
        auto mapHeight = Broodwar->mapHeight() * 4;
        auto offset = unit.getType().isFlyer() ? 12 : 0; // offset for flyers to try not to get stuck on map edges

        // Boundaries
        auto left = max(start.x - radius, offset);
        auto right = min(start.x + radius + walkWidth, mapWidth - offset);
        auto top = max(start.y - radius, offset);
        auto bot = min(start.y + radius + walkHeight, mapHeight - offset);
        
        const auto viablePosition = [&](WalkPosition here, Position p) {
            // If not a flyer and position blocks a building, has collision or a splash threat
            if (!unit.getType().isFlyer() &&
                (Buildings::overlapsQueue(unit.getType(), unit.getTilePosition()) || Grids::getESplash(here) > 0))
                return false;

            // If too close of a command, is in danger or isn't walkable
            if (p.getDistance(unit.getPosition()) > radius * 8
                || (!unit.getType().isFlyer() && !Broodwar->isWalkable(here))
                || isInDanger(unit, p)
                || !Util::isWalkable(unit, here))
                return false;
            return true;
        };

        auto bestPosition = Positions::Invalid;
        auto best = 0.0;
        for (int x = left; x <= right; x++) {
            for (int y = top; y <= bot; y++) {
                auto w = WalkPosition(x, y);
                auto p = Position((x * 8) + 4, (y * 8) + 4);

                auto current = score(w);
                if (current > best && viablePosition(w, p)) {
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

    vector <Action>& getMyActions() { return myActions; }
    vector <Action>& getEnemyActions() { return enemyActions; }
}
