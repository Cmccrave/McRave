#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB {

    class Station
    {
        const BWEM::Base* base = nullptr;
        const BWEM::Base * partnerBase = nullptr;
        const BWEM::ChokePoint* choke = nullptr;
        BWAPI::Position resourceCentroid, defenseCentroid, anglePosition;
        std::set<BWAPI::TilePosition> secondaryLocations;
        std::set<BWAPI::TilePosition> defenses;
        bool main, natural;
        double defenseAngle = 0.0;
        double baseAngle = 0.0;
        double chokeAngle = 0.0;
        int defenseArrangement = -1;
        BWAPI::TilePosition  smallPosition, mediumPosition, pocketDefense;

        void initialize();
        void findChoke();
        void findAngles();
        void findSecondaryLocations();
        void findDefenses();
        void findNestedDefenses();
        void addResourceReserves();
        void cleanup();

    public:
        Station(const BWEM::Base* _base, bool _main, bool _natural)
        {
            base                = _base;
            main                = _main;
            natural             = _natural;
            resourceCentroid    = BWAPI::Position(0, 0);
            defenseCentroid     = BWAPI::Position(0, 0);
            smallPosition       = BWAPI::TilePositions::Invalid;
            mediumPosition      = BWAPI::TilePositions::Invalid;
            pocketDefense       = BWAPI::TilePositions::Invalid;

            initialize();
            findChoke();
            findAngles();
            findSecondaryLocations();
            //findNestedDefenses();
            findDefenses();
            addResourceReserves();
            cleanup();
        }

        /// <summary> Returns the central position of the resources associated with this Station including geysers. </summary>
        const BWAPI::Position getResourceCentroid() const { return resourceCentroid; }

        /// <summary> Returns a backup base placement in case the main is blocked or a second one is desired for walling purposes. </summary>
        const std::set<BWAPI::TilePosition>& getSecondaryLocations() const { return secondaryLocations; }

        /// <summary> Returns the set of defense locations associated with this Station. </summary>
        const std::set<BWAPI::TilePosition>& getDefenses() const { return defenses; }

        /// <summary> Returns a pocket defense placement </summary>
        const BWAPI::TilePosition getPocketDefense() const { return pocketDefense; }

        /// <summary> Returns a medium placement that is useful for placing first Depot/Pool </summary>
        const BWAPI::TilePosition getMediumPosition() const { return mediumPosition; }

        /// <summary> Returns a small placement that is useful for placing first Spire </summary>
        const BWAPI::TilePosition getSmallPosition() const { return smallPosition; }

        /// <summary> Returns the BWEM Base associated with this Station. </summary>
        const BWEM::Base * getBase() const { return base; }

        /// <summary> Returns the BWEM Choke that should be used for generating a Wall at. </summmary>
        const BWEM::ChokePoint * getChokepoint() const { return choke; }

        /// <summary> Returns true if the Station is a main Station. </summary>
        const bool isMain() const { return main; }

        /// <summary> Returns true if the Station is a natural Station. </summary>
        const bool isNatural() const { return natural; }

        /// <summary> Draws all the features of the Station. </summary>
        const void draw() const;

        ///
        const double getDefenseAngle() const { return defenseAngle; }

        const bool operator== (const Station* s) const {
            return s && base == s->getBase();
        }
        const bool operator== (const Station s) const {
            return base == s.getBase();
        }
    };

    namespace Stations {

        /// <summary> Returns a vector containing every BWEB::Station </summary>
        const std::vector<Station>& getStations();

        /// <summary> Returns the BWEB::Station of the starting main. </summary>
        const Station * getStartingMain();

        /// <summary> Returns the BWEB::Station of the starting natural. </summary>
        const Station * getStartingNatural();

        /// <summary> Returns the closest BWEB::Station to the given TilePosition. </summary>
        const Station * getClosestStation(BWAPI::TilePosition);

        /// <summary> Returns the closest main BWEB::Station to the given TilePosition. </summary>
        const Station * getClosestMainStation(BWAPI::TilePosition);

        /// <summary> Returns the closest natural BWEB::Station to the given TilePosition. </summary>
        const Station * getClosestNaturalStation(BWAPI::TilePosition);

        /// <summary> Initializes the building of every BWEB::Station on the map, call it only once per game. </summary>
        void findStations();

        /// <summary> Calls the draw function for each Station that exists. </summary>
        void draw();
    }
}