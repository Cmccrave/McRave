#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

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
            else if (unit.getType() == Zerg_Overlord && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Pneumatized_Carapace) == 0)
                unit.setDestination(Stations::getClosestStation(PlayerState::Self, unit.getPosition()));

            // Detectors want to stay close to their target if we have a unit that can engage it
            else if (unit.getType().isDetector() && unit.hasTarget() && isntAssigned(unit.getTarget().getPosition()))
                unit.setDestination(unit.getTarget().getPosition());

            // Find the highest combat cluster that doesn't overlap a current support action of this UnitType
            else {
                auto highestCluster = 0.0;

                for (auto &[cluster, position] : Combat::getCombatClusters()) {

                    const auto score = cluster / position.getDistance(Terrain::getAttackPosition());

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
        }

        void updateDecision(UnitInfo& unit)
        {
            // If this unit is a scanner sweep, add the action and return
            if (unit.getType() == Spell_Scanner_Sweep) {
                Actions::addAction(unit.unit(), unit.getPosition(), Spell_Scanner_Sweep, PlayerState::Self);
                return;
            }

            // Arbiters cast stasis on a target        
            else if (unit.getType() == Protoss_Arbiter && unit.canStartCast(TechTypes::Stasis_Field) && !Actions::overlapsActions(unit.unit(), unit.getTarget().getPosition(), TechTypes::Psionic_Storm, PlayerState::Self, 96)) {
                unit.unit()->useTech(TechTypes::Stasis_Field, unit.getTarget().unit());
                Actions::addAction(unit.unit(), unit.getTarget().getPosition(), TechTypes::Stasis_Field, PlayerState::Self);
            }

            else
                Command::escort(unit);
        }

        void updateUnits()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;
                if (unit.getRole() == Role::Support && unit.unit()->isCompleted()) {
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