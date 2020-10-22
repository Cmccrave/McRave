#include "McRave.h"

using namespace BWAPI;
using namespace std;
using namespace UnitTypes;

namespace McRave::Resources {

    namespace {

        set<shared_ptr<ResourceInfo>> myMinerals;
        set<shared_ptr<ResourceInfo>> myGas;
        set<shared_ptr<ResourceInfo>> myBoulders;
        bool minSat, gasSat, halfMinSat, halfGasSat;
        int miners, gassers;
        int minCount, gasCount;
        int incomeMineral, incomeGas;

        void updateIncome(const shared_ptr<ResourceInfo>& r)
        {
            auto &resource = *r;
            auto cnt = resource.getGathererCount();
            if (resource.getType().isMineralField())
                incomeMineral += cnt == 1 ? 65 : 126;
            else
                incomeGas += resource.getRemainingResources() ? 103 * cnt : 26 * cnt;
        }

        void updateInformation(const shared_ptr<ResourceInfo>& r)
        {
            auto &resource = *r;
            if (resource.unit()->exists())
                resource.updateResource();

            UnitType geyserType = Broodwar->self()->getRace().getRefinery();

            // If resource is blocked from usage
            if (resource.getType().isRefinery() && resource.getTilePosition().isValid()) {
                for (auto block = mapBWEM.GetTile(resource.getTilePosition()).GetNeutral(); block; block = block->NextStacked()) {
                    if (block && block->Unit() && block->Unit()->exists() && block->Unit()->isInvincible() && !block->IsGeyser())
                        resource.setResourceState(ResourceState::None);
                }
            }

            // Update resource state
            if (resource.hasStation()) {
                auto base = Util::getClosestUnit(resource.getPosition(), PlayerState::Self, [&](auto &u) {
                    return u.getType().isResourceDepot() && u.getPosition() == resource.getStation()->getBase()->Center();
                });

                if (base)
                    (base->unit()->isCompleted() || base->getType() == Zerg_Lair || base->getType() == Zerg_Hive) ? resource.setResourceState(ResourceState::Mineable) : resource.setResourceState(ResourceState::Assignable);
                else
                    resource.setResourceState(ResourceState::None);
            }

            // Update saturation            
            if (resource.getType().isMineralField() && resource.getResourceState() != ResourceState::None)
                miners += resource.getGathererCount();
            else if (resource.getType() == geyserType && resource.unit()->isCompleted() && resource.getResourceState() != ResourceState::None)
                gassers += resource.getGathererCount();

            if (resource.getResourceState() == ResourceState::Mineable
                || (resource.getResourceState() == ResourceState::Assignable && Stations::getMyStations().size() >= 3 && !Players::vP()))
                resource.getType().isMineralField() ? minCount++ : gasCount++;
        }

        void updateResources()
        {
            minCount = 0;
            gasCount = 0;
            miners = 0;
            gassers = 0;

            const auto update = [&](const shared_ptr<ResourceInfo>& r) {
                updateInformation(r);
                updateIncome(r);
            };

            for (auto &r : myBoulders)
                update(r);

            for (auto &r : myMinerals)
                update(r);

            for (auto &r : myGas)
                update(r);

            minSat = miners >= minCount * 2;
            halfMinSat = miners >= minCount;
            gasSat = gassers >= gasCount * 3;
            halfGasSat = gassers >= gasCount;
        }
    }

    void recheckSaturation()
    {
        miners = 0;
        gassers = 0;

        for (auto &r : myMinerals) {
            auto &resource = *r;
            miners += resource.getGathererCount();
        }

        for (auto &r : myGas) {
            auto &resource = *r;
            gassers += resource.getGathererCount();
        }

        auto saturatedAt = Broodwar->self()->getRace() == Races::Zerg ? 1 : 2;
        minSat = (miners >= minCount * saturatedAt);
        gasSat = (gassers >= gasCount * 3);
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateResources();
        Visuals::endPerfTest("Resources");
    }

    void storeResource(Unit resource)
    {
        auto info = ResourceInfo(resource);
        auto &resourceList = (resource->getResources() > 50 ? (resource->getType().isMineralField() ? myMinerals : myGas) : myBoulders);

        // Check if we already stored this resource
        for (auto &u : resourceList) {
            if (u->unit() == resource)
                return;
        }

        // Add station
        auto newStation = BWEB::Stations::getClosestStation(resource->getTilePosition());
        info.setStation(newStation);
        if (Stations::ownedBy(newStation) == PlayerState::Self)
            info.setResourceState(ResourceState::Assignable);

        auto ptr = make_shared<ResourceInfo>(info);
        resourceList.insert(make_shared<ResourceInfo>(info));
    }

    void removeResource(Unit unit)
    {
        auto &resource = getResourceInfo(unit);

        if (resource) {

            // Remove assignments
            for (auto &u : resource->targetedByWhat()) {
                if (!u.expired())
                    u.lock()->setResource(nullptr);
            }

            // Remove dead resources
            if (myMinerals.find(resource) != myMinerals.end())
                myMinerals.erase(resource);
            else if (myBoulders.find(resource) != myBoulders.end())
                myBoulders.erase(resource);
            else if (myGas.find(resource) != myGas.end())
                myGas.erase(resource);
        }
    }

    shared_ptr<ResourceInfo> getResourceInfo(BWAPI::Unit unit)
    {
        for (auto &m : myMinerals) {
            if (m->unit() == unit)
                return m;
        }
        for (auto &b : myBoulders) {
            if (b->unit() == unit)
                return b;
        }
        for (auto &g : myGas) {
            if (g->unit() == unit)
                return g;
        }
        return nullptr;
    }

    int getMinCount() { return minCount; }
    int getGasCount() { return gasCount; }
    int getIncomeMineral() { return incomeMineral; }
    int getIncomeGas() { return incomeGas; }
    bool isMinSaturated() { return minSat; }
    bool isHalfMinSaturated() { return halfMinSat; }
    bool isGasSaturated() { return gasSat; }
    bool isHalfGasSaturated() { return halfGasSat; }
    set<shared_ptr<ResourceInfo>>& getMyMinerals() { return myMinerals; }
    set<shared_ptr<ResourceInfo>>& getMyGas() { return myGas; }
    set<shared_ptr<ResourceInfo>>& getMyBoulders() { return myBoulders; }
}