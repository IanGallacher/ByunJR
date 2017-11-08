#include "ByunJRBot.h"
#include "common/Common.h"
#include "information/BaseLocationManager.h"
#include "util/Util.h"
 
BaseLocationManager::BaseLocationManager(sc2::Agent & bot, const MapTools & map)
    : bot_(bot)
    , map_(map)
{
    
}

void BaseLocationManager::OnStart()
{
    // Make a vector of vectors to represent the map.
    tile_base_locations_ = std::vector<std::vector<BaseLocation*>>(map_.TrueMapWidth(), std::vector<BaseLocation*>(map_.TrueMapHeight(), nullptr));
    player_starting_base_locations_[sc2::Unit::Alliance::Self]  = nullptr;
    player_starting_base_locations_[sc2::Unit::Alliance::Enemy] = nullptr; 

    // construct the sets of occupied base locations
    occupied_base_locations_[sc2::Unit::Alliance::Self] = std::set<const BaseLocation*>();
    occupied_base_locations_[sc2::Unit::Alliance::Enemy] = std::set<const BaseLocation*>();
    enemy_base_scouted_ = false;
    
    // a BaseLocation will be anything where there are minerals to mine
    // so we will first look over all minerals and cluster them based on some distance
    const int cluster_distance = 15;
    
    // Stores each cluster of resources based on some ground distance.
    // These will be used to identify base locations.
    std::vector<std::vector<const sc2::Unit*>> resource_clusters;

    // For every mineral field and gas geyser out there, add it to a resource cluster.
    for (auto resource : bot_.Observation()->GetUnits(sc2::Unit::Alliance::Neutral))
    {
        // skip minerals that don't have more than 100 starting minerals
        // these are probably stupid map-blocking minerals to confuse us.

        // Skip any unit that is not a gas geyser or mineral field.
        if (!Util::IsMineral(resource) && !Util::IsGeyser(resource)) continue;

        bool found_cluster = false;
        for (auto & cluster : resource_clusters)
        {
            const float dist = Util::Dist(resource->pos, Util::CalcCenterOfUnitGroup(cluster));
            
            // quick initial air distance check to eliminate most resources
            if (dist < cluster_distance)
            {
                // now do a more expensive ground distance check
                const float ground_dist = dist; //bot_.InformationManager().Map().getGroundDistance(mineral.pos, Util::CalcCenterOfUnitGroup(cluster));
                if (ground_dist >= 0 && ground_dist < cluster_distance)
                {
                    cluster.push_back(resource);
                    found_cluster = true;
                    break;
                }
            }
        }

        if (!found_cluster)
        {
            resource_clusters.push_back(std::vector<const sc2::Unit*>());
            resource_clusters.back().push_back(resource);
        }
    }

    // add the base locations if there are more than 4 resouces in the cluster
    for (auto & cluster : resource_clusters)
    {
        if (cluster.size() > 4)
        {
            base_location_data_.push_back(BaseLocation(bot_, cluster));
        }
    }

    // construct the vectors of base location pointers, this is safe since they will never change
    for (auto & base_location : base_location_data_)
    {
        base_location_ptrs_.push_back(&base_location);
        // if it's a start location, add it to the start locations
        if (base_location.IsStartLocation())
        {
            starting_base_locations_.push_back(&base_location);
        }

        // if it's our starting location, set the pointer
        if (base_location.IsPlayerStartLocation())
        {
            player_starting_base_locations_[sc2::Unit::Alliance::Self] = &base_location;
        }

        // If there is only one enemy spawn location, we know where the enemy is. 
        if (bot_.Observation()->GetGameInfo().enemy_start_locations.size() == 1 
         && base_location.IsPotentialEnemyStartLocation())
        {
            // Make sure that there really only is one enemy base. 
            assert(enemy_base_scouted_ == false);
            player_starting_base_locations_[sc2::Unit::Alliance::Enemy] = &base_location;
            enemy_base_scouted_ = true;
        }
    }

    // construct the map of tile positions to base locations
    for (float x=0; x < map_.TrueMapWidth(); ++x)
    {
        for (int y=0; y < map_.TrueMapHeight(); ++y)
        {
            for (auto & base_location : base_location_data_)
            {
                const sc2::Point2D pos(x + 0.5f, y + 0.5f);

                if (base_location.ContainsPosition(pos))
                {
                    tile_base_locations_[static_cast<int>(x)][static_cast<int>(y)] = &base_location;
                    
                    break;
                }
            }
        }
    }
}

void BaseLocationManager::OnFrame(InformationManager & info)
{   
    // reset the player occupation information for each location
    for (auto & base_location : base_location_data_)
    {
        base_location.SetPlayerOccupying(sc2::Unit::Alliance::Self, false);
        base_location.SetPlayerOccupying(sc2::Unit::Alliance::Enemy, false);
    }

    // for each unit on the map, update which base location it may be occupying
    for (auto & unit : bot_.Observation()->GetUnits())
    {
        // we only care about buildings on the ground
        if (!Util::IsBuilding(unit->unit_type) || unit->is_flying)
        {
            continue;
        }

        BaseLocation* base_location = GetBaseLocation(unit->pos);

        if (base_location != nullptr)
        {
            base_location->SetPlayerOccupying(Util::GetPlayer(unit), true);
        }
    }

    // update enemy base occupations
    for (const auto & kv : info.UnitInfo().GetUnitInfoMap(sc2::Unit::Alliance::Enemy))
    {
        const UnitInfo & ui = kv.second;

        if (!Util::IsBuilding(ui.type))
        {
            continue;
        }

        BaseLocation* base_location = GetBaseLocation(ui.lastPosition);

        if (base_location != nullptr)
        {
            base_location->SetPlayerOccupying(sc2::Unit::Alliance::Enemy, true);
        }
    }

    // If we have not yet scouted the enemy base, try to figure out where they started. 
    // This can happen one of two ways. 
    if (!enemy_base_scouted_)
    {
        // 1. we've seen the enemy base directly, so the baselocation will know the enemy location.
        if (player_starting_base_locations_[sc2::Unit::Alliance::Enemy] == nullptr)
        {
            for (auto & base_location : base_location_data_)
            {
                if (base_location.IsPlayerStartLocation())
                {
                     player_starting_base_locations_[sc2::Unit::Alliance::Enemy] = &base_location;
                     enemy_base_scouted_ = true;
                }
            }
        }
    
        // 2. we've explored every other start location and haven't seen the enemy yet
        if (player_starting_base_locations_[sc2::Unit::Alliance::Enemy] == nullptr)
        {
            const int num_start_locations = static_cast<int>(GetStartingBaseLocations().size());
            int num_explored_locations = 0;
            BaseLocation* unexplored = nullptr;

            for (auto & base_location : base_location_data_)
            {
                if (!base_location.IsStartLocation())
                {
                    continue;
                }

                if (base_location.IsExplored())
                {
                    num_explored_locations++;
                }
                else
                {
                    unexplored = &base_location;
                }
            }

            // if we have explored all but one location, then the unexplored one is the enemy start location
            if (num_explored_locations == num_start_locations - 1 && unexplored != nullptr)
            {
                player_starting_base_locations_[sc2::Unit::Alliance::Enemy] = unexplored;
                unexplored->SetPlayerOccupying(sc2::Unit::Alliance::Enemy, true);
                enemy_base_scouted_ = true;
            }
        }
    }

    // update the occupied base locations for each player
    occupied_base_locations_[sc2::Unit::Alliance::Self] = std::set<const BaseLocation*>();
    occupied_base_locations_[sc2::Unit::Alliance::Enemy] = std::set<const BaseLocation*>();
    for (auto & base_location : base_location_data_)
    {
        if (base_location.IsOccupiedByPlayer(sc2::Unit::Alliance::Self))
        {
            occupied_base_locations_[sc2::Unit::Alliance::Self].insert(&base_location);
        }

        if (base_location.IsOccupiedByPlayer(sc2::Unit::Alliance::Enemy))
        {
            occupied_base_locations_[sc2::Unit::Alliance::Enemy].insert(&base_location);
        }
    }

    // draw the debug information for each base location
}

BaseLocation* BaseLocationManager::GetBaseLocation(const sc2::Point2D & pos) const
{
    if (!map_.IsOnMap(pos)) { std::cout << "Warning: requested base location not on map" << std::endl; return nullptr; }

    return tile_base_locations_[static_cast<int>(pos.x)][static_cast<int>(pos.y)];
}

const std::vector<const BaseLocation*> & BaseLocationManager::GetBaseLocations() const
{
    return base_location_ptrs_;
}

const std::vector<const BaseLocation*> & BaseLocationManager::GetStartingBaseLocations() const
{
    return starting_base_locations_;
}

const BaseLocation* BaseLocationManager::GetPlayerStartingBaseLocation(const sc2::Unit::Alliance player) const
{
    return player_starting_base_locations_.at(player);
}

const std::set<const BaseLocation*> & BaseLocationManager::GetOccupiedBaseLocations(const sc2::Unit::Alliance player) const
{
    return occupied_base_locations_.at(player);
}

sc2::Point2D BaseLocationManager::GetNextExpansion(const sc2::Unit::Alliance player) const
{
    const BaseLocation* home_base = GetPlayerStartingBaseLocation(player);
    const BaseLocation* closest_base = nullptr;
    int min_distance = std::numeric_limits<int>::max();

    sc2::Point2D home_tile = home_base->GetPosition();
    
    for (auto & base : GetBaseLocations())
    {
        // skip mineral only and starting locations (TODO: fix this)
        if (base->IsMineralOnly() || base->IsStartLocation())
        {
            continue;
        }

        if (base->IsOccupiedByPlayer(sc2::Unit::Alliance::Self)
         || base->IsOccupiedByPlayer(sc2::Unit::Alliance::Enemy))
        {
            continue;
        }

        // get the tile position of the base
        const auto tile = base->GetTownHallPosition();

        // the base's distance from our main nexus
        const int distance_from_home = bot_.Query()->PathingDistance(tile, home_base->GetPosition());

        // if it is not connected, continue
        if (distance_from_home < 0)
        {
            continue;
        }

        if (!closest_base || distance_from_home < min_distance)
        {
            closest_base = base;
            min_distance = distance_from_home;
        }
    }

    return closest_base ? closest_base->GetTownHallPosition() : sc2::Point2D(0.0f, 0.0f);
}
