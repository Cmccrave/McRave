#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Command {

    namespace {

        map<double, UnitInfo*> defendingUnitsByDist;

        void updateEnemyCommands()
        {
            // Clear cache
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

        constexpr tuple commands{ misc, special, attack, approach, kite, defend, hunt, escort, retreat, move };
        void updateDecision(UnitInfo& unit)
        {
            if (!unit.unit() || !unit.unit()->exists()																							// Prevent crashes			
                || unit.unit()->isLoaded()
                || unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())	// If the unit is locked down, maelstrommed, stassised, or not completed
                return;

            // Convert our commands to strings to display what the unit is doing for debugging
            map<int, string> commandNames{
                make_pair(0, "Misc"),
                make_pair(1, "Special"),
                make_pair(2, "Attack"),
                make_pair(3, "Approach"),
                make_pair(4, "Kite"),
                make_pair(5, "Defend"),
                make_pair(6, "Hunt"),
                make_pair(7, "Escort"),
                make_pair(8, "Retreat"),
                make_pair(9, "Move")
            };

            // Iterate commands, if one is executed then don't try to execute other commands
            int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
            int i = Util::iterateCommands(commands, unit);
            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", Text::White, commandNames[i].c_str());
        }

        void updateUnits()
        {
            myCommands.clear();
            defendingUnitsByDist.clear();

            for (auto &u : Units::getMyUnits()) {
                auto &unit = u.second;
                if (unit.getType() == UnitTypes::Protoss_Interceptor || unit.getType() == UnitTypes::Protoss_Scarab)
                    continue;

                if (unit.getRole() == Role::Fighting)
                    updateDecision(unit);
            }

            for (auto &u : defendingUnitsByDist) {
                auto unit = *u.second;

                Position bestPosition = Positions::Invalid;
                if (Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Dragoon) == 0 && Players::vT())
                    bestPosition = Util::getConcavePosition(unit, nullptr, BWEB::Map::getMainPosition());
                else
                    bestPosition = Util::getConcavePosition(unit, nullptr, Terrain::getDefendPosition());

                if (bestPosition.isValid()) {
                    addCommand(unit.unit(), bestPosition, UnitTypes::None);
                    unit.command(UnitCommandTypes::Move, bestPosition);
                    unit.setDestination(bestPosition);
                }
            }
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
        if (unit.hasTarget()) {
            auto shouldAttack = unit.hasTarget() && unit.getTarget().unit()->exists() && unit.getLocalState() == LocalState::Engaging;
            auto canAttack = unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() < Broodwar->getRemainingLatencyFrames() : unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames();

            // Special Case: Carriers
            if (shouldAttack && unit.getType() == UnitTypes::Protoss_Carrier) {
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

            // If unit should be attacking
            else if (shouldAttack && canAttack) {
                // Flyers don't want to decel when out of range, so we move to the target then attack when in range
                if ((unit.getType().isFlyer() || unit.getType().isWorker()) && unit.hasTarget() && !Util::unitInRange(unit))
                    unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition());
                else
                    unit.command(UnitCommandTypes::Attack_Unit, &unit.getTarget());
                return true;
            }
        }
        return false;
    }

    bool approach(UnitInfo& unit)
    {
        const auto canApproach = [&]() {
            auto canAttack = unit.getTarget().getType().isFlyer() ? unit.unit()->getAirWeaponCooldown() < Broodwar->getRemainingLatencyFrames() : unit.unit()->getGroundWeaponCooldown() < Broodwar->getRemainingLatencyFrames();
            if (unit.getSpeed() <= 0.0 || canAttack)
                return false;
            return true;
        };

        const auto shouldApproach = [&]() {
            auto allyRange = (unit.getTarget().getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange());
            auto enemyRange = (unit.getType().isFlyer() ? unit.getTarget().getAirRange() : unit.getTarget().getGroundRange());

            if (unit.isCapitalShip() || unit.getTarget().isSuicidal())
                return false;
            if (unit.getType() == UnitTypes::Protoss_Dragoon && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Singularity_Charge) == 0)
                return false;

            if ((unit.getSimValue() >= 10.0 && !unit.getTarget().getType().isWorker())
                || (allyRange < enemyRange && !unit.getTarget().getType().isBuilding())
                || unit.isLightAir())
                return true;
            return false;
        };

        if (unit.getLocalState() != LocalState::Engaging || !unit.hasTarget() || !shouldApproach() || !canApproach())
            return false;

        if (unit.getTarget().getPosition().isValid()) {
            unit.command(UnitCommandTypes::Move, unit.getTarget().getPosition());
            return true;
        }
        return false;
    }

    bool move(UnitInfo& unit)
    {
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            Position p = Position(w) + Position(4, 4);
            double distance = (unit.getType().isFlyer() || Terrain::isIslandMap()) ? p.getDistance(unit.getDestination()) : BWEB::Map::getGroundDistance(p, unit.getDestination());
            double threat = Util::getHighestThreat(w, unit);
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = 1.0 / (threat * distance * grouping);
            return score;
        };

        if (unit.getRole() == Role::Fighting) {
            if (unit.hasTarget() && unit.getTarget().unit()->exists() && Util::unitInRange(unit))
                return false;

            if (unit.getLocalState() == LocalState::Retreating)
                return false;

            if (unit.getDestination().isValid()) {
                if (!Terrain::isInEnemyTerritory((TilePosition)unit.getDestination())) {
                    Position bestPosition = Util::getConcavePosition(unit, mapBWEM.GetArea(TilePosition(unit.getDestination())));
                    if (bestPosition.isValid() && (bestPosition != unit.getPosition() || unit.unit()->getLastCommand().getType() == UnitCommandTypes::None)) {
                        if (unit.unit()->getLastCommand().getTargetPosition() != Position(bestPosition) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
                            unit.command(UnitCommandTypes::Move, Position(bestPosition));
                    }
                }
                else if (unit.unit()->getLastCommand().getTargetPosition() != Position(unit.getDestination()) || unit.unit()->getLastCommand().getType() != UnitCommandTypes::Move)
                    unit.command(UnitCommandTypes::Move, unit.getDestination());

                return true;
            }

            // If unit has a transport, move to it or load into it
            else if (unit.hasTransport() && unit.getTransport().unit()->exists()) {
                unit.command(UnitCommandTypes::Right_Click_Unit, &unit.getTransport());
                return true;
            }

            // If target doesn't exist, move towards it
            else if (unit.hasTarget() && unit.getTarget().getPosition().isValid() && Grids::getMobility(WalkPosition(unit.getEngagePosition())) > 0 && (unit.getPosition().getDistance(unit.getTarget().getPosition()) < SIM_RADIUS || unit.getType().isFlyer())) {
                unit.setDestination(unit.getTarget().getPosition());
            }

            else if (Terrain::getAttackPosition().isValid()) {
                unit.setDestination(Terrain::getAttackPosition());
            }

            // If no target and no enemy bases, move to a base location (random if we have found the enemy once already)
            else if (unit.unit()->isIdle()) {
                if (Terrain::getEnemyStartingPosition().isValid()) {
                    unit.setDestination(Terrain::randomBasePosition());
                }
                else {
                    for (auto &start : Broodwar->getStartLocations()) {
                        if (start.isValid() && !Broodwar->isExplored(start) && !overlapsCommands(unit.unit(), unit.getType(), Position(start), 32)) {
                            unit.setDestination(Position(start));
                        }
                    }
                }
            }
        }

        if (unit.getDestination().isValid()) {

            auto bestPosition = findViablePosition(unit, scoreFunction);
            if (bestPosition.isValid()) {

                // Draw a path from this unit and from best position to determine if this position is closer or not
                BWEB::PathFinding::Path unitPath;
                BWEB::PathFinding::Path bPath;

                unitPath.createUnitPath(unit.getPosition(), unit.getDestination());
                bPath.createUnitPath(bestPosition, unit.getDestination());

                Visuals::displayPath(unitPath.getTiles());

                if (bPath.getDistance() < unitPath.getDistance()) {
                    unit.command(UnitCommandTypes::Move, bestPosition);
                    return true;
                }
            }

            unit.command(UnitCommandTypes::Move, unit.getDestination());
            return true;
        }

        return false;
    }

    bool kite(UnitInfo& unit)
    {
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {
            Position p = Position(w) + Position(4, 4);
            double distance = unit.hasTarget() ? 1.0 / (p.getDistance(unit.getTarget().getPosition())) : p.getDistance(BWEB::Map::getMainPosition());
            double threat = Util::getHighestThreat(w, unit);
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = 1.0 / (threat * distance * grouping);
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

            // Special Case: Carriers
            if (unit.getType() == UnitTypes::Protoss_Carrier) {
                auto leashRange = 320;
                for (auto &interceptor : unit.unit()->getInterceptors()) {
                    if (interceptor->getOrder() != Orders::InterceptorAttack && interceptor->isCompleted())
                        return false;
                }
                if (unit.getPosition().getApproxDistance(unit.getTarget().getPosition()) >= leashRange)
                    return false;
                return true;
            }

            if (unit.getTarget().getType().isBuilding() && !unit.getType().isFlyer())
                return false;

            if (unit.getType() == UnitTypes::Protoss_Reaver
                //|| (Strategy::enemyPressure() && allyRange >= 64.0) // HACK: Added this for 3fact aggresion, we should check to see if a mine is near our target instead
                || (unit.getType() == UnitTypes::Terran_Vulture)
                || (unit.getType() == UnitTypes::Zerg_Mutalisk)
                || (unit.getType() == UnitTypes::Protoss_Carrier)
                || (allyRange >= 32.0 && unit.unit()->isUnderAttack() && allyRange >= enemyRange)
                || (unit.getTarget().getType() == UnitTypes::Terran_Vulture_Spider_Mine)
                || ((enemyRange <= allyRange && unit.unit()->getDistance(unit.getTarget().getPosition()) <= allyRange - enemyRange)))
                return true;
            return false;
        };

        if (unit.getLocalState() != LocalState::Engaging || !unit.hasTarget() || !shouldKite() || !canKite())
            return false;

        // If we found a valid position, move to it
        auto bestPosition = findViablePosition(unit, scoreFunction);
        if (bestPosition.isValid()) {
            unit.command(UnitCommandTypes::Move, bestPosition);
            return true;
        }
        return false;
    }

    bool defend(UnitInfo& unit)
    {
        bool defendingExpansion = unit.getDestination().isValid() && !Terrain::isInEnemyTerritory((TilePosition)unit.getDestination());
        bool closeToDefend = Terrain::getDefendPosition().getDistance(unit.getPosition()) < 640.0 || Terrain::isInAllyTerritory(unit.getTilePosition());

        if (!closeToDefend || unit.getLocalState() != LocalState::Retreating)
            return false;

        // Probe Cannon surround
        if (unit.getType().isWorker() && Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Photon_Cannon) > 0) {
            auto cannon = Util::getClosestUnit(mapBWEM.Center(), Broodwar->self(), UnitTypes::Protoss_Photon_Cannon);
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

        else {
            defendingUnitsByDist[Grids::getDistanceHome(unit.getWalkPosition())] = &unit;
            return true;
        }
        return false;
    }

    bool hunt(UnitInfo& unit)
    {
        function <double(WalkPosition)> scoreFunction = [&](WalkPosition w) -> double {

            Position p = Position(w) + Position(4, 4);
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
            Position p = Position(w) + Position(4, 4);
            double distance = ((unit.getType().isFlyer() || Terrain::isIslandMap()) ? p.getDistance(BWEB::Map::getMainPosition()) : Grids::getDistanceHome(w));
            double threat = Util::getHighestThreat(w, unit);
            double grouping = unit.getType().isFlyer() ? max(0.1f, Grids::getAAirCluster(w)) : 1.0;
            double score = 1.0 / (threat * distance * grouping);
            return score;
        };

        // Retreating is only valid when local state is retreating
        auto shouldRetreat = unit.getLocalState() == LocalState::Retreating || unit.getRole() == Role::Scouting;
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
            Position p = Position(w) + Position(4, 4);
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
        auto radius = 8 + int(unit.getSpeed()) + (8 * (int)unit.getType().isFlyer());
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
        auto topOk = Grids::getCollision(topWalkPosition) == 0 && Broodwar->isWalkable(topWalkPosition);

        auto botWalkPosition = center + WalkPosition(0, walkHeight * 2);
        auto botOk = Grids::getCollision(botWalkPosition) == 0 && Broodwar->isWalkable(botWalkPosition);

        auto leftWalkPosition = center - WalkPosition(walkWidth * 2, 0);
        auto leftOk = Grids::getCollision(leftWalkPosition) == 0 && Broodwar->isWalkable(leftWalkPosition);

        auto rightWalkPosition = center + WalkPosition(walkWidth * 2, 0);
        auto rightOk = Grids::getCollision(rightWalkPosition) == 0 && Broodwar->isWalkable(rightWalkPosition);


        auto topLeftWalkPosition = center - WalkPosition(walkWidth * 2, walkHeight * 2);
        auto topLeftOk = Grids::getCollision(topLeftWalkPosition) == 0 && Broodwar->isWalkable(topLeftWalkPosition);

        auto topRightWalkPosition = center + WalkPosition(walkWidth * 2, -walkHeight * 2);
        auto topRightOk = Grids::getCollision(topRightWalkPosition) == 0 && Broodwar->isWalkable(topRightWalkPosition);

        auto botLeftWalkPosition = center - WalkPosition(walkWidth * 2, -walkHeight * 2);
        auto botLeftOk = Grids::getCollision(botLeftWalkPosition) == 0 && Broodwar->isWalkable(botLeftWalkPosition);

        auto botRightWalkPosition = center + WalkPosition(walkWidth * 2, walkHeight * 2);
        auto botRightOk = Grids::getCollision(botRightWalkPosition) == 0 && Broodwar->isWalkable(botRightWalkPosition);


        Broodwar->drawCircleMap(Position(topWalkPosition), 4, topOk ? Colors::Green : Colors::Red);
        Broodwar->drawCircleMap(Position(botWalkPosition), 4, botOk ? Colors::Green : Colors::Red);
        Broodwar->drawCircleMap(Position(leftWalkPosition), 4, leftOk ? Colors::Green : Colors::Red);
        Broodwar->drawCircleMap(Position(rightWalkPosition), 4, rightOk ? Colors::Green : Colors::Red);

        Broodwar->drawCircleMap(Position(topLeftWalkPosition), 4, topLeftOk ? Colors::Green : Colors::Red);
        Broodwar->drawCircleMap(Position(topRightWalkPosition), 4, topRightOk ? Colors::Green : Colors::Red);
        Broodwar->drawCircleMap(Position(botLeftWalkPosition), 4, botLeftOk ? Colors::Green : Colors::Red);
        Broodwar->drawCircleMap(Position(botRightWalkPosition), 4, botRightOk ? Colors::Green : Colors::Red);

        const auto directionsOkay = [&](WalkPosition here) {
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

                if (directionsOkay(w))
                    Broodwar->drawBoxMap(p, p + Position(8, 8), Colors::Black);

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
        updateEnemyCommands();
        updateUnits();
        Visuals::endPerfTest("Commands");
    }

    vector <CommandType>& getMyCommands() { return myCommands; }
    vector <CommandType>& getEnemyCommands() { return enemyCommands; }
}
