#include "Main/McRave.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace McRave {

    void ResourceInfo::updatePocketStatus()
    {
        if (!getType().isMineralField())
            return;

        // Determine if this resource is a pocket resource, useful to deter choosing a building worker (they get stuck)
        vector<TilePosition> directions ={ {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 1}, {2, -1}, {2, 0}, {2, 1} };
        vector<TilePosition> pocketTiles;
        for (auto &dir : directions) {
            auto tile = tilePosition + dir;
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
        // Determine if this resource is threatened based on a substantial threat nearby
        threatened = false;
        for (auto &enemy : Units::getUnits(PlayerState::Enemy)) {
            if (enemy->getType().isWorker()
                || !enemy->hasTarget() || !enemy->getTarget().lock()->getType().isWorker()
                || !enemy->isCompleted() || !enemy->canAttackGround())
                continue;

            const auto inRangeOfResource = enemy->getPosition().getDistance(position) < max(160.0, enemy->getGroundRange());
            threatened = inRangeOfResource;
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
                    framesPerTrips[t]=0;

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
            type                = thisUnit->getType();
            remainingResources  = thisUnit->getResources();
            position            = thisUnit->getPosition();
            tilePosition        = thisUnit->getTilePosition();
        }

        updatePocketStatus();
        updateThreatened();
        updateWorkerCap();
    }

    void ResourceInfo::addTargetedBy(std::weak_ptr<UnitInfo> unit) {
        targetedBy.push_back(unit);
        framesPerTrips.emplace(unit, 0);
    }

    void ResourceInfo::removeTargetedBy(std::weak_ptr<UnitInfo> unit) {
        for (auto itr = targetedBy.begin(); itr != targetedBy.end(); itr++) {
            if (*itr == unit) {
                targetedBy.erase(itr);
                break;
            }
        }
        for (auto itr = framesPerTrips.begin(); itr != framesPerTrips.end(); itr++) {
            if (itr->first == unit) {
                framesPerTrips.erase(itr);
                break;
            }
        }
    }
}