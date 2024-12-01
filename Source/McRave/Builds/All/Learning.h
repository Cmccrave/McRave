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

        void setOpeners(std::vector<std::string_view> newOpeners) {
            for (auto &opener : newOpeners)
                openers.push_back(BuildComponent(static_cast<std::string>(opener)));
        }

        void setTransitions(std::vector<std::string_view> newTransitions) {
            for (auto &transition : newTransitions)
                transitions.push_back(BuildComponent(static_cast<std::string>(transition)));
        }

        Build(std::string_view _name) {
            name = static_cast<std::string>(_name);
        }
    };

    std::vector<Build>& getBuilds();
    void onEnd(bool);
    void onStart();
}