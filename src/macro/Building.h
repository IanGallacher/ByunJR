#pragma once
#include <sc2api/sc2_api.h>

namespace BuildingStatus
{
    enum { Unassigned = 0, Assigned = 1, UnderConstruction = 2, Size = 3 };
}

class Building
{
public:

    sc2::Point2D    desiredPosition;
    sc2::Point2D    finalPosition;
    sc2::Point2D    position;
    sc2::UnitTypeID type;
    sc2::Tag        buildingUnitTag;
    sc2::Tag        builderUnitTag;
    size_t          status;
    int             lastOrderFrame;
    bool            buildCommandGiven;
    bool            underConstruction;

    Building();

    // constructor we use most often
    Building(sc2::UnitTypeID t, sc2::Point2D desired);

    // equals operator
    bool operator == (const Building & b);
};
