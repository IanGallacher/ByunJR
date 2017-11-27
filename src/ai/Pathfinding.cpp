#include <map>

#include "ByunJRBot.h"
#include "TechLab/util/Util.h"

#include "ai/Pathfinding.h"
#include "micro/Micro.h"

void Pathfinding::AddCandidatePoint(const CandidatePoint candidate_point)
{
    const auto p = candidate_point.nominated_point;

    if (visited_.find(p) != visited_.end()) return;
    if (p.x < 0) return;
    if (p.x > map_width_ - 1) return;
    if (p.y < 0) return;
    if (p.y > map_height_ - 1) return;

    visitable_points_.insert(candidate_point);
}

// Look through the visitable_points_ and find the best one to visit. 
inline CandidatePoint Pathfinding::BestPotentialPoint(const CandidatePoint fallback_point) const
{
    CandidatePoint optimal_point = fallback_point;
    for (const auto & point : visitable_points_)
    {
        // By definition, we should not be searching points that we have already visited. 
        assert(visited_.find(point.nominated_point) == visited_.end());

        const auto smallest_distance = optimal_point.weight;
        const float dist = distance_map_.at(point.nominated_point);
        // If the distance to the node is 1, we have not yet found a path that leads to it.
        if (dist < smallest_distance || optimal_point == fallback_point)
        {
            optimal_point = point;
        }
    }
    return optimal_point;
}


// Will only test and update if the path has not yet been visited, garunteeing that the path will never loop on itself. 
inline void Pathfinding::TestPointAndUpdateInformation(const CandidatePoint p,
                                                       const std::vector<std::vector<float>>& map_to_path)
{
    const Path current_shortest_path = shortest_path_to_vector_.at(p.origin_point);
    // The second value of the pair is the current weight of the node we are testing. 
    const float new_weight = p.weight + map_to_path[p.nominated_point.y][p.nominated_point.x];

    // If we have visited the node we are about to test, the path we are considering taking loops back on itself. 
    // Test only the paths that do not loop back on themselves. 
    float & current_weight = distance_map_.at(p.nominated_point);
    // If the distance to the node is 1, we have not yet found a path that leads to it. 
    // Did we discover a more optimal path (or a path for the first time?)
    // If we did, update the stored optimal path data.
    if (new_weight < current_weight || current_weight == 0.01f)
    {
        shortest_path_to_vector_[p.nominated_point] = current_shortest_path;
        shortest_path_to_vector_[p.nominated_point].push_back(Util::ToPoint2D(p.nominated_point));
    }
}

void Pathfinding::DjikstraInit(const std::vector<std::vector<float>>& map_to_path)
{// Setup the values in the distance map.
    visited_.clear();
    distance_map_.clear();
    map_width_ = map_to_path[0].size();
    map_height_ = map_to_path.size();

    for (int y = 0; y < map_to_path.size(); ++y)
    {
        for (int x = 0; x < map_to_path[y].size(); ++x)
        {
            distance_map_.insert(std::make_pair(sc2::Point2DI{x, y}, map_to_path[y][x]));

            shortest_path_to_vector_[sc2::Point2DI{x, y}] = Path{};
        }
    }
}

// Djikstra will require a quick glance over. We no longer iterate through all the points on the map, but instead
// all the points that have been recomended to visit. 
Path Pathfinding::Djikstra(const sc2::Point2DI start_point,
    const sc2::Point2DI end_point,
    const std::vector<std::vector<float>>& map_to_path)
{
    DjikstraInit(map_to_path);

    AddCandidatePoint(CandidatePoint
    {
        start_point,
        start_point,
        1 });

    // If we have not yet found the shortest path between the two points, keep searching for a new path. 
    while (visitable_points_.size() > 0)
    {
        CandidatePoint p = BestPotentialPoint(CandidatePoint{ start_point, start_point, 2 });

        const int best_potential_x = p.nominated_point.x;
        const int best_potential_y = p.nominated_point.y;
        AddCandidatePoint(CandidatePoint
        {
            sc2::Point2DI(best_potential_x - 1, best_potential_y),
            p.nominated_point,
            1 });
        AddCandidatePoint(CandidatePoint
        {
            sc2::Point2DI(best_potential_x + 1, best_potential_y),
            p.nominated_point,
            1 });
        AddCandidatePoint(CandidatePoint
        {
            sc2::Point2DI(best_potential_x, best_potential_y - 1),
            p.nominated_point,
            1 });
        AddCandidatePoint(CandidatePoint
        {
            sc2::Point2DI(best_potential_x, best_potential_y + 1),
            p.nominated_point,
            1 });

        visited_.insert(p.nominated_point);

        // We can't compare best_potential_x to something that is off the map. 
        TestPointAndUpdateInformation(p, map_to_path);

        if (p.nominated_point == end_point) 
            break;

        visitable_points_.erase(p);
    }
    return shortest_path_to_vector_.at(end_point);
}

// Find the optimal path, searching no farther than max_run_dist. 
// Uses a modified Djikstra algorithm. 
Path Pathfinding::OptimalPath(const sc2::Point2DI start_point,
    const int max_run_dist,
    const std::vector<std::vector<float>>& map_to_path)
{
    // Setup the values in the distance map.
    DjikstraInit(map_to_path);

    sc2::Point2DI current_point;

    // If we have not yet found the optimal path between the two points, keep searching for a new path. 
    while (true)
    {
        CandidatePoint p = BestPotentialPoint(CandidatePoint { start_point, start_point, 2 });

        const int best_potential_x = p.nominated_point.x;
        const int best_potential_y = p.nominated_point.y;
        AddCandidatePoint(CandidatePoint
            { 
              sc2::Point2DI(best_potential_x - 1, best_potential_y),
              p.nominated_point,
              1});
        AddCandidatePoint(CandidatePoint
            {
              sc2::Point2DI(best_potential_x + 1, best_potential_y),
              p.nominated_point,
              1});
        AddCandidatePoint(CandidatePoint
            { 
              sc2::Point2DI(best_potential_x, best_potential_y - 1),
              p.nominated_point,
              1});
        AddCandidatePoint(CandidatePoint
            { 
              sc2::Point2DI(best_potential_x, best_potential_y + 1),
              p.nominated_point,
              1});

        visited_.insert(p.nominated_point);

        // We can't compare best_potential_x to something that is off the map. 
        TestPointAndUpdateInformation(p, map_to_path);
        current_point = sc2::Point2DI{ best_potential_x, best_potential_y };

        if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
            break;

        visitable_points_.erase(p);
    }
    return shortest_path_to_vector_.at(current_point);
}

bool Pathfinding::TestDjikstra()
{
    const sc2::Point2DI start_point = { 2, 5 };
    const sc2::Point2DI end_point = { 7, 6 };
    const std::vector<std::vector<float>> map_to_path =
    {
        { 5, 1, 5, 5, 5 },
        { 5, 1, 5, 5, 5 },
        { 5, 1, 1, 1, 5 },
        { 5, 5, 5, 1, 5 },
        { 5, 5, 5, 1, 1 }
    };

    //const std::vector<std::vector<int>> map_to_path =
    //{
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1, },
    //    { 1,1,1,1,1,1,1,1,1,1  }
    //};
    const Path expected_result =
    {
        sc2::Point2D{ 1,0 },
        sc2::Point2D{ 1,1 },
        sc2::Point2D{ 1,2 },
        sc2::Point2D{ 2,2 },
        sc2::Point2D{ 3,2 },
        sc2::Point2D{ 3,3 },
        sc2::Point2D{ 3,4 },
        sc2::Point2D{ 4,4 }
    };

 /*   Path result = Djikstra(start_point, end_point, map_to_path);
    if (result[0] == expected_result[0])
    {
        return true;
    }*/
    return false;
}

void Pathfinding::SmartPathfind(const sc2::Unit* unit, const sc2::Point2D & target_position, ByunJRBot & bot)
{
    // Sometimes after we remove the floating points, it will turn out we are trying to move to is almost the same as our current position.
    // No need to run the pathfinding algorithm in that case. 
    if (Util::ToPoint2DI(unit->pos)
        == Util::ToPoint2DI(target_position))
    {
        Micro::SmartMove(unit, target_position, bot);
        return;
    }

    Pathfinding p;
    std::vector<sc2::Point2D> move_path = p.Djikstra(Util::ToPoint2DI(unit->pos),
        Util::ToPoint2DI(target_position),
        bot.Info().GetDPSMap());

    DrawPath(move_path, bot);
    if(move_path.size() > 0)
        Micro::SmartMove(unit, move_path[0], bot);
}


void Pathfinding::SmartRunAway(const sc2::Unit* unit, const int run_distance, ByunJRBot & bot)
{
    Pathfinding p;
    Path move_path = p.OptimalPath(Util::ToPoint2DI(unit->pos),
        run_distance,
        bot.Info().GetDPSMap());
    //SmartMove(unit, move_path[0], bot, false);
    //SmartMove(unit, move_path[1], bot, true);
    //SmartMove(unit, move_path[2], bot, true);

    DrawPath(move_path, bot);

    Micro::SmartMove(unit, move_path[3], bot, false);
    //for (const auto & j : move_path)
    //{
    //    SmartMove(unit, j, bot, true);
    //}
}

void Pathfinding::DrawPath(const Path path, ByunJRBot & bot)
{
    for (int i = 1; i < path.size(); ++i)
    {
        bot.DebugHelper().DrawLine(path[i-1], path[i]);
    }
}
