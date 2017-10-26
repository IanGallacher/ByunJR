#pragma once
#include <vector>
#include <map>

#include <sc2api/sc2_api.h>
#include <unordered_set>

namespace std
{
    template <> struct hash<sc2::Point2DI>
    {
        size_t operator()(const sc2::Point2DI& k) const
        {
            // return (53 + hash<int>(k.x)) * 53 + hash<int>(k.y);
            return k.x * 10000 + k.y;
        }
    };
}

class Pathfinding {
    std::pair<sc2::Point2DI, int> BestPotentialPair(std::pair<sc2::Point2DI, int> fallback_pair) const;
    void TestPointAndUpdateInformation(const int x, const int y, const int current_path_weight,
                                       const std::vector<std::vector<int>>& map_to_path,
                                       const std::vector<sc2::Point2D>& current_shortest_path);

    // We can't use a set because Point2DI does not have a comparison function, and sets must be "ordered" somehow.
    std::unordered_set<sc2::Point2DI> visited_;

    // The distance of the optimal route to that path.
    std::unordered_map<sc2::Point2DI, int> distance_map_;
    std::unordered_map<sc2::Point2DI, std::vector<sc2::Point2D>> shortest_path_to_vector_;

    void DjikstraInit(const std::vector<std::vector<int>>& map_to_path);
public:
    // Returns the optimal path. 
    std::vector<sc2::Point2D> Djikstra(const sc2::Point2DI start_point, const sc2::Point2DI end_point,
                                       const std::vector<std::vector<int>>& map_to_path);
    std::vector<sc2::Point2D> DjikstraLimit(const sc2::Point2DI start_point, const int max_run_dist,
                                            const std::vector<std::vector<int>>& map_to_path);
    bool TestDjikstra();
};
