#pragma once
#include <vector>
#include <sc2api/sc2_api.h>

#include "util/DistanceMap.h"

class ByunJRBot;

class MapTools
{
    ByunJRBot& bot_;
    int     true_map_width_;
    int     true_map_height_;
    int     playable_map_height_;
    int     playable_map_width_;
    float   max_z_;
    int     frame_;
    

    // a cache of already computed distance maps, which is mutable since it only acts as a cache
    mutable std::map<std::pair<int, int>, DistanceMap>   all_maps_;   

    std::vector<std::vector<bool>>  walkable_;          // whether a tile is buildable (includes static resources)
    std::vector<std::vector<bool>>  buildable_;         // whether a tile is buildable (includes static resources)
    std::vector<std::vector<bool>>  depot_buildable_;   // whether a depot is buildable on a tile (illegal within 3 tiles of static resource)
    std::vector<std::vector<int>>   last_seen_;         // the last time any of our units has seen this position on the map
    std::vector<std::vector<int>>   sector_number_;     // connectivity sector number, two tiles are ground connected if they have the same number
    std::vector<std::vector<float>> terrain_height_;    // height of the map at x+0.5, y+0.5
    
    void ComputeConnectivity();
        
    void PrintMap() const;


    enum class MapTileType
    {
        Free,
        Ramp,
        CantBuild,
        CantWalk
    };


public:

    MapTools(ByunJRBot& bot);

    void    OnStart();
    void    OnFrame();

    // Only needs to be public in order to draw debug information on the map. 
    int GetSectorNumber(int x, int y) const;
    int GetSectorNumber(const sc2::Point2DI& pos) const;

    // Map size can be bigger than the area the player can actually play on. 
    // "TrueMapSize" is the complete map dimensions, including places units can not move to. 
    int     TrueMapWidth() const;
    int     TrueMapHeight() const;

    // Map size can be bigger than the area the player can actually play on. 
    // The "PlayableMapSize" is the area units can actually move to.
    int     PlayableMapWidth() const;
    int     PlayableMapHeight() const;

    float   TerrainHeight(float x, float y) const;
    
    bool    IsOnMap(int x, int y) const;
    bool    IsOnMap(const sc2::Point2D& pos) const;
    bool    IsOnMap(const sc2::Point2DI& pos) const;
    bool    IsPowered(const sc2::Point2DI& pos) const;
    bool    IsExplored(const sc2::Point2D& pos) const;
    bool    IsVisible(const sc2::Point2D& pos) const;
    bool    CanBuildTypeAtPosition(int x, int y, sc2::UnitTypeID type) const;

    const   DistanceMap& GetDistanceMap(const sc2::Point2DI& tile) const;
    int     GetGroundDistance(const sc2::Point2DI& src, const sc2::Point2DI& dest) const;
    int     GetGroundDistance(const sc2::Point2D& src, const sc2::Point2D& dest) const;
    bool    IsConnected(int x1, int y1, int x2, int y2) const;
    bool    IsConnected(const sc2::Point2DI& from, const sc2::Point2DI& to) const;
    bool    IsWalkable(const sc2::Point2DI& pos) const;
    bool    IsWalkable(int x, int y) const;
    
    bool    IsBuildable(const sc2::Point2DI& pos) const;
    bool    IsBuildable(int x, int y) const;
    bool    IsDepotBuildableTile(const sc2::Point2D& pos) const;


    
    sc2::Point2DI GetLeastRecentlySeenPosition() const;
    bool IsTileTypeOf(const int x, const int y, const MapTileType tile_type) const;
    bool IsTileAdjacentToTileType(const sc2::Point2DI p, const MapTileType tile_type) const;
    bool IsTileCornerOfTileType(const sc2::Point2DI p, const MapTileType tile_type) const;
    bool IsAnyTileAdjacentToTileType(const sc2::Point2DI p, const MapTileType tile_type,
                                     sc2::UnitTypeID building_type) const;
    sc2::Point2D  GetNextCoordinateToWallWithBuilding(sc2::UnitTypeID building_type) const;

    // returns a list of all tiles on the map, sorted by 4-direcitonal walk distance from the given position
    const std::vector<sc2::Point2DI>& GetClosestTilesTo(const sc2::Point2DI& pos) const;
};

