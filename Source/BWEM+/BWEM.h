#pragma once
#include <BWAPI.h>
#include "Area.h"
#include "Chokepoint.h"

namespace BWEM {
    class Neutral;
    class Mineral;
    class Geyser;
    class StaticBuilding;
    class ChokePointPath;
}

namespace BWEM::Map
{
    void initialize();

    bool automaticPathUpdate();
    void enableAutomaticPathAnalysis();

    bool findBasesForStartingLocations();

    int	getMaxAltitude();

    int	getBaseCount();
    int	getChokePointCount();
    int getAreaCount();

    const std::vector<BWEM::ChokePoint>& getChokePoints(const BWEM::Area * a, const Area * b);
    const std::vector<BWAPI::TilePosition>& getStartingLocations();

    const std::vector<std::unique_ptr<Geyser>>&	getGeysers();
    const std::vector<std::unique_ptr<Mineral>>& getMinerals();
    const std::vector<std::unique_ptr<StaticBuilding>>& getStaticBuildings();

    Mineral *					getMineral(BWAPI::Unit);
    Geyser *					getGeyser(BWAPI::Unit);

    void						onMineralDestroyed(BWAPI::Unit);
    void						onStaticBuildingDestroyed(BWAPI::Unit);

    const std::vector<Area>&    getAreas();

    
    const Area *				getArea(int id);

    const Area *				getArea(BWAPI::WalkPosition);
    const Area *				getArea(BWAPI::TilePosition);

    const Area *				getNearestArea(BWAPI::WalkPosition);
    const Area *				getNearestArea(BWAPI::TilePosition);


    const ChokePointPath&	    getPath(const BWAPI::Position & a, const BWAPI::Position & b, int * pLength = nullptr);
}