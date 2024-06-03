#include "Main\McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Spy::Terran {

    namespace {

        void enemyTerranBuilds(PlayerInfo& player, StrategySpy& theSpy)
        {
            for (auto &u : player.getUnits()) {
                UnitInfo &unit = *u;

                // Marine timing
                if (unit.getType() == Terran_Marine) {
                    if (unit.timeArrivesWhen().minutes > 0 && unit.timeArrivesWhen() < theSpy.rushArrivalTime)
                        theSpy.rushArrivalTime = unit.timeArrivesWhen();
                }

                // FE Detection
                if (Util::getTime() < Time(4, 00) && unit.getType() == Terran_Bunker && Terrain::getEnemyNatural() && Terrain::getEnemyMain()) {
                    auto closestMain = BWEB::Stations::getClosestMainStation(unit.getTilePosition());
                    if (closestMain && Stations::ownedBy(closestMain) != PlayerState::Self) {
                        auto natDistance = unit.getPosition().getDistance(Position(Terrain::getEnemyNatural()->getChokepoint()->Center()));
                        auto mainDistance = unit.getPosition().getDistance(Position(Terrain::getEnemyMain()->getChokepoint()->Center()));
                        if (natDistance < mainDistance)
                            theSpy.expand.possible = true;
                    }
                }
            }

            auto hasMech = Players::getVisibleCount(PlayerState::Enemy, Terran_Vulture) > 0
                || Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0
                || Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) > 0
                || Players::getVisibleCount(PlayerState::Enemy, Terran_Goliath) > 0;

            // 2Rax
            if ((theSpy.rushArrivalTime < Time(3, 10) && Util::getTime() < Time(3, 25) && Players::getTotalCount(PlayerState::Enemy, Terran_Marine) >= 3)
                || (Util::getTime() < Time(2, 55) && Players::getTotalCount(PlayerState::Enemy, Terran_Barracks) >= 2)
                || (Util::getTime() < Time(4, 00) && Players::getTotalCount(PlayerState::Enemy, Terran_Barracks) >= 2 && Players::getTotalCount(PlayerState::Enemy, Terran_Refinery) == 0)
                || completesBy(3, Terran_Barracks, Time(5, 15))
                || completesBy(12, Terran_Marine, Time(5, 00))
                || arrivesBy(6, Terran_Marine, Time(4, 10))
                || arrivesBy(8, Terran_Marine, Time(4, 30))
                || arrivesBy(10, Terran_Marine, Time(4, 50))
                || arrivesBy(12, Terran_Marine, Time(5, 10))
                || arrivesBy(2, Terran_Medic, Time(5, 45))
                || arrivesBy(3, Terran_Medic, Time(6, 15)))
                theSpy.build.name = "2Rax";

            // RaxCC
            if ((completesBy(2, Terran_Command_Center, Time(4, 30)))
                || (theSpy.expand.possible && Util::getTime() < Time(4, 00))
                || (theSpy.proxy.possible && Players::getVisibleCount(PlayerState::Enemy, Terran_Barracks) == 1))
                theSpy.build.name = "RaxCC";

            // RaxFact
            if (completesBy(1, Terran_Factory, Time(4, 00))
                || (Util::getTime() < Time(5, 15) && hasMech)
                || theSpy.rushArrivalTime < Time(2, 45)
                || completesBy(1, Terran_Refinery, Time(2, 30))
                || completesBy(1, Terran_Barracks, Time(1, 40))
                || arrivesBy(1, Terran_Wraith, Time(6, 00)))
                theSpy.build.name = "RaxFact";

            // 2Rax Proxy - No info estimation
            if (Scouts::gotFullScout() && Util::getTime() > Time(2, 20) && Util::getTime() < Time(3, 45) && Players::getVisibleCount(PlayerState::Enemy, Terran_Barracks) == 0 && Players::getVisibleCount(PlayerState::Enemy, Terran_Refinery) == 0 && Players::getVisibleCount(PlayerState::Enemy, Terran_Command_Center) <= 1) {
                theSpy.build.name = "2Rax";
                theSpy.proxy.possible = true;
            }

            // Rax/Gas/Rax estimation - consider 2Rax
            if (completesBy(1, Terran_Academy, Time(3, 30)) || arrivesBy(1, Terran_Medic, Time(5, 30))) {
                theSpy.build.name = "2Rax";
            }
        }

        void enemyTerranOpeners(PlayerInfo& player, StrategySpy& theSpy)
        {
            // Bio builds
            if (theSpy.build.name == "2Rax") {
                if (theSpy.expand.possible)
                    theSpy.opener.name = "Expand";
                else if (theSpy.proxy.possible
                    || completesBy(2, Terran_Barracks, Time(2, 35))
                    || completesBy(2, Terran_Marine, Time(2, 40))
                    || completesBy(4, Terran_Marine, Time(2, 55))
                    || arrivesBy(2, Terran_Marine, Time(2, 55))
                    || arrivesBy(4, Terran_Marine, Time(3, 25)))
                    theSpy.opener.name = "BBS";
                else if (completesBy(2, Terran_Barracks, Time(3, 00))
                    || completesBy(3, Terran_Marine, Time(2, 55))
                    || completesBy(5, Terran_Marine, Time(3, 15))
                    || arrivesBy(3, Terran_Marine, Time(3, 40))
                    || arrivesBy(5, Terran_Marine, Time(4, 00)))
                    theSpy.opener.name = "11/13";
            }

            // Mech builds
            if (theSpy.build.name == "RaxFact") {
                if (theSpy.expand.possible)
                    theSpy.opener.name = "1FactFE";
            }

            // Expand builds
            if (theSpy.build.name == "RaxCC") {
                if (theSpy.expand.possible)
                    theSpy.opener.name = "1RaxFE";
            }

            // 8Rax opener
            if (theSpy.build.name != "2Rax") {
                if (arrivesBy(1, Terran_Marine, Time(3, 00))
                    || arrivesBy(2, Terran_Marine, Time(3, 20))
                    || completesBy(1, Terran_Barracks, Time(2, 00))
                    || completesBy(1, Terran_Marine, Time(2, 15)))
                    theSpy.opener.name = "8Rax";
            }
        }

        void enemyTerranTransitions(PlayerInfo& player, StrategySpy& theSpy)
        {
            auto hasTanks = Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0 || Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Tank_Mode) > 0;
            auto hasGols = Players::getVisibleCount(PlayerState::Enemy, Terran_Goliath) > 0;
            auto hasWraiths = Players::getVisibleCount(PlayerState::Enemy, Terran_Wraith) > 0;

            if (theSpy.workersPulled >= 4 && Util::getTime() < Time(3, 00))
                theSpy.transition.name = "WorkerRush";

            // PvT
            if (Players::PvT()) {
                if ((Players::getVisibleCount(PlayerState::Enemy, Terran_Siege_Tank_Siege_Mode) > 0 && Players::getVisibleCount(PlayerState::Enemy, Terran_Vulture) == 0)
                    || (theSpy.expand.possible && Players::getVisibleCount(PlayerState::Enemy, Terran_Machine_Shop) > 0))
                    theSpy.transition.name = "SiegeExpand";

                if ((Players::getTotalCount(PlayerState::Enemy, Terran_Vulture_Spider_Mine) > 0 && Util::getTime() < Time(6, 00))
                    || (Players::getTotalCount(PlayerState::Enemy, Terran_Factory) >= 2 && theSpy.typeUpgrading.find(Terran_Machine_Shop) != theSpy.typeUpgrading.end())
                    || (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 3 && Util::getTime() < Time(5, 30))
                    || (Players::getTotalCount(PlayerState::Enemy, Terran_Vulture) >= 5 && Util::getTime() < Time(6, 00)))
                    theSpy.transition.name = "2Fact";
            }

            // ZvT
            if (Players::ZvT()) {

                // RaxFact
                if (theSpy.build.name == "RaxFact") {
                    if (arrivesBy(1, Terran_Wraith, Time(5, 45))
                        || arrivesBy(2, Terran_Wraith, Time(6, 30)))
                        theSpy.transition.name = "2PortWraith";

                    if ((Players::getTotalCount(PlayerState::Enemy, Terran_Machine_Shop) >= 2 && (theSpy.typeUpgrading.find(Terran_Machine_Shop) != theSpy.typeUpgrading.end() || Players::getTotalCount(PlayerState::Enemy, Terran_Vulture_Spider_Mine) > 0))
                        || (Players::getTotalCount(PlayerState::Enemy, Terran_Machine_Shop) >= 2 && Util::getTime() < Time(6, 00))
                        || completesBy(1, UpgradeTypes::Ion_Thrusters, Time(6, 15)))
                        theSpy.transition.name = "2Fact";
                }

                // 2Rax
                if (theSpy.build.name == "2Rax") {
                    if (theSpy.expand.possible && (hasTanks || Players::getVisibleCount(PlayerState::Enemy, Terran_Machine_Shop) > 0) && Players::getVisibleCount(PlayerState::Enemy, Terran_Factory) <= 1 && Players::getVisibleCount(PlayerState::Enemy, Terran_Barracks) >= 3 && Util::getTime() < Time(10, 30))
                        theSpy.transition.name = "1FactTanks";
                    else if (theSpy.proxy.confirmed
                        || arrivesBy(3, Terran_Marine, Time(3, 10))
                        || completesBy(2, Terran_Barracks, Time(2, 35))
                        || completesBy(3, Terran_Barracks, Time(4, 00)))
                        theSpy.transition.name = "MarineRush";
                    else if (!theSpy.expand.possible && (completesBy(1, Terran_Academy, Time(5, 10)) || player.hasTech(TechTypes::Stim_Packs) || arrivesBy(1, Terran_Medic, Time(6, 00)) || arrivesBy(1, Terran_Firebat, Time(6, 00))))
                        theSpy.transition.name = "Academy";
                    else if (theSpy.expand.possible && Players::getVisibleCount(PlayerState::Enemy, Terran_Barracks) >= 5 && Util::getTime() < Time(7, 00))
                        theSpy.transition.name = "5Rax";
                    else if (Util::getTime() < Time(7, 00) && Players::getVisibleCount(PlayerState::Enemy, Terran_Academy) > 0 && theSpy.typeUpgrading.find(Terran_Engineering_Bay) != theSpy.typeUpgrading.end())
                        theSpy.transition.name = "Sparks";
                    else if (Util::getTime() < Time(5, 30) && (Players::getVisibleCount(PlayerState::Enemy, Terran_Medic) > 0 || Players::getVisibleCount(PlayerState::Enemy, Terran_Firebat) > 0))
                        theSpy.transition.name = "Academy";
                }

                // RaxCC
                if (theSpy.build.name == "RaxCC") {
                    if (theSpy.expand.possible && (hasTanks || Players::getVisibleCount(PlayerState::Enemy, Terran_Machine_Shop) > 0) && Players::getVisibleCount(PlayerState::Enemy, Terran_Factory) <= 1 && Players::getVisibleCount(PlayerState::Enemy, Terran_Barracks) >= 3 && Util::getTime() < Time(10, 30))
                        theSpy.transition.name = "1FactTanks";
                    else if (theSpy.expand.possible && Players::getVisibleCount(PlayerState::Enemy, Terran_Barracks) >= 5 && Util::getTime() < Time(7, 00))
                        theSpy.transition.name = "+1 5Rax";
                }

                // Goliaths builds
                if (completesBy(2, Terran_Goliath, Time(5, 00))
                    || completesBy(4, Terran_Goliath, Time(5, 30))
                    || completesBy(6, Terran_Goliath, Time(6, 00))
                    || completesBy(1, UpgradeTypes::Charon_Boosters, Time(7, 15)))
                    theSpy.transition.name = "3FactGoliath";
                if (completesBy(3, Terran_Goliath, Time(6, 15))
                    || completesBy(5, Terran_Goliath, Time(6, 45))
                    || completesBy(7, Terran_Goliath, Time(7, 15))
                    || completesBy(1, UpgradeTypes::Charon_Boosters, Time(8, 30)))
                    theSpy.transition.name = "5FactGoliath";
            }
        }
    }

    void updateTerran(StrategySpy& theSpy)
    {
        for (auto &p : Players::getPlayers()) {
            PlayerInfo &player = p.second;
            if (player.isEnemy() && player.getCurrentRace() == Races::Terran) {
                if (!theSpy.build.confirmed || theSpy.build.changeable)
                    enemyTerranBuilds(player, theSpy);
                if (!theSpy.opener.confirmed || theSpy.opener.changeable)
                    enemyTerranOpeners(player, theSpy);
                if (!theSpy.transition.confirmed || theSpy.transition.changeable)
                    enemyTerranTransitions(player, theSpy);
            }
        }
    }
}