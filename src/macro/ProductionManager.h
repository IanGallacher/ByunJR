#pragma once
#include <sc2api/sc2_api.h>

#include "macro/BuildOrder.h"
#include "macro/BuildOrderQueue.h"
#include "macro/BuildingManager.h"

class ByunJRBot;

class ProductionManager
{
    ByunJRBot &      bot_;

    BuildingManager  building_manager_;
    BuildOrderQueue  queue_;
    int              planned_production_; 

    const sc2::Unit* GetClosestUnitToPosition(const std::vector<const sc2::Unit*> & units, sc2::Point2D closest_to) const;
    
    // If distance is not set to negative one, and is greater or equal to 0, 
    // we take into account the money we make while the worker travels to the build location.
    bool             MeetsReservedResources(sc2::UnitTypeID type, int distance = -1) const;
    bool             CanMakeNow(const sc2::Unit* producer, sc2::UnitTypeID t) const;
    void             Create(const sc2::Unit* producer, BuildOrderItem & item);
    void             ManageBuildOrderQueue();
    void             AddPrerequisitesToQueue(sc2::UnitTypeID unit_type);
    void             PreventSupplyBlock();
    void             MacroUp();
    int              ProductionCapacity() const;

public:

    ProductionManager(ByunJRBot & bot);

    void             OnStart();
    void             OnFrame();
    void             OnUnitDestroyed(const sc2::Unit* unit);

    size_t           NumberOfBuildingsQueued(sc2::UnitTypeID unit_type) const;
    size_t           BuildingsIncompleteCount(sc2::UnitTypeID unit_type) const;
    int              TrueUnitCount(sc2::UnitTypeID unit_type);

    const sc2::Unit* GetProducer(sc2::UnitTypeID t, sc2::Point2D closest_to = sc2::Point2D(0, 0)) const;

    std::string      ToString() const;
    std::string BuildingInfoString() const;
};
