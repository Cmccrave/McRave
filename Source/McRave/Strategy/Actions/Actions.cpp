#include "Main/McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Actions {

    namespace {

        bool draw = true;

        void drawActions()
        {
            // TODO
            if (draw) {
                for (auto &action : neutralActions) {
                    auto topLeft = action.pos - Position(48, 48);
                    auto botRight = action.pos + Position(48, 48);
                    Visuals::drawBox(topLeft, botRight, Colors::Blue);
                }
                for (auto &action : enemyActions) {
                    auto topLeft = action.pos - Position(48, 48);
                    auto botRight = action.pos + Position(48, 48);
                    Visuals::drawBox(topLeft, botRight, Colors::Red);
                }
                for (auto &action : myActions) {
                    auto topLeft = action.pos - Position(48, 48);
                    auto botRight = action.pos + Position(48, 48);
                    Visuals::drawBox(topLeft, botRight, Colors::Green);
                }
            }
        }

        void updateActions()
        {
            // Clear cache
            neutralActions.clear();
            myActions.clear();
            allyActions.clear();
            enemyActions.clear();

            const auto trackable = [&](TechType type) {
                return type == TechTypes::Dark_Swarm
                    || type == TechTypes::Disruption_Web
                    || type == TechTypes::EMP_Shockwave
                    || type == TechTypes::Ensnare
                    || type == TechTypes::Irradiate
                    || type == TechTypes::Maelstrom
                    || type == TechTypes::Nuclear_Strike
                    || type == TechTypes::Plague
                    || type == TechTypes::Psionic_Storm;
            };

            const auto whatTech = [&](Order order) {
                switch (order) {
                case Orders::CastEMPShockwave:
                    return TechTypes::EMP_Shockwave;
                case Orders::CastDarkSwarm:
                    return TechTypes::Dark_Swarm;
                case Orders::CastDisruptionWeb:
                    return TechTypes::Disruption_Web;
                case Orders::CastPsionicStorm:
                    return TechTypes::Psionic_Storm;
                }
                return TechTypes::None;
            };

            // Check bullet based abilities, store as neutral PlayerState as it affects both sides
            for (auto &b : Broodwar->getBullets()) {
                if (b && b->exists()) {
                    if (b->getType() == BulletTypes::EMP_Missile)
                        addAction(nullptr, b->getTargetPosition(), TechTypes::EMP_Shockwave, PlayerState::Neutral, Util::getCastRadius(TechTypes::EMP_Shockwave));
                    if (b->getType() == BulletTypes::Psionic_Storm)
                        addAction(nullptr, b->getPosition(), TechTypes::Psionic_Storm, PlayerState::Neutral, Util::getCastRadius(TechTypes::Psionic_Storm));
                }
            }

            // Check nuke dots, store as neutral PlayerState as it affects both sides
            for (auto &dot : Broodwar->getNukeDots())
                addAction(nullptr, dot, TechTypes::Nuclear_Strike, PlayerState::Neutral, Util::getCastRadius(TechTypes::Nuclear_Strike));

            // Check enemy Actions, store outgoing bullet based abilities as neutral PlayerState
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;

                if (!unit.unit() || (unit.unit()->exists() && (unit.unit()->isLockedDown() || unit.unit()->isMaelstrommed() || unit.unit()->isStasised() || !unit.unit()->isCompleted())))
                    continue;

                // TODO: Sight range != detection range
                if (unit.getType().isDetector())
                    addAction(unit.unit(), unit.getPosition(), unit.getType(), PlayerState::Enemy, unit.getType().sightRange() / 2);

                // Add action for previous orders that may or may not physically exist yet
                for (auto &[frame, order] : unit.getOrderHistory()) {
                    auto techUsed = whatTech(order.first);
                    if (frame >= Broodwar->getFrameCount() - 7 && trackable(techUsed))
                        addAction(unit.unit(), order.second, techUsed, PlayerState::Neutral, Util::getCastRadius(techUsed));
                }

                // Spider mines are hard to dodge abruptly
                if (unit.getType() == Terran_Vulture_Spider_Mine && unit.hasTarget()) {
                    addAction(unit.unit(), unit.getTarget().lock()->getPosition(), UnitTypes::Terran_Vulture_Spider_Mine, PlayerState::Neutral, Util::getCastRadius(TechTypes::Spider_Mines));
                    addAction(unit.unit(), unit.getPosition(), UnitTypes::Terran_Vulture_Spider_Mine, PlayerState::Neutral, Util::getCastRadius(TechTypes::Spider_Mines));
                }

                // Scarabs eventually go to their target
                // TODO: Hardcoded the radius at 30
                if (unit.getType() == Protoss_Scarab && unit.hasTarget())
                    addAction(unit.unit(), unit.getTarget().lock()->getPosition(), UnitTypes::Protoss_Scarab, PlayerState::Enemy, 30);
            }

            // Check my Actions
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;

                if (unit.getType().isDetector() && unit.unit()->isCompleted())
                    addAction(unit.unit(), unit.getPosition(), unit.getType(), PlayerState::Self, unit.getType().sightRange() / 2);

                if (Broodwar->getFrameCount() - unit.unit()->getLastCommandFrame() < 7 && unit.unit()->getLastCommand().getTechType() == TechTypes::Scanner_Sweep)
                    addAction(unit.unit(), unit.unit()->getLastCommand().getTargetPosition(), Spell_Scanner_Sweep, PlayerState::Self, Util::getCastRadius(TechTypes::Scanner_Sweep));

                // Add action for previous orders that may or may not physically exist yet
                for (auto &[frame, order] : unit.getOrderHistory()) {
                    auto techUsed = whatTech(order.first);
                    if (frame >= Broodwar->getFrameCount() - 7 && trackable(techUsed))
                        addAction(unit.unit(), order.second, techUsed, PlayerState::Neutral, Util::getCastRadius(techUsed));
                }

                // Spider mines are hard to dodge abruptly
                if (unit.getType() == Terran_Vulture_Spider_Mine && unit.hasTarget()) {
                    addAction(unit.unit(), unit.getTarget().lock()->getPosition(), UnitTypes::Terran_Vulture_Spider_Mine, PlayerState::Neutral, Util::getCastRadius(TechTypes::Spider_Mines));
                    addAction(unit.unit(), unit.getPosition(), UnitTypes::Terran_Vulture_Spider_Mine, PlayerState::Neutral, Util::getCastRadius(TechTypes::Spider_Mines));
                }
            }

            // Check neutral actions
            for (auto &u : Units::getUnits(PlayerState::Neutral)) {
                UnitInfo &unit = *u;

                if (unit.getType() == Spell_Dark_Swarm)
                    addAction(unit.unit(), unit.getPosition(), TechTypes::Dark_Swarm, PlayerState::Neutral, Util::getCastRadius(TechTypes::Dark_Swarm));
                if (unit.getType() == Spell_Disruption_Web)
                    addAction(unit.unit(), unit.getPosition(), TechTypes::Disruption_Web, PlayerState::Neutral, Util::getCastRadius(TechTypes::Dark_Swarm));
            }
        }

        bool boxOverlap(Action& action, vector<Position>& checkPositions, int distance)
        {
            auto topLeft = action.pos - BWAPI::Position(distance, distance);
            auto botRight = action.pos + BWAPI::Position(distance, distance);

            // Bounding box of current Action
            for (auto &position : checkPositions) {
                if (Util::rectangleIntersect(topLeft, botRight, position))
                    return true;
            }
            return false;
        }

        bool circleOverlap(Action& action, vector<Position>& checkPositions, int distance)
        {
            // Rough circle of current Action
            for (auto &position : checkPositions) {
                if (action.pos.getDistance(position) <= distance)
                    return true;
            }
            return false;
        }
    }

    bool isInDanger(UnitInfo& unit, Position here)
    {
        const int hW = int(ceil(unit.getType().width() / 2));
        const int hH = int(ceil(unit.getType().height() / 2));

        if (here == Positions::Invalid)
            here = unit.getPosition();

        vector<Position> checkPositions ={
            {here + Position(-hW, -hH)},
            {here + Position(hW, -hH)},
            {here + Position(-hW, hH)},
            {here + Position(hW, hH)} };

        // Check that we're not in danger of Storm, DWEB, EMP
        const auto checkDangers = [&](vector<Action>& actions) {
            for (auto &command : actions) {

                if (!unit.isFlying() && (command.type == UnitTypes::Protoss_Scarab || command.type == UnitTypes::Terran_Vulture_Spider_Mine)) {
                    if (circleOverlap(command, checkPositions, 64))
                        return true;
                }

                // Made these bigger to help kiting
                if (command.tech == TechTypes::Psionic_Storm
                    || command.tech == TechTypes::Disruption_Web) {

                    if (boxOverlap(command, checkPositions, 60))
                        return true;
                }

                if (command.tech == TechTypes::Stasis_Field
                    || command.tech == TechTypes::EMP_Shockwave) {

                    if (boxOverlap(command, checkPositions, 48))
                        return true;
                }

                if (command.tech == TechTypes::Nuclear_Strike) {
                    if (circleOverlap(command, checkPositions, 160))
                        return true;
                }
            }
            return false;
        };

        return checkDangers(neutralActions) || checkDangers(enemyActions);
    }

    bool overlapsDetection(Unit unit, Position here, PlayerState player)
    {
        vector<Action>* actions = getActions(player);

        if (!actions)
            return false;

        // Detection checks use a radial check
        for (auto &action : *actions) {
            if (action.type == Spell_Scanner_Sweep) {
                if (action.pos.getDistance(here) < 420.0)
                    return true;
            }
            else if (action.unit != unit && action.type.isDetector()) {
                double range = action.type.isBuilding() ? 224.0 : action.type.sightRange();
                if (action.pos.getDistance(here) < range)
                    return true;
            }
        }
        return false;
    }

    bool overlapsActions(Unit unit, Position here, UnitType type, PlayerState player, int distance)
    {
        vector<Action>* actions = getActions(player);
        vector<Position> checkPositions ={ {here} };

        if (!actions)
            return false;

        // Check outgoing UnitType Actions for this PlayerState
        for (auto &action : *actions) {
            if (action.unit == unit || (type != None && type != action.type))
                continue;
            if (circleOverlap(action, checkPositions, distance))
                return true;
        }
        return false;
    }

    bool overlapsActions(Unit unit, Position here, TechType type, PlayerState player, int distance)
    {
        vector<Action>* actions = getActions(player);

        if (!actions)
            return false;

        const auto hD = distance / 2;

        // Grab all positions we want to check
        vector<Position> checkPositions ={
            {here + Position(-hD, -hD)},
            {here + Position(hD, -hD)},
            {here + Position(-hD, hD)},
            {here + Position(hD, hD)} };

        // Check outgoing TechType Actions for this PlayerState
        for (auto &action : *actions) {
            if (action.unit == unit || action.type != type)
                continue;

            if (action.tech == TechTypes::Stasis_Field || action.tech == TechTypes::EMP_Shockwave || action.tech == TechTypes::Dark_Swarm) {
                if (boxOverlap(action, checkPositions, Util::getCastRadius(action.tech)))
                    return true;
            }
            else if (circleOverlap(action, checkPositions, Util::getCastRadius(action.tech)))
                return true;
        }

        // Check outgoing TechType Actions for neutral PlayerState
        for (auto &action : neutralActions) {
            if (action.unit == unit || action.type != type)
                continue;

            if (action.tech == TechTypes::Stasis_Field || action.tech == TechTypes::EMP_Shockwave || action.tech == TechTypes::Dark_Swarm) {
                if (boxOverlap(action, checkPositions, Util::getCastRadius(action.tech)))
                    return true;
            }
            else if (circleOverlap(action, checkPositions, Util::getCastRadius(action.tech)))
                return true;
        }
        return false;
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        drawActions();
        updateActions();
        Visuals::endPerfTest("Actions");
    }

    vector <Action>* getActions(PlayerState player) {
        if (player == PlayerState::Enemy)
            return &enemyActions;
        if (player == PlayerState::Self)
            return &myActions;
        if (player == PlayerState::Ally)
            return &allyActions;
        if (player == PlayerState::Neutral)
            return &neutralActions;
        return nullptr;
    }
}