#include "Units.h"

#include "Info/Player/PlayerInfo.h"
#include "Info/Roles.h"
#include "Main/Common.h"
#include "Main/Events.h"
#include "Map/Grids.h"
#include "Map/Terrain.h"
#include "Micro/Scout/Scouts.h"
#include "Strategy/Actions/Actions.h"
#include "Strategy/Spy/Spy.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Units {

    namespace {

        struct DamageRatio {
            map<DamageType, double> ratio;
            int armor;
            int count;
        };
        map<PlayerState, DamageRatio> grdDamageRatios;
        map<PlayerState, DamageRatio> airDamageRatios;

        set<shared_ptr<UnitInfo>> enemyUnits;
        set<shared_ptr<UnitInfo>> myUnits;
        set<shared_ptr<UnitInfo>> neutralUnits;
        set<shared_ptr<UnitInfo>> allyUnits;
        set<shared_ptr<UnitInfo>> noneUnits;
        map<UnitSizeType, int> allyGrdSizes;
        map<UnitSizeType, int> enemyGrdSizes;
        map<UnitSizeType, int> allyAirSizes;
        map<UnitSizeType, int> enemyAirSizes;
        bool immThreat;
        Position enemyArmyCenter;

        vector<UnitInfo *> commandQueue;

        void updateEnemies()
        {
            enemyArmyCenter     = Position(0, 0);
            auto enemyArmyCount = 0;

            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;
                if (!player.isEnemy())
                    continue;

                // Iterate and update important information
                for (auto &u : player.getUnits()) {
                    UnitInfo &unit = *u;

                    // If this is a morphed unit that we haven't updated
                    if (unit.unit()->exists() && unit.getType() != unit.unit()->getType())
                        Events::onUnitMorph(unit.unit());

                    // Update
                    unit.update();

                    // Must see a 3x3 grid of Tiles to set a unit to invalid position
                    if (!unit.unit()->exists() && Broodwar->isVisible(TilePosition(unit.getPosition())) &&
                        (!unit.isBurrowed() || Actions::overlapsDetection(unit.unit(), unit.getPosition(), PlayerState::Self) ||
                         (unit.getPosition().isValid() && Grids::getGroundDensity(unit.getPosition(), PlayerState::Self) > 0)))
                        Events::onUnitDisappear(unit);

                    // If a unit is threatening our position
                    if (unit.isThreatening()) {
                        if (!unit.getType().isWorker())
                            immThreat = true;
                        unit.circle(Colors::Red);
                    }

                    // Add to army center
                    if (unit.getPosition().isValid() && !unit.getType().isBuilding() && !unit.getType().isWorker() && !unit.movedFlag) {
                        enemyArmyCenter += unit.getPosition();
                        enemyArmyCount++;
                    }

                    // Enemy units are assumed targets or order targets
                    if (unit.unit()->exists() && unit.unit()->getOrderTarget()) {
                        auto &targetInfo = getUnitInfo(unit.unit()->getOrderTarget());
                        if (targetInfo) {
                            unit.setTarget(&*targetInfo);
                            targetInfo->getUnitsTargetingThis().push_back(unit.weak_from_this());
                        }
                    }

                    // Backup target closest self
                    if (unit.getType() != Terran_Vulture_Spider_Mine && !unit.hasTarget()) {
                        auto closest = Util::getClosestUnit(unit.getPosition(), PlayerState::Self,
                                                            [&](auto &u) { return (u->isFlying() && unit.getAirDamage() > 0.0) || (!u->isFlying() && unit.getGroundDamage() > 0.0); });
                        if (closest)
                            unit.setTarget(&*closest);
                    }
                }
            }

            enemyArmyCenter /= enemyArmyCount;
        }

        void updateAllies() {}

        void updateSelf()
        {
            commandQueue.clear();
            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;
                if (!player.isSelf())
                    continue;

                // Iterate and update important information
                for (auto &u : player.getUnits()) {
                    UnitInfo &unit = *u;
                    unit.update();
                    auto validRole = unit.getRole() == Role::Combat || unit.getRole() == Role::Defender || unit.getRole() == Role::Scout || unit.getRole() == Role::Support ||
                                     unit.getRole() == Role::Transport || unit.getRole() == Role::Worker;
                    if (!validRole)
                        continue;

                    auto frames = 6;
                    if (unit.getType() == Zerg_Overlord)
                        frames = 12;
                    if (unit.isLightAir())
                        frames = 2;

                    unit.nextCommandFrame = unit.lastCommandFrame + frames;
                    commandQueue.push_back(&unit);
                }
            }

            // Sort by stalest and truncate anything past 64, but reserve 3 slots for production/upgrades/research for now and 1 for padding
            sort(commandQueue.begin(), commandQueue.end(), [](UnitInfo *a, UnitInfo *b) { return a->nextCommandFrame < b->nextCommandFrame; });
            int maxCommandsPerFrame = 60;
            int maxCommandsPerTurn  = maxCommandsPerFrame / Broodwar->getLatencyFrames();
            if (int(commandQueue.size()) > maxCommandsPerTurn) {
                commandQueue.resize(maxCommandsPerTurn);
            }
        }

        void updateNeutrals()
        {
            // Neutrals
            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;
                if (!player.isNeutral())
                    continue;

                for (auto &u : player.getUnits()) {
                    UnitInfo &unit = *u;
                    neutralUnits.insert(u);
                    noneUnits.insert(u);

                    unit.update();

                    if (!unit.unit() || !unit.unit()->exists())
                        continue;
                }
            }
        }

        void updateCounters()
        {
            immThreat = 0.0;

            enemyUnits.clear();
            myUnits.clear();
            neutralUnits.clear();
            allyUnits.clear();
            noneUnits.clear();

            allyGrdSizes.clear();
            enemyGrdSizes.clear();
            grdDamageRatios.clear();
            airDamageRatios.clear();

            const auto addToRatios = [&](auto &ratios, auto &unit) {
                auto hp     = double(unit.getType().maxHitPoints());
                auto shield = double(unit.getType().maxShields());
                if (unit.getType().size() == UnitSizeTypes::Small) {
                    ratios.ratio[DamageTypes::Explosive] += (hp + shield) / (hp * 2.0 + shield);
                    ratios.ratio[DamageTypes::Concussive] += 1.0;
                }

                if (unit.getType().size() == UnitSizeTypes::Medium) {
                    ratios.ratio[DamageTypes::Explosive] += (hp + shield) / (hp * 1.333 + shield);
                    ratios.ratio[DamageTypes::Concussive] += (hp + shield) / (hp * 2.0 + shield);
                }

                if (unit.getType().size() == UnitSizeTypes::Large) {
                    ratios.ratio[DamageTypes::Explosive] += 1.0;
                    ratios.ratio[DamageTypes::Concussive] += (hp + shield) / (hp * 4.0 + shield);
                }

                ratios.armor += unit.getArmor();
            };

            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;
                if (player.isSelf()) {
                    for (auto &u : player.getUnits()) {
                        UnitInfo &unit = *u;

                        myUnits.insert(u);
                        noneUnits.insert(u);
                        auto canAttack     = (unit.canAttackGround() || unit.canAttackAir());
                        auto attackingUnit = (unit.getType().isBuilding() && canAttack) || (!unit.getType().isBuilding() && !unit.getType().isWorker() && canAttack);
                        if (attackingUnit) {
                            unit.getType().isFlyer() ? allyAirSizes[unit.getType().size()]++ : allyGrdSizes[unit.getType().size()]++;

                            auto &ratios = unit.isFlying() ? airDamageRatios[PlayerState::Enemy] : grdDamageRatios[PlayerState::Enemy];
                            ratios.count++;
                            addToRatios(ratios, unit);
                        }
                    }
                }
                if (player.isEnemy()) {
                    for (auto &u : player.getUnits()) {
                        UnitInfo &unit = *u;

                        enemyUnits.insert(u);
                        noneUnits.insert(u);
                        auto canAttack     = (unit.canAttackGround() || unit.canAttackAir());
                        auto attackingUnit = (unit.getType().isBuilding() && canAttack) || (!unit.getType().isBuilding() && !unit.getType().isWorker() && canAttack);
                        if (attackingUnit) {
                            unit.getType().isFlyer() ? enemyAirSizes[unit.getType().size()]++ : enemyGrdSizes[unit.getType().size()]++;
                            auto &ratios = unit.isFlying() ? airDamageRatios[PlayerState::Self] : grdDamageRatios[PlayerState::Self];
                            ratios.count++;
                            addToRatios(ratios, unit);
                        }
                    }
                }
            }

            // Now divide the damage ratios by the count
            for (auto &[_, grd] : grdDamageRatios) {
                if (grd.count > 0) {
                    for (auto &[type, ratio] : grd.ratio)
                        ratio /= double(grd.count);
                    grd.armor /= double(grd.count);
                }
            }

            // Now divide the damage ratios by the count
            for (auto &[_, air] : airDamageRatios) {
                if (air.count > 0) {
                    for (auto &[type, ratio] : air.ratio)
                        ratio /= double(air.count);
                    air.armor /= double(air.count);
                }
            }
        }

        void updateUnits()
        {
            updateEnemies();
            updateAllies();
            updateNeutrals();
            updateSelf();
        }
    } // namespace

    void onFrame()
    {
        Visuals::startPerfTest();
        updateCounters();
        updateUnits();
        Visuals::endPerfTest("Units");
    }

    shared_ptr<UnitInfo> getUnitInfo(Unit unit)
    {
        for (auto &[_, player] : Players::getPlayers()) {
            for (auto &u : player.getUnits()) {
                if (u->unit() == unit)
                    return u;
            }
        }
        return nullptr;
    }

    set<shared_ptr<UnitInfo>> &getUnits(PlayerState state)
    {
        switch (state) {

            case PlayerState::Ally:
                return allyUnits;
            case PlayerState::Enemy:
                return enemyUnits;
            case PlayerState::Neutral:
                return neutralUnits;
            case PlayerState::Self:
                return myUnits;
            case PlayerState::None:
                return noneUnits;
        }
        Broodwar << "Returning all unit set, not intended?" << endl;
        return noneUnits;
    }

    double getDamageRatioGrd(PlayerState state, DamageType type)
    {
        if (type == DamageTypes::Normal || grdDamageRatios[state].count == 0)
            return 1.0;
        return grdDamageRatios[state].ratio[type];
    }
    double getDamageRatioAir(PlayerState state, DamageType type)
    {
        if (type == DamageTypes::Normal || airDamageRatios[state].count == 0)
            return 1.0;
        return airDamageRatios[state].ratio[type];
    }

    double getDamageReductionGrd(PlayerState state) { return grdDamageRatios[state].armor; }
    double getDamageReductionAir(PlayerState state) { return airDamageRatios[state].armor; }

    map<UnitSizeType, int> &getAllyGrdSizes() { return allyGrdSizes; }
    map<UnitSizeType, int> &getEnemyGrdSizes() { return enemyGrdSizes; }
    map<UnitSizeType, int> &getAllyAirSizes() { return allyAirSizes; }
    map<UnitSizeType, int> &getEnemyAirSizes() { return enemyAirSizes; }
    Position getEnemyArmyCenter() { return enemyArmyCenter; }
    bool enemyThreatening() { return immThreat; }

    bool inBoundUnit(UnitInfo &unit, int seconds)
    {
        if (unit.movedFlag || unit.getType().isBuilding() || unit.getType().isWorker() || unit.isToken())
            return false;

        const auto visDiff = Broodwar->getFrameCount() - unit.getLastVisibleFrame();
        const auto inbound = Time(unit.frameArrivesWhen() - visDiff) <= Util::getTime() + Time(0, seconds);

        // Check if we know they weren't at home and are missing on the map for arg seconds, ZvZ is always inbound
        if (!Terrain::getEnemyNatural() || !Terrain::getEnemyMain() || !Scouts::gatheringInformation() || Players::ZvZ())
            return inbound;

        const auto notInNatural = !Terrain::inArea(Terrain::getEnemyNatural()->getBase()->GetArea(), unit.getPosition());
        const auto notInMain    = !Terrain::inArea(Terrain::getEnemyMain()->getBase()->GetArea(), unit.getPosition());

        return (Spy::enemyFastExpand() && notInNatural && notInMain) || (!Spy::enemyFastExpand() && notInMain);
    }

    bool commandAllowed(UnitInfo &unit)
    {
        auto it = std::find(commandQueue.begin(), commandQueue.end(), &unit);
        if (it != commandQueue.end()) {
            unit.circle(Colors::Blue);
            unit.lastCommandFrame = Broodwar->getFrameCount();
            return true;
        }
        return false;
    }
} // namespace McRave::Units