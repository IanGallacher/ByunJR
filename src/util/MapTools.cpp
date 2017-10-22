#include <sstream>
#include <fstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "common/BotAssert.h"
#include "util/MapTools.h"
#include "util/Util.h"

const size_t LegalActions = 4;
const int actionX[LegalActions] ={1, -1, 0, 0};
const int actionY[LegalActions] ={0, 0, 1, -1};

// constructor for MapTools
MapTools::MapTools(ByunJRBot& bot)
    : bot_     (bot)
    , width_   (0)
    , height_  (0)
    , max_z_    (0.0f)
    , frame_   (0)
{

}

void MapTools::OnStart()
{
    width_  = bot_.Observation()->GetGameInfo().width;
    height_ = bot_.Observation()->GetGameInfo().height;

    walkable_       = vvb(width_, std::vector<bool>(height_, true));
    buildable_      = vvb(width_, std::vector<bool>(height_, false));
    depot_buildable_ = vvb(width_, std::vector<bool>(height_, false));
    last_seen_       = vvi(width_, std::vector<int>(height_, 0));
    sector_number_   = vvi(width_, std::vector<int>(height_, 0));
    terrain_height_  = vvf(width_, std::vector<float>(height_, 0.0f));

    // Set the boolean grid data from the Map
    for (size_t x(0); x < width_; ++x)
    {
        for (size_t y(0); y < height_; ++y)
        {
            buildable_[x][y]       = Util::Placement(bot_.Observation()->GetGameInfo(), sc2::Point2D(x+0.5f, y+0.5f));
            walkable_[x][y]        = buildable_[x][y] || Util::Pathable(bot_.Observation()->GetGameInfo(), sc2::Point2D(x+0.5f, y+0.5f));
            terrain_height_[x][y]   = Util::TerainHeight(bot_.Observation()->GetGameInfo(), sc2::Point2D(x+0.5f, y+0.5f));
        }
    }

    for (auto& unit : bot_.Observation()->GetUnits(sc2::Unit::Alliance::Neutral))
    {
        max_z_ = std::max(unit->pos.z, max_z_);
    }

    ComputeConnectivity();
}

void MapTools::OnFrame()
{
 /*   frame++;

    for (int x=0; x<m_width; ++x)
    {
        for (int y=0; y<m_height; ++y)
        {
            if (isVisible(sc2::Point2DI((float)x, (float)y)))
            {
                lastSeen[x][y] = frame;
            }
        }
    }

    draw();*/
}

void MapTools::ComputeConnectivity()
{
    // the fringe data structe we will use to do our BFS searches
    std::vector<sc2::Point2DI> fringe;
    fringe.reserve(width_*height_);
    int sector_number = 0;

    // for every tile on the map, do a connected flood fill using BFS
    for (int x=0; x<width_; ++x)
    {
        for (int y=0; y<height_; ++y)
        {
            // if the sector is not currently 0, or the map isn't walkable here, then we can skip this tile
            if (GetSectorNumber(x, y) != 0 || !IsWalkable(x, y))
            {
                continue;
            }

            // increase the sector number, so that walkable tiles have sectors 1-N
            sector_number++;

            // reset the fringe for the search and add the start tile to it
            fringe.clear();
            fringe.push_back(sc2::Point2DI(x+0.5f, y+0.5f));
            sector_number_[x][y] = sector_number;

            // do the BFS, stopping when we reach the last element of the fringe
            for (size_t fringe_index=0; fringe_index<fringe.size(); ++fringe_index)
            {
                auto& tile = fringe[fringe_index];

                // check every possible child of this tile
                for (size_t a=0; a<LegalActions; ++a)
                {
                    const sc2::Point2DI next_tile(tile.x + actionX[a], tile.y + actionY[a]);

                    // if the new tile is inside the map bounds, is walkable, and has not been assigned a sector, add it to the current sector and the fringe
                    if (IsOnMap(next_tile.x, next_tile.y) && IsWalkable(next_tile) && (GetSectorNumber(next_tile) == 0))
                    {
                        sector_number_[next_tile.x][next_tile.y] = sector_number;
                        fringe.push_back(next_tile);
                    }
                }
            }
        }
    }
}

bool MapTools::IsExplored(const sc2::Point2D& pos) const
{
    if (!IsOnMap(pos)) { return false; }

    sc2::Visibility vis = bot_.Observation()->GetVisibility(pos);
    return vis == sc2::Visibility::Fogged || vis == sc2::Visibility::Visible;
}

bool MapTools::IsVisible(const sc2::Point2D& pos) const
{
    if (!IsOnMap(pos)) { return false; }

    return bot_.Observation()->GetVisibility(pos) == sc2::Visibility::Visible;
}

bool MapTools::IsPowered(const sc2::Point2DI& pos) const
{
    for (auto& power_source : bot_.Observation()->GetPowerSources())
    {
        if (Util::Dist(pos, power_source.position) < power_source.radius)
        {
            return true;
        }
    }

    return false;
}

float MapTools::TerrainHeight(float x, float y) const
{
    return terrain_height_[x][y];
}

//int MapTools::getGroundDistance(const sc2::Point2DI& src, const sc2::Point2DI& dest) const
//{
//    return Util::Dist(src, dest);
//}

int MapTools::GetGroundDistance(const sc2::Point2DI& src, const sc2::Point2DI& dest) const
{
    if (all_maps_.size() > 50)
    {
        all_maps_.clear();
    }

    return GetDistanceMap(dest).GetDistance(src);
}

int MapTools::GetGroundDistance(const sc2::Point2D& src, const sc2::Point2D& dest) const
{
    return GetGroundDistance(
        sc2::Point2DI(static_cast<int>(src.x),  static_cast<int>(src.y)),
        sc2::Point2DI(static_cast<int>(dest.x), static_cast<int>(dest.y))
    );
}

const DistanceMap& MapTools::GetDistanceMap(const sc2::Point2DI& tile) const
{
    const std::pair<int, int> int_tile(tile.x, tile.y);

    if (all_maps_.find(int_tile) == all_maps_.end())
    {
        all_maps_[int_tile] = DistanceMap();
        all_maps_[int_tile].ComputeDistanceMap(bot_, tile);
    }

    return all_maps_[int_tile];
}

int MapTools::GetSectorNumber(const int x, const int y) const
{
    if (!IsOnMap(x, y))
    {
        return 0;
    }

    return sector_number_[x][y];
}

int MapTools::GetSectorNumber(const sc2::Point2DI& pos) const
{
    return GetSectorNumber(pos.x, pos.y);
}

// Returns true if the point is on the map, and not just the playable portions of the map.
bool MapTools::IsOnMap(const int x, const int y) const
{
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

// Returns true if the point is on the map, and not just the playable portions of the map.
bool MapTools::IsOnMap(const sc2::Point2D& pos) const
{
    return IsOnMap(pos.x, pos.y);
}

// Returns true if the point is on the map, and not just the playable portions of the map.
bool MapTools::IsOnMap(const sc2::Point2DI& pos) const
{
    return IsOnMap(pos.x, pos.y);
}

void MapTools::Draw() const
{
    const sc2::Point2D camera = bot_.Observation()->GetCameraPos();
    for (float x = camera.x - 16.0f; x < camera.x + 16.0f; ++x)
    {
        for (float y = camera.y - 16.0f; y < camera.y + 16.0f; ++y)
        {
            if (!IsOnMap(x, y))
            {
                continue;
            }

            if (bot_.Config().DrawWalkableSectors)
            {
                std::stringstream ss;
                ss << GetSectorNumber(x, y);
                bot_.Debug()->DebugTextOut(ss.str(), sc2::Point3D(x + 0.5f, y + 0.5f, max_z_ + 0.1f), sc2::Colors::Yellow);
            }

            if (bot_.Config().DrawTileInfo)
            {
                sc2::Color color = IsWalkable(x, y) ? sc2::Colors::Green : sc2::Colors::Red;
                if (IsWalkable(x, y) && !IsBuildable(x, y))
                {
                    color = sc2::Colors::Yellow;
                }

                bot_.DebugHelper().DrawSquare(x, y, x+1, y+1, color);
            }
        }
    }
}

bool MapTools::IsConnected(const int x1, const int y1, const int x2, const int y2) const
{
    if (!IsOnMap(x1, y1) || !IsOnMap(x2, y2))
    {
        return false;
    }

    const int s1 = GetSectorNumber(x1, y1);
    const int s2 = GetSectorNumber(x2, y2);

    return s1 != 0 && (s1 == s2);
}

bool MapTools::IsConnected(const sc2::Point2DI& p1, const sc2::Point2DI& p2) const
{
    return IsConnected(p1.x, p1.y, p2.x, p2.y);
}

bool MapTools::IsBuildable(const int x, const int y) const
{
    if (!IsOnMap(x, y))
    {
        return false;
    }

    return buildable_[x][y];
}

bool MapTools::CanBuildTypeAtPosition(const int x, const int y, const sc2::UnitTypeID type) const
{
    return bot_.Query()->Placement(Util::UnitTypeIDToAbilityID(type), sc2::Point2D(static_cast<float>(x), static_cast<float>(y)));
}

bool MapTools::IsBuildable(const sc2::Point2DI& tile) const
{
    return IsBuildable(tile.x, tile.y);
}

void MapTools::PrintMap() const
{
    std::stringstream ss;
    for (int y(0); y < height_; ++y)
    {
        for (int x(0); x < width_; ++x)
        {
            ss << IsWalkable(x, y);
        }

        ss << std::endl;
    }

    std::ofstream out("map.txt");
    out << ss.str();
    out.close();
}

bool MapTools::IsDepotBuildableTile(const sc2::Point2D& tile) const
{
    if (!IsOnMap(tile))
    {
        return false;
    }

    return depot_buildable_[tile.x][tile.y];
}

bool MapTools::IsWalkable(const int x, const int y) const
{
    if (!IsOnMap(x, y))
    {
        return false;
    }

    return walkable_[x][y];
}

bool MapTools::IsWalkable(const sc2::Point2DI& tile) const
{
    return IsWalkable(tile.x, tile.y);
}

int MapTools::Width() const
{
    return width_;
}

int MapTools::Height() const
{
    return height_;
}

const std::vector<sc2::Point2DI>& MapTools::GetClosestTilesTo(const sc2::Point2DI& pos) const
{
    return GetDistanceMap(pos).GetSortedTiles();
}

sc2::Point2DI MapTools::GetLeastRecentlySeenPosition() const
{
    int min_seen = std::numeric_limits<int>::max();
    sc2::Point2DI least_seen(0, 0);
    const BaseLocation* base_location = bot_.Bases().GetPlayerStartingBaseLocation(PlayerArrayIndex::Self);

    for (auto& tile : base_location->GetClosestTiles())
    {
        BOT_ASSERT(IsOnMap(tile), "How is this tile not valid?");

        const int last_seen = last_seen_[static_cast<int>(tile.x)][static_cast<int>(tile.y)];
        if (last_seen < min_seen)
        {
            min_seen = last_seen;
            least_seen = tile;
        }
    }

    return least_seen;
}