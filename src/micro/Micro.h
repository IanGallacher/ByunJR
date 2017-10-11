#pragma once
#include <sc2api/sc2_api.h>

class ByunJRBot;

namespace Micro
{   
    void SmartAttackUnit(const sc2::Tag & attackerTag,   const sc2::Tag & targetTag, ByunJRBot & bot);
    void SmartAttackMove(const sc2::Tag & attackerTag,   const sc2::Point2D & targetPosition, ByunJRBot & bot);
    void SmartMove      (const sc2::Tag & unitTag,       const sc2::Point2D & targetPosition, ByunJRBot & bot);
    void SmartRightClick(const sc2::Tag & unitTag,       const sc2::Tag & targetTag, ByunJRBot & bot);
    void SmartRepair    (const sc2::Tag & unitTag,       const sc2::Tag & targetTag, ByunJRBot & bot);
    void SmartKiteTarget(const sc2::Tag & rangedUnitTag, const sc2::Tag & targetTag, ByunJRBot & bot);
    void SmartBuild     (const sc2::Tag & builderTag,    const sc2::UnitTypeID & buildingType, sc2::Point2D pos, ByunJRBot & bot);
    void SmartBuildTag  (const sc2::Tag & builderTag,    const sc2::UnitTypeID & buildingType, sc2::Tag targetTag, ByunJRBot & bot);
    void SmartTrain     (const sc2::Tag & builderTag,    const sc2::UnitTypeID & buildingType, ByunJRBot & bot);
};