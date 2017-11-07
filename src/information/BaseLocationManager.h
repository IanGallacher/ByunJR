#pragma once
#include <sc2api/sc2_api.h>

#include "information/BaseLocation.h"
#include "information/MapTools.h"

class InformationManager;

class BaseLocationManager
{
    sc2::Agent & bot_;
    const MapTools & map_;

    std::vector<BaseLocation>                                    base_location_data_;
    std::vector<const BaseLocation*>                             base_location_ptrs_;
    std::vector<const BaseLocation*>                             starting_base_locations_;
    std::map<sc2::Unit::Alliance, const BaseLocation*>           player_starting_base_locations_;
    std::map<sc2::Unit::Alliance, std::set<const BaseLocation*>> occupied_base_locations_;
    std::vector<std::vector<BaseLocation*>>                      tile_base_locations_;

    // If the enemy base is not yet scouted, the enemy base location will be set to the next unexplored enemy spawn location.
    bool                                                         enemy_base_scouted_;

    BaseLocation* GetBaseLocation(const sc2::Point2D & pos) const;

public:

    BaseLocationManager(sc2::Agent & bot, const MapTools & map);
    
    void OnStart();
    void OnFrame(InformationManager & info);

    const std::vector<const BaseLocation*> & GetBaseLocations() const;
    const std::vector<const BaseLocation*> & GetStartingBaseLocations() const;
    const std::set<const BaseLocation*> & GetOccupiedBaseLocations(sc2::Unit::Alliance player) const;
    const BaseLocation* GetPlayerStartingBaseLocation(sc2::Unit::Alliance player) const;
    
    sc2::Point2D GetNextExpansion(sc2::Unit::Alliance player) const;
};
