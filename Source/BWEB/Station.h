#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB {

    class Station
    {
        const BWEM::Base* base;
        const BWEM::ChokePoint* chokepoint;
        const BWEM::Base * partnerBase = nullptr;
        std::set<BWAPI::TilePosition> defenses;
        BWAPI::Position resourceCentroid, defenseCentroid;
        bool main, natural;

        void addResourceReserve(BWAPI::Position, BWAPI::Position, BWAPI::Position);
        void initialize();
        void findChoke();
        void findDefenses();

    public:
        Station(const BWEM::Base* _base, bool _main, bool _natural)
        {
            base                = _base;
            main                = _main;
            natural             = _natural;
            resourceCentroid    = BWAPI::Position(0,0);
            defenseCentroid     = BWAPI::Position(0,0);
            initialize();
            findChoke();
            findDefenses();
        }

        /// <summary> Returns the central position of the resources associated with this Station including geysers. </summary>
        BWAPI::Position getResourceCentroid() { return resourceCentroid; }

        /// <summary> Returns the set of defense locations associated with this Station. </summary>
        std::set<BWAPI::TilePosition>& getDefenseLocations() { return defenses; }

        /// <summary> Returns the BWEM Base associated with this Station. </summary>
        const BWEM::Base * getBase() { return base; }

        /// <summary> Returns the BWEM Choke that should be used for generating a Wall at. </summmary>
        const BWEM::ChokePoint * getChokepoint() { return chokepoint; }

        /// <summary> Returns the number of ground defenses associated with this Station. </summary>
        int getGroundDefenseCount();

        /// <summary> Returns the number of air defenses associated with this Station. </summary>
        int getAirDefenseCount();

        /// <summary> Returns true if the Station is a main Station. </summary>
        bool isMain() { return main; }

        /// <summary> Returns true if the Station is a natural Station. </summary>
        bool isNatural() { return natural; }

        /// <summary> Draws all the features of the Station. </summary>
        void draw();

        bool operator== (Station* s) {
            return base == s->getBase();
        }
    };

    namespace Stations {

        /// <summary> Returns a vector containing every BWEB::Station </summary>
        std::vector<Station>& getStations();

        /// <summary> Returns the closest BWEB::Station to the given TilePosition. </summary>
        Station * getClosestStation(BWAPI::TilePosition);

        /// <summary> Returns the closest main BWEB::Station to the given TilePosition. </summary>
        Station * getClosestMainStation(BWAPI::TilePosition);

        /// <summary> Returns the closest natural BWEB::Station to the given TilePosition. </summary>
        Station * getClosestNaturalStation(BWAPI::TilePosition);

        /// <summary> Initializes the building of every BWEB::Station on the map, call it only once per game. </summary>
        void findStations();

        /// <summary> Calls the draw function for each Station that exists. </summary>
        void draw();
    }
}