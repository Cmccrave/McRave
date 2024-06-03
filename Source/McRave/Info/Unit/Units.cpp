#include "Main/McRave.h"
#include "Main/Events.h"

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
        map<UnitSizeType, int> allyGrdSizes;
        map<UnitSizeType, int> enemyGrdSizes;
        map<UnitSizeType, int> allyAirSizes;
        map<UnitSizeType, int> enemyAirSizes;
        double immThreat;
        Position enemyArmyCenter;

        // This is hacky
        vector<UnitInfo*> commandQueue;

        void updateEnemies()
        {
            enemyArmyCenter = Position(0, 0);
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
                    if (!unit.unit()->exists() && Broodwar->isVisible(TilePosition(unit.getPosition())) && (!unit.isBurrowed() || Actions::overlapsDetection(unit.unit(), unit.getPosition(), PlayerState::Self) || (unit.getPosition().isValid() && Grids::getGroundDensity(unit.getPosition(), PlayerState::Self) > 0)))
                        Events::onUnitDisappear(unit);

                    // If a unit is threatening our position
                    if (unit.isThreatening()) {
                        if (!unit.getType().isWorker())
                            immThreat += unit.getVisibleGroundStrength();
                        unit.circle(Colors::Red);
                    }

                    // Add to army center
                    if (unit.getPosition().isValid() && !unit.getType().isBuilding() && !unit.getType().isWorker() && !unit.movedFlag) {
                        enemyArmyCenter += unit.getPosition();
                        enemyArmyCount++;
                    }

                    //Broodwar->drawTextMap(unit.getPosition(), "%.2f", unit.getPriority());
                }
            }

            enemyArmyCenter /= enemyArmyCount;
        }

        void updateAllies()
        {

        }

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
                    auto validRole = unit.getRole() == Role::Combat || unit.getRole() == Role::Defender
                        || unit.getRole() == Role::Scout || unit.getRole() == Role::Support || unit.getRole() == Role::Transport
                        || unit.getRole() == Role::Worker;

                    auto frames = unit.isLightAir() ? 3 : 9;
                    auto newCommandFrame = Broodwar->getFrameCount() - unit.commandFrame > frames;

                    if (newCommandFrame && validRole)
                        commandQueue.push_back(&unit);
                }
            }

            // Sort command queue
            sort(commandQueue.begin(), commandQueue.end(), [&](auto &u1, auto &u2) {
                return u1->commandFrame < u2->commandFrame;
            });
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

            allyGrdSizes.clear();
            enemyGrdSizes.clear();
            grdDamageRatios.clear();
            airDamageRatios.clear();

            const auto addToRatios = [&](auto &ratios, auto& unit) {
                auto hp = double(unit.getType().maxHitPoints());
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

                ratios.armor += unit.data.armor;
            };

            for (auto &p : Players::getPlayers()) {
                PlayerInfo &player = p.second;
                if (player.isSelf()) {
                    for (auto &u : player.getUnits()) {
                        UnitInfo &unit = *u;

                        myUnits.insert(u);
                        auto canAttack = (unit.canAttackGround() || unit.canAttackAir());
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
                        auto canAttack = (unit.canAttackGround() || unit.canAttackAir());
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
    }

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

    set<shared_ptr<UnitInfo>>& getUnits(PlayerState state)
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
        }
        return myUnits;
    }

    double getDamageRatioGrd(PlayerState state, DamageType type) {
        if (type == DamageTypes::Normal || grdDamageRatios[state].count == 0)
            return 1.0;
        return grdDamageRatios[state].ratio[type];
    }
    double getDamageRatioAir(PlayerState state, DamageType type) {
        if (type == DamageTypes::Normal || airDamageRatios[state].count == 0)
            return 1.0;
        return airDamageRatios[state].ratio[type];
    }

    double getDamageReductionGrd(PlayerState state) {
        return grdDamageRatios[state].armor;
    }
    double getDamageReductionAir(PlayerState state) {
        return airDamageRatios[state].armor;
    }

    map<UnitSizeType, int>& getAllyGrdSizes() { return allyGrdSizes; }
    map<UnitSizeType, int>& getEnemyGrdSizes() { return enemyGrdSizes; }
    map<UnitSizeType, int>& getAllyAirSizes() { return allyAirSizes; }
    map<UnitSizeType, int>& getEnemyAirSizes() { return enemyAirSizes; }
    Position getEnemyArmyCenter() { return enemyArmyCenter; }
    double getImmThreat() { return immThreat; }

    bool commandAllowed(UnitInfo& unit) {
        auto idx = find_if(commandQueue.begin(), commandQueue.end(), [&](auto &u) {
            return u == &unit;
        });
        auto allowed = idx != commandQueue.end() && idx - commandQueue.begin() < 50;
        if (allowed)
            unit.commandFrame = Broodwar->getFrameCount();
        return allowed;
    }
}