#pragma once
#include "macro/Building.h"

class ByunJRBot;

class BuildingPlacer
{
    ByunJRBot & bot_;

    // All the tiles that either have a building planned, or currently have a building on them. 
    // They are only unreserved if the building dies. 
    std::vector< std::vector<bool> > reserve_map_;
    // Cache where a building is most likely going to be created. 
    // Useful for deciding if we should send a scv to go build. (accounting for current income)
    mutable std::map<sc2::UnitTypeID, sc2::Point2DI> build_location_cache_;

    // queries for various BuildingPlacer data
    bool            Buildable(int x, int y, const sc2::UnitTypeID type) const;
    bool            IsInResourceBox(int x, int y) const;

public:
    BuildingPlacer(ByunJRBot & bot);

	void OnStart();
	void OnFrame();

    sc2::Point2DI   GetBuildLocationForType(const sc2::UnitTypeID) const;
	sc2::Point2DI   GetNextCoordinateToWallWithBuilding(sc2::UnitTypeID building_type) const;
    void            ReserveTiles(sc2::UnitTypeID building_type, sc2::Point2DI building_location);
    void            FreeTiles(sc2::UnitTypeID building_type, sc2::Point2DI building_location);
    bool            IsReserved(int x, int y) const;
	bool            IsTileCornerReserved(const sc2::Point2DI p) const;

    // determines whether we can build at a given location
    bool            CanBuildHere(int bx, int by, const sc2::UnitTypeID type) const;
    bool            CanBuildHereWithSpace(int bx, int by, const sc2::UnitTypeID type, int build_dist) const;

    sc2::Point2DI   GetBuildLocationNear(const sc2::Point2DI desired_loc, const sc2::UnitTypeID building_type, const int build_dist) const;

    void            DrawReservedTiles();
    void            DrawBuildLocationCache();
    sc2::Point2DI   GetRefineryPosition() const;
};
