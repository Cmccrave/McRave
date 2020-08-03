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

            BWEB::Station * stationNeedsDetection = nullptr;
            BWEB::Wall * wallNeedsDetection = nullptr;

            // Check If any station needs detection
            if (false) {
                for (auto &[_, station] : Stations::getMyStations()) {
                    if (!Actions::overlapsDetection(unit.unit(), station->getBase()->Center(), PlayerState::Self)) {
                        stationNeedsDetection = station;
                        break;
                    }
                }
            }

            // Check if any wall needs detection
            if (false) {

            }

            // Set goal as destination
            if (unit.getGoal().isValid())
                unit.setDestination(unit.getGoal());

            // Send Overlords to safety if needed
            else if (unit.getType() == Zerg_Overlord && (Players::getStrength(PlayerState::Self).groundToAir == 0.0 || Players::getStrength(PlayerState::Self).airToAir == 0.0)) {

                auto closestSpore = Util::getClosestUnit(unit.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getType() == Zerg_Spore_Colony;
                });

                auto closestStation = Stations::getClosestStation(PlayerState::Self, unit.getPosition());
                if (closestSpore)
                    unit.setDestination(closestSpore->getPosition());
                else if (closestStation)
                    unit.setDestination(closestStation->getBase()->Center());
            }

            // Send detection to a wall that needs it
            else if (unit.getType().isDetector() && wallNeedsDetection && !Actions::overlapsDetection(unit.unit(), wallNeedsDetection->getCentroid(), PlayerState::Self))
                unit.setDestination(Walls::getNaturalWall()->getCentroid());

            // Send detection to a station that needs it
            else if (unit.getType().isDetector() && stationNeedsDetection && !Actions::overlapsDetection(unit.unit(), stationNeedsDetection->getBase()->Center(), PlayerState::Self))
                unit.setDestination(stationNeedsDetection->getBase()->Center());

            // Send Overlords to watch for proxy structures at our natural choke
            else if (unit.getType() == Zerg_Overlord && Util::getTime() < Time(3, 00) && !Actions::overlapsDetection(unit.unit(), Position(BWEB::Map::getNaturalChoke()->Center()), PlayerState::Self))
                unit.setDestination(Position(BWEB::Map::getNaturalChoke()->Center()));

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