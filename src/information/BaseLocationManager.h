#pragma once
#include <sc2api/sc2_api.h>

#include "information/BaseLocation.h"

class ByunJRBot;

class BaseLocationManager
{
    ByunJRBot & bot_;

    std::vector<BaseLocation>                                   base_location_data_;
    std::vector<const BaseLocation*>                            base_location_ptrs_;
    std::vector<const BaseLocation*>                            starting_base_locations_;
    std::map<PlayerArrayIndex, const BaseLocation*>             player_starting_base_locations_;
    std::map<PlayerArrayIndex, std::set<const BaseLocation*>>   occupied_base_locations_;
    std::vector<std::vector<BaseLocation*>>                     tile_base_locations_;

    // If the enemy base is not yet scouted, the enemy base location will be set to the next unexplored enemy spawn location.
    bool                                                        enemy_base_scouted_;

    BaseLocation* GetBaseLocation(const sc2::Point2D & pos) const;

public:

    BaseLocationManager(ByunJRBot & bot);
    
    void OnStart();
    void OnFrame();
    void DrawBaseLocations();

    const std::vector<const BaseLocation*> & GetBaseLocations() const;
    const std::vector<const BaseLocation*> & GetStartingBaseLocations() const;
    const std::set<const BaseLocation*> & GetOccupiedBaseLocations(PlayerArrayIndex player) const;
    const BaseLocation* GetPlayerStartingBaseLocation(PlayerArrayIndex player) const;
    
    sc2::Point2D GetNextExpansion(PlayerArrayIndex player) const;
};
