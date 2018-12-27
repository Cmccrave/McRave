#include "McRave.h"

namespace McRave::Visuals
{
    namespace {
        chrono::steady_clock::time_point start;
        int screenOffset = 0;
        map <string, double> myTest;

        bool targets = false;
        bool builds = true;
        bool bweb = false;
        bool sim = true;
        bool paths = true;
        bool strengths = false;
        bool orders = true;
        bool local = false;
        bool resources = false;
        bool timers = true;
        bool scores = true;

        void drawInformation()
        {
            // BWAPIs orange text doesn't point to the right color so this will be see occasionally
            int color = Broodwar->self()->getColor();
            int textColor = color == 156 ? 17 : Broodwar->self()->getTextColor();

            // Reset the screenOffset for the performance tests
            screenOffset = 0;

            //WalkPosition mouse(Broodwar->getMousePosition() + Broodwar->getScreenPosition());
            //Broodwar->drawTextScreen(Broodwar->getMousePosition() - Position(0, 16), "%d", mapBWEM.GetMiniTile(mouse).Altitude());

            // Builds
            if (builds) {
                Broodwar->drawTextScreen(432, 16, "%c%s: %c%s %s", Text::White, BuildOrder::getCurrentBuild().c_str(), Text::Grey, BuildOrder::getCurrentOpener().c_str(), BuildOrder::getCurrentTransition().c_str());
                Broodwar->drawTextScreen(432, 28, "%cvs %c%s", Text::White, Text::Grey, Strategy().getEnemyBuild().c_str());
            }

            // Scores
            if (scores) {
                int offset = 0;
                for (auto &unit : Strategy().getUnitScores()) {
                    if (unit.first.isValid() && unit.second > 0.0) {
                        Broodwar->drawTextScreen(0, offset, "%c%s: %c%.2f", textColor, unit.first.c_str(), Text::White, unit.second);
                        offset += 10;
                    }
                }
            }

            // Timers
            if (timers) {
                int time = Broodwar->getFrameCount() / 24;
                int seconds = time % 60;
                int minute = time / 60;
                Broodwar->drawTextScreen(432, 40, "%c%d", Text::Grey, Broodwar->getFrameCount());
                Broodwar->drawTextScreen(482, 40, "%c%d:%02d", Text::Grey, minute, seconds);
            }

            // Resource
            if (resources) {
                for (auto &r : Resources::getMyMinerals()) {
                    auto &resource = r.second;
                    Broodwar->drawTextMap(resource.getPosition() + Position(-8, 8), "%c%d", Text::GreyBlue, r.second.getRemainingResources());
                    Broodwar->drawTextMap(resource.getPosition() - Position(32, 8), "%cGatherers: %d", textColor, resource.getGathererCount());
                }
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = r.second;
                    Broodwar->drawTextMap(resource.getPosition() + Position(-8, 8), "%c%d", Text::Green, r.second.getRemainingResources());
                    Broodwar->drawTextMap(resource.getPosition() - Position(32, 8), "%cGatherers: %d", textColor, resource.getGathererCount());
                }
            }


            // BWEB
            if (bweb) {
                BWEB::Map::draw();
            }
        }

        void drawAllyInfo()
        {
            for (auto &u : Units().getMyUnits()) {
                UnitInfo &unit = u.second;

                int color = unit.getPlayer()->getColor();
                int textColor = color == 156 ? 17 : unit.getPlayer()->getTextColor();

                if (unit.unit()->isLoaded())
                    continue;

                if (targets) {
                    if (unit.hasResource())
                        Broodwar->drawLineMap(unit.getResource().getPosition(), unit.getPosition(), color);
                    else if (unit.hasTarget())
                        Broodwar->drawLineMap(unit.getTarget().getPosition(), unit.getPosition(), color);
                }

                if (strengths) {
                    if (unit.getVisibleGroundStrength() > 0.0 || unit.getVisibleAirStrength() > 0.0) {
                        Broodwar->drawTextMap(unit.getPosition() + Position(5, -10), "Grd: %c %.2f", Text::Brown, unit.getVisibleGroundStrength());
                        Broodwar->drawTextMap(unit.getPosition() + Position(5, 2), "Air: %c %.2f", Text::Blue, unit.getVisibleAirStrength());
                    }
                }

                if (orders) {
                    int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
                    if (unit.unit() && unit.unit()->exists())
                        Broodwar->drawTextMap(unit.getPosition() + Position(width, -8), "%c%s", textColor, unit.unit()->getOrder().c_str());
                }

                if (local) {
                    Broodwar->drawTextMap(unit.getPosition(), "%c%s", textColor, unit.getLocalState());
                }

                if (sim) {
                    int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
                    Broodwar->drawTextMap(unit.getPosition() + Position(width, 8), "%c%.2f", Text::White, unit.getSimValue());
                }

                if (true) {
                    int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
                    Broodwar->drawTextMap(unit.getPosition() + Position(width, 16), "%c%d", Text::White, unit.getRole());
                }
            }
        }

        void drawEnemyInfo()
        {
            for (auto &u : Units().getEnemyUnits()) {
                UnitInfo &unit = u.second;

                int color = unit.getPlayer()->getColor();
                int textColor = color == 156 ? 17 : unit.getPlayer()->getTextColor();

                if (targets) {
                    if (unit.hasTarget())
                        Broodwar->drawLineMap(unit.getTarget().getPosition(), unit.getPosition(), color);
                }

                if (strengths) {
                    if (unit.getVisibleGroundStrength() > 0.0 || unit.getVisibleAirStrength() > 0.0) {
                        Broodwar->drawTextMap(unit.getPosition() + Position(5, -10), "Grd: %c %.2f", Text::Brown, unit.getVisibleGroundStrength());
                        Broodwar->drawTextMap(unit.getPosition() + Position(5, 2), "Air: %c %.2f", Text::Blue, unit.getVisibleAirStrength());
                    }
                }

                if (orders) {
                    if (unit.unit() && unit.unit()->exists())
                        Broodwar->drawTextMap(unit.getPosition(), "%c%s", textColor, unit.unit()->getOrder().c_str());
                }
            }
        }
    }

    void onFrame()
    {
        drawInformation();
        drawAllyInfo();
        drawEnemyInfo();
    }

    void endPerfTest(string function)
    {
        double dur = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
        myTest[function] = myTest[function] * 0.99 + dur * 0.01;
        if (myTest[function] > 0.2) {
            Broodwar->drawTextScreen(180, screenOffset, "%c%s", Text::White, function);
            Broodwar->drawTextScreen(372, screenOffset, "%c%.2f ms", Text::White, myTest[function]);
            screenOffset += 10;
        }
    }

    void startPerfTest()
    {
        start = chrono::high_resolution_clock::now();
    }

    void onSendText(string text)
    {
        if (text == "/targets")				targets = !targets;
        else if (text == "/sim")			sim = !sim;
        else if (text == "/strength")		strengths = !strengths;
        else if (text == "/builds")			builds = !builds;
        else if (text == "/bweb")			bweb = !bweb;
        else if (text == "/paths")			paths = !paths;
        else if (text == "/orders")			orders = !orders;
        else if (text == "/local")			local = !local;
        else if (text == "/resources")		resources = !resources;
        else if (text == "/timers")			timers = !timers;
        else								Broodwar->sendText("%s", text.c_str());
        return;
    }

    void displayPath(vector<TilePosition> path)
    {
        int color = Broodwar->self()->getColor();
        if (paths && !path.empty()) {
            TilePosition next = TilePositions::Invalid;
            for (auto &tile : path) {

                if (tile == *(path.begin()))
                    continue;

                if (next.isValid() && tile.isValid())
                    Broodwar->drawLineMap(Position(next) + Position(16, 16), Position(tile) + Position(16, 16), color);

                next = tile;
            }
        }
    }
}