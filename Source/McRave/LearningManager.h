#pragma once
#include <BWAPI.h>
#include <sstream>
#include <set>

namespace McRave::Learning {

    // Is our bots learning?
    struct Build {
        std::vector <std::string> transitions;
        std::vector <std::string> openers;
    };

    enum class LearningToken {
        None, Build, Opener, Transition
    };

    void onEnd(bool);
    void onStart();
}