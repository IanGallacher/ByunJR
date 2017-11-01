#pragma once
#include <sc2api/sc2_api.h>

class ByunJRBot;

class BuildingManager
{
    ByunJRBot &   bot_;

    std::vector<Building> buildings_;

    bool            IsBuildingPositionExplored(const Building & b) const;
    void            RemoveBuildings(const std::vector<Building> & to_remove);
    bool            IsValidBuildLocation(const int x, const int y, sc2::UnitTypeID type) const;

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

    bool                IsBeingBuilt(sc2::UnitTypeID type);

    std::vector<sc2::UnitTypeID> BuildingsQueued() const;
};
