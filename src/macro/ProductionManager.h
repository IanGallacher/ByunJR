#pragma once
#include <sc2api/sc2_api.h>

#include "macro/BuildOrder.h"
#include "BuildOrderQueue.h"
#include "macro/BuildingManager.h"

class ByunJRBot;

class ProductionManager
{
    ByunJRBot &     bot_;

    BuildingManager buildingManager;
    BuildOrderQueue queue;
    int planned_production;
    int planned_supply_depots; // don't spend all our money on depots if capped. 


    sc2::Tag getClosestUnitToPosition(const std::vector<sc2::Tag> & units, sc2::Point2D closestTo) const;
    bool    meetsReservedResources(sc2::UnitTypeID type);
    bool    canMakeNow(sc2::Tag producer, sc2::UnitTypeID t);
    bool    detectBuildOrderDeadlock() const;
    void    setBuildOrder(const BuildOrder & buildOrder);
    void    create(sc2::Tag producer, BuildOrderItem & item);
    void    manageBuildOrderQueue();
    void    preventSupplyBlock();
    int     productionCapacity() const;
    int     getFreeMinerals();
    int     getFreeGas();

public:

    ProductionManager(ByunJRBot & bot);

    void    onStart();
    void    OnFrame();
    void    onBuildingConstructionComplete(const sc2::Unit* unit);
    void    onUnitDestroy(const sc2::Unit* unit);
    void    drawProductionInformation() const;

    sc2::Tag getProducer(sc2::UnitTypeID t, sc2::Point2D closestTo = sc2::Point2D(0, 0));
};
