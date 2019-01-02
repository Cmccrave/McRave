#include "UnitInfo.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Math {

    double getMaxGroundStrength(UnitInfo& unit)
    {
        // HACK: Some hardcoded values
        if (unit.getType() == UnitTypes::Terran_Medic)
            return 5.0;
        else if (unit.getType() == UnitTypes::Protoss_Scarab || unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Zerg_Egg || unit.getType() == UnitTypes::Zerg_Larva || unit.getGroundRange() <= 0.0)
            return 0.0;
        else if (unit.getType() == UnitTypes::Protoss_Interceptor)
            return 2.0;
        else if (unit.getType() == UnitTypes::Protoss_Carrier) {
            double cnt = 0.0;
            for (auto &i : unit.unit()->getInterceptors()) {
                if (i && !i->exists()) {
                    cnt += 2.0;
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
        if (unit.getType() == UnitTypes::Protoss_Scarab || unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Zerg_Egg || unit.getType() == UnitTypes::Zerg_Larva || unit.getAirRange() <= 0.0)
            return 0.0;

        else if (unit.getType() == UnitTypes::Protoss_Interceptor)
            return 2.0;

        else if (unit.getType() == UnitTypes::Protoss_Carrier) {
            double cnt = 0.0;
            for (auto &i : unit.unit()->getInterceptors()) {
                if (i && !i->exists()) {
                    cnt += 2.0;
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
        if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine || unit.getType() == UnitTypes::Terran_Science_Vessel)
            return 3.0;
        if ((unit.unit()->isRepairing() || unit.unit()->isConstructing()) && Units::isThreatening(unit))
            return 100.0;
        if (Broodwar->getFrameCount() < 6000 && Strategy::enemyProxy() && unit.getType() == UnitTypes::Protoss_Pylon)
            return -5.0;
        if (unit.unit()->isBeingConstructed() && unit.getType() == UnitTypes::Terran_Bunker && Terrain::isInAllyTerritory(unit.getTilePosition()))
            return -100.0;

        if (unit.getTilePosition().isValid()) {
            const auto area = mapBWEM.GetArea(unit.getTilePosition());
            if (area && Terrain::isInAllyTerritory(unit.getTilePosition())) {
                for (auto &gas : area->Geysers()) {
                    if (gas->TopLeft().getDistance(unit.getTilePosition()) < 2 && !unit.unit()->isInvincible())
                        return 100.0;
                }
            }
        }

        double mineral, gas;

        if (unit.getType() == UnitTypes::Protoss_Archon)
            mineral = 100.0, gas = 300.0;
        else if (unit.getType() == UnitTypes::Protoss_Dark_Archon)
            mineral = 250.0, gas = 200.0;
        else if (unit.getType() == UnitTypes::Zerg_Sunken_Colony || unit.getType() == UnitTypes::Zerg_Spore_Colony)
            mineral = 175.0, gas = 0.0;
        else
            mineral = unit.getType().mineralPrice(), gas = unit.getType().gasPrice();

        double dps = 1.00 + max({ groundDPS(unit), airDPS(unit), 1.0 });
        double cost = max(log((mineral * 0.33) + (gas * 0.66)) * (double)max(unit.getType().supplyRequired(), 1), 5.00);
        double surv = survivability(unit);

        return log(10.0 + (dps * cost / surv));
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
        if (unit.getType() == UnitTypes::Terran_Bunker) return 15.0;
        else if (unit.getType() == UnitTypes::Protoss_Reaver) return 60.0;
        else if (unit.getType() == UnitTypes::Protoss_High_Templar) return 224.0;
        else if (unit.getType() == UnitTypes::Zerg_Infested_Terran) return 500.0;
        else if (unit.getType() == UnitTypes::Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Adrenal_Glands)) return 6.0;
        else if (unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->hasResearched(TechTypes::Stim_Packs)) return 7.5;
        return unit.getType().groundWeapon().damageCooldown();
    }

    double aWeaponCooldown(UnitInfo& unit)
    {
        if (unit.getType() == UnitTypes::Terran_Bunker) return 15.0;
        else if (unit.getType() == UnitTypes::Protoss_High_Templar) return 224.0;
        else if (unit.getType() == UnitTypes::Zerg_Scourge) return 110.0;
        else if (unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->hasResearched(TechTypes::Stim_Packs)) return 7.5;
        return unit.getType().airWeapon().damageCooldown();
    }

    double splashModifier(UnitInfo& unit)
    {
        if (unit.getType() == UnitTypes::Protoss_Archon || unit.getType() == UnitTypes::Terran_Firebat) return 1.25;
        if (unit.getType() == UnitTypes::Protoss_Reaver) return 1.25;
        if (unit.getType() == UnitTypes::Protoss_High_Templar) return 6.00;
        if (unit.getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode) return 2.50;
        if (unit.getType() == UnitTypes::Terran_Valkyrie || unit.getType() == UnitTypes::Zerg_Mutalisk) return 1.50;
        if (unit.getType() == UnitTypes::Zerg_Lurker) return 2.00;
        return 1.00;
    }

    double effectiveness(UnitInfo& unit)
    {
        auto effectiveness = 1.0;
        auto sizes = unit.getPlayer() == Broodwar->self() ? Units::getEnemySizes() : Units::getAllySizes();

        auto large = sizes[UnitSizeTypes::Large];
        auto medium = sizes[UnitSizeTypes::Medium];
        auto small = sizes[UnitSizeTypes::Small];
        auto total = double(large + medium + small);

        if (total > 0.0) {
            if (unit.getType().groundWeapon().damageType() == DamageTypes::Explosive)
                effectiveness = ((large*1.0) + (medium*0.75) + (small*0.5)) / total;
            else if (unit.getType().groundWeapon().damageType() == DamageTypes::Concussive)
                effectiveness = ((large*0.25) + (medium*0.5) + (small*1.0)) / total;
        }
        return effectiveness;
    }

    double survivability(UnitInfo& unit)
    {
        double speed, armor, health;
        speed = (unit.getType().isBuilding()) ? 0.5 : max(1.0, log(unit.getSpeed()));
        armor = 2.0 + double(unit.getType().armor() + unit.getPlayer()->getUpgradeLevel(unit.getType().armorUpgrade()));
        health = log(((double)unit.getType().maxHitPoints() + (double)unit.getType().maxShields()) / 20.0);
        return speed * armor * health;
    }

    double groundRange(UnitInfo& unit)
    {
        if (unit.getType() == UnitTypes::Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge)) return 192.0;
        if ((unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines))) return 160.0;
        if (unit.getType() == UnitTypes::Protoss_High_Templar) return 288.0;
        if (unit.getType() == UnitTypes::Protoss_Carrier) return 256;
        if (unit.getType() == UnitTypes::Protoss_Reaver) return 256.0;
        if (unit.getType() == UnitTypes::Terran_Bunker) {
            if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) return 192.0;
            return 160.0;
        }
        if (unit.getType() == UnitTypes::Terran_Vulture_Spider_Mine) return 96.0;
        return double(unit.getType().groundWeapon().maxRange());
    }

    double airRange(UnitInfo& unit)
    {
        if (unit.getType() == UnitTypes::Protoss_Dragoon && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Singularity_Charge)) return 192.0;
        if ((unit.getType() == UnitTypes::Terran_Marine && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Grooved_Spines))) return 160.0;
        if (unit.getType() == UnitTypes::Terran_Goliath && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Charon_Boosters)) return 256.0;
        if (unit.getType() == UnitTypes::Protoss_High_Templar) return 288.0;
        if (unit.getType() == UnitTypes::Protoss_Carrier) return 256;
        if (unit.getType() == UnitTypes::Terran_Bunker) {
            if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::U_238_Shells)) return 192.0;
            return 160.0;
        }
        return double(unit.getType().airWeapon().maxRange());
    }

    double groundDamage(UnitInfo& unit)
    {
        // TODO Check Reaver upgrade type functional here or if needed hardcoding
        int upLevel = unit.getPlayer()->getUpgradeLevel(unit.getType().groundWeapon().upgradeType());
        if (unit.getType() == UnitTypes::Protoss_Reaver) {
            if (unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Scarab_Damage)) return 125.00;
            else return 100.00;
        }
        if (unit.getType() == UnitTypes::Terran_Bunker) return 24.0 + (4.0 * upLevel);
        if (unit.getType() == UnitTypes::Terran_Firebat || unit.getType() == UnitTypes::Protoss_Zealot) return 16.0 + (2.0 * upLevel);
        if (unit.getType() == UnitTypes::Protoss_High_Templar) return 112.0;
        return unit.getType().groundWeapon().damageAmount() + (unit.getType().groundWeapon().damageBonus() * upLevel);
    }

    double airDamage(UnitInfo& unit)
    {
        int upLevel = unit.getPlayer()->getUpgradeLevel(unit.getType().airWeapon().upgradeType());
        if (unit.getType() == UnitTypes::Terran_Bunker)	return 24.0 + (4.0 * upLevel);
        if (unit.getType() == UnitTypes::Protoss_Scout)	return 28.0 + (2.0 * upLevel);
        if (unit.getType() == UnitTypes::Terran_Valkyrie) return 48.0 + (8.0 * upLevel);
        if (unit.getType() == UnitTypes::Protoss_High_Templar) return 112.0;
        return unit.getType().airWeapon().damageAmount() + (unit.getType().airWeapon().damageBonus() * upLevel);
    }

    double speed(UnitInfo& unit)
    {
        double speed = unit.getType().topSpeed();

        if ((unit.getType() == UnitTypes::Zerg_Zergling && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Metabolic_Boost)) || (unit.getType() == UnitTypes::Zerg_Hydralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments)) || (unit.getType() == UnitTypes::Zerg_Ultralisk && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Anabolic_Synthesis)) || (unit.getType() == UnitTypes::Protoss_Shuttle && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Drive)) || (unit.getType() == UnitTypes::Protoss_Observer && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Gravitic_Boosters)) || (unit.getType() == UnitTypes::Protoss_Zealot && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Leg_Enhancements)) || (unit.getType() == UnitTypes::Terran_Vulture && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Ion_Thrusters)))
            return speed * 1.5;
        if (unit.getType() == UnitTypes::Zerg_Overlord && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace)) return speed * 4.01;
        if (unit.getType() == UnitTypes::Protoss_Scout && unit.getPlayer()->getUpgradeLevel(UpgradeTypes::Muscular_Augments)) return speed * 1.33;
        if (unit.getType().isBuilding()) return 0.0;
        if (unit.isBurrowed()) return 0.0;
        return speed;
    }

    int getMinStopFrame(UnitType unitType)
    {
        if (unitType == UnitTypes::Protoss_Dragoon)	return 9;
        else if (unitType == UnitTypes::Zerg_Devourer) return 11;
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