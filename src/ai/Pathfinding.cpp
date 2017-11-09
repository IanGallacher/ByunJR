#include <map>

#include "ByunJRBot.h"
#include "TechLab/util/Timer.hpp"

#include "ai/Pathfinding.h"
#include "micro/Micro.h"

inline std::pair<sc2::Point2DI, int> Pathfinding::BestPotentialPair(const std::pair<sc2::Point2DI, int> fallback_pair) const
{
    int smallest_distance = std::numeric_limits<int>::max();
    std::pair<sc2::Point2DI, int> optimal_pair = fallback_pair;
    for (const auto & distance_pair : distance_map_)
    {
        const sc2::Point2DI point = distance_pair.first;
        const int dist = distance_pair.second;
        // If the distance to the node is 1, we have not yet found a path that leads to it.
        if (dist < smallest_distance && dist != 1)
        {
            // Find the best pair that we have not yet visited. If we have already visited that point, don't visit it again. 
            if (visited_.find(point) == visited_.end())
            {
                smallest_distance = dist;
                optimal_pair = distance_pair;
            }
        }
    }
    return optimal_pair;
}


// Will only test and update if the path has not yet been visited, garunteeing that the path will never loop on itself. 
inline void Pathfinding::TestPointAndUpdateInformation(const int x, const int y, const int current_path_weight,
                                                       const std::vector<std::vector<int>>& map_to_path,
                                                       const std::vector<sc2::Point2D>& current_shortest_path)
{
    Timer t;
    t.Start();
    // The second value of the pair is the current weight of the node we are testing. 
    const int new_weight = current_path_weight + map_to_path[y][x];
    // If we have visited the node we are about to test, the path we are considering taking loops back on itself. 
    // Test only the paths that do not loop back on themselves. 
    const sc2::Point2DI point{ x,y };
    if (visited_.find(point) == visited_.end())
    {
        int & current_tentative_distance_at_point = distance_map_.at(sc2::Point2DI(x, y));
        // If the distance to the node is 1, we have not yet found a path that leads to it. 
        // Did we discover a more optimal path (or a path for the first time?)
        // If we did, update the stored optimal path data.
        if (new_weight < current_tentative_distance_at_point || current_tentative_distance_at_point == 1)
        {
            current_tentative_distance_at_point = new_weight;
            shortest_path_to_vector_[point] = current_shortest_path;
            shortest_path_to_vector_[point].push_back(sc2::Point2D(point.x, point.y));
        }
    }/*
    double ms = t.GetElapsedTimeInMilliSec();
    printf("TestPointAndUpdateInformation %lf ms\n", ms);*/
}

void Pathfinding::DjikstraInit(const std::vector<std::vector<int>>& map_to_path)
{
    const int width = map_to_path.size();
    const int height = map_to_path[0].size();

    // Setup the values in the distance map.
    distance_map_.clear();
    for (int y = 0; y < map_to_path.size(); ++y)
    {
        for (int x = 0; x < map_to_path[y].size(); ++x)
        {
            distance_map_.insert(std::make_pair(sc2::Point2DI(x, y), 1));

            shortest_path_to_vector_[sc2::Point2DI(x, y)] = std::vector<sc2::Point2D>();
        }
    }
}

std::vector<sc2::Point2D> Pathfinding::Djikstra(const sc2::Point2DI start_point,
    const sc2::Point2DI end_point,
    const std::vector<std::vector<int>>& map_to_path)
{
    DjikstraInit(map_to_path);

    // If we have not yet found the shortest path between the two points, keep searching for a new path. 
    while (true)
    {
        std::pair<sc2::Point2DI, int> p = BestPotentialPair(std::pair<sc2::Point2DI, int> { start_point, 2 });
        // Hard copy the current point into the visited set, and test nearby nodes if it has not yet been visited. 
        // visited.set.insert( ... ).second is set to true if we have not yet visited the element. =
        if (visited_.insert(p.first).second)
        {
            const int best_potential_x = p.first.x;
            const int best_potential_y = p.first.y;
            const int weight = p.second;
            // We can't compare best_potential_x to something that is off the map. 
            if (best_potential_x > 0)
            {
                TestPointAndUpdateInformation(best_potential_x - 1, best_potential_y, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                const sc2::Point2DI current_point(best_potential_x - 1, best_potential_y);
                if (current_point == end_point)
                    break;
            }
            if (best_potential_x < map_to_path[0].size() - 1)
            {
                TestPointAndUpdateInformation(best_potential_x + 1, best_potential_y, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                const sc2::Point2DI current_point(best_potential_x + 1, best_potential_y);
                if (current_point == end_point)
                    break;
            }
            if (best_potential_y > 0)
            {
                TestPointAndUpdateInformation(best_potential_x, best_potential_y - 1, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                const sc2::Point2DI current_point(best_potential_x, best_potential_y - 1);
                if (current_point == end_point)
                    break;
            }
            if (best_potential_y < map_to_path.size() - 1)
            {
                TestPointAndUpdateInformation(best_potential_x, best_potential_y + 1, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                const sc2::Point2DI current_point(best_potential_x, best_potential_y + 1);
                if (current_point == end_point)
                    break;
            }
        }
    }
    return shortest_path_to_vector_.at(end_point);
}

std::vector<sc2::Point2D> Pathfinding::DjikstraLimit(const sc2::Point2DI start_point,
    const int max_run_dist,
    const std::vector<std::vector<int>>& map_to_path)
{
    // Setup the values in the distance map.
    DjikstraInit(map_to_path);

    sc2::Point2DI current_point;

    // If we have not yet found the shortest path between the two points, keep searching for a new path. 
    while (true)
    {
        std::pair<sc2::Point2DI, int> p = BestPotentialPair(std::pair<sc2::Point2DI, int> { start_point, 2 });
        // Hard copy the current point into the visited set, and test nearby nodes if it has not yet been visited. 
        // visited.set.insert( ... ).second is set to true if we have not yet visited the element. =
        if (visited_.insert(p.first).second)
        {
            const int best_potential_x = p.first.x;
            const int best_potential_y = p.first.y;
            const int weight = p.second;
            // We can't compare best_potential_x to something that is off the map. 
            if (best_potential_x > 0)
            {
                TestPointAndUpdateInformation(best_potential_x - 1, best_potential_y, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                current_point = sc2::Point2DI(best_potential_x - 1, best_potential_y);
                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
                    break;
            }
            if (best_potential_x < map_to_path[0].size() - 1)
            {
                TestPointAndUpdateInformation(best_potential_x + 1, best_potential_y, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                current_point = sc2::Point2DI(best_potential_x + 1, best_potential_y);
                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
                    break;
            }
            if (best_potential_y > 0)
            {
                TestPointAndUpdateInformation(best_potential_x, best_potential_y - 1, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                current_point = sc2::Point2DI(best_potential_x, best_potential_y - 1);
                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
                    break;
            }
            if (best_potential_y < map_to_path.size() - 1)
            {
                TestPointAndUpdateInformation(best_potential_x, best_potential_y + 1, weight, map_to_path, shortest_path_to_vector_.at(p.first));
                current_point = sc2::Point2DI(best_potential_x, best_potential_y + 1);
                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
                    break;
            }
        }
    }
    return shortest_path_to_vector_.at(current_point);
}

bool Pathfinding::TestDjikstra()
{
    const sc2::Point2DI start_point = { 2, 5 };
    const sc2::Point2DI end_point = { 7, 6 };
    const std::vector<std::vector<int>> map_to_path =
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
    const std::vector<sc2::Point2D> expected_result =
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

    std::vector<sc2::Point2D> result = Djikstra(start_point, end_point, map_to_path);
    if (result[0] == expected_result[0])
    {
        return true;
    }
    return false;
}

void Pathfinding::SmartPathfind(const sc2::Unit* unit, const sc2::Point2D & target_position, ByunJRBot & bot)
{
    // Sometimes after we remove the floating points, it will turn out we are trying to move to is almost the same as our current position.
    // No need to run the pathfinding algorithm in that case. 
    if (sc2::Point2DI(unit->pos.x, unit->pos.y)
        == sc2::Point2DI(target_position.x, target_position.y))
    {
        Micro::SmartMove(unit, target_position, bot);
        return;
    }

    Pathfinding p;
    std::vector<sc2::Point2D> move_path = p.Djikstra(sc2::Point2DI(unit->pos.x, unit->pos.y),
        sc2::Point2DI(target_position.x, target_position.y),
        bot.InformationManager().GetDPSMap());
    Micro::SmartMove(unit, move_path[0], bot);
}


void Pathfinding::SmartRunAway(const sc2::Unit* unit, const int run_distance, ByunJRBot & bot)
{
    Pathfinding p;
    std::vector<sc2::Point2D> move_path = p.DjikstraLimit(sc2::Point2DI(unit->pos.x, unit->pos.y),
        run_distance,
        bot.InformationManager().GetDPSMap());
    //SmartMove(unit, move_path[0], bot, false);
    //SmartMove(unit, move_path[1], bot, true);
    //SmartMove(unit, move_path[2], bot, true);
    Micro::SmartMove(unit, move_path[3], bot, false);
    //for (const auto & j : move_path)
    //{
    //    SmartMove(unit, j, bot, true);
    //}
}