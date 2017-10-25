#include <map>

#include "ai/Pathfinding.h"

// We can't use a set because Point2DI does not have a comparison function, and sets must be "ordered" somehow.
std::vector<sc2::Point2DI> visited;

// The distance of the optimal route to that path.
std::vector<std::pair<sc2::Point2DI, int>> distance_map;
std::map<sc2::Point2D, std::vector<sc2::Point2D>> shortest_path_to_vector;

std::pair<sc2::Point2DI, int> Pathfinding::BestPotentialPair(std::pair<sc2::Point2DI, int> fallback_pair) const
{
    int smallest_distance = std::numeric_limits<int>::max();
    std::pair<sc2::Point2DI, int> optimal_pair = fallback_pair;
    for (const auto & distance_pair : distance_map)
    {
        const sc2::Point2DI point = distance_pair.first;
        const int dist = distance_pair.second;
        // If the distance to the node is 0, we have not yet found a path that leads to it. 
        if (dist < smallest_distance && dist!=0)
        {
            // Find the best pair that we have not yet visited. If we have already visited that point, don't visit it again. 
            if (std::find(visited.begin(), visited.end(), point) == visited.end())
            {
                smallest_distance = dist;
                optimal_pair = distance_pair;
            }
        }
    }
    return optimal_pair;
}


// Will only test and update if the path has not yet been visited, garunteeing that the path will never loop on itself. 
int Pathfinding::TestPointAndUpdateInformation(const int x, const int y, const int current_path_weight,
    const std::vector<std::vector<int>>& map_to_path)
{
    //     The second value of the pair is the current weight of the node we are testing. 
    int new_weight = current_path_weight + map_to_path[y][x];
    // If we have visited the node we are about to test, the path we are considering taking loops back on itself. 
    // Test only the paths that do not loop back on themselves. 
    if (std::find(visited.begin(), visited.end(), sc2::Point2DI{ x,y }) == visited.end())
    {
        std::vector<std::pair<sc2::Point2DI, int>> distance_map;

    }
    return 1;
}


std::vector<sc2::Point2D> Pathfinding::Djikstra(const sc2::Point2DI start_point,
    const sc2::Point2DI end_point,
    const std::vector<std::vector<int>>& map_to_path)
{
    const int width = map_to_path.size();
    const int height = map_to_path[0].size();

    bool have_reached_destination = false;


    const sc2::Point2DI current_point(start_point.x, start_point.y);

    // Setup the values in the distance map.
    for(int y = 0; y < map_to_path.size(); ++y)
    {
        for(int x = 0; x < map_to_path[y].size(); ++x)
        {
            distance_map.push_back(std::pair<sc2::Point2DI, int>{sc2::Point2DI(x, y), 0});
        }
    }

    // If we have not yet found the shortest path between the two points, keep searching for a new path. 
    while (!have_reached_destination)
    {
        std::pair<sc2::Point2DI, int> p = BestPotentialPair(std::pair<sc2::Point2DI, int> { start_point, 1 });
        // Hard copy the current point into the visited set, and test nearby nodes if it has not yet been visited. 
        // visited.set.insert( ... ).second is set to true if we have not yet visited the element. 
        if (std::find(visited.begin(), visited.end(), current_point) == visited.end())
        {
            const int x = p.first.x;
            const int y = p.first.y;
            const int weight = p.second;
            if(x!=0)
                TestPointAndUpdateInformation(x - 1, y, weight, map_to_path);
            TestPointAndUpdateInformation(x + 1, y, weight, map_to_path);
            if (y != 0)
                TestPointAndUpdateInformation(x, y - 1, weight, map_to_path);
            TestPointAndUpdateInformation(x, y + 1, weight, map_to_path);
            visited.push_back(p.first);
        }
        if (current_point == end_point)
            break;
    }
return std::vector<sc2::Point2D>();
}


bool Pathfinding::TestDjikstra()
{
    const sc2::Point2DI start_point = { 0, 0 };
    const sc2::Point2DI end_point = { 4, 4 };
    const std::vector<std::vector<int>> map_to_path =
    {
        { 5, 0, 5, 5, 5 },
        { 5, 0, 5, 5, 5 },
        { 5, 0, 0, 0, 5 },
        { 5, 5, 5, 0, 5 },
        { 5, 5, 5, 0, 0 }
    };
    const std::vector<sc2::Point2D> expected_result =
    {
        sc2::Point2D{ 0,1 } ,
        sc2::Point2D{ 1,1 } ,
        sc2::Point2D{ 1,2 } ,
        sc2::Point2D{ 2,2 } ,
        sc2::Point2D{ 3,2 } ,
        sc2::Point2D{ 3,3 } ,
        sc2::Point2D{ 3,4 } ,
        sc2::Point2D{ 4,4 }
    };

    std::vector<sc2::Point2D> result = Djikstra(start_point, end_point, map_to_path);
    if (result[0] == expected_result[0])
    {
        return true;
    }
    return false;
}
