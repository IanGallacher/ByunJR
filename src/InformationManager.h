#pragma once
#include <sc2api/sc2_api.h>

#include "information/BaseLocationManager.h"
#include "information/MapTools.h"
#include "information/UnitInfoManager.h"

class ByunJRBot;

// InformationManager is not inside the information folder in order to remind people not to include individual things from the information folder.
// Only include InformationManager, and use it to get and set specific information about the game.
class InformationManager
{
    // map_ must be be before bases_ in order to satisfy dependancies.
    sc2::Agent &                    bot_;
    MapTools                        map_;
    BaseLocationManager             bases_;
    UnitInfoManager                 unit_info_;


    std::map<sc2::Unit::Alliance, sc2::Race>   player_race_;
    vvi                             dps_map_;

public:
    InformationManager(sc2::Agent & bot);
    void OnStart();
    void OnUnitCreated(const sc2::Unit* unit);
    void OnUnitDestroyed(const sc2::Unit* unit);
    void OnFrame();

    const BaseLocationManager & Bases() const;
    const MapTools & Map() const;
    UnitInfoManager & UnitInfo();

    const sc2::Race & GetPlayerRace(sc2::Unit::Alliance player) const;

    const ::UnitInfo* GetClosestUnitInfoWithJob(const sc2::Point2D point, const UnitMission) const;
    const sc2::Unit* GetClosestUnitWithJob(const sc2::Point2D reference_point, const UnitMission unit_mission) const;
    const ::UnitInfo* GetClosestUnitInfoWithJob(const sc2::Point2D point, const std::vector<UnitMission> mission) const;
    const sc2::Unit* GetClosestUnitWithJob(const sc2::Point2D point,
                                           const std::vector<UnitMission> mission_vector) const;
    const sc2::Unit* GetClosestUnitOfType(const sc2::Unit* unit, const sc2::UnitTypeID) const;
    vvi GetDPSMap() const;
};
