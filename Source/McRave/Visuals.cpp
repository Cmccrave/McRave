#include "McRave.h"
#include <chrono>

using namespace BWAPI;
using namespace std;

namespace McRave::Visuals {

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
        bool roles = false;

        int gridSelection = 0;

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
                Broodwar->drawTextScreen(432, 28, "%cvs %c%s", Text::White, Text::Grey, Strategy::getEnemyBuild().c_str());
            }

            // Scores
            if (scores) {
                int offset = 0;
                for (auto &unit : Strategy::getUnitScores()) {
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

                multimap<double, string> sortMap;
                for (auto test : myTest) {
                    sortMap.emplace(test.second, test.first);
                }

                for (auto itr = sortMap.rbegin(); itr != sortMap.rend(); itr++) {
                    auto test = *itr;
                    if (test.first > 0.25) {
                        Broodwar->drawTextScreen(280, screenOffset, "%c%s", Text::White, test.second.c_str());
                        Broodwar->drawTextScreen(372, screenOffset, "%c%.5f ms", Text::White, test.first);
                        screenOffset += 10;
                    }
                }
            }

            // Resource
            if (resources) {
                for (auto &r : Resources::getMyMinerals()) {
                    auto &resource = *r;
                    Broodwar->drawTextMap(resource.getPosition() + Position(-8, 8), "%c%d", Text::GreyBlue, (*r).getRemainingResources());
                    Broodwar->drawTextMap(resource.getPosition() - Position(32, 8), "%cGatherers: %d", textColor, resource.getGathererCount());
                    Broodwar->drawTextMap(resource.getPosition() - Position(32, 16), "%cState: %d", textColor, (int)resource.getResourceState());

                }
                for (auto &r : Resources::getMyGas()) {
                    auto &resource = *r;
                    Broodwar->drawTextMap(resource.getPosition() + Position(-8, 8), "%c%d", Text::Green, (*r).getRemainingResources());
                    Broodwar->drawTextMap(resource.getPosition() - Position(32, 8), "%cGatherers: %d", textColor, resource.getGathererCount());
                    Broodwar->drawTextMap(resource.getPosition() - Position(32, 16), "%cState: %d", textColor, (int)resource.getResourceState());
                }
            }


            // BWEB
            if (bweb) {
                BWEB::Map::draw();
            }


            if (gridSelection != 0) {
                for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                    for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
                        WalkPosition w(x, y);

                        // If "draw mobility"
                        if (gridSelection == 1) {
                            auto gridColor = Colors::Black;
                            switch (Grids::getMobility(w)) {
                            case 1:
                                color = Colors::White;
                                break;
                            case 2:
                                color = Colors::Grey;
                                break;
                            case 3:
                                color = Colors::Red;
                                break;
                            case 4:
                                color = Colors::Orange;
                                break;
                            case 5:
                                color = Colors::Yellow;
                                break;
                            case 6:
                                color = Colors::Green;
                                break;
                            case 7:
                                color = Colors::Cyan;
                                break;
                            case 8:
                                color = Colors::Blue;
                                break;
                            case 9:
                                color = Colors::Purple;
                                break;
                            case 10:
                                color = Colors::Brown;
                                break;
                            }
                            Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, gridColor);
                        }

                        if (gridSelection == 2) {
                            if (Grids::getEAirThreat(w) > 0.0)
                                Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::White);
                        }
                        if (gridSelection == 3) {
                            if (Grids::getEGroundThreat(w) > 0.0)
                                Broodwar->drawCircleMap(Position(WalkPosition(x, y)) + Position(4, 4), 2, Colors::White);
                        }
                    }
                }
            }
        }

        void drawAllyInfo()
        {
            for (auto &u : Units::getUnits(PlayerState::Self)) {
                UnitInfo &unit = *u;

                int color = unit.getPlayer()->getColor();
                int textColor = color == 156 ? 17 : unit.getPlayer()->getTextColor();

                if (unit.unit()->isLoaded())
                    continue;

                if (targets) {
                    if (unit.hasResource())
                        Broodwar->drawLineMap(unit.getResource().getPosition(), unit.getPosition(), color);
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
                    int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
                    if (unit.getRole() == Role::Production && (unit.getType() == UnitTypes::Zerg_Egg || (unit.unit()->isTraining() && !unit.unit()->getTrainingQueue().empty()))) {
                        auto trainType = unit.getType() == UnitTypes::Zerg_Egg ? unit.unit()->getBuildType().c_str() : unit.unit()->getTrainingQueue().front().c_str();
                        auto trainOrder =  unit.getType() == UnitTypes::Zerg_Egg ? "Morphing" : "Training";
                        Broodwar->drawTextMap(unit.getPosition() + Position(width, -8), "%c%s", textColor, trainOrder);
                        Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", textColor, trainType);
                    }
                    else if (unit.unit() && unit.unit()->exists() && unit.unit()->isCompleted()) {
                        if (unit.unit()->getOrder() != Orders::Nothing)
                            Broodwar->drawTextMap(unit.getPosition() + Position(width, -8), "%c%s", textColor, unit.unit()->getOrder().c_str());
                        if (unit.unit()->isUpgrading())
                            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", textColor, unit.unit()->getUpgrade().c_str());
                        else if (unit.unit()->isResearching())
                            Broodwar->drawTextMap(unit.getPosition() + Position(width, 0), "%c%s", textColor, unit.unit()->getTech().c_str());
                    }
                }

                if (local) {
                    if (unit.getRole() == Role::Combat)
                        Broodwar->drawTextMap(unit.getPosition(), "%c%d", textColor, unit.getLocalState());
                }

                if (sim) {
                    if (unit.getRole() == Role::Combat) {
                        int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
                        Broodwar->drawTextMap(unit.getPosition() + Position(width, 8), "%c%.2f", Text::White, unit.getSimValue());
                    }
                }

                if (roles) {
                    int width = unit.getType().isBuilding() ? -16 : unit.getType().width() / 2;
                    Broodwar->drawTextMap(unit.getPosition() + Position(width, 16), "%c%d", Text::White, unit.getRole());
                }
            }
        }

        void drawEnemyInfo()
        {
            for (auto &u : Units::getUnits(PlayerState::Enemy)) {
                UnitInfo &unit = *u;

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
    }

    void startPerfTest()
    {
        start = chrono::high_resolution_clock::now();
    }

    void onSendText(string text)
    {
        if (text == "/targets")				targets = !targets;
        else if (text == "/sim")			sim = !sim;
        else if (text == "/strengths")		strengths = !strengths;
        else if (text == "/builds")			builds = !builds;
        else if (text == "/bweb")			bweb = !bweb;
        else if (text == "/paths")			paths = !paths;
        else if (text == "/orders")			orders = !orders;
        else if (text == "/local")			local = !local;
        else if (text == "/resources")		resources = !resources;
        else if (text == "/timers")			timers = !timers;
        else if (text == "/roles")          roles = !roles;

        else if (text == "/grids -o")        gridSelection = 0;
        else if (text == "/grids -m")        gridSelection = 1;
        else if (text == "/grids -a")        gridSelection = 2;
        else if (text == "/grids -g")        gridSelection = 3;

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

    void drawDebugText(std::string s, double d) {
        Broodwar->drawTextScreen(Position(0, 50 + screenOffset), "%s: %.2f", s.c_str(), d);
        screenOffset += 10;
    }

    void drawDebugText(std::string s, int i) {
        Broodwar->drawTextScreen(Position(0, 50 + screenOffset), "%s: %d", s.c_str(), i);
        screenOffset += 10;
    }
}