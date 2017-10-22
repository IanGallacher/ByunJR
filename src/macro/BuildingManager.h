#pragma once
#include <sc2api/sc2_api.h>

#include "macro/BuildingPlacer.h"

class ByunJRBot;

class BuildingManager
{
    ByunJRBot &   bot_;

    BuildingPlacer  building_placer_;
    std::vector<Building> buildings_;

    bool            debug_mode_;
    int             reserved_minerals_;                // minerals reserved for planned buildings
    int             reserved_gas_;                     // gas reserved for planned buildings

    bool            IsBuildingPositionExplored(const Building & b) const;
    void            RemoveBuildings(const std::vector<Building> & to_remove);

    void            ValidateWorkersAndBuildings();            // STEP 1
    void            AssignWorkersToUnassignedBuildings();     // STEP 2
    void            ConstructAssignedBuildings();             // STEP 3
    void            CheckForStartedConstruction();            // STEP 4
    void            CheckForDeadTerranBuilders();             // STEP 5
    void            CheckForCompletedBuildings();             // STEP 6

public:

    BuildingManager(ByunJRBot & bot);

    void                OnStart();
    void                OnFrame();
    void                AddBuildingTask(const sc2::UnitTypeID & type, const sc2::Point2DI& desired_position);
    void                DrawBuildingInformation();
    sc2::Point2DI       GetBuildingLocation(const Building & b) const;

    int                 GetReservedMinerals() const;
    int                 GetReservedGas() const;

    bool                IsBeingBuilt(sc2::UnitTypeID type);

    std::vector<sc2::UnitTypeID> BuildingsQueued() const;
};
