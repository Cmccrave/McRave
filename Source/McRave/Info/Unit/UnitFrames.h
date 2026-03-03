#pragma once
#include <BWAPI.h>
#include "Main/Common.h"

namespace McRave {
    class UnitFrames {
    protected:
        int lastAttackFrame = -999;
        int lastRepairFrame = -999;
        int lastVisibleFrame = -999;
        int lastMoveFrame = -999;
        int lastStuckFrame = 0;
        int lastStimFrame = 0;
        int lastStateChangeFrame = 0;
        int threateningFrames = 0;
        int resourceHeldFrames = -999;
        int remainingTrainFrame = -999;
        int startedFrame = -999;
        int completeFrame = -999;
        int arriveFrame = -999;
        int minStopFrame = 0;

    public:
        void setRemainingTrainFrame(int newFrame) { remainingTrainFrame = newFrame; }

        int getLastAttackFrame() { return lastAttackFrame; }
        int getLastVisibleFrame() { return lastVisibleFrame; }
        int getRemainingTrainFrames() { return remainingTrainFrame; }
        int framesHoldingResource() { return resourceHeldFrames; }

        bool hasAttackedRecently() { return (BWAPI::Broodwar->getFrameCount() - lastAttackFrame < 120); }
        bool hasRepairedRecently() { return (BWAPI::Broodwar->getFrameCount() - lastRepairFrame < 120); }

        bool changedStateRecently() { return BWAPI::Broodwar->getFrameCount() - lastStateChangeFrame < 60; }

        bool isStale() { return BWAPI::Broodwar->getFrameCount() - lastVisibleFrame > 240; }
        bool isStimmed() { return BWAPI::Broodwar->getFrameCount() - lastStimFrame < 300; }
        bool isStuck() { return BWAPI::Broodwar->getFrameCount() - lastMoveFrame > 48; }

        bool wasStuckRecently() { return BWAPI::Broodwar->getFrameCount() - lastStuckFrame < 240; }

        // Information about frame timings
        int frameStartedWhen()
        {
            return startedFrame;
        }
        int frameCompletesWhen()
        {
            return completeFrame;
        }
        int frameArrivesWhen()
        {
            return arriveFrame;
        }
        Time timeStartedWhen()
        {
            int started = frameStartedWhen();
            return Time(started);
        }
        Time timeCompletesWhen()
        {
            int completes = frameCompletesWhen();
            return Time(completes);
        }
        Time timeArrivesWhen()
        {
            int arrival = frameArrivesWhen();
            return Time(arrival);
        }
    };
}