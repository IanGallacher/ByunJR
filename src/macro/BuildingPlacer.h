#pragma once
#include "macro/Building.h"

class Building;
class ByunJRBot;
class BaseLocation;

class BuildingPlacer
{
    ByunJRBot & bot_;

    std::vector< std::vector<bool> > reserve_map_;

    // queries for various BuildingPlacer data
    bool            Buildable(int x, int y, const sc2::UnitTypeID type) const;
    bool            IsReserved(int x, int y) const;
    bool            IsInResourceBox(int x, int y) const;
    bool            TileOverlapsBaseLocation(int x, int y, sc2::UnitTypeID type) const;


public:
    BuildingPlacer(ByunJRBot & bot);

    void OnStart();

    // determines whether we can build at a given location
    bool            CanBuildHere(int bx, int by, const sc2::UnitTypeID type) const;
    bool            CanBuildHereWithSpace(int bx, int by, const sc2::UnitTypeID type, int build_dist) const;

    // returns a build location near a building's desired location
    sc2::Point2DI   GetBuildLocationNear(const Building & b, int build_dist) const;

    void            DrawReservedTiles();

    void            ReserveTiles(int x, int y, int width, int height);
    void            FreeTiles(int x, int y, int width, int height);
    sc2::Point2DI   GetRefineryPosition() const;
};
