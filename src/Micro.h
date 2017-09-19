#pragma once

#include "Common.h"
#include "sc2api/sc2_api.h"

class ByunJRBot;

namespace Micro
{   
    void SmartStop      (const UnitTag & attacker,  ByunJRBot & bot);
    void SmartAttackUnit(const UnitTag & attacker,  const UnitTag & target, ByunJRBot & bot);
    void SmartAttackMove(const UnitTag & attacker,  const sc2::Point2D & targetPosition, ByunJRBot & bot);
    void SmartMove      (const UnitTag & attacker,  const sc2::Point2D & targetPosition, ByunJRBot & bot);
    void SmartRightClick(const UnitTag & unit,      const UnitTag & target, ByunJRBot & bot);
    void SmartRepair    (const UnitTag & unit,      const UnitTag & target, ByunJRBot & bot);
    void SmartKiteTarget(const UnitTag & rangedUnit,const UnitTag & target, ByunJRBot & bot);
    void SmartBuild     (const UnitTag & builder,   const sc2::UnitTypeID & buildingType, sc2::Point2D pos, ByunJRBot & bot);
    void SmartBuildTag  (const UnitTag & builder,   const sc2::UnitTypeID & buildingType, UnitTag targetTag, ByunJRBot & bot);
    void SmartTrain     (const UnitTag & builder,   const sc2::UnitTypeID & buildingType, ByunJRBot & bot);
};