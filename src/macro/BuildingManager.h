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

    void            StopConstructingDeadBuildings();          // STEP 1
    void            FindBuildingLocation();                   // STEP 2
    void            ConstructAssignedBuildings();             // STEP 3
    void            CheckForStartedConstruction();            // STEP 4
    void            AssignWorkersToUnassignedBuildings();     // STEP 5
    void            CheckForCompletedBuildings();             // STEP 6

public:

    BuildingManager(ByunJRBot & bot);

    void                OnStart();
    void                OnFrame();
    void                AddBuildingTask(const sc2::UnitTypeID & type);
    void                DrawBuildingInformation();

    bool                IsBeingBuilt(sc2::UnitTypeID type);
    size_t              NumberOfBuildingTypePlanned(sc2::UnitTypeID unit_type) const;
    size_t              NumberOfBuildingTypeInProduction(sc2::UnitTypeID unit_type) const;

    std::vector<sc2::UnitTypeID> BuildingsQueued() const;
};
