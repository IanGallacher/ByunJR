#pragma once
#include "macro/Building.h"

class ByunJRBot;

class BuildingPlacer
{
    ByunJRBot & bot_;

    // All the tiles that either have a building planned, or currently have a building on them. 
    // They are only unreserved if the building dies. 
    std::vector< std::vector<bool> > reserve_map_;

    // queries for various BuildingPlacer data
    bool            Buildable(int x, int y, const sc2::UnitTypeID type) const;
    bool            IsInResourceBox(int x, int y) const;
    bool            TileOverlapsBaseLocation(int x, int y, sc2::UnitTypeID type) const;


public:
    BuildingPlacer(ByunJRBot & bot);

    void OnStart();

    void            ReserveTiles(sc2::UnitTypeID building_type, sc2::Point2DI building_location);
    void            FreeTiles(sc2::UnitTypeID building_type, sc2::Point2DI building_location);
    bool            IsReserved(int x, int y) const;

    // determines whether we can build at a given location
    bool            CanBuildHere(int bx, int by, const sc2::UnitTypeID type) const;
    bool            CanBuildHereWithSpace(int bx, int by, const sc2::UnitTypeID type, int build_dist) const;

    // returns a build location near a building's desired location
    sc2::Point2DI   GetBuildLocationNear(const Building & b, int build_dist) const;

    void            DrawReservedTiles();
    sc2::Point2DI   GetRefineryPosition() const;
};
