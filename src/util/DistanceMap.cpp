#include <sstream>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "util/DistanceMap.h"
#include "util/Util.h"

const size_t LegalActions = 4;
const int actionX[LegalActions] = {1, -1, 0, 0};
const int actionY[LegalActions] = {0, 0, 1, -1};

DistanceMap::DistanceMap() 
{
    
}

int DistanceMap::GetDistance(int tile_x, int tile_y) const
{ 
    BOT_ASSERT(tile_x < width_ && tile_y < height_, "Index out of range: X = %d, Y = %d", tile_x, tile_y);
    return dist_[tile_x][tile_y]; 
}

int DistanceMap::GetDistance(const sc2::Point2DI & pos) const
{ 
    return GetDistance(pos.x, pos.y); 
}

const std::vector<sc2::Point2DI>& DistanceMap::GetSortedTiles() const
{
    return sorted_tile_positions_;
}

// Computes dist[x][y] = ground distance from (startX, startY) to (x,y)
// Uses BFS, since the map is quite large and DFS may cause a stack overflow
void DistanceMap::ComputeDistanceMap(ByunJRBot& bot, const sc2::Point2DI & start_tile)
{
    start_tile_ = start_tile;
    width_ = bot.Map().Width();
    height_ = bot.Map().Width();
    dist_ = vvi (width_, std::vector<int>(height_, -1));
    sorted_tile_positions_.reserve(width_ * height_);

    // the fringe for the BFS we will perform to calculate distances
    std::vector<sc2::Point2DI> fringe;
    fringe.reserve(width_ * height_);
    fringe.push_back(start_tile_);
    sorted_tile_positions_.push_back(start_tile);

    dist_[static_cast<int>(start_tile.x)][static_cast<int>(start_tile.y)] = 0;

    for (size_t fringe_index=0; fringe_index<fringe.size(); ++fringe_index)
    {
        const sc2::Point2DI & tile = fringe[fringe_index];

        // check every possible child of this tile
        for (size_t a=0; a<LegalActions; ++a)
        {
            const sc2::Point2DI next_tile(tile.x + actionX[a], tile.y + actionY[a]);

            // if the new tile is inside the map bounds, is walkable, and has not been visited yet, set the distance of its parent + 1
            if (bot.Map().IsWalkable(next_tile) && GetDistance(next_tile) == -1)
            {
                dist_[static_cast<int>(next_tile.x)][static_cast<int>(next_tile.y)] = dist_[(int)tile.x][(int)tile.y] + 1;
                fringe.push_back(next_tile);
                sorted_tile_positions_.push_back(next_tile);
            }
        }
    }
}

void DistanceMap::Draw(ByunJRBot & bot) const
{
    const int tiles_to_draw = 200;
    for (size_t i(0); i < tiles_to_draw; ++i)
    {
        auto & tile = sorted_tile_positions_[i];
        const int dist = GetDistance(tile.x, tile.y);

        const sc2::Point2D text_pos(tile.x + 0.5f, tile.y + 0.5f);
        std::stringstream ss;
        ss << dist;

        bot.DebugHelper().DrawText(text_pos, ss.str());
    }
}

const sc2::Point2DI& DistanceMap::GetStartTile() const
{
    return start_tile_;
}