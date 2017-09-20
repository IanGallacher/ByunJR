#pragma once
#include "sc2api/sc2_api.h"
#include "Common.h"
#include "BuildOrder.h"
#include "BuildingManager.h"
#include "BuildOrderQueue.h"

class ByunJRBot;

class ProductionManager
{
    ByunJRBot &       m_bot;

    BuildingManager m_buildingManager;
    BuildOrderQueue m_queue;
    int m_planned_production;
    int m_planned_supply_depots; // don't spend all our money on depots if capped. 


    UnitTag getClosestUnitToPosition(const std::vector<UnitTag> & units, sc2::Point2D closestTo);
    bool    meetsReservedResources(sc2::UnitTypeID type);
    bool    canMakeNow(UnitTag producer, sc2::UnitTypeID t);
    bool    detectBuildOrderDeadlock();
    void    setBuildOrder(const BuildOrder & buildOrder);
    void    create(UnitTag producer, BuildOrderItem & item);
    void    manageBuildOrderQueue();
    void    preventSupplyBlock();
    int     productionCapacity();
    int     getFreeMinerals();
    int     getFreeGas();

public:

    ProductionManager(ByunJRBot & bot);

    void    onStart();
    void    onFrame();
    void    onBuildingConstructionComplete(const sc2::Unit unit);
    void    onUnitDestroy(const sc2::Unit & unit);
    void    drawProductionInformation();

    UnitTag getProducer(sc2::UnitTypeID t, sc2::Point2D closestTo = sc2::Point2D(0, 0));
};
