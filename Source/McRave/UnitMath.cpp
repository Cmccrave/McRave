#include "McRave.h"

// All calculations involved here can be found here:
// https://docs.google.com/spreadsheets/d/15_2BlFi27EguWciAGbWKCxLLacCnh9QFl1zSkc799Xo

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Math {

    double getMaxGroundStrength(UnitInfo& unit)
    {
        // HACK: Some hardcoded values
        if (unit.getType() == Terran_Medic)
            return 5.0;
        else if (unit.getType() == Protoss_Scarab || unit.getType() == Terran_Vulture_Spider_Mine || unit.getType() == Zerg_Egg || unit.getType() == Zerg_Larva || unit.getGroundDamage() <= 0.0)
            return 0.0;
        else if (unit.getType() == Protoss_Carrier) {
            double cnt = 0.0;
            for (auto &i : unit.unit()->getInterceptors()) {
                if (i && !i->exists()) {
                    auto &interceptor = Units::getUnitInfo(i);
                    if (interceptor)
                        cnt += interceptor->getMaxGroundStrength();
                }
            }
            return cnt;
        }

        double dps, surv, eff;
        dps = groundDPS(unit);
        surv = log(survivability(unit));
        eff = effectiveness(unit);

        return dps * surv * eff;
    }

    double getVisibleGroundStrength(UnitInfo& unit)
    {
        if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised())
            return 0.0;
        return unit.getPercentTotal() * unit.getMaxGroundStrength();
    }

    double getMaxAirStrength(UnitInfo& unit)
    {
        // HACK: Some hardcoded values
        if (unit.getType() == Protoss_Scarab || unit.getType() == Terran_Vulture_Spider_Mine || unit.getType() == Zerg_Egg || unit.getType() == Zerg_Larva || unit.getAirDamage() <= 0.0)
            return 0.0;

        else if (unit.getType() == Protoss_Carrier) {
            double cnt = 0.0;
            for (auto &i : unit.unit()->getInterceptors()) {
                if (i && !i->exists()) {
                    auto &interceptor = Units::getUnitInfo(i);
                    if (interceptor)
                        cnt += interceptor->getMaxAirStrength();
                }
            }
            return cnt;
        }

        double dps, surv, eff;
        dps = airDPS(unit);
        surv = log(survivability(unit));
        eff = effectiveness(unit);

        return dps * surv * eff;
    }

    double getVisibleAirStrength(UnitInfo& unit)
    {
        if (unit.unit()->isMaelstrommed() || unit.unit()->isStasised())
            return 0.0;
        return unit.getPercentTotal() * unit.getMaxAirStrength();
    }

    double getPriority(UnitInfo& unit)
    {
        // According to sheet linked above, these are the maximum values for normalizing
        auto maxGrdDps = 13.884;
        auto maxAirDps = 11.326;
        auto maxCost = 69.589;
        auto maxSurv = 60.624;

        // Bunch of priority hacks     
        if (unit.getType() == Terran_Vulture_Spider_Mine || unit.getType() == Terran_Science_Vessel || unit.getType() == Protoss_Arbiter)
            return 1.0;
        if ((unit.unit()->isRepairing() || unit.unit()->isConstructing()) && unit.isThreatening())
            return 5.0;
        if (Broodwar->getFrameCount() < 6000 && Strategy::enemyProxy() && unit.getType() == Protoss_Pylon)
            return 0.0;
        if (unit.unit()->isBeingConstructed() && unit.getType() == Terran_Bunker && Terrain::isInAllyTerritory(unit.getTilePosition())) {
            unit.circleBlack();
            return 0.0;
        }
        if (unit.getType() == Protoss_Carrier)
            return 0.5;

        // HACK: Kill neutrals blocking geysers for Sparkle
        if (unit.getTilePosition().isValid()) {
            const auto area = mapBWEM.GetArea(unit.getTilePosition());
            if (area && Terrain::isInAllyTerritory(unit.getTilePosition())) {
                for (auto &gas : area->Geysers()) {
                    if (gas->TopLeft().getDistance(unit.getTilePosition()) < 2 && !unit.unit()->isInvincible())
                        return 10.0;
                }
            }
        }

        auto ff = unit.canAttackGround() || unit.canAttackAir() || !unit.getType().isBuilding() ? 1 : 0;
        auto dps = ff + max(groundDPS(unit) / maxGrdDps, airDPS(unit) / maxAirDps);
        auto cost = relativeCost(unit) / maxCost;
        auto surv = survivability(unit) / maxSurv;

        //Broodwar->drawTextMap(unit.getPosition(), "%.2f, %.2f, %.2f", dps, cost, surv);
        //Broodwar->drawTextMap(unit.getPosition(), "%.2f", clamp((dps * cost / surv), 0.01, DBL_MAX));

        return clamp((dps * cost / surv), 0.1, DBL_MAX);
    }

    double relativeCost(UnitInfo& unit)
    {
        double mineral, gas;

        if (unit.getType() == Protoss_Archon)
            mineral = 100.0, gas = 300.0;
        else if (unit.getType() == Protoss_Dark_Archon)
            mineral = 250.0, gas = 200.0;
        else if (unit.getType() == Zerg_Sunken_Colony || unit.getType() == Zerg_Spore_Colony)
            mineral = 175.0, gas = 0.0;
        else
            mineral = unit.getType().mineralPrice(), gas = unit.getType().gasPrice();
        return max(log((mineral * 0.33) + (gas * 0.66)) * (double)max(unit.getType().supplyRequired(), 1), 5.00);
    }

    double groundDPS(UnitInfo& unit)
    {
        double splash, damage, range, cooldown;
        splash = splashModifier(unit);
        damage = unit.getGroundDamage();
        range = log(unit.getGroundRange());
        cooldown = gWeaponCooldown(unit);
        if (damage <= 0)
            return 0.0;
        return splash * damage * range / cooldown;
    }

    double airDPS(UnitInfo& unit)
    {
        double splash, damage, range, cooldown;
        splash = splashModifier(unit);
        damage = unit.getAirDamage();
        range = log(unit.getAirRange());
        cooldown = aWeaponCooldown(unit);

        if (damage <= 0)
            return 0.0;
        return splash * damage * range / cooldown;
    }

    double gWeaponCooldown(UnitInfo& unit)
    {
        if (unit.getType() == Terran_Bunker)
            return 15.0;
        if (unit.getType() == Protoss_Reaver)
            return 60.0;
        if (unit.getType() == Protoss_High_Templar)
            return 224.0;
        if (unit.getType() == Zerg_Infested_Terran)
            return 500.0;
        if (unit.getType() == Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Adrenal_Glands))
            return 6.0;
        if (unit.getType() == Terran_Marine && unit.getPlayer()->hasResearched(TechTypes::Stim_Packs))
            return 7.5;
        if (unit.getType() == Protoss_Interceptor)
            return 36.0;
        return unit.getType().groundWeapon().damageCooldown();
    }

    double aWeaponCooldown(UnitInfo& unit)
    {
        if (unit.getType() == Terran_Bunker)
            return 15.0;
        if (unit.getType() == Protoss_High_Templar)
            return 224.0;
        if (unit.getType() == Zerg_Scourge)
            return 110.0;
        if (unit.getType() == Terran_Marine && unit.getPlayer()->hasResearched(TechTypes::Stim_Packs))
            return 7.5;
        if (unit.getType() == Protoss_Interceptor)
            return 36.0;
        return unit.getType().airWeapon().damageCooldown();
    }

    double splashModifier(UnitInfo& unit)
    {
        if (unit.getType() == Protoss_Archon || unit.getType() == Terran_Firebat)
            return 1.25;
        if (unit.getType() == Protoss_Reaver)
            return 1.50;
        if (unit.getType() == Protoss_High_Templar)
            return 4.00;
        if (unit.getType() == Terran_Siege_Tank_Siege_Mode)
            return 2.50;
        if (unit.getType() == Terran_Valkyrie || unit.getType() == Zerg_Mutalisk)
            return 1.50;
        if (unit.getType() == Zerg_Lurker)
            return 2.00;
        return 1.00;
    }

    double effectiveness(UnitInfo& unit)
    {
        auto sizes = unit.getPlayer() == Broodwar->self() ? Units::getEnemySizes() : Units::getAllySizes();
        auto large = sizes[UnitSizeTypes::Large];
        auto medium = sizes[UnitSizeTypes::Medium];
        auto small = sizes[UnitSizeTypes::Small];
        auto total = double(large + medium + small);

        if (total > 0.0) {
            if (unit.getType().groundWeapon().damageType() == DamageTypes::Explosive)
                return ((large*1.0) + (medium*0.75) + (small*0.5)) / total;
            else if (unit.getType().groundWeapon().damageType() == DamageTypes::Concussive)
                return ((large*0.25) + (medium*0.5) + (small*1.0)) / total;
        }
        return 1.0;
    }

    double survivability(UnitInfo& unit)
    {
        double speed, armor, health;
        speed = (unit.getType().isBuilding()) ? 1.0 : max(1.0, log(unit.getSpeed()));
        armor = 2.0 + double(unit.getType().armor() + unit.getPlayer()->getUpgradeLevel(unit.getType().armorUpgrade()));
        health = log(double(unit.getType().maxHitPoints() + unit.getType().maxShields()));
        return speed * armor * health;
    }

    double groundRange(UnitInfo& unit)
    {
        if (unit.getType() == Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge))
            return 192.0;
        if ((unit.getType() == Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines)))
            return 160.0;
        if (unit.getType() == Protoss_High_Templar)
            return 288.0;
        if (unit.getType() == Protoss_Carrier)
            return 256.0;
        if (unit.getType() == Protoss_Reaver)
            return 256.0;
        if (unit.getType() == Terran_Bunker) {
            if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells))
                return 192.0;
            return 160.0;
        }
        if (unit.getType() == Terran_Vulture_Spider_Mine)
            return 160.0;
        return double(unit.getType().groundWeapon().maxRange());
    }

    double airRange(UnitInfo& unit)
    {
        if (unit.getType() == Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge))
            return 192.0;
        if ((unit.getType() == Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines)))
            return 160.0;
        if (unit.getType() == Terran_Goliath && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Charon_Boosters))
            return 256.0;
        if (unit.getType() == Protoss_High_Templar)
            return 288.0;
        if (unit.getType() == Protoss_Carrier)
            return 256;
        if (unit.getType() == Terran_Bunker) {
            if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells))
                return 192.0;
            return 160.0;
        }
        return double(unit.getType().airWeapon().maxRange());
    }

    double groundDamage(UnitInfo& unit)
    {
        // TODO Check Reaver upgrade type functional here or if needed hardcoding
        int upLevel = unit.getPlayer()->getUpgradeLevel(unit.getType().groundWeapon().upgradeType());
        if (unit.getType() == Protoss_Reaver) {
            if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Scarab_Damage))
                return 125.00;
            return 100.00;
        }
        if (unit.getType() == Terran_Bunker)
            return 24.0 + (4.0 * upLevel);
        if (unit.getType() == Terran_Firebat || unit.getType() == Protoss_Zealot)
            return 16.0 + (2.0 * upLevel);
        if (unit.getType() == Protoss_Interceptor)
            return 12.0 + (2.0 * upLevel);
        if (unit.getType() == Protoss_High_Templar)
            return 112.0;
        return unit.getType().groundWeapon().damageAmount() + (unit.getType().groundWeapon().damageBonus() * upLevel);
    }

    double airDamage(UnitInfo& unit)
    {
        int upLevel = unit.getPlayer()->getUpgradeLevel(unit.getType().airWeapon().upgradeType());
        if (unit.getType() == Terran_Bunker)
            return 24.0 + (4.0 * upLevel);
        if (unit.getType() == Protoss_Scout)
            return 28.0 + (2.0 * upLevel);
        if (unit.getType() == Protoss_Interceptor)
            return 12.0 + (2.0 * upLevel);
        if (unit.getType() == Terran_Valkyrie)
            return 48.0 + (8.0 * upLevel);
        if (unit.getType() == Protoss_High_Templar)
            return 112.0;
        return unit.getType().airWeapon().damageAmount() + (unit.getType().airWeapon().damageBonus() * upLevel);
    }

    double speed(UnitInfo& unit)
    {
        double speed = unit.getType().topSpeed();

        if ((unit.getType() == Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost)) || (unit.getType() == Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments)) || (unit.getType() == Zerg_Ultralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Anabolic_Synthesis)) || (unit.getType() == Protoss_Shuttle && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Drive)) || (unit.getType() == Protoss_Observer && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Boosters)) || (unit.getType() == Protoss_Zealot && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements)) || (unit.getType() == Terran_Vulture && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)))
            return speed * 1.5;
        if (unit.getType() == Zerg_Overlord && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace))
            return speed * 4.01;
        if (unit.getType() == Protoss_Scout && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments))
            return speed * 1.33;
        if (unit.getType().isBuilding())
            return 0.0;
        if (unit.isBurrowed())
            return 0.0;
        return speed;
    }

    int getMinStopFrame(UnitType unitType)
    {
        if (unitType == Protoss_Dragoon)
            return 9;
        if (unitType == Zerg_Devourer)
            return 11;
        return 0;
    }

    WalkPosition getWalkPosition(Unit unit)
    {
        auto walkWidth = unit->getType().isBuilding() ? unit->getType().tileWidth() * 4 : (int)ceil(unit->getType().width() / 8.0);
        auto walkHeight = unit->getType().isBuilding() ? unit->getType().tileHeight() * 4 : (int)ceil(unit->getType().height() / 8.0);
        if (!unit->getType().isBuilding())
            return WalkPosition(unit->getPosition()) - WalkPosition(walkWidth / 2, walkHeight / 2);
        else
            return WalkPosition(unit->getTilePosition());
        return WalkPositions::None;
    }
}