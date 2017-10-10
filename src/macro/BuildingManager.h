#pragma once
#include <sc2api/sc2_api.h>

#include "macro/BuildingPlacer.h"

class ByunJRBot;

class BuildingManager
{
    ByunJRBot &   m_bot;

    BuildingPlacer  m_buildingPlacer;
    std::vector<Building> m_buildings;

    bool            m_debugMode;
    int             m_reservedMinerals;				// minerals reserved for planned buildings
    int             m_reservedGas;					// gas reserved for planned buildings

    bool            isBuildingPositionExplored(const Building & b) const;
    void            removeBuildings(const std::vector<Building> & toRemove);

    void            validateWorkersAndBuildings();		    // STEP 1
    void            assignWorkersToUnassignedBuildings();	// STEP 2
    void            constructAssignedBuildings();			// STEP 3
    void            checkForStartedConstruction();			// STEP 4
    void            checkForDeadTerranBuilders();			// STEP 5
    void            checkForCompletedBuildings();			// STEP 6

    char            getBuildingWorkerCode(const Building & b) const;

public:

    BuildingManager(ByunJRBot & bot);

    void                onStart();
    void                onFrame();
    void                addBuildingTask(const sc2::UnitTypeID & type, const sc2::Point2D & desiredPosition);
    void                drawBuildingInformation();
    sc2::Point2D        getBuildingLocation(const Building & b) const;

    int                 getReservedMinerals() const;
    int                 getReservedGas() const;

    bool                isBeingBuilt(sc2::UnitTypeID type);

    std::vector<sc2::UnitTypeID> buildingsQueued() const;
};
