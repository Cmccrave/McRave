#pragma once
#include <BWAPI.h>

namespace McRave::Resources
{
    int getMineralCount();
    int getGasCount();
    int getIncomeMineral();
    int getIncomeGas();
    bool isMineralSaturated();
    bool isGasSaturated();
    bool isHalfMineralSaturated();
    bool isHalfGasSaturated();
    std::set <std::shared_ptr<ResourceInfo>>& getMyMinerals();
    std::set <std::shared_ptr<ResourceInfo>>& getMyGas();
    std::set <std::shared_ptr<ResourceInfo>>& getMyBoulders();

    void recheckSaturation();
    void onFrame();
    void onStart();
    void onEnd();
    void storeResource(BWAPI::Unit);
    void removeResource(BWAPI::Unit);

    std::shared_ptr<ResourceInfo> getResourceInfo(BWAPI::Unit);

    template<typename F>
    ResourceInfo* getClosestMineral(BWAPI::Position here, F &&pred) {
        auto distBest = DBL_MAX;
        auto &minerals = Resources::getMyMinerals();
        ResourceInfo* best = nullptr;

        for (auto &mineral : minerals) {
            if (!pred(mineral))
                continue;

            auto dist = here.getDistance(mineral->getPosition());
            if (dist < distBest) {
                best = &*mineral;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    ResourceInfo* getClosestGas(BWAPI::Position here, F &&pred) {
        auto distBest = DBL_MAX;
        auto &gases = Resources::getMyGas();
        ResourceInfo* best = nullptr;

        for (auto &gas : gases) {
            if (!pred(gas))
                continue;

            auto dist = here.getDistance(gas->getPosition());
            if (dist < distBest) {
                best = &*gas;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    ResourceInfo* getClosestBoulder(BWAPI::Position here, F &&pred) {
        auto distBest = DBL_MAX;
        auto &boulders = Resources::getMyBoulders();
        ResourceInfo* best = nullptr;

        for (auto &boulder : boulders) {
            if (!pred(boulder))
                continue;

            auto dist = here.getDistance(boulder->getPosition());
            if (dist < distBest) {
                best = &*boulder;
                distBest = dist;
            }
        }
        return best;
    }

    template<typename F>
    ResourceInfo* getClosestResource(BWAPI::Position here, F &&pred) {
        auto distBest = DBL_MAX;
        ResourceInfo* best = nullptr;

        for (auto &gas : Resources::getMyGas()) {
            if (!pred(gas))
                continue;

            auto dist = here.getDistance(gas->getPosition());
            if (dist < distBest) {
                best = &*gas;
                distBest = dist;
            }
        }
        for (auto &mineral : Resources::getMyMinerals()) {
            if (!pred(mineral))
                continue;

            auto dist = here.getDistance(mineral->getPosition());
            if (dist < distBest) {
                best = &*mineral;
                distBest = dist;
            }
        }
        for (auto &boulder : Resources::getMyBoulders()) {
            if (!pred(boulder))
                continue;

            auto dist = here.getDistance(boulder->getPosition());
            if (dist < distBest) {
                best = &*boulder;
                distBest = dist;
            }
        }
        return best;
    }
}
