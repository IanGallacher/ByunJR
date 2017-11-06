#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "information/BaseLocation.h"
#include "util/Util.h"

const int NearBaseLocationTileDistance = 20;

BaseLocation::BaseLocation(ByunJRBot & bot, const int baseID, const std::vector<const sc2::Unit*> & resources)
    : bot_(bot)
    , baseID               (baseID)
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

    // add each of the resources to its corresponding container
    for (auto & resource : resources)
    {
        if (Util::IsMineral(resource))
        {
            minerals_.push_back(resource);
            mineral_positions_.push_back(resource->pos);

            // add the position of the minerals to the center
            resource_center_x += resource->pos.x;
            resource_center_y += resource->pos.y;
        }
        else
        {
            geysers_.push_back(resource);
            geyser_positions_.push_back(resource->pos);

            // pull the resource center toward the geyser if it exists
            resource_center_x += resource->pos.x;
            resource_center_y += resource->pos.y;
        }

        // set the limits of the base location bounding box
        const float res_width = 1;
        const float res_height = 0.5;

        left_   = std::min(left_,   resource->pos.x - res_width);
        right_  = std::max(right_,  resource->pos.x + res_width);
        top_    = std::max(top_,    resource->pos.y + res_height);
        bottom_ = std::min(bottom_, resource->pos.y - res_height);
    }

    // calculate the center of the resources
    size_t num_resources = minerals_.size() + geysers_.size();

    center_of_resources_ = sc2::Point2D(left_ + (right_-left_)/2.0f, top_ + (bottom_-top_)/2.0f);

    // compute this BaseLocation's DistanceMap, which will compute the ground distance
    // from the center of its recourses to every other tile on the map
    distance_map_ = bot_.InformationManager().Map().GetDistanceMap(sc2::Point2DI(center_of_resources_.x, center_of_resources_.y));

    // check to see if this is a start location for the map
    for (auto & pos : bot_.Observation()->GetGameInfo().enemy_start_locations)
    {
        if (ContainsPosition(pos))
        {
            is_start_location_ = true;
            is_player_start_location_[sc2::Unit::Alliance::Enemy] = true;
            depot_position_ = pos;
        }
    }
    
    // if this base location position is near our own resource depot, it's our start location
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
    if (!IsStartLocation())
    {
        // the position of the depot will be the closest spot we can build one from the resource center
        for (auto & tile : GetClosestTiles())
        {
            // TODO: depotPosition = depot position for this base location
        }
    }
}

// TODO: calculate the actual depot position
const sc2::Point2D & BaseLocation::GetDepotPosition() const
{
    return GetPosition();
}

void BaseLocation::SetPlayerOccupying(const sc2::Unit::Alliance player, const bool occupying)
{
    is_player_occupying_[player] = occupying;

    // if this base is a start location that's occupied by the enemy, it's that enemy's start location
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
// isPlayerStartLocation returns if you spawned at the given location. 
// isPlayerStartLocation.at(sc2::Unit::Alliance::Enemy) only gives POTENTIAL enemy spawn locations. 
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

bool BaseLocation::ContainsPosition(const sc2::Point2D & pos) const
{
    if (!bot_.InformationManager().Map().IsOnMap(pos) || (pos.x == 0 && pos.y == 0))
    {
        return false;
    }

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
    //return depotPosition;
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

const std::vector<sc2::Point2DI> & BaseLocation::GetClosestTiles() const
{
    return distance_map_.GetSortedTiles();
}

void BaseLocation::Draw()
{
    bot_.DebugHelper().DrawSphere(center_of_resources_, 1.0f, sc2::Colors::Yellow);

    std::stringstream ss;
    ss << "BaseLocation: " << baseID << std::endl;
    ss << "Start Loc:    " << (IsStartLocation() ? "true" : "false") << std::endl;
    ss << "Minerals:     " << mineral_positions_.size() << std::endl;
    ss << "Geysers:      " << geyser_positions_.size() << std::endl;
    ss << "Occupied By:  ";

    if (IsOccupiedByPlayer(sc2::Unit::Alliance::Self))
    {
        ss << "Self ";
    }

    if (IsOccupiedByPlayer(sc2::Unit::Alliance::Enemy))
    {
        ss << "Enemy ";
    }

    bot_.DebugHelper().DrawText(sc2::Point2D(left_, top_+3), ss.str().c_str());
    bot_.DebugHelper().DrawText(sc2::Point2D(left_, bottom_), ss.str().c_str());

    // draw the base bounding box
    bot_.DebugHelper().DrawBox(left_, top_, right_, bottom_);

    for (float x=left_; x < right_; ++x)
    {
        bot_.DebugHelper().DrawLine(x, top_, x, bottom_, sc2::Color(160, 160, 160));
    }

    for (float y=bottom_; y<top_; ++y)
    {
        bot_.DebugHelper().DrawLine(left_, y, right_, y, sc2::Color(160, 160, 160));
    }

    for (auto & mineral_pos : mineral_positions_)
    {
        bot_.DebugHelper().DrawSphere(mineral_pos, 1.0f, sc2::Colors::Teal);
    }

    for (auto & geyser_pos : geyser_positions_)
    {
        bot_.DebugHelper().DrawSphere(geyser_pos, 1.0f, sc2::Colors::Green);
    }

    if (is_start_location_)
    {
        bot_.DebugHelper().DrawSphere(depot_position_, 1.0f, sc2::Colors::Red);
    }
    
    const float cc_width = 5;
    const float cc_height = 4;
    const sc2::Point2D boxtl = depot_position_ - sc2::Point2D(cc_width/2, -cc_height/2);
    const sc2::Point2D boxbr = depot_position_ + sc2::Point2D(cc_width/2, -cc_height/2);

    bot_.DebugHelper().DrawBox(boxtl.x, boxtl.y, boxbr.x, boxbr.y, sc2::Colors::Red);

    //m_distanceMap.draw(bot_);
}