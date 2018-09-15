#pragma once
#include <map>
#include <unordered_set>
#include <vector>

#include <sc2api/sc2_api.h>

namespace std
{
    template <> struct hash<sc2::Point2DI>
    {
        size_t operator()(const sc2::Point2DI& k) const
        {
            // This is probably a better hash function to use in the future. 
            // return (53 + hash<int>(k.x)) * 53 + hash<int>(k.y);

            // For now, this hash function is good enough for our uses. 
            return k.x * 10000 + k.y;
        }
    };
}

typedef std::vector<sc2::Point2D> Path;
typedef sc2::Point2DI Tile;
class Pathfinding {
    // We can't use a set because Point2DI does not have a comparison function, and sets must be "ordered" somehow.
    std::unordered_set<sc2::Point2DI> visited_;
    Path GetPath(const std::unordered_map<Tile, Tile>& parent, const Tile destination);
    Path bfs(const std::vector<std::vector<float>>& map_to_path);

public:
    Path SafestPath(const std::vector<std::vector<float>>& dps_map, const Tile origin, const int max_depth);
    //std::vector<sc2::Point2D> DjikstraLimit(const sc2::Point2DI start_point, const int max_run_dist,
    //                                        const std::vector<std::vector<float>>& map_to_path);
    bool TestSafestPath();
    bool TestBFS();
    bool TestDjikstra();
    void SmartPathfind(const sc2::Unit * unit, const sc2::Point2D & target_position, ByunJRBot & bot);
    void SmartRunAway(const sc2::Unit * unit, const int run_distance, ByunJRBot & bot);
};
