#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Support {

    void onFrame()
    {
        for (auto &u : Units::getUnits(PlayerState::Self)) {
            UnitInfo &unit = *u;
            if (unit.getRole() == Role::Support)
                updateDecision(unit);
        }
    }

    void updateDecision(UnitInfo& unit)
    {
        auto scoreBest = 0.0;
        auto posBest = Positions::Invalid; //Units::getArmyCenter();
        auto start = unit.getWalkPosition();
        auto building = Broodwar->self()->getRace().getResourceDepot();
        auto destination = Positions::Invalid;// Units::getArmyCenter();

        auto highestCluster = 0.0;
        for (auto itr = Combat::getCombatClusters().rbegin(); itr != Combat::getCombatClusters().rend(); itr++) {
            auto currentCluster = (*itr).first;
            auto currentPos = (*itr).second;
            if (currentCluster > highestCluster && !Command::overlapsActions(unit.unit(), unit.getType(), currentPos, 64)) {
                highestCluster = currentCluster;
                destination = currentPos;
            }
        }

        Broodwar->drawLineMap(unit.getPosition(), destination, Colors::Green);

        // HACK: Spells dont move
        if (unit.getType() == UnitTypes::Spell_Scanner_Sweep) {
            Command::addAction(unit.unit(), unit.getPosition(), UnitTypes::Spell_Scanner_Sweep);
            return;
        }

        // Detectors want to stay close to their target
        if (unit.getType().isDetector() && unit.hasTarget() && unit.getTarget().getPosition().isValid()) {
            auto closest = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                return true;
            });

            if (closest && (closest->getGroundDamage() > 0.0 || closest->getAirDamage() > 0.0) && closest->getPosition().getDistance(unit.getTarget().getPosition()) < SIM_RADIUS)
                destination = unit.getTarget().getPosition();
        }

        else if (unit.getType() == UnitTypes::Zerg_Overlord) {
            destination = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
            unit.circleBlack();
        }

        // TODO: Overlord scouting, need to use something different to spread overlords
        // Disabled
        if (!Terrain::getEnemyStartingPosition().isValid())
            posBest = BWEB::Map::getMainPosition();//Terrain::closestUnexploredStart();

        // Check if any expansions need detection on them
        else if (unit.getType().isDetector() && com(unit.getType()) >= 1 && BuildOrder::buildCount(building) > vis(building) && !Command::overlapsActions(unit.unit(), unit.getType(), (Position)Buildings::getCurrentExpansion(), 320))
            posBest = Position(Buildings::getCurrentExpansion());

        // Arbiters cast stasis on a target		
        else if (unit.getType() == UnitTypes::Protoss_Arbiter && unit.hasTarget() && unit.getDistance(unit.getTarget()) < SIM_RADIUS && unit.getTarget().unit()->exists() && unit.unit()->getEnergy() >= TechTypes::Stasis_Field.energyCost() && !Command::overlapsActions(unit.unit(), TechTypes::Psionic_Storm, unit.getTarget().getPosition(), 96)) {
            if ((Grids::getEGroundCluster(unit.getTarget().getWalkPosition()) + Grids::getEAirCluster(unit.getTarget().getWalkPosition())) > STASIS_LIMIT) {
                unit.unit()->useTech(TechTypes::Stasis_Field, unit.getTarget().unit());
                Command::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Stasis_Field);
                return;
            }
        }

        else {
            for (int x = start.x - 20; x <= start.x + 20; x++) {
                for (int y = start.y - 20; y <= start.y + 20; y++) {
                    WalkPosition w(x, y);
                    Position p(w);

                    // If not valid, too close, in danger or overlaps existing commands
                    if (!w.isValid()
                        || p.getDistance(unit.getPosition()) <= 64
                        || Command::isInDanger(unit, p)
                        || Command::overlapsActions(unit.unit(), unit.getType(), p, 96))
                        continue;

                    auto threat = Util::getHighestThreat(w, unit);
                    auto cluster = 1.0;// +Grids::getAGroundCluster(w) + Grids::getAAirCluster(w);
                    auto dist = 1.0 + p.getDistance(destination);

                    // Try to keep the unit alive if it's low or cloaked inside detection
                    if (unit.getPercentShield() <= LOW_SHIELD_PERCENT_LIMIT && threat > 0.0)
                        continue;/*
                    if (unit.unit()->isCloaked() && Commands().overlapsEnemyDetection(p) && threat > 0.0)
                        continue;*/

                        // Score this move
                    auto score = 1.0 / (threat * cluster * dist);
                    if (score > scoreBest) {
                        scoreBest = score;
                        posBest = p;
                    }
                }
            }
        }

        // Move and update commands
        if (posBest.isValid()) {
            unit.setEngagePosition(posBest);
            unit.unit()->move(posBest);
            Command::addAction(unit.unit(), posBest, unit.getType());
        }
    }
}