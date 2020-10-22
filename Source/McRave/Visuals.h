#pragma once
#include <BWAPI.h>

namespace McRave::Visuals {

    void onFrame();
    void startPerfTest();
    void endPerfTest(std::string);
    void onSendText(std::string);
    void displayPath(BWEB::Path&);


    void drawDebugText(std::string, double);
    void drawDebugText(std::string, int);

    void tileBox(BWAPI::TilePosition, BWAPI::Color, bool solid = false);
    void walkBox(BWAPI::WalkPosition, BWAPI::Color, bool solid = false);
};