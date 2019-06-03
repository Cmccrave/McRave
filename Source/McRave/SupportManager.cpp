#include "McRave.h"

using namespace BWAPI;
using namespace std;

namespace McRave::Support {

    namespace {

        map<Position, UnitType> futureAssignment;

        void updateCounters()
        {
            futureAssignment.clear();
        }

        void updateDestination(UnitInfo& unit)
        {
            auto isntAssigned = [&](Position here) {
                for (auto &[pos, type] : futureAssignment) {
                    if (type == unit.getType() && pos.getDistance(here) < 256.0)
                        return false;
                }
                return true;
            };

            if (unit.getGoal().isValid())
                unit.setDestination(unit.getGoal());

            // Overlords move towards the closest stations for now
            else if (unit.getType() == UnitTypes::Zerg_Overlord && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0)
                unit.setDestination(Stations::getClosestStation(PlayerState::Self, unit.getPosition()));

            // Detectors want to stay close to their target if we have a unit that can engage it
            else if (unit.getType().isDetector() && unit.hasTarget() && isntAssigned(unit.getTarget().getPosition()))
                unit.setDestination(unit.getTarget().getPosition());

            // Find the highest combat cluster that doesn't overlap a current support action of this UnitType
            else {
                auto highestCluster = 0.0;

                for (auto &[score, position] : Combat::getCombatClusters()) {
                    if (score > highestCluster && isntAssigned(position)) {
                        highestCluster = score;
                        unit.setDestination(position);
                    }
                }
            }

            // Resort to going to the army center as long as we have an army
            if (!unit.getDestination().isValid() && !Combat::getCombatClusters().empty()) {
                auto highestClusterPosition = (*Combat::getCombatClusters().rbegin()).second;
                unit.setDestination(highestClusterPosition);
            }

            if (!unit.getDestination().isValid())
                unit.setDestination(BWEB::Map::getMainPosition());            

            futureAssignment.emplace(make_pair(unit.getDestination(), unit.getType()));

            Broodwar->drawLineMap(unit.getPosition(), unit.getDestination(), Colors::Purple);
        }

        void updateDecision(UnitInfo& unit)
        {
            // If this unit is a scanner sweep, add the action and return
            if (unit.getType() == UnitTypes::Spell_Scanner_Sweep) {
                Command::addAction(unit.unit(), unit.getPosition(), UnitTypes::Spell_Scanner_Sweep, PlayerState::Self);
                return;
            }

            // Arbiters cast stasis on a target		
            else if (unit.getType() == UnitTypes::Protoss_Arbiter && unit.hasTarget() && unit.getPosition().getDistance(unit.getTarget().getPosition()) < SIM_RADIUS && unit.getTarget().unit()->exists() && unit.unit()->getEnergy() >= TechTypes::Stasis_Field.energyCost() && !Command::overlapsActions(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm, PlayerState::Self, 96)) {
                if ((Grids::getEGroundCluster(unit.getTarget().getWalkPosition()) + Grids::getEAirCluster(unit.getTarget().getWalkPosition())) > STASIS_LIMIT) {
                    unit.unit()->useTech(TechTypes::Stasis_Field, unit.getTarget().unit());
                    Command::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Stasis_Field, PlayerState::Self);
                    return;
                }
            }

            else
                Command::escort(unit);
        }

        void updateUnits()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.getRole() == Role::Support) {
                    updateDestination(unit);
                    updateDecision(unit);
                }
            }
        }
    }

    void onFrame()
    {
        updateCounters();
        updateUnits();
    }
}