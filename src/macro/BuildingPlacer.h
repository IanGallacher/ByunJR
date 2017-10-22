#pragma once
#include "macro/Building.h"

class Building;
class ByunJRBot;
class BaseLocation;

class BuildingPlacer
{
    ByunJRBot & bot_;

    std::vector< std::vector<bool> > reserveMap;

    // queries for various BuildingPlacer data
    bool            Buildable(const Building & b, int x, int y) const;
    bool            IsReserved(int x, int y) const;
    bool            IsInResourceBox(int x, int y) const;
    bool            TileOverlapsBaseLocation(int x, int y, sc2::UnitTypeID type) const;


public:

    BuildingPlacer(ByunJRBot & bot);

    void OnStart();

    // determines whether we can build at a given location
    bool            CanBuildHere(int bx, int by, const Building & b) const;
    bool            CanBuildHereWithSpace(int bx, int by, const Building & b, int buildDist) const;

    // returns a build location near a building's desired location
    sc2::Point2DI   GetBuildLocationNear(const Building & b, int buildDist) const;

    void            DrawReservedTiles();

    void            ReserveTiles(int x, int y, int width, int height);
    void            FreeTiles(int x, int y, int width, int height);
    sc2::Point2DI   GetRefineryPosition() const;
};
