#pragma once
#include <vector>
#include <map>

#include <sc2api/sc2_api.h>

class Pathfinding {
    std::pair<sc2::Point2DI, int> BestPotentialPair(std::pair<sc2::Point2DI, int> fallback_pair) const;
    int TestPointAndUpdateInformation(const int x, const int y, const int current_path_weight,
                                      const std::vector<std::vector<int>>& map_to_path);
public:
    // Returns the optimal path. 
    std::vector<sc2::Point2D> Djikstra(const sc2::Point2DI start_point, const sc2::Point2DI end_point,
                                       const std::vector<std::vector<int>>& map_to_path);
    bool TestDjikstra();
};
