#pragma once
#include <map>
#include <unordered_set>
#include <vector>

#include <sc2api/sc2_api.h>

typedef std::vector<sc2::Point2D> Path;

struct CandidatePoint
{
    // origin_point is the tip of the path that leads to nominated_point
    sc2::Point2DI nominated_point;
    sc2::Point2DI origin_point;
    float weight; // Weight will normally be the same as distance for most pathfinding algorithms, but does not have to be.

    bool operator==(const CandidatePoint& a) const
    {
        return nominated_point.x == a.nominated_point.x && nominated_point.y == a.nominated_point.y;
    }
};
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

    template <> struct hash<CandidatePoint>
    {
        size_t operator()(const CandidatePoint& k) const
        {
            // This is probably a better hash function to use in the future. 
            // return (53 + hash<int>(k.x)) * 53 + hash<int>(k.y);

            // For now, this hash function is good enough for our uses. 
            return k.nominated_point.x * 10000 + k.nominated_point.y;
        }
    };
}

class Pathfinding {
    CandidatePoint BestPotentialPoint(CandidatePoint fallback_pair) const;
    void TestPointAndUpdateInformation(const int x, const int y, const float current_path_weight,
                                       const std::vector<std::vector<float>>& map_to_path,
                                       const std::vector<sc2::Point2D>& current_shortest_path);

    int map_width_;
    int map_height_;

    // We can't use a set because Point2DI does not have a comparison function, and sets must be "ordered" somehow.
    std::unordered_set<sc2::Point2DI> visited_;
    std::unordered_set<CandidatePoint> visitable_points_;

    // The distance of the optimal route to that path.
    // Initially setup with every point on the map, with a value of one. 
    std::unordered_map<sc2::Point2DI, float> distance_map_;
    std::unordered_map<sc2::Point2DI, Path> shortest_path_to_vector_;

    void DjikstraInit(const std::vector<std::vector<float>>& map_to_path);
    void AddCandidatePoint(const CandidatePoint c);
public:
    // Returns the optimal path. 
    //std::vector<sc2::Point2D> Djikstra(const sc2::Point2DI start_point, const sc2::Point2DI end_point,
    //                                   const std::vector<std::vector<float>>& map_to_path);
    //std::vector<sc2::Point2D> DjikstraLimit(const sc2::Point2DI start_point, const int max_run_dist,
    //                                        const std::vector<std::vector<float>>& map_to_path);
    Path OptimalPath(const sc2::Point2DI start_point, const int max_run_dist, const std::vector<std::vector<float>>& map_to_path);
    bool TestDjikstra();
    void SmartPathfind(const sc2::Unit * unit, const sc2::Point2D & target_position, ByunJRBot & bot);
    void SmartRunAway(const sc2::Unit * unit, const int run_distance, ByunJRBot & bot);
};
