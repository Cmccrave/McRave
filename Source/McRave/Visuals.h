#pragma once
#include <BWAPI.h>

namespace McRave::Visuals
{
    void onFrame();
    void startPerfTest();
    void endPerfTest(string);
    void onSendText(string);
    void displayPath(vector<TilePosition>);
};