#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

namespace McRave
{
    class UnitInfo;
    class InterfaceManager
    {
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


        void drawAllyInfo(), drawEnemyInfo(), drawInformation();
    public:
        void onFrame(), startClock(), performanceTest(string), onSendText(string);
        void displayPath(vector<TilePosition>);
    };
}

typedef Singleton<McRave::InterfaceManager> InterfaceSingleton;