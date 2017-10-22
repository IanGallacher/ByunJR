#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitInfoManager.h"

class ByunJRBot;

class InformationManager
{
    ByunJRBot &              bot_;

    std::vector<sc2::Tag>    scout_units_;
    UnitInfoManager          unit_info_;

    bool                     initial_scout_set_;

    sc2::Race                player_race_[2];

    void SetScoutUnits(bool should_send_initial_scout);

public:
    InformationManager(ByunJRBot & bot);
    void OnStart();
    void OnUnitCreated(const sc2::Unit* unit);
    void OnUnitDestroyed(const sc2::Unit* unit);
    void OnFrame();

    sc2::Point2DI GetProxyLocation() const;
    UnitInfoManager & UnitInfo();

    sc2::Tag GetBuilder(Building& b, bool set_job_as_builder = true);
    void assignUnit(const sc2::Tag & unit, UnitMission job);
    void finishedWithUnit(const sc2::Tag& unit);

    const sc2::Race & GetPlayerRace(PlayerArrayIndex player) const;
    const sc2::Unit* GetClosestUnitOfType(const sc2::Unit* unit, const sc2::UnitTypeID) const;
    const sc2::Unit* GetClosestBase(const sc2::Unit* reference_unit) const;
    const ::UnitInfo* GetClosestUnitWithJob(const sc2::Point2D point, const UnitMission) const;
    sc2::Tag GetClosestUnitTagWithJob(const sc2::Point2D point, const UnitMission mission) const;
    sc2::Tag GetClosestUnitTagWithJob(const sc2::Point2D point, const std::vector<UnitMission> mission) const;

    void HandleUnitAssignments();
};
