#pragma once
#include <sc2api/sc2_api.h>

class ByunJRBot;

namespace Micro
{   
    void SmartAttackUnit(const sc2::Unit* attacker, const sc2::Unit* target, ByunJRBot& bot);
    void SmartAttackMove(const sc2::Unit* attacker, const sc2::Point2D& targetPosition, ByunJRBot& bot);
    void SmartMove      (const sc2::Unit* unit, const sc2::Point2D& targetPosition, ByunJRBot& bot);
    void SmartRightClick(const sc2::Unit* unit, const sc2::Unit* target, ByunJRBot& bot);
    void SmartRepair    (const sc2::Unit* unit, const sc2::Unit*& target, ByunJRBot& bot);
    void SmartKiteTarget(const sc2::Unit* rangedUnit, const sc2::Unit* target, ByunJRBot& bot);
    void SmartBuild     (const sc2::Unit* builder, const sc2::UnitTypeID& buildingType, const sc2::Point2D pos, ByunJRBot& bot);
    void SmartBuildTag  (const sc2::Unit* builder, const sc2::UnitTypeID& buildingType, const sc2::Unit* target, ByunJRBot& bot);
    void SmartTrain     (const sc2::Unit* builder, const sc2::UnitTypeID& buildingType, ByunJRBot& bot);
};