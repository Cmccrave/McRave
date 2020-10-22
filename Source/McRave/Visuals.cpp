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
        bool sim = false;
        bool paths = true;
        bool strengths = false;
        bool orders = false;
        bool local = false;
        bool resources = true;
        bool timers = true;
        bool scores = true;
        bool roles = false;

        int gridSelection = 0;

        void drawInformation()
        {
            // BWAPIs green text doesn't point to the right color so this will be see occasionally
            int color = Broodwar->self()->getColor();
            int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

            // Reset the screenOffset for the performance tests
            screenOffset = 0;

            // Builds
            if (builds) {
                Broodwar->drawTextScreen(432, 16, "%c%s: %c%s %s", Text::White, BuildOrder::getCurrentBuild().c_str(), Text::Grey, BuildOrder::getCurrentOpener().c_str(), BuildOrder::getCurrentTransition().c_str());
                Broodwar->drawTextScreen(432, 26, "%c%s: %c%s %s", Text::White, Strategy::getEnemyBuild().c_str(), Text::Grey, Strategy::getEnemyOpener().c_str(), Strategy::getEnemyTransition().c_str());
                Broodwar->drawTextScreen(160, 0, "%cExpanding", BuildOrder::shouldExpand() ? Text::White : Text::Grey);
                Broodwar->drawTextScreen(160, 10, "%cRamping", BuildOrder::shouldRamp() ? Text::White : Text::Grey);
                Broodwar->drawTextScreen(160, 20, "%cTeching", BuildOrder::getTechUnit() != UnitTypes::None ? Text::White : Text::Grey);
            }

            // Scores
            if (scores) {
                int offset = 0;
                for (auto &type : UnitTypes::allUnitTypes()) {
                    auto scoreColor = BuildOrder::isTechUnit(type) ? Text::White : Text::Grey;
                    if ((total(type) > 0 && !type.isBuilding()) || BuildOrder::getCompositionPercentage(type) > 0.00) {
                        Broodwar->drawTextScreen(0, offset, "%c%s  %d/%d  %.2f", scoreColor, type.c_str(), vis(type), total(type), BuildOrder::getCompositionPercentage(type));
                        offset += 10;
                    }
                }
                Broodwar->drawTextScreen(0, offset, "%c%s", Text::Grey, BuildOrder::getTechUnit().c_str());
            }

            // Timers
            if (timers) {
                int time = Broodwar->getFrameCount() / 24;
                int seconds = time % 60;
                int minute = time / 60;
                Broodwar->drawTextScreen(432, 36, "%c%d", Text::Grey, Broodwar->getFrameCount());
                Broodwar->drawTextScreen(482, 36, "%c%d:%02d", Text::Grey, minute, seconds);

                multimap<double, string> sortMap;
                for (auto test : myTest) {
                    sortMap.emplace(test.second, test.first);
                }

                for (auto itr = sortMap.rbegin(); itr != sortMap.rend(); itr++) {
                    auto test = *itr;
                    if (test.first > 0.25) {
                        Broodwar->drawTextScreen(280, screenOffset, "%c%s", Text::White, test.second.c_str());
                        Broodwar->drawTextScreen(372, screenOffset, "%c%.3f ms", Text::White, test.first);
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

                Broodwar->drawTextScreen(Position(4, 64), "RM: %d", Production::getReservedMineral());
                Broodwar->drawTextScreen(Position(4, 80), "RG: %d", Production::getReservedGas());
                Broodwar->drawTextScreen(Position(4, 96), "QM: %d", Buildings::getQueuedMineral());
                Broodwar->drawTextScreen(Position(4, 112), "QG: %d", Buildings::getQueuedGas());
            }


            // BWEB
            if (bweb) {
                BWEB::Map::draw();
            }

            if (gridSelection != 0) {
                for (int x = 0; x <= Broodwar->mapWidth() * 4; x++) {
                    for (int y = 0; y <= Broodwar->mapHeight() * 4; y++) {
                        WalkPosition w(x, y);

                        auto value = Grids::getMobility(w);

                        // If "draw mobility"
                        if (gridSelection == 1) {
                            auto gridColor = Colors::Black;

                            switch (value) {
                            case 1:
                                gridColor = Colors::White;
                                break;
                            case 2:
                                gridColor = Colors::Grey;
                                break;
                            case 3:
                                gridColor = Colors::Red;
                                break;
                            case 4:
                                gridColor = Colors::Orange;
                                break;
                            case 5:
                                gridColor = Colors::Yellow;
                                break;
                            case 6:
                                gridColor = Colors::Green;
                                break;
                            case 7:
                                gridColor = Colors::Cyan;
                                break;
                            case 8:
                                gridColor = Colors::Blue;
                                break;
                            case 9:
                                gridColor = Colors::Purple;
                                break;
                            case 10:
                                gridColor = Colors::Brown;
                                break;
                            }

                            walkBox(w, gridColor);
                        }

                        if (gridSelection == 2) {
                            if (Grids::getEGroundThreat(w) > 0.0)
                                walkBox(w, Color(int(Grids::getEGroundThreat(w) * 120), 0, 0));
                        }
                        if (gridSelection == 3) {
                            if (Grids::getEAirThreat(w) > 0.0)
                                walkBox(w, Colors::Black);
                        }
                        if (gridSelection == 4) {

                            if (Grids::getCollision(w) > 0)
                                walkBox(w, Colors::Black);
                        }
                        if (gridSelection == 5) {
                            if (Grids::getAGroundCluster(w) > 0.0)
                                walkBox(w, Colors::Black);
                        }
                        if (gridSelection == 6) {
                            if (Grids::getAGroundCluster(w) > 4.0f)
                                walkBox(w, Colors::Red);
                            else if (Grids::getAAirCluster(w) >= 0.9)
                                walkBox(w, Colors::Black);
                        }
                        if (gridSelection == 7) {
                            if (mapBWEM.GetMiniTile(w).Walkable())
                                walkBox(w, Colors::Cyan);
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
                int textColor = color == 185 ? textColor = Text::DarkGreen : unit.getPlayer()->getTextColor();

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
                int textColor = color == 185 ? textColor = Text::DarkGreen : unit.getPlayer()->getTextColor();

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

        void centerCameraOn(Position here)
        {
            Broodwar->setScreenPosition(here - Position(320, 180));
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
        if (text == "/targets")              targets = !targets;
        else if (text == "/sim")             sim = !sim;
        else if (text == "/strengths")       strengths = !strengths;
        else if (text == "/builds")          builds = !builds;
        else if (text == "/bweb")            bweb = !bweb;
        else if (text == "/paths")           paths = !paths;
        else if (text == "/orders")          orders = !orders;
        else if (text == "/local")           local = !local;
        else if (text == "/resources")       resources = !resources;
        else if (text == "/timers")          timers = !timers;
        else if (text == "/roles")           roles = !roles;

        else if (text == "/grids 0")        gridSelection = 0;
        else if (text == "/grids 1")        gridSelection = 1;
        else if (text == "/grids 2")        gridSelection = 2;
        else if (text == "/grids 3")        gridSelection = 3;
        else if (text == "/grids 4")        gridSelection = 4;
        else if (text == "/grids 5")        gridSelection = 5;
        else if (text == "/grids 6")        gridSelection = 6;
        else if (text == "/grids 7")        gridSelection = 7;

        else                                Broodwar->sendText("%s", text.c_str());
        return;
    }

    void displayPath(BWEB::Path& path)
    {
        int color = Broodwar->self()->getColor();
        if (paths && !path.getTiles().empty()) {
            TilePosition next = path.getSource();
            for (auto &tile : path.getTiles()) {
                if (next.isValid() && tile.isValid()) {
                    Broodwar->drawLineMap(Position(next) + Position(16, 16), Position(tile) + Position(16, 16), color);
                    Broodwar->drawCircleMap(Position(next) + Position(16, 16), 4, color, true);
                }

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

    void tileBox(TilePosition here, Color color, bool solid) {
        Broodwar->drawBoxMap(Position(here), Position(here) + Position(32, 32), color, solid);
    }

    void walkBox(WalkPosition here, Color color, bool solid) {
        Broodwar->drawBoxMap(Position(here), Position(here) + Position(8, 8), color, solid);
    }
}