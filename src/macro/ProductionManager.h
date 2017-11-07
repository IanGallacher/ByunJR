#pragma once
#include <sc2api/sc2_api.h>

#include "macro/BuildOrder.h"
#include "BuildOrderQueue.h"
#include "macro/BuildingManager.h"

class ByunJRBot;

class ProductionManager
{
    ByunJRBot &      bot_;

    BuildingManager  building_manager_;
    BuildOrderQueue  queue_;
    int              planned_production_;
    int              planned_supply_depots_; // don't spend all our money on depots if capped. 

    const sc2::Unit* GetClosestUnitToPosition(const std::vector<const sc2::Unit*> & units, sc2::Point2D closest_to) const;
    bool             MeetsReservedResources(sc2::UnitTypeID type) const;
    bool             CanMakeNow(const sc2::Unit* producer, sc2::UnitTypeID t) const;
    bool             DetectBuildOrderDeadlock() const;
    void             SetBuildOrder(const BuildOrder & build_order);
    void             Create(const sc2::Unit* producer, BuildOrderItem & item);
    void             ManageBuildOrderQueue();
    void             PreventSupplyBlock();
    int              TrueUnitCount(sc2::UnitTypeID unit_type);
    void             MacroUp();
    int              ProductionCapacity() const;
    int              GetFreeMinerals() const;
    int              GetFreeGas() const;

public:

    ProductionManager(ByunJRBot & bot);

    void             OnStart();
    void             OnFrame();
    void             OnBuildingConstructionComplete(const sc2::Unit* unit);
    void             OnUnitDestroyed(const sc2::Unit* unit);
    void             DrawProductionInformation() const;

    size_t           NumberOfUnitsInProductionOfType(sc2::UnitTypeID unit_type) const;

    const sc2::Unit* GetProducer(sc2::UnitTypeID t, sc2::Point2D closest_to = sc2::Point2D(0, 0)) const;

};
