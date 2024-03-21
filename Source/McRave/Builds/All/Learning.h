#pragma once
#include <BWAPI.h>
#include <sstream>
#include <set>

namespace McRave::Learning {
    struct BuildComponent {
        int w = 0;
        int l = 0;
        double ucb1 = 0.0;
        std::string name = "";
        BuildComponent(std::string _name) {
            name = _name;
        }
    };

    struct Build {
        int w = 0;
        int l = 0;
        double ucb1 = 0.0;
        std::string name = "";
        std::vector<BuildComponent> openers, transitions;
        BuildComponent * getComponent(std::string name) {
            for (auto &opener : openers) {
                if (opener.name == name)
                    return &opener;
            }
            for (auto &transition : transitions) {
                if (transition.name == name)
                    return &transition;
            }
            return nullptr;
        }

        void setOpeners(std::vector<std::string> newOpeners) {
            for (auto &opener : newOpeners)
                openers.push_back(BuildComponent(opener));
        }

        void setTransitions(std::vector<std::string> newTransitions) {
            for (auto &transition : newTransitions)
                transitions.push_back(BuildComponent(transition));
        }

        Build(std::string _name) {
            name = _name;
        }
    };

    std::vector<Build>& getBuilds();
    void onEnd(bool);
    void onStart();
}