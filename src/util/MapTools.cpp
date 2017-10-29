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
    , true_map_width_   (0)
    , true_map_height_  (0)
    , max_z_    (0.0f)
    , frame_   (0)
{

}

void MapTools::OnStart()
{
    true_map_width_  = bot_.Observation()->GetGameInfo().width;
    true_map_height_ = bot_.Observation()->GetGameInfo().height;
    BOT_ASSERT(true_map_width_ != 0, "True map width is zero!");
    BOT_ASSERT(true_map_height_ != 0, "Play area height is zero!");


    playable_map_width_ = static_cast<int>(bot_.Observation()->GetGameInfo().playable_max.x - bot_.Observation()->GetGameInfo().playable_min.x);
    playable_map_height_ = static_cast<int>(bot_.Observation()->GetGameInfo().playable_max.y - bot_.Observation()->GetGameInfo().playable_min.y);
    BOT_ASSERT(playable_map_width_ != 0, "Play area with is zero!");
    BOT_ASSERT(playable_map_height_ != 0, "Play area height is zero!");


    walkable_        = vvb(true_map_width_, std::vector<bool>(true_map_height_, true));
    buildable_       = vvb(true_map_width_, std::vector<bool>(true_map_height_, false));
    depot_buildable_ = vvb(true_map_width_, std::vector<bool>(true_map_height_, false));
    last_seen_       = vvi(true_map_width_, std::vector<int>(true_map_height_, 0));
    sector_number_   = vvi(true_map_width_, std::vector<int>(true_map_height_, 0));
    terrain_height_  = vvf(true_map_width_, std::vector<float>(true_map_height_, 0.0f));

    // Set the boolean grid data from the Map
    for (size_t x(0); x < true_map_width_; ++x)
    {
        for (size_t y(0); y < true_map_height_; ++y)
        {
            buildable_[x][y]        = Util::Placement(bot_.Observation()->GetGameInfo(), sc2::Point2D(x, y));
            walkable_[x][y]         = buildable_[x][y] || Util::Pathable(bot_.Observation()->GetGameInfo(), sc2::Point2D(x, y));
            terrain_height_[x][y]   = bot_.Observation()->TerrainHeight(sc2::Point2D(x, y));
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
    fringe.reserve(true_map_width_*true_map_height_);
    int sector_number = 0;

    // for every tile on the map, do a connected flood fill using BFS
    for (int x=0; x<true_map_width_; ++x)
    {
        for (int y=0; y<true_map_height_; ++y)
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

    const sc2::Visibility vis = bot_.Observation()->GetVisibility(pos);
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
    return x >= 0 && y >= 0 && x < true_map_width_ && y < true_map_height_;
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
    for (int y(0); y < true_map_height_; ++y)
    {
        for (int x(0); x < true_map_width_; ++x)
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


// There are two coordinate systems for storing the map locations.
// "True Map Space" - Some maps are larger than the total play area.
// "Training Space" or "Arena Space" - The play area only.
// To convert from training space to true map space, add playable_min.
#pragma region Map Dimension Utilities
int MapTools::TrueMapWidth() const
{
    return true_map_width_;
}

int MapTools::TrueMapHeight() const
{
    return true_map_height_;
}

int MapTools::PlayableMapWidth() const
{
    return playable_map_width_;
}

int MapTools::PlayableMapHeight() const
{
    return playable_map_height_;
}
#pragma endregion


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

bool MapTools::IsTileTypeOf(const int x, const int y, const MapTileType tile_type) const
{
    if (tile_type == MapTileType::CantWalk
     && !bot_.Map().IsWalkable(x, y))
         return true;;
    if (tile_type == MapTileType::CantBuild
     && !bot_.Map().IsBuildable(x, y))
         return true;
    if (tile_type == MapTileType::Ramp &&
        bot_.Map().IsWalkable(x, y) && !bot_.Map().IsBuildable(x, y))
        return true;
    return false;
}

bool MapTools::IsTileAdjacentToTileType(const sc2::Point2DI p, const MapTileType tile_type) const
{
    if(p.x > 0 && IsTileTypeOf(p.x-1, p.y, tile_type))
        return true;
    if (p.x < true_map_width_-1 && IsTileTypeOf(p.x+1, p.y, tile_type))
        return true;
    if (p.y > 0 && IsTileTypeOf(p.x, p.y-1, tile_type))
        return true;
    if (p.y < true_map_height_-1 && IsTileTypeOf(p.x, p.y+1, tile_type))
        return true;
    return false;
}

bool MapTools::IsTileCornerOfTileType(const sc2::Point2DI p, const MapTileType tile_type) const
{
    if (p.x > 0 && p.y < true_map_height_ - 1 &&
        IsTileTypeOf(p.x - 1, p.y + 1, tile_type))
        return true;
    if (p.x < true_map_width_ - 1 && p.y < true_map_height_ - 1 &&
        IsTileTypeOf(p.x + 1, p.y + 1, tile_type))
        return true;
    if (p.x > 0 && p.y > 0 &&
        IsTileTypeOf(p.x - 1, p.y - 1, tile_type))
        return true;
    if (p.x < true_map_width_ - 1 && p.y > 0 &&
        IsTileTypeOf(p.x + 1, p.y - 1, tile_type))
        return true;
    return false;
}

bool MapTools::IsAnyTileAdjacentToTileType(const sc2::Point2DI p, const MapTileType tile_type, const sc2::UnitTypeID building_type) const
{
    const int width = Util::GetUnitTypeWidth(building_type, bot_);
    const int height = Util::GetUnitTypeHeight(building_type, bot_);

    // TODO: make sure we leave space for add-ons. These types of units can have addons:

    // define the rectangle of the building spot
    const int startx = p.x-width/2;
    const int starty = p.y-width/2;
    const int endx = p.x + width/2;
    const int endy = p.y + height/2;

    // TODO: recalculate start and end positions for addons

    // if this rectangle doesn't fit on the map we can't build here
    if (endx < 0 || endy < 0 || startx > bot_.Map().TrueMapWidth() || starty > bot_.Map().TrueMapHeight())
    {
        return false;
    }

    // if we can't build here, or space is reserved, or it's in the resource box, we can't build here.
    // Yes, < is correct. Don't use <=.
    // Due to how starcraft calculates the tile type, things are sort of "shifted down and left" slightly. 
    // Turn on DrawTileInfo to see the "shifting" in action. 
    for (int x = startx; x < endx; ++x)
    {
        for (int y = starty; y < endy; ++y)
        {
            if (IsTileAdjacentToTileType(sc2::Point2DI(x,y), tile_type))
            {
                return true;
            }
        }
    }
    return false;
}

sc2::Point2D MapTools::GetNextCoordinateToWallWithBuilding(const sc2::UnitTypeID building_type) const
{
    sc2::Point2D closest_point(0, 0);
    double closest_distance = std::numeric_limits<double>::max();

    // Get the closest ramp to our starting base. 
    const sc2::Point2D base_location = bot_.Bases().GetPlayerStartingBaseLocation(PlayerArrayIndex::Self)->GetPosition();

    // No need to iterate through the edges of the map, as the edge can never be part of our wall. 
    // The smallest building is width 2, so shrink the iteration dimensions by that amount. 
    for (int y = 2; y < (true_map_height_ - 2); ++y)
    {
        for (int x = 2; x < (true_map_width_ - 2); ++x)
        {
            // If we can walk on it, but not build on it, it is most likely a ramp.
            // TODO: That is not actually correct, come up with a beter way to detect ramps. 
            if (IsAnyTileAdjacentToTileType(sc2::Point2DI(x,y),MapTileType::Ramp, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT)
                && bot_.Query()->Placement(Util::UnitTypeIDToAbilityID(building_type), sc2::Point2D(static_cast<float>(x), static_cast<float>(y))))
            {
            // The first depot in a wall has to be next to, well, a wall. 
            // This allows the depot wall to be built correctly on AbyssalReefLE.
            if (!IsTileCornerOfTileType( sc2::Point2DI(x, y), MapTileType::CantWalk)
                && bot_.InformationManager().UnitInfo().GetNumDepots(PlayerArrayIndex::Self) == 0)
                continue;
                const sc2::Point2D point(x, y);
                const double distance = Util::DistSq(point, base_location);
                if (distance < closest_distance)
                {
                    closest_point = point;
                    closest_point.x;
                    closest_distance = distance;
                }
            }
        }
    }
    return closest_point;
}