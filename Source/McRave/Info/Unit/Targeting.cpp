#include "Targeting.h"

#include "Builds/All/BuildOrder.h"
#include "Info/Player/Players.h"
#include "Info/Unit/Units.h"
#include "Main/Common.h"
#include "Map/Grids.h"
#include "Map/Stations.h"
#include "Map/Terrain.h"
#include "Strategy/Actions/Actions.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave::Targets {

    struct Context {
        shared_ptr<UnitInfo> unit, target;
        double dist;
        double enemyRange, enemyReach, selfRange, selfReach;
        double wH, wF, wP;
    };

    set<UnitType> cancelPriority = {Terran_Missile_Turret, Terran_Barracks, Terran_Bunker, Terran_Factory, Terran_Starport, Terran_Armory, Terran_Bunker};
    set<UnitType> proxyTargeting = {Protoss_Pylon, Terran_Barracks, Terran_Bunker, Zerg_Sunken_Colony};
    map<UnitInfo *, int> meleeSpotsAvailable;
    multimap<double, std::weak_ptr<UnitInfo>> sortedUnits;

    double maxPriority;
    int earliest;

    enum class Priority { None, Ignore, Trivial, Minor, Normal, Major, Critical };

    bool enemyHasGround;
    bool enemyHasAir;
    bool enemyCanHitAir;
    bool enemyCanHitGround;

    bool selfHasGround;
    bool selfHasAir;
    bool selfCanHitAir;
    bool selfCanHitGround;

    int combatTargeters = 0; // Not ideal, this is global but applies to individual targets

    bool allowWorkerTarget(UnitInfo &unit, UnitInfo &target)
    {
        if (unit.getType().isWorker()) {
            return Spy::getEnemyTransition() == U_WorkerRush || target.hasAttackedRecently() || target.hasRepairedRecently() || target.isThreatening() || combatTargeters == 0;
        }

        // Rushes gotta not chase workers in our base
        if (BuildOrder::isRush() && !Terrain::inTerritory(PlayerState::Enemy, target.getPosition()) && Spy::getEnemyTransition() != U_WorkerRush)
            return false;

        if (Util::getTime() > Time(8, 00)) {
            return unit.isLightAir() || combatTargeters <= 4;
        }

        auto allowed = unit.getType().isWorker() || unit.isWithinRange(target) || unit.attemptingRunby() || unit.isLightAir() || target.isThreatening() || BuildOrder::isHideTech() ||
                       (combatTargeters == 0 && !Players::ZvZ()) || Spy::getEnemyTransition() == U_WorkerRush || Terrain::inTerritory(PlayerState::Enemy, target.getPosition());

        if (Util::getTime() < Time(5, 00)) {
            return allowed || target.hasAttackedRecently() || target.hasRepairedRecently();
        }
        return allowed;
    }

    // P
    Priority dtPriority(UnitInfo &unit, UnitInfo &target)
    {
        if (target.getSpeed() > unit.getSpeed() && !unit.isWithinRange(target) && !target.isThreatening())
            return Priority::Ignore;

        // Add bonus for a DT to kill important counters
        if (unit.isWithinEngage(target) && !Actions::overlapsDetection(target.unit(), target.getPosition(), PlayerState::Enemy) &&
            (target.getType().isWorker() || target.isSiegeTank() || target.getType().isDetector() || target.getType() == Protoss_Observatory || target.getType() == Protoss_Robotics_Facility))
            return Priority::Critical;

        return Priority::Normal;
    }

    Priority htPriority(UnitInfo &unit, UnitInfo &target) { return Priority::Normal; }

    // Z
    Priority zerglingPriority(UnitInfo &unit, UnitInfo &target)
    {
        // Avoid targeting
        if (Util::getTime() < Time(5, 00)) {
            if (Players::ZvZ() && !target.canAttackGround() && !Spy::enemyFastExpand())
                return Priority::Ignore;
            if (Players::ZvT() && target.getType() == Terran_Vulture && combatTargeters >= 2)
                return Priority::Ignore;
        }

        // Handle workers and cannons if we're in the same area in enemy territory
        if (unit.isWithinReach(target) && Terrain::inTerritory(PlayerState::Enemy, unit.getPosition()) && Terrain::inArea(mapBWEM.GetArea(target.getWalkPosition()), unit.getPosition())) {
            if (target.getType().isWorker())
                return Priority::Major;
            if (target.getType() == Protoss_Photon_Cannon && target.hasAttackedRecently())
                return Priority::Critical;
        }

        // Already in range, continue to target it if possible
        if (unit.isWithinRange(target) && !target.getType().isBuilding() && !target.getType().isWorker()) {
            if (Players::ZvZ())
                return Priority::Critical;
            else
                return Priority::Major;
        }
        return Priority::Normal;
    }

    Priority queenPriority(UnitInfo &unit, UnitInfo &target)
    {
        for (auto &t : target.getUnitsTargetingThis()) {
            if (auto &targeter = t.lock()) {
                if (targeter->getType() == Zerg_Queen)
                    return Priority::Ignore;
            }
        }

        if (target.isSiegeTank() && unit.isWithinRange(target))
            return Priority::Critical;
        if (target.getType() == Terran_Goliath)
            return Priority::Major;
        return Priority::Ignore;
    }

    Priority mutaliskPriority(UnitInfo &unit, UnitInfo &target)
    {
        auto anythingTime     = Util::getTime() > (Players::ZvZ() ? Time(7, 00) : Time(10, 00));
        auto anythingSupply   = !Players::ZvZ() && Players::getSupply(PlayerState::Enemy, Races::None) < 20;
        auto defendExpander   = BuildOrder::shouldExpand() && unit.getGoal().isValid();
        auto somethingInRange = Grids::getAirThreat(unit.getPosition(), PlayerState::Enemy) > 0.0f || Grids::getAirThreat(target.getPosition(), PlayerState::Enemy) > 0.0f;

        if (Players::ZvP()) {

            // Clean Zealots up vs rushes
            if (Util::getTime() < Time(9, 00) && target.getType() == Protoss_Zealot && Spy::getEnemyTransition() == P_Rush)
                return Priority::Major;
        }

        if (Players::ZvT()) {

            // Ignore vultures so we don't chase them too much
            if (target.getType() == Terran_Vulture && !unit.isWithinRange(target) && !unit.getGoal().isValid() && !target.isThreatening())
                return Priority::Ignore;
            if (target.getType() == Terran_Vulture_Spider_Mine && Grids::getAirThreat(target.getTilePosition(), PlayerState::Enemy) > 0)
                return Priority::Ignore;
            if (target.isSiegeTank()) {
                if (target.isThreatening() && unit.isWithinEngage(target))
                    return Priority::Critical;
            }

            // Handle turrets and repairs
            if (target.getType() == Terran_Missile_Turret) {
                if (target.hasRepairedRecently() && !Combat::Clusters::canDecimate(unit, target))
                    return Priority::Trivial;
            }
        }

        if (Players::ZvZ()) {

            // Kill scourge immediately
            if (target.getType() == Zerg_Scourge && unit.isWithinRange(target))
                return Priority::Critical;
        }

        // If our build is hitting a timing, kill workers
        if (BuildOrder::isPressure(Zerg_Mutalisk) && !target.getType().isWorker() && !target.isThreatening() && Util::getTime() < Time(8, 00))
            return Priority::Ignore;

        // One/two shot is high priority to hit
        if (!Players::ZvZ() && unit.isWithinReach(target) && (Combat::Clusters::canDecimate(unit, target) || target.isSplasher()))
            return Priority::Critical;

        // If a building is unprotected
        if (!BuildOrder::isPressure(Zerg_Mutalisk) && Players::getStrength(PlayerState::Enemy).airDefense > 0.0 && target.getType().isBuilding() && !target.canAttackAir() &&
            target.getUnitsInRangeOfThis().empty() && unit.getUnitsInRangeOfThis().empty() && unit.isWithinRange(target)) {
            Priority::Major;
        }

        if (unit.isWithinRange(target) && !target.getType().isBuilding() && target.canAttackAir())
            return Priority::Major;

        if ((target.getType().isBuilding() || somethingInRange) && !target.canAttackAir())
            return Priority::Minor;

        return Priority::Normal;
    }

    Priority scourgePriority(UnitInfo &unit, UnitInfo &target)
    {
        if ((!Stations::getStations(PlayerState::Enemy).empty() && target.getType().isBuilding()) // Avoid targeting buildings if the enemy has a base still
            || target.getType() == Zerg_Overlord                                                  // Don't target overlords
            || target.getType() == Protoss_Interceptor)                                           // Don't target interceptors
            return Priority::Ignore;

        // Determine if we have enough scourge on this target already
        int cnt = 0;
        for (auto &t : target.getUnitsTargetingThis()) {
            if (auto targeter = t.lock()) {
                if (targeter->getType() == Zerg_Scourge)
                    cnt += 110;
            }
        }
        if (cnt >= unit.getHealth())
            return Priority::Ignore;
        return Priority::Normal;
    }

    Priority defilerPriority(UnitInfo &unit, UnitInfo &target)
    {
        if (unit.getType() == Zerg_Defiler) {
            if (target.getType().isBuilding() || target.getType().isWorker() || target.isToken() || target.isFlying() || (unit.targetsFriendly() && target.getType() != Zerg_Zergling) ||
                (unit.targetsFriendly() && target.getRole() != Role::Combat))
                return Priority::Ignore;

            if (Actions::overlapsActions(unit.unit(), target.getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, Util::getCastRadius(TechTypes::Dark_Swarm)))
                return Priority::Ignore;

            if (!unit.targetsFriendly() && target.isSiegeTank() && unit.isWithinReach(target))
                return Priority::Major;
        }
        return Priority::Normal;
    }

    Priority getPriority(UnitInfo &unit, UnitInfo &target)
    {
        combatTargeters = 0;
        for (auto &t : target.getUnitsTargetingThis()) {
            if (auto &targeter = t.lock()) {
                if (targeter->getRole() == Role::Combat)
                    combatTargeters++;
            }
        }

        if (unit.getType() == Zerg_Zergling && Util::getTime() < Time(5, 00)) {
            if (Players::ZvT() && target.getType() == Terran_Vulture && combatTargeters >= 2 && !unit.isWithinRange(target))
                return Priority::Minor;
        }

        bool targetCanAttack = !unit.isHidden() && (((unit.getType().isFlyer() && target.canAttackAir()) || (!unit.getType().isFlyer() && target.canAttackGround()) ||
                                                     (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine)));
        bool unitCanAttack   = !target.isHidden() && ((target.isFlying() && unit.canAttackAir()) || (!target.isFlying() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));

        if (unit.getRole() != Role::Support && (target.movedFlag || !unitCanAttack))
            return Priority::Ignore;

        // Check if the target is important right now to attack
        auto anyTime = Time(6, 00);
        if (BuildOrder::isRush())
            anyTime = Time(7, 00);

        bool targetMatters = (target.canAttackAir() && selfHasAir) || (target.canAttackGround() && selfHasGround) || (target.getType().spaceProvided() > 0) || (target.getType().isDetector()) ||
                             (!target.canAttackAir() && !target.canAttackGround() && !unit.hasTransport()) || (!enemyHasGround && !enemyHasAir) ||
                             (target.getType() == Terran_Bunker && !target.isCompleted()) || (Players::ZvZ() && Spy::enemyFastExpand()) || Util::getTime() > anyTime || Spy::enemyGreedy();

        // Support Role
        if (unit.getRole() == Role::Support) {
            if (target.isHidden() && (unit.getType().isDetector() || unit.getType() == Terran_Comsat_Station))
                return Priority::Critical;
            if (unit.getType() == Terran_Comsat_Station)
                return Priority::Ignore;
        }

        // Combat Role
        if (unit.getRole() == Role::Combat) {

            // Ignore if we need to runby
            if (unit.attemptingRunby() && Players::getDeadCount(PlayerState::Enemy, Broodwar->enemy()->getRace().getWorker()) < 8) {
                if (Players::ZvZ()) {
                    if (!target.getType().isWorker())
                        return Priority::Ignore;
                    return Priority::Critical;
                }
                if (target.getType().isBuilding() && !target.canAttackGround() && target.getUnitsInRangeOfThis().empty() && unit.getHealth() < unit.getType().maxHitPoints())
                    return Priority::Critical;
                if (target.getType() != Terran_Marine && !target.hasAttackedRecently() && (!target.getType().isWorker() || !Terrain::inTerritory(PlayerState::Enemy, target.getPosition())))
                    return Priority::Ignore;
                if (target.getPosition().isValid() && Grids::getGroundThreat(target.getPosition(), PlayerState::Enemy) <= 0.1f && Util::getTime() < Time(7, 30))
                    return Priority::Major;
                if (!Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), target.getPosition()) && Util::getTime() < Time(7, 30))
                    return Priority::Ignore;
            }

            if (target.getType().isWorker() && !allowWorkerTarget(unit, target))
                return Priority::Ignore;

            // Vulnerable important units
            if (target.getType() == Protoss_Reaver && unit.isWithinRange(target))
                return Priority::Critical;

            // Generic trivial
            if (!targetMatters || (target.getType() == Zerg_Egg) || (target.getType() == Zerg_Larva))
                return Priority::Trivial;

            // Proxy worker
            if (target.isProxy() && target.getType().isWorker() && target.unit()->exists() && !BuildOrder::isRush()) {
                if (target.unit()->isConstructing())
                    return Priority::Critical;
                else if (unit.isWithinReach(target) && !Spy::enemyRush() && Spy::getEnemyBuild() != P_2Gate)
                    return Priority::Major;
            }

            // Proxy building
            if (target.isProxy() && target.getType().isBuilding() && Spy::enemyProxy() && unit.getType() != Zerg_Mutalisk) {
                Visuals::drawCircle(target.getPosition(), 5, Colors::Yellow);
                if ((target.unit()->exists() && target.unit()->getBuildUnit()) || (target.getType().getRace() == Races::Terran && !target.isCompleted()))
                    return Priority::Minor;
                else if (target.getType() == Protoss_Photon_Cannon && target.frameCompletesWhen() <= earliest)
                    return Priority::Critical;
                else if (target.getType() == Protoss_Pylon && Spy::getEnemyOpener() == P_Horror_9_9)
                    return Priority::Critical;
                else if (proxyTargeting.find(target.getType()) != proxyTargeting.end() && Players::getCompleteCount(PlayerState::Enemy, Protoss_Photon_Cannon) == 0 &&
                         Players::getCompleteCount(PlayerState::Enemy, Terran_Marine) == 0 && Players::getCompleteCount(PlayerState::Enemy, Protoss_Zealot) == 0)
                    return Priority::Major;
                else if (target.canAttackGround())
                    return Priority::Major;
                else
                    return Priority::Ignore;
            }

            // Threatening priority
            if (target.isThreatening() && (unit.isWithinReach(target) || Util::getTime() < Time(5, 00) || Terrain::inTerritoryPath(PlayerState::Self, unit.getPosition(), target.getPosition()))) {
                if (!unit.getType().isWorker() && !target.getType().isWorker() && unit.isFlying() == target.isFlying())
                    return Priority::Major;
                if (target.getType().isWorker() && target.isThreatening())
                    return Priority::Critical;
            }

            // Sacrifice unit away from army
            if (unit.isTargetedBySuicide() && (target.isTransport() || target.getType() == Protoss_Reaver || target.getType() == Protoss_High_Templar))
                return Priority::Critical;

            // Generic ignore
            if (target.getType().isSpell()                                                                                                     //
                || (target.getType() == Terran_Vulture_Spider_Mine && int(target.getUnitsTargetingThis().size()) >= 4 && !target.isBurrowed()) // Don't over target spider mines
                || (target.getType() == Protoss_Interceptor && unit.isFlying())                                                                // Don't target interceptors as a flying unit
                || (target.getType() == Protoss_Corsair && !unit.isFlying() && !target.isThreatening() && !target.hasAttackedRecently() && !unit.isWithinRange(target)) //
                || (target.isHidden() && (!targetCanAttack || (!Players::hasDetection(PlayerState::Self) && Players::PvP())) &&
                    !unit.getType().isDetector()) // Don't target if invisible and can't attack this unit or we have no detectors in PvP
                ||
                (target.isFlying() && !unit.isFlying() && !BWEB::Map::isWalkable(target.getTilePosition(), unit.getType()) && !unit.isWithinRange(target))) // Don't target flyers that we can't reach
                return Priority::Ignore;

            // Handle photon cannons when in range
            if (!unit.isMelee() && unit.isWithinRange(target)) {
                if (target.getType() == Protoss_Photon_Cannon && target.hasAttackedRecently())
                    return Priority::Critical;
                if (target.getType() == Protoss_Photon_Cannon)
                    return Priority::Major;
            }

            // Handle detector sniping
            if (target.getType().isDetector() || target.getType() == Terran_Comsat_Station) {
                if (unit.isWithinRange(target) && !target.isHidden() && (vis(Zerg_Lurker) > 0 || vis(Protoss_Dark_Templar) > 0))
                    return Priority::Major;
            }

            // Zealot
            if (unit.getType() == Protoss_Zealot) {
                if ((target.getType() == Terran_Vulture_Spider_Mine && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Protoss_Ground_Weapons) < 2)      // Avoid mines without +2 to oneshot them
                    || ((target.getSpeed() > unit.getSpeed() || target.getType().isBuilding()) && !target.getType().isWorker() && BuildOrder::isRush())) // Avoid faster units when we're rushing
                    return Priority::Ignore;
            }

            // Ghost
            if (unit.getType() == Terran_Ghost) {
                if (!target.getType().isResourceDepot())
                    return Priority::Ignore;
            }

            // Medic
            if (unit.getType() == Terran_Medic) {
                if (target.getType() == Terran_Marine || target.getType() == Terran_Firebat || (target.getType() == Terran_Medic && target.getHealth() < target.getType().maxHitPoints()))
                    return Priority::Critical;
                else if (target.getType() == Terran_SCV)
                    return Priority::Major;
                else if (target.getType().isOrganic())
                    return Priority::Minor;
                return Priority::Ignore;
            }
        }

        // P
        if (unit.getType() == Protoss_Dark_Templar)
            return dtPriority(unit, target);
        if (unit.getType() == Protoss_High_Templar)
            return htPriority(unit, target);

        // Z
        if (unit.getType() == Zerg_Zergling)
            return zerglingPriority(unit, target);
        if (unit.getType() == Zerg_Mutalisk)
            return mutaliskPriority(unit, target);
        if (unit.getType() == Zerg_Scourge)
            return scourgePriority(unit, target);
        if (unit.getType() == Zerg_Defiler)
            return defilerPriority(unit, target);
        if (unit.getType() == Zerg_Queen)
            return queenPriority(unit, target);

        return Priority::Normal;
    }

    double healthScore(UnitInfo &unit, UnitInfo &target) //
    {
        return pow(2.0, (1.0 - target.getPercentTotal()));
    }

    double focusScore(UnitInfo &unit, UnitInfo &target)
    {
        auto range             = target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
        auto reach             = target.getType().isFlyer() ? unit.getAirReach() : unit.getGroundReach();
        auto enemyRange        = unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange();
        auto enemyReach        = unit.getType().isFlyer() ? target.getAirReach() : target.getGroundReach();
        const auto boxDistance = double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));

        if (unit.isLightAir() || Players::ZvZ())
            return 1.0;
        else if (unit.getType().isWorker() && target.getType().isWorker() && Spy::getEnemyTransition() == U_WorkerRush && int(target.getUnitsTargetingThis().size()) < 6) {
            return (1.0 + (1.0 * double(target.getUnitsTargetingThis().size())));
        }

        if (unit.isMelee()) {
            const auto targetSize     = max(target.getType().width(), target.getType().height());
            const auto targetingCount = count_if(target.getUnitsTargetingThis().begin(), target.getUnitsTargetingThis().end(), [&](auto &u) { return u.lock()->isMelee(); });
            if (!target.getType().isBuilding() && targetingCount >= targetSize / 4)
                return 0.05;
        }

        const auto withinReachHigherRange = range > 32.0 && range >= enemyRange && boxDistance <= reach;
        const auto withinReachMelee       = range <= 32.0 && boxDistance <= reach;

        const auto withinRangeLessRange = range > 32.0 && range < enemyRange && boxDistance <= range;
        const auto withinRangeMelee     = range <= 32.0 && boxDistance <= 64.0;

        if (withinReachHigherRange || withinRangeLessRange || withinRangeMelee)
            return (1.0 + (0.5 * double(target.getUnitsTargetingThis().size())));
        return 1.0;
    }

    double priorityScore(UnitInfo &unit, UnitInfo &target) //
    {
        return target.getPriority() / maxPriority;
    }

    double distScore(UnitInfo &unit, UnitInfo &target)
    {
        auto range      = target.getType().isFlyer() ? unit.getAirRange() : unit.getGroundRange();
        auto reach      = target.getType().isFlyer() ? unit.getAirReach() : unit.getGroundReach();
        auto enemyRange = unit.getType().isFlyer() ? target.getAirRange() : target.getGroundRange();
        auto enemyReach = unit.getType().isFlyer() ? target.getAirReach() : target.getGroundReach();

        if (unit.isSiegeTank()) {
            range = 384.0;
            reach = range + double(unit.getType().width() / 2) + 64.0;
        }

        const auto boxDistance = double(Util::boxDistance(unit.getType(), unit.getPosition(), target.getType(), target.getPosition()));
        const auto useGrd      = !unit.getType().isWorker() && !unit.isFlying() && !target.isFlying() && mapBWEM.GetArea(unit.getTilePosition()) && mapBWEM.GetArea(target.getTilePosition()) &&
                            mapBWEM.GetArea(unit.getTilePosition())->AccessibleFrom(mapBWEM.GetArea(target.getTilePosition())) && boxDistance < unit.getEngageRadius() && boxDistance > reach;
        const auto actualDist = (useGrd ? BWEB::Map::getGroundDistance(unit.getPosition(), target.getPosition()) : boxDistance);

        auto minuteScalar = Players::ZvZ() ? 8.0 : 4.0;

        // Make distance targeting scaling with minutes in the game
        const auto earlyScale = max(1.0, 4.0 - Util::getTime().minutes * 0.3);
        const auto dist       = exp(actualDist / (48.0 * earlyScale));
        return dist;
    }

    double scoreTarget(UnitInfo &unit, UnitInfo &target)
    {
        auto health            = healthScore(unit, target);
        auto focus             = focusScore(unit, target);
        auto priority          = priorityScore(unit, target);
        auto dist              = distScore(unit, target);
        const auto targetScore = health * focus * priority / dist;

        // Detector targeting (distance to nearest ally added)
        if ((target.isBurrowed() || target.isCloaked()) && ((unit.getType().isDetector() && !unit.getType().isBuilding()) || unit.getType() == Terran_Comsat_Station)) {
            if (auto closestAlly = Util::getClosestUnit(target.getPosition(), PlayerState::Self, [&](auto &u) {
                    return *u != unit && u->getRole() == Role::Combat && target.getType().isFlyer() ? u->getAirRange() > 0 : u->getGroundRange() > 0;
                })) {

                // Detectors want to stay close to their target if we have a friendly nearby
                auto closestDist = closestAlly->getPosition().getDistance(target.getPosition());
                if (closestDist < 480.0)
                    return priorityScore(unit, target) / (dist * closestDist);
            }
            return 0.0;
        }

        // Cluster targeting (grid score added)
        else if (unit.getType() == Protoss_High_Templar || unit.getType() == Protoss_Arbiter) {
            const auto eGrid     = Grids::getGroundDensity(target.getPosition(), PlayerState::Enemy) + Grids::getAirDensity(target.getPosition(), PlayerState::Enemy);
            const auto aGrid     = 1.0 + Grids::getGroundDensity(target.getPosition(), PlayerState::Self) + Grids::getAirDensity(target.getPosition(), PlayerState::Self);
            const auto gridScore = eGrid / aGrid;
            return gridScore * targetScore;
        }

        // Defender targeting (distance not used)
        else if (unit.getRole() == Role::Defender)
            return healthScore(unit, target) * priorityScore(unit, target);

        // Proximity targeting (targetScore not used)
        else if (unit.getType().isWorker() || unit.getType() == Protoss_Reaver || unit.getType() == Zerg_Queen || unit.isLightAir() || unit.getType() == Zerg_Guardian ||
                 (unit.getType() == Zerg_Lurker && unit.isBurrowed())) {
            if (target.getType().isBuilding() && !target.canAttackGround() && !target.canAttackAir())
                return 0.1 / dist;
            return 1.0 / dist;
        }
        return targetScore;
    }

    void getSimTarget(UnitInfo &unit, PlayerState pState)
    {
        double distBest = DBL_MAX;
        for (auto &t : Units::getUnits(pState)) {
            UnitInfo &target     = *t;
            auto targetCanAttack = ((unit.isFlying() && target.canAttackAir()) || (!unit.isFlying() && target.canAttackGround()) ||
                                    (!unit.getType().isFlyer() && target.getType() == Terran_Vulture_Spider_Mine));
            auto unitCanAttack   = ((target.isFlying() && unit.canAttackAir()) || (!target.isFlying() && unit.canAttackGround()) || (unit.getType() == Protoss_Carrier));

            if (!targetCanAttack && (!unit.hasTarget() || target != *unit.getTarget().lock()))
                continue;

            if (!target.isWithinEngage(unit))
                continue;

            if (!allowWorkerTarget(unit, target))
                continue;

            // Set sim position
            auto dist = unit.getPosition().getDistance(target.getPosition()) * (1 + int(!targetCanAttack));
            if (dist < distBest) {
                unit.setSimTarget(&target);
                distBest = dist;
            }
        }

        if (!unit.hasSimTarget() && unit.hasTarget())
            unit.setSimTarget(&*unit.getTarget().lock());
    }

    void getBestTarget(UnitInfo &unit, PlayerState pState)
    {
        const auto checkGroundAccess = [&](UnitInfo &target) {
            if (unit.isFlying() || unit.unit()->isLoaded())
                return true;

            if (target.isFlying() && BWEB::Map::isWalkable(target.getTilePosition(), Zerg_Ultralisk)) {
                auto grdDist = BWEB::Map::getGroundDistance(unit.getPosition(), target.getPosition());
                auto airDist = unit.getPosition().getDistance(target.getPosition());
                if (grdDist > airDist * 2)
                    return false;
            }
            return true;
        };

        const auto isValidTarget = [&](auto &target) {
            if (!target.unit() || (unit.targetsFriendly() && !target.isCompleted()) || target.isInvincible() || unit == target || !target.getPosition().isValid())
                return false;
            return true;
        };

        auto scoreBest    = 0.0;
        auto priorityBest = Priority::Ignore;

        int i = 0;
        for (auto &t : Units::getUnits(pState)) {
            UnitInfo &target = *t;

            if (!isValidTarget(target))
                continue;

            auto priority = getPriority(unit, target);
            if (priority == Priority::Ignore || priority < priorityBest)
                continue;

            if (priority > priorityBest) {
                scoreBest    = 0.0;
                priorityBest = priority;
            }

            // If this target is more important to target, set as current target
            const auto thisUnit = scoreTarget(unit, target);
            if (unit.getType() == Zerg_Hydralisk)
                Broodwar->drawTextMap(target.getPosition(), "%.2f", thisUnit);
            if (thisUnit > scoreBest && checkGroundAccess(target)) {
                scoreBest = thisUnit;
                unit.setTarget(&target);
                unit.setSimTarget(&target);
            }
        }

        //// Check for any self targets marked for death
        // for (auto &t : Units::getUnits(PlayerState::Self)) {
        //    UnitInfo &target = *t;

        //    if (!isValidTarget(target)
        //        || !target.isMarkedForDeath()
        //        || unit.isLightAir())
        //        continue;

        //    auto priority = getPriority(unit, target);
        //    if (priority == Priority::Ignore || priority < priorityBest)
        //        continue;

        //    if (priority > priorityBest) {
        //        scoreBest = 0.0;
        //        priorityBest = priority;
        //    }

        //    // If this target is more important to target, set as current target
        //    const auto thisUnit = scoreTarget(unit, target);
        //    if (thisUnit > scoreBest && checkGroundAccess(target)) {
        //        scoreBest = thisUnit;
        //        unit.setTarget(&target);
        //    }
        //}

        // Check for any neutral targets marked for death
        for (auto &t : Units::getUnits(PlayerState::Neutral)) {
            UnitInfo &target = *t;

            if (!isValidTarget(target) || !target.isMarkedForDeath() || unit.isLightAir())
                continue;

            auto priority = getPriority(unit, target);

            if (priority == Priority::Ignore || priority < priorityBest)
                continue;

            if (priority > priorityBest) {
                scoreBest    = 0.0;
                priorityBest = priority;
            }

            // If this target is more important to target, set as current target
            const auto thisUnit = scoreTarget(unit, target);
            if (thisUnit > scoreBest && checkGroundAccess(target)) {
                scoreBest = thisUnit;
                unit.setTarget(&target);
            }
        }

        // Add this unit to the targeted by vector
        if (unit.hasTarget() && (unit.getRole() == Role::Combat || unit.getRole() == Role::Support)) {
            auto target = unit.getTarget().lock();
            target->addTargeter(unit);
        }
    }

    void getTarget(UnitInfo &unit)
    {
        // Don't select targets for incomplete units
        if (!unit.isCompleted())
            return;

        auto pState = unit.targetsFriendly() ? PlayerState::Self : PlayerState::Enemy;

        // Spider mines have a set order target, possibly scarabs too
        if (unit.getType() == Terran_Vulture_Spider_Mine && unit.unit()->exists()) {
            if (unit.unit()->getOrderTarget())
                unit.setTarget(&*Units::getUnitInfo(unit.unit()->getOrderTarget()));
        }

        // Self units are assigned targets
        if (unit.getRole() == Role::Combat || unit.getRole() == Role::Support || unit.getRole() == Role::Defender || unit.getRole() == Role::Worker || unit.getRole() == Role::Scout) {
            getBestTarget(unit, pState);
            getSimTarget(unit, PlayerState::Enemy);
        }
    }

    void updateTargets()
    {
        // Sort my units by distance to closest enemy
        static auto lastUpdate = Time(0, 0);
        if (Util::getTime() - lastUpdate > Time(0, 1)) {
            sortedUnits.clear();
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit    = *u;
                auto closestEnemy = Util::getClosestUnit(unit.getPosition(), PlayerState::Enemy, [&](auto &u) {
                    return (u->isFlying() && unit.canAttackAir()) || (!u->isFlying() && unit.canAttackGround()) || (unit.getRole() == Role::Support);
                });
                if (closestEnemy) {
                    const auto dist = unit.getPosition().getDistance(closestEnemy->getPosition());
                    sortedUnits.emplace(dist, unit.weak_from_this());
                }
            }
        }

        for (auto &[_, u] : sortedUnits) {
            if (auto ptr = u.lock())
                getTarget(*ptr);
        }
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            UnitInfo &unit = *u;
            getTarget(unit);

            // Enemy units are assumed targets or order targets
            if (unit.unit()->exists() && unit.unit()->getOrderTarget()) {
                auto &targetInfo = Units::getUnitInfo(unit.unit()->getOrderTarget());
                if (targetInfo) {
                    unit.setTarget(&*targetInfo);
                    targetInfo->addTargeter(unit);
                }
            }

            // Assign a simtarget for now
            auto simTarget = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &s) { return unit.canAttack(*s) || s->canAttack(unit); });
            if (simTarget)
                unit.setSimTarget(simTarget);
        }
    }

    void updateStatistics()
    {
        auto &enemyStrength = Players::getStrength(PlayerState::Enemy);
        auto &myStrength    = Players::getStrength(PlayerState::Self);

        maxPriority = 1.0;
        earliest    = 999999;
        for (auto &u : Units::getUnits(PlayerState::Enemy)) {
            UnitInfo &unit = *u;
            if (unit.getPriority() > maxPriority)
                maxPriority = unit.getPriority();
            if (unit.getType() == Protoss_Photon_Cannon && unit.frameCompletesWhen() < earliest)
                earliest = unit.frameCompletesWhen();
        }

        enemyHasGround    = true;
        enemyHasAir       = enemyStrength.airToGround > 0.0 || enemyStrength.airToAir > 0.0 || enemyStrength.airDefense > 0.0 || Players::getTotalCount(PlayerState::Enemy, Zerg_Overlord) > 0;
        enemyCanHitAir    = enemyStrength.groundToAir > 0.0 || enemyStrength.airToAir > 0.0 || enemyStrength.airDefense > 0.0;
        enemyCanHitGround = enemyStrength.groundToGround > 0.0 || enemyStrength.airToGround > 0.0 || enemyStrength.groundDefense > 0.0;

        selfHasGround    = true;
        selfHasAir       = myStrength.airToGround > 0.0 || myStrength.airToAir > 0.0 || total(Zerg_Overlord) > 0;
        selfCanHitAir    = myStrength.groundToAir > 0.0 || myStrength.airToAir > 0.0 || myStrength.airDefense > 0.0;
        selfCanHitGround = myStrength.groundToGround > 0.0 || myStrength.airToGround > 0.0 || myStrength.groundDefense > 0.0;
    }

    void onFrame()
    {
        updateStatistics();
        updateTargets();
    }
} // namespace McRave::Targets