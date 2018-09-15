#include <map>

#include "ByunJRBot.h"
#include "TechLab/util/Timer.hpp"
#include "TechLab/util/Util.h"

#include "ai/Pathfinding.h"
#include "micro/Micro.h"

Tile null_tile = { -1,-1 };

// Return adjacent tiles, assuming they are on the map. 
std::vector<Tile> adj(const Tile tile, const int width, const int height)
{
    std::vector<Tile> adj_tiles;
    if (tile.x > 0)        adj_tiles.push_back(Tile{ tile.x - 1, tile.y     });
    if (tile.x < width-1)  adj_tiles.push_back(Tile{ tile.x + 1, tile.y     });
    if (tile.y > 0)        adj_tiles.push_back(Tile{ tile.x    , tile.y - 1 });
    if (tile.y < height-1) adj_tiles.push_back(Tile{ tile.x    , tile.y + 1 });
    return adj_tiles;
}

Path Pathfinding::GetPath(const std::unordered_map<Tile, Tile>& parent, const Tile destination)
{
    Tile current_parrent = parent.at(destination);
    Path path = { Util::ToPoint2D(destination) };
    while (current_parrent != null_tile)
    {
        path.push_back( Util::ToPoint2D(current_parrent) );
        current_parrent = parent.at(current_parrent);
    }
    // We have followed the path in backwards order. Flip to forwards order before returning. 
    std::reverse(path.begin(), path.end());
    return path;
}

Path Pathfinding::bfs(const std::vector<std::vector<float>>& map_to_path)
{
    const int height = static_cast<int>( map_to_path.size() );
    const int width = static_cast<int>( map_to_path[0].size() );

    // The parent node to create the shortest path. 
    std::unordered_map<Tile, Tile> parent;

    // What level was the path at?
    std::unordered_map<Tile, int> level;

    std::unordered_set<Tile> current_frontier = { Tile(0,0) };

    int i = 1;
    while (current_frontier.size() > 0)
    {
        std::unordered_set<Tile> next_frontier;
        for (const auto parent_v : current_frontier)
        {
            for (const auto v : adj(parent_v, width, height))
            {
                // Only bother searching if the level has not yet been found. 
                if (level.find(v) != level.end()) continue;

                level[v] = i;
                parent[v] = parent_v;
                next_frontier.insert(v);
            }
        }
        current_frontier = next_frontier;
        ++i;
    }

    Path return_path;
    return return_path;
}

Path Pathfinding::SafestPath(const std::vector<std::vector<float>>& dps_map, const Tile origin, const int max_depth)
{
    const int height = static_cast<int>(dps_map.size());
    const int width = static_cast<int>(dps_map[0].size());

    // The parent node to create the shortest path. 
    // The origin has no parent, so let's init it to null.
    std::unordered_map<Tile, Tile> parent = { { origin, null_tile} };

    // What level was the path at?
    std::unordered_map<Tile, int> danger_at_point;

    std::unordered_set<Tile> current_frontier = { origin };

    Tile destination;

    int current_level = 1;
    while (current_frontier.size() > 0 && current_level <= max_depth)
    {
        std::unordered_set<Tile> next_frontier;
            int safest_weight = std::numeric_limits<int>().max();
        for (const auto parent_v : current_frontier)
        {
            for (const auto v : adj(parent_v, width, height))
            {
                const int new_danger = dps_map[v.y][v.x] + danger_at_point[parent_v];
                // If we have not yet exploered the point on the map, or if the new path is safer than the old path.
                if ((danger_at_point[v] == 0 || new_danger < danger_at_point[v]) && v != origin)
                {
                    // The danger at the current point in the path is equal to 
                    danger_at_point[v] = dps_map[v.y][v.x] + danger_at_point[parent_v];
                    parent[v] = parent_v;
                    next_frontier.insert(v);

                    if (new_danger < safest_weight)
                    {
                        destination = v;
                        safest_weight = new_danger;
                    }
                }
            }
        }
        current_frontier = next_frontier;
        ++current_level;
    }

    return GetPath(parent, destination);

}

bool Pathfinding::TestSafestPath()
{
    const Tile start_point = { 0, 0 };
    const Tile end_point = { 3, 3 };
    const std::vector<std::vector<float>> map_to_path =
    {
        { 5, 1, 5, 5, 5 },
        { 5, 1, 5, 5, 5 },
        { 5, 1, 1, 1, 5 },
        { 5, 5, 5, 1, 5 },
        { 5, 5, 5, 1, 1 }
    };

    const auto j = SafestPath(map_to_path, start_point, 10);

    return false;
}

bool Pathfinding::TestBFS()
{
    const Tile start_point = { 0, 0 };
    const Tile end_point = { 3, 3 };
    const std::vector<std::vector<float>> map_to_path =
    {
        { 5, 1, 5, 5, 5 },
        { 5, 1, 5, 5, 5 },
        { 5, 1, 1, 1, 5 },
        { 5, 5, 5, 1, 5 },
        { 5, 5, 5, 1, 1 }
    };

    bfs(map_to_path);

    return false;
}

//
//// Find the optimal path, searching no farther than max_run_dist. 
//// Uses a modified Djikstra algorithm. 
//std::vector<sc2::Point2D> Pathfinding::DjikstraLimit(const sc2::Point2DI start_point,
//    const int max_run_dist,
//    const std::vector<std::vector<float>>& map_to_path)
//{
//    // Setup the values in the distance map.
//    DjikstraInit(map_to_path);
//
//    sc2::Point2DI current_point;
//
//    // If we have not yet found the optimal path between the two points, keep searching for a new path. 
//    while (true)
//    {
//        std::pair<sc2::Point2DI, int> p = BestPotentialPair(std::pair<sc2::Point2DI, int> { start_point, 2 });
//        // Hard copy the current point into the visited set, and test nearby nodes if it has not yet been visited. 
//        // visited.set.insert( ... ).second is set to true if we have not yet visited the element.
//        if (visited_.insert(p.first).second)
//        {
//            const int best_potential_x = p.first.x;
//            const int best_potential_y = p.first.y;
//            const int weight = p.second;
//            // We can't compare best_potential_x to something that is off the map. 
//            if (best_potential_x > 0)
//            {
//                TestPointAndUpdateInformation(best_potential_x - 1, best_potential_y, weight, map_to_path, shortest_path_to_vector_.at(p.first));
//                current_point = sc2::Point2DI{best_potential_x - 1, best_potential_y};
//                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
//                    break;
//            }
//            if (best_potential_x < map_to_path[0].size() - 1)
//            {
//                TestPointAndUpdateInformation(best_potential_x + 1, best_potential_y, weight, map_to_path, shortest_path_to_vector_.at(p.first));
//                current_point = sc2::Point2DI{best_potential_x + 1, best_potential_y};
//                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
//                    break;
//            }
//            if (best_potential_y > 0)
//            {
//                TestPointAndUpdateInformation(best_potential_x, best_potential_y - 1, weight, map_to_path, shortest_path_to_vector_.at(p.first));
//                current_point = sc2::Point2DI{best_potential_x, best_potential_y - 1};
//                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
//                    break;
//            }
//            if (best_potential_y < map_to_path.size() - 1)
//            {
//                TestPointAndUpdateInformation(best_potential_x, best_potential_y + 1, weight, map_to_path, shortest_path_to_vector_.at(p.first));
//                current_point = sc2::Point2DI{best_potential_x, best_potential_y + 1};
//                if (shortest_path_to_vector_.at(current_point).size() >= max_run_dist)
//                    break;
//            }
//        }
//    }
//    return shortest_path_to_vector_.at(current_point);
//}

bool Pathfinding::TestDjikstra()
{
    const Tile start_point = { 2, 5 };
    const Tile end_point = { 7, 6 };
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
    //const std::vector<sc2::Point2D> expected_result =
    //{
    //    sc2::Point2D{ 1,0 },
    //    sc2::Point2D{ 1,1 },
    //    sc2::Point2D{ 1,2 },
    //    sc2::Point2D{ 2,2 },
    //    sc2::Point2D{ 3,2 },
    //    sc2::Point2D{ 3,3 },
    //    sc2::Point2D{ 3,4 },
    //    sc2::Point2D{ 4,4 }
    //};

    //std::vector<sc2::Point2D> result = Djikstra(start_point, end_point, map_to_path);
    //if (result[0] == expected_result[0])
    //{
    //    return true;
    //}
    return false;
}

void Pathfinding::SmartPathfind(const sc2::Unit* unit, const sc2::Point2D & target_position, ByunJRBot & bot)
{
    //// Sometimes after we remove the floating points, it will turn out we are trying to move to is almost the same as our current position.
    //// No need to run the pathfinding algorithm in that case. 
    //if (Util::ToPoint2DI(unit->pos)
    //    == Util::ToPoint2DI(target_position))
    //{
    //    Micro::SmartMove(unit, target_position, bot);
    //    return;
    //}

    //Pathfinding p;
    //std::vector<sc2::Point2D> move_path = p.Djikstra(Util::ToPoint2DI(unit->pos),
    //    Util::ToPoint2DI(target_position),
    //    bot.Info().GetDPSMap());
    //Micro::SmartMove(unit, move_path[0], bot);
}


void Pathfinding::SmartRunAway(const sc2::Unit* unit, const int run_distance, ByunJRBot & bot)
{
    Pathfinding p;
    Path move_path = p.SafestPath(bot.Info().GetDPSMap(), Util::ToPoint2DI(unit->pos),
        run_distance
        );
    //SmartMove(unit, move_path[0], bot, false);
    //SmartMove(unit, move_path[1], bot, true);
    //SmartMove(unit, move_path[2], bot, true);

    bot.DebugHelper().DrawLine(unit->pos, move_path[0]);
    bot.DebugHelper().DrawLine(move_path[0], move_path[1]);
    bot.DebugHelper().DrawLine(move_path[1], move_path[2]);
    bot.DebugHelper().DrawLine(move_path[2], move_path[3]);
    bot.DebugHelper().DrawLine(move_path[3], move_path[4]);
    bot.DebugHelper().DrawLine(move_path[4], move_path[5]);
    bot.DebugHelper().DrawLine(move_path[5], move_path[6]);
    Micro::SmartMove(unit, move_path[3], bot, false);
    //for (const auto & j : move_path)
    //{
    //    SmartMove(unit, j, bot, true);
    //}
}