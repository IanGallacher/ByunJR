#pragma once
#include <sc2api/sc2_api.h>

class sc2::Agent;

namespace Micro
{   
    void SmartAttackUnit(const sc2::Unit* attacker, const sc2::Unit* target, sc2::Agent& bot);
    void SmartAttackMove(const sc2::Unit* attacker, const sc2::Point2D& target_position, sc2::Agent& bot);
    void SmartMove      (const sc2::Unit* unit, const sc2::Point2D& target_position, sc2::Agent& bot, bool queued_command = false);
    void SmartRightClick(const sc2::Unit* unit, const sc2::Unit* target, sc2::Agent& bot);
    void SmartRepair    (const sc2::Unit* scv, const sc2::Unit* target, sc2::Agent& bot);
    void SmartRepairWithSCVCount(const sc2::Unit* unit_to_repair, const int num_repair_workers, InformationManager & info);
    void SmartBuild     (const sc2::Unit* builder, const sc2::UnitTypeID& building_type, const sc2::Point2D pos, sc2::Agent& bot);
    void SmartBuildGeyser  (const sc2::Unit* builder, const sc2::UnitTypeID& building_type, const sc2::Unit* target, sc2::Agent& bot);
    void SmartTrain     (const sc2::Unit* production_building, const sc2::UnitTypeID& type_to_train, sc2::Agent& bot);
};