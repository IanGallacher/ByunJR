#include <sstream>

#include "ByunJRBot.h"
#include "TechLab/information/BaseLocation.h"
#include "TechLab/util/Util.h"

const int NearBaseLocationTileDistance = 5;

BaseLocation::BaseLocation(sc2::Agent & bot, const std::vector<const sc2::Unit*> & resources)
    : bot_(bot)
    , is_start_location_      (false)
    , left_                 (std::numeric_limits<float>::max())
    , right_                (std::numeric_limits<float>::lowest())
    , top_                  (std::numeric_limits<float>::lowest())
    , bottom_               (std::numeric_limits<float>::max())
{
    is_player_start_location_[sc2::Unit::Alliance::Self] = false;
    // Enemy start locations are simply POTENTIAL spawn locations.
    is_player_start_location_[sc2::Unit::Alliance::Enemy] = false;
    is_player_occupying_[sc2::Unit::Alliance::Self] = false;
    is_player_occupying_[sc2::Unit::Alliance::Enemy] = false;

    float resource_center_x = 0;
    float resource_center_y = 0;

    // Add each of the resources to its corresponding container.
    for (auto & resource : resources)
    {
        if (Util::IsMineral(resource))
        {
            minerals_.push_back(resource);

            // Add the position of the minerals to the center.
            resource_center_x += resource->pos.x;
            resource_center_y += resource->pos.y;
        }
        else
        {
            geysers_.push_back(resource);

            // Pull the resource center toward the geyser if it exists.
            resource_center_x += resource->pos.x;
            resource_center_y += resource->pos.y;
        }

        // Set the limits of the base location bounding box.
        const float res_width = 1;
        const float res_height = 0.5;

        left_   = std::min(left_,   resource->pos.x - res_width);
        right_  = std::max(right_,  resource->pos.x + res_width);
        top_    = std::max(top_,    resource->pos.y + res_height);
        bottom_ = std::min(bottom_, resource->pos.y - res_height);
    }

    // Calculate the center of the resources.
    size_t num_resources = minerals_.size() + geysers_.size();

    center_of_resources_ = sc2::Point2D(left_ + (right_-left_)/2.0f, top_ + (bottom_-top_)/2.0f);

    // Check to see if this is a start location for the map.
    for (auto & pos : bot_.Observation()->GetGameInfo().enemy_start_locations)
    {
        if (ContainsPosition(pos))
        {
            is_start_location_ = true;
            is_player_start_location_[sc2::Unit::Alliance::Enemy] = true;
            depot_position_ = pos;
        }
    }
    
    // If this base location position is near our own resource depot, it's our start location.
    for (auto & unit : bot_.Observation()->GetUnits())
    {
        if (Util::GetPlayer(unit) == sc2::Unit::Alliance::Self && Util::IsTownHall(unit) && ContainsPosition(unit->pos))
        {
            is_player_start_location_[sc2::Unit::Alliance::Self] = true;
            is_start_location_ = true;
            is_player_occupying_[sc2::Unit::Alliance::Self] = true;
            break;
        }
    }
    
    // if it's not a start location, we need to calculate the depot position
    //if (!IsStartLocation())
    //{
    //    // the position of the depot will be the closest spot we can build one from the resource center
    //    for (auto & tile : GetClosestTiles())
    //    {
    //        // TODO: depotPosition = depot position for this base location
    //    }
    //}
}

// TODO: Calculate the actual town hall position.
const sc2::Point2D & BaseLocation::GetTownHallPosition() const
{
    return GetPosition();
}

void BaseLocation::SetPlayerOccupying(const sc2::Unit::Alliance player, const bool occupying)
{
    is_player_occupying_[player] = occupying;

    // If this base is a start location that's occupied by the enemy, it's that enemy's start location.
    if (occupying && player == sc2::Unit::Alliance::Enemy && IsStartLocation() && is_player_start_location_[player] == false)
    {
        is_player_start_location_[player] = true;
    }
}

bool BaseLocation::IsInResourceBox(const int x, const int y) const
{
    return x >= left_ && x < right_ && y < top_ && y >= bottom_;
}

bool BaseLocation::IsOccupiedByPlayer(const sc2::Unit::Alliance player) const
{
    return is_player_occupying_.at(player);
}

bool BaseLocation::IsExplored() const
{
    return false;
}

// IsPlayerStartLocation returns if you spawned at the given location. 
// IsPlayerStartLocation.at(sc2::Unit::Alliance::Enemy) only gives POTENTIAL enemy spawn locations. 
// For clarity, there is a seprate function for that. 
bool BaseLocation::IsPlayerStartLocation() const
{
    return is_player_start_location_.at(sc2::Unit::Alliance::Self);
}

bool BaseLocation::IsPotentialEnemyStartLocation() const
{
    return is_player_start_location_.at(sc2::Unit::Alliance::Enemy);
}

bool BaseLocation::IsMineralOnly() const
{
    return GetGeysers().empty();
}

// Warning: does not check to see if the tile is on the map. 
bool BaseLocation::ContainsPosition(const sc2::Point2D & pos) const
{
    return GetGroundDistance(pos) < NearBaseLocationTileDistance;
}

const std::vector<const sc2::Unit*>& BaseLocation::GetGeysers() const
{
    return geysers_;
}

const std::vector<const sc2::Unit*>& BaseLocation::GetMinerals() const
{
    return minerals_;
}

const sc2::Point2D & BaseLocation::GetPosition() const
{
    return center_of_resources_;
}

int BaseLocation::GetGroundDistance(const sc2::Point2D & pos) const
{
    return Util::Dist(pos, center_of_resources_);
    //return distanceMap.getDistance(pos);
}

bool BaseLocation::IsStartLocation() const
{
    return is_start_location_;
}