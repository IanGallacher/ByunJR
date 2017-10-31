#include "ByunJRBot.h"
#include "common/Common.h"
#include "macro/BuildingPlacer.h"
#include "util/Util.h"
#include "util/Timer.hpp"

BuildingPlacer::BuildingPlacer(ByunJRBot & bot)
    : bot_(bot)
{

}

void BuildingPlacer::OnStart()
{
    reserve_map_ = std::vector< std::vector<bool> >(bot_.Map().TrueMapWidth(), std::vector<bool>(bot_.Map().TrueMapHeight(), false));
}

bool BuildingPlacer::IsInResourceBox(const int x, const int y) const
{
    return bot_.Bases().GetPlayerStartingBaseLocation(PlayerArrayIndex::Self)->IsInResourceBox(x, y);
}

// makes final checks to see if a building can be built at a certain location
bool BuildingPlacer::CanBuildHere(const int bx, const int by, const sc2::UnitTypeID type) const
{
    if (IsInResourceBox(by, by))
    {
        return false;
    }

    // check the reserve map
    for (int x = bx; x < bx + Util::GetUnitTypeWidth(type, bot_); x++)
    {
        for (int y = by; y < by + Util::GetUnitTypeHeight(type, bot_); y++)
        {
            if (!bot_.Map().IsOnMap(x, y) || reserve_map_[x][y])
            {
                return false;
            }
        }
    }

    // if it overlaps a base location return false
    if (TileOverlapsBaseLocation(bx, by, type))
    {
        return false;
    }

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

    // TODO: make sure we leave space for add-ons. These types of units can have addons:

    // define the rectangle of the building spot
    const int startx = bx - (width / 2) - build_dist;
    const int starty = by - (height / 2) - build_dist;
    // endx is not const to account for add-ons. 
    int endx   = bx + (width/2) + build_dist;
    const int endy   = by + (height/2) + build_dist;

    // if this rectangle doesn't fit on the map we can't build here
    if (startx < 0 || starty < 0 || endx > bot_.Map().TrueMapWidth() || endy > bot_.Map().TrueMapHeight() || build_dist < 0)
    {
        return false;
    }

    // The starcraft 2 api will check every square on the building for us. No need to include it in the following loop. 
    if(!Buildable(bx, by, type))
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

sc2::Point2DI BuildingPlacer::GetBuildLocationNear(const Building & b, const int build_dist) const
{
    Timer t;
    t.Start();

    // get the precomputed vector of tile positions which are sorted closes to this location
    auto & closest_to_building = bot_.Map().GetClosestTilesTo(b.desiredPosition);

    double ms1 = t.GetElapsedTimeInMilliSec();

    // iterate through the list until we've found a suitable location
    for (size_t i(0); i < closest_to_building.size(); ++i)
    {
        auto & pos = closest_to_building[i];

        if (CanBuildHereWithSpace(pos.x, pos.y, b.type, build_dist))
        {
            double ms = t.GetElapsedTimeInMilliSec();
            //printf("Building Placer Took %d iterations, lasting %lf ms @ %lf iterations/ms, %lf setup ms\n", (int)i, ms, (i / ms), ms1);

            return pos;
        }
    }

    double ms = t.GetElapsedTimeInMilliSec();
    //printf("Building Placer Took %lf ms\n", ms);

    return sc2::Point2DI(0, 0);
}

bool BuildingPlacer::TileOverlapsBaseLocation(const int x, const int y, const sc2::UnitTypeID type) const
{
    // if it's a resource depot we don't care if it overlaps
    if (Util::IsTownHallType(type))
    {
        return false;
    }

    // dimensions of the proposed location
    const int tx1 = x;
    const int ty1 = y;
    const int tx2 = tx1 + Util::GetUnitTypeWidth(type, bot_);
    const int ty2 = ty1 + Util::GetUnitTypeHeight(type, bot_);

    // for each base location
    for (const BaseLocation* base : bot_.Bases().GetBaseLocations())
    {
        // dimensions of the base location
        const int bx1 = static_cast<int>(base->GetDepotPosition().x);
        const int by1 = static_cast<int>(base->GetDepotPosition().y);
        const int bx2 = bx1 + Util::GetUnitTypeWidth(Util::GetTownHall(bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self)), bot_);
        const int by2 = by1 + Util::GetUnitTypeHeight(Util::GetTownHall(bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self)), bot_);

        // conditions for non-overlap are easy
        const bool no_overlap = (tx2 < bx1) || (tx1 > bx2) || (ty2 < by1) || (ty1 > by2);

        // if the reverse is true, return true
        if (!no_overlap)
        {
            return true;
        }
    }

    // otherwise there is no overlap
    return false;
}

bool BuildingPlacer::Buildable(const int x, const int y, const sc2::UnitTypeID type) const
{
    // TODO: does this take units on the map into account?
    if (!bot_.Map().IsOnMap(x, y) || !bot_.Map().CanBuildTypeAtPosition(x, y, type))
    {
        return false;
    }

    // todo: check that it won't block an addon

    return true;
}

void BuildingPlacer::ReserveTiles(const int bx, const int by, const int width, const int height)
{
    const size_t rwidth = reserve_map_.size();
    const size_t rheight = reserve_map_[0].size();
    for (size_t x = bx; x < bx + width && x < rwidth; x++)
    {
        for (size_t y = by; y < by + height && y < rheight; y++)
        {
            reserve_map_[x][y] = true;
        }
    }
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
                const float x1 = x * 32 + 8;
                const float y1 = y*32 + 8;
                const float x2 = (x+1)*32 - 8;
                const float y2 = (y+1)*32 - 8;

                bot_.DebugHelper().DrawBox(x1, y1, x2, y2, sc2::Colors::Yellow);
            }
        }
    }
}

void BuildingPlacer::FreeTiles(const int bx, const int by, const int width, const int height)
{
    const int rwidth = static_cast<int>(reserve_map_.size());
    const int rheight = static_cast<int>(reserve_map_[0].size());

    for (int x = bx; x < bx + width && x < rwidth; x++)
    {
        for (int y = by; y < by + height && y < rheight; y++)
        {
            reserve_map_[x][y] = false;
        }
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
        for (auto & base : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
        {
            if ( Util::IsTownHall(base) )
            {
                const double geyser_distance = Util::Dist(geyser->pos, base->pos);

                if (geyser_distance < min_geyser_distance_from_home)
                {
                    if (bot_.Map().CanBuildTypeAtPosition(static_cast<int>(geyser_pos.x), static_cast<int>(geyser_pos.y), sc2::UNIT_TYPEID::TERRAN_REFINERY))
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

