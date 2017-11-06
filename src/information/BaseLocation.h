#pragma once
#include <map>
#include <vector>
#include <sc2api/sc2_api.h>

#include "util/DistanceMap.h"

class ByunJRBot;

class BaseLocation
{
    ByunJRBot &                              bot_;
    DistanceMap                              distance_map_;

    sc2::Point2D                             depot_position_;
    sc2::Point2D                             center_of_resources_;
    std::vector<const sc2::Unit*>            geysers_;
    std::vector<const sc2::Unit*>            minerals_;

    std::vector<sc2::Point2D>                mineral_positions_;
    std::vector<sc2::Point2D>                geyser_positions_;

    std::map<sc2::Unit::Alliance, bool>         is_player_occupying_;
    std::map<sc2::Unit::Alliance, bool>         is_player_start_location_;
        
    int                                      baseID;
    float                                    left_;
    float                                    right_;
    float                                    top_;
    float                                    bottom_;
    bool                                     is_start_location_;
    
public:

    BaseLocation(ByunJRBot & bot, int baseID, const std::vector<const sc2::Unit*> & resources);
    
    int GetGroundDistance(const sc2::Point2D & pos) const;
    bool IsStartLocation() const;
    bool IsPlayerStartLocation() const;
    bool IsPotentialEnemyStartLocation() const;
    bool IsMineralOnly() const;
    bool ContainsPosition(const sc2::Point2D & pos) const;
    const sc2::Point2D & GetDepotPosition() const;
    const sc2::Point2D & GetPosition() const;
    const std::vector<const sc2::Unit*>& GetGeysers() const;
    const std::vector<const sc2::Unit*>& GetMinerals() const;
    bool IsOccupiedByPlayer(sc2::Unit::Alliance player) const;
    bool IsExplored() const;
    bool IsInResourceBox(int x, int y) const;

    void SetPlayerOccupying(sc2::Unit::Alliance player, bool occupying);

    const std::vector<sc2::Point2DI> & GetClosestTiles() const;

    void Draw();
};
