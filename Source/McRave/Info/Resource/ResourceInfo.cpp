#include "ResourceInfo.h"

#include "Info/Player/Players.h"
#include "Info/Unit/UnitInfo.h"
#include "Info/Unit/Units.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave {

    static vector<TilePosition> directions = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 1}, {2, -1}, {2, 0}, {2, 1}};

    void ResourceInfo::updatePocketStatus()
    {
        if (!getType().isMineralField())
            return;

        // Determine if this resource is a pocket resource, useful to deter choosing a building worker (they get stuck)
        vector<TilePosition> pocketTiles;
        for (auto &dir : directions) {
            auto tile   = tilePosition + dir;
            auto center = Position(tile) + Position(16, 16);

            // If the build type is empty or somehow a resource depot is here (FPM possibility)
            auto buildType = BWEB::Map::isUsed(tile);
            if (buildType == UnitTypes::None || buildType.isResourceDepot()) {
                if (center.getDistance(getStation()->getBase()->Center()) < position.getDistance(getStation()->getBase()->Center()) - 16.0)
                    pocketTiles.push_back(tile);
            }
        }
        pocket = int(pocketTiles.size()) <= 1;
    }

    void ResourceInfo::updateThreatened()
    {
        threatened = false;
        if (safe && station->isMain())
            return;

        // Determine if this resource is threatened based on a substantial threat nearby
        for (auto &enemy : Units::getUnits(PlayerState::Enemy)) {
            if (enemy->getType().isWorker())
                continue;

            if (!enemy->hasTarget() || (!Players::ZvZ() && !enemy->getTarget().lock()->getType().isWorker()) || !enemy->isCompleted() || !enemy->canAttackGround())
                continue;

            const auto inRangeOfResource = enemy->getPosition().getDistance(position) < max(200.0, enemy->getGroundReach());
            if (inRangeOfResource && enemy->isThreatening())
                threatened = true;
        }

        // Sometimes we need to just not mine minerals that would just lose workers
        if (Players::ZvZ()) {
            if (Spy::enemyPressure() && Util::getTime() < Time(6, 00) && station && station->isNatural())
                threatened = true;
        }
    }

    void ResourceInfo::updateWorkerCap()
    {
        // Calculate the worker cap for the resource
        // Default 2 for mineral, 3 for gas
        workerCap = 2 + !type.isMineralField();
        for (auto &t : targetedBy) {
            if (t.expired())
                continue;

            if (auto targeter = t.lock()) {
                if (targeter->unit()->isCarryingGas() || targeter->unit()->isCarryingMinerals())
                    framesPerTrips[t]++;
                else if (Util::boxDistance(type, position, targeter->getType(), targeter->getPosition()) < 16.0)
                    framesPerTrips[t] = 0;

                if (!type.isMineralField() && framesPerTrips[t] > 52)
                    workerCap = 4;
                if (targeter->getBuildPosition().isValid())
                    workerCap++;
            }
        }
    }

    void ResourceInfo::updateResource()
    {
        if (thisUnit->exists()) {
            type               = thisUnit->getType();
            remainingResources = thisUnit->getResources();
            position           = thisUnit->getPosition();
            tilePosition       = thisUnit->getTilePosition();
        }

        updatePocketStatus();
        updateThreatened();
        updateWorkerCap();
    }

    void ResourceInfo::addTargetedBy(std::weak_ptr<UnitInfo> unit)
    {
        targetedBy.push_back(unit);
        framesPerTrips.emplace(unit, 0);
    }

    void ResourceInfo::removeTargetedBy(std::weak_ptr<UnitInfo> unit)
    {
        for (auto itr = targetedBy.begin(); itr != targetedBy.end(); itr++) {
            if (*itr->lock() == unit.lock()) {
                targetedBy.erase(itr);
                break;
            }
        }
        for (auto itr = framesPerTrips.begin(); itr != framesPerTrips.end(); itr++) {
            if (itr->first.lock() == unit.lock()) {
                framesPerTrips.erase(itr);
                break;
            }
        }
    }
} // namespace McRave