#include "ByunJRBot.h"
#include "TechLab/util/Util.h"
#include "TechLab/util/Timer.hpp"

#include "macro/BuildingPlacer.h"

BuildingPlacer::BuildingPlacer(ByunJRBot & bot)
    : bot_(bot)
{ }

void BuildingPlacer::OnStart()
{
    reserve_map_ = std::vector<std::vector<bool>> (bot_.Info().Map().TrueMapWidth(), std::vector<bool>(bot_.Info().Map().TrueMapHeight(), false));
}

sc2::Point2DI BuildingPlacer::GetBuildLocationForType(const sc2::UnitTypeID type) const
{
    if (build_location_cache_.find(type) != build_location_cache_.end())
    {
        return build_location_cache_.at(type);
    }

    sc2::Point2DI desired_loc;
    if (Util::IsRefineryType(type))
    {
        desired_loc = bot_.Strategy().BuildingPlacer().GetRefineryPosition();
    }

    else if (type == sc2::UNIT_TYPEID::TERRAN_BARRACKS)
    {
        desired_loc = bot_.GetProxyManager().GetProxyLocation();
    }

    // Make a wall if necessary.
    else if (type == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT && bot_.Info().UnitInfo().GetNumDepots(sc2::Unit::Alliance::Self) < 3)
    {
        desired_loc = bot_.Info().Map().GetNextCoordinateToWallWithBuilding(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    }

    // Find the next expansion location. 
    else if (Util::IsTownHallType(type))
    {
        const sc2::Point2D next_expansion_location = bot_.Info().Bases().GetNextExpansion(sc2::Unit::Alliance::Self);
        desired_loc = sc2::Point2DI(next_expansion_location.x, next_expansion_location.y);
    }
    // If no special placement code is required, get a position somewhere in our starting base.
    else
    {
        desired_loc = sc2::Point2DI(bot_.GetStartLocation().x, bot_.GetStartLocation().y);
    }

    // Return a "null" point if the desired_loc was not on the map. 
    if (!bot_.Info().Map().IsOnMap(desired_loc))
    {
        return sc2::Point2DI(0, 0);
    }

    build_location_cache_[type] = GetBuildLocationNear(desired_loc, type, bot_.Config().BuildingSpacing);
    return build_location_cache_[type];
}

void BuildingPlacer::ReserveTiles(sc2::UnitTypeID building_type, sc2::Point2DI building_location)
{
    // Remember to take add-ons into account!
    int space_for_add_on = 0;
    if (building_type == sc2::UNIT_TYPEID::TERRAN_BARRACKS || building_type == sc2::UNIT_TYPEID::TERRAN_FACTORY || building_type == sc2::UNIT_TYPEID::TERRAN_STARPORT)
        space_for_add_on = 2;

    const int building_width = Util::GetUnitTypeWidth(building_type, bot_) + space_for_add_on;
    const int building_height = Util::GetUnitTypeHeight(building_type, bot_);
    const size_t rwidth = reserve_map_.size();
    const size_t rheight = reserve_map_[0].size();
    for (size_t x = building_location.x; x < building_location.x + building_width && x < rwidth; x++)
    {
        for (size_t y = building_location.y; y < building_location.y + building_height && y < rheight; y++)
        {
            reserve_map_[x][y] = true;
        }
    }
    // Remove the placed building from the cache.
    const auto build_loc_iter = build_location_cache_.find(building_type);
    if (build_loc_iter != build_location_cache_.end())
    {
        build_location_cache_.erase(build_loc_iter);
    }
}

void BuildingPlacer::FreeTiles(sc2::UnitTypeID building_type, sc2::Point2DI building_location)
{
    // Remember to take add-ons into account!
    int space_for_add_on = 0;
    if (building_type == sc2::UNIT_TYPEID::TERRAN_BARRACKS || building_type == sc2::UNIT_TYPEID::TERRAN_FACTORY || building_type == sc2::UNIT_TYPEID::TERRAN_STARPORT)
        space_for_add_on = 2;

    const int building_width = Util::GetUnitTypeWidth(building_type, bot_) + space_for_add_on;
    const int building_height = Util::GetUnitTypeHeight(building_type, bot_);

    const int rwidth = static_cast<int>(reserve_map_.size());
    const int rheight = static_cast<int>(reserve_map_[0].size());

    for (size_t x = building_location.x; x < building_location.x + building_width && x < rwidth; x++)
    {
        for (size_t y = building_location.y; y < building_location.y + building_height && y < rheight; y++)
        {
            reserve_map_[x][y] = false;
        }
    }
}

bool BuildingPlacer::IsReserved(const int x, const int y) const
{
    const int rwidth = static_cast<int>(reserve_map_.size());
    const int rheight = static_cast<int>(reserve_map_[0].size());
    if (x < 0 || y < 0 || x >= rwidth || y >= rheight)
    {
        return false;
    }

    return reserve_map_[x][y];
}

bool BuildingPlacer::IsInResourceBox(const int x, const int y) const
{
    // GetBaseAtLocation will only return a base if the given point is inside that bases resource box.
    if(bot_.Info().Bases().GetBaseAtLocation(x,y)) return true;
    return false;
}

// makes final checks to see if a building can be built at a certain location
bool BuildingPlacer::CanBuildHere(const int bx, const int by, const sc2::UnitTypeID type) const
{
    // Don't build to close to a base unless that building is a command center.
    if (IsInResourceBox(by, by) && !Util::IsTownHallType(type))
        return false;

    // We are not allowed to build on any tile that we have reserved.
    for (int x = bx; x < bx + Util::GetUnitTypeWidth(type, bot_); x++)
    {
        for (int y = by; y < by + Util::GetUnitTypeHeight(type, bot_); y++)
        {
            if (!bot_.Info().Map().IsOnMap(x, y) || reserve_map_[x][y])
            {
                return false;
            }
        }
    }

    // Will the starcraft engine allow the building to be built at the location?
    if (!Buildable(bx, by, type))
        return false;

    // If none of the above conditions failed, we must be allowed to build at the location.
    return true;
}

//returns true if we can build this type of unit here with the specified amount of space.
bool BuildingPlacer::CanBuildHereWithSpace(const int bx, const int by, const sc2::UnitTypeID type, const int build_dist) const
{
    //if we can't build here, we of course can't build here with space
    if (!CanBuildHere(bx, by, type))
    {
        return false;
    }

    // height and width of the building
    const int width  = Util::GetUnitTypeWidth(type, bot_);
    const int height = Util::GetUnitTypeHeight(type, bot_);

    // define the rectangle of the building spot
    const int startx = bx - (width / 2) - build_dist;
    const int starty = by - (height / 2) - build_dist;
    // endx is not const to account for add-ons. 
    int endx = bx + (width/2) + build_dist;
    const int endy = by + (height/2) + build_dist;

    // Make sure the building fits on the map. 
    if (startx < 0 
     || starty < 0 
	 || endx > bot_.Info().Map().TrueMapWidth() 
	 || endy > bot_.Info().Map().TrueMapHeight() 
	 || build_dist < 0)
        return false;

    // Account for add-ons.
    if( type == sc2::UNIT_TYPEID::TERRAN_BARRACKS 
     || type == sc2::UNIT_TYPEID::TERRAN_FACTORY
     || type == sc2::UNIT_TYPEID::TERRAN_STARPORT)
    {
        endx += 3;

        // Make sure there is room to build the addon. 
        if (!Buildable(bx+3, by, type))
            return false;
    }
    
    for(int y = starty; y < endy; ++y)
    {
        for(int x  = startx; x < endx; ++x)
        {
            // Refineries can only be built in one spot, it does not matter what is next to them. 
            // Supply depots are usually part of a wall. They have to be built flush with the buildings next to them. 
            if (!Util::IsRefineryType(type) && type != sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT)
            {
                // Make sure we have space for our units to walk through. Don't let them get stuck!
                if (reserve_map_[x][y])
                {
                    return false;
                }
            }
        }
    }

    return true;
}

sc2::Point2DI BuildingPlacer::GetBuildLocationNear(const sc2::Point2DI desired_loc, const sc2::UnitTypeID building_type, const int build_dist) const
{
    // get the precomputed vector of tile positions which are sorted closes to this location
    auto & closest_to_building = bot_.Info().Map().GetClosestTilesTo(desired_loc);

    // iterate through the list until we've found a suitable location
    for (auto & pos : closest_to_building)
    {
        if (CanBuildHereWithSpace(pos.x, pos.y, building_type, build_dist))
        {
            return pos;
        }
    }

    return sc2::Point2DI(0, 0);
}

bool BuildingPlacer::Buildable(const int x, const int y, const sc2::UnitTypeID type) const
{
    if (!bot_.Info().Map().IsOnMap(x, y) || !bot_.Info().Map().CanBuildTypeAtPosition(x, y, type))
        return false;
    return true;
}

void BuildingPlacer::DrawReservedTiles()
{
    if (!bot_.Config().DrawReservedBuildingTiles)
    {
        return;
    }

    const size_t rwidth = reserve_map_.size();
    const size_t rheight = reserve_map_[0].size();

    for (int x = 0; x < rwidth; ++x)
    {
        for (int y = 0; y < rheight; ++y)
        {
            if (reserve_map_[x][y] || IsInResourceBox(x, y))
            {
                bot_.DebugHelper().DrawSquareOnMap(sc2::Point2DI(x, y), sc2::Colors::Yellow);
            }
        }
    }
}

void BuildingPlacer::DrawBuildLocationCache()
{
    for (const auto & cache_data : build_location_cache_)
    {
        const auto & type = cache_data.first;
        const auto & loc = cache_data.second;
        bot_.DebugHelper().DrawBoxAroundUnit(type, loc, sc2::Colors::Yellow);
    }
}

sc2::Point2DI BuildingPlacer::GetRefineryPosition() const
{
    sc2::Point2DI closest_geyser(0, 0);
    double min_geyser_distance_from_home = std::numeric_limits<double>::max();

    for (auto & geyser : bot_.Observation()->GetUnits())
    {
        if (!Util::IsGeyser(geyser))
        {
            continue;
        }

        const sc2::Point2D geyser_pos(geyser->pos);

        // For each of our bases, see if we can build refineries there. 
        for (auto & base : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
        {
            if ( Util::IsTownHall(base) )
            {
                const double geyser_distance = Util::Dist(geyser->pos, base->pos);

                if (geyser_distance < min_geyser_distance_from_home)
                {
                    if (bot_.Info().Map().CanBuildTypeAtPosition(static_cast<int>(geyser_pos.x), static_cast<int>(geyser_pos.y), sc2::UNIT_TYPEID::TERRAN_REFINERY))
                    {
                        min_geyser_distance_from_home = geyser_distance;
                        closest_geyser = sc2::Point2DI(geyser->pos.x, geyser->pos.y);
                    }
                }
            }
        }
    }

    return closest_geyser;
}

