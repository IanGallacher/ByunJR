#pragma once
#include <sc2api/sc2_api.h>



enum class UnitMission { Idle, Wait, Move, Minerals, Gas, Build, Attack, Defend, Harass, Repair, Scout, Proxy };

class UnitInfo
{
    // we need to store all of this data because if the unit is not visible, we
    // can't reference it from the unit pointer

public:
    int             tag;
    float           lastHealth;
    float           lastShields;
    int             player;
    sc2::Unit       unit;
    sc2::Point3D    lastPosition;
    sc2::UnitTypeID type;
    float           progress;
    UnitMission     mission;

    UnitInfo()
        : tag(0)
        , lastHealth(0)
        , player(-1)
        , lastPosition(sc2::Point3D(0, 0, 0))
        , type(0)
        , progress(1.0)
    {

    }

    const char getJobCode(const sc2::Tag & unit)
    {
        const UnitMission j = mission;

        if (j == UnitMission::Build)     return 'B';
        if (j == UnitMission::Attack)    return 'A';
        if (j == UnitMission::Attack)    return 'D';
        if (j == UnitMission::Wait)      return 'W';
        if (j == UnitMission::Gas)       return 'G';
        if (j == UnitMission::Idle)      return 'I';
        if (j == UnitMission::Minerals)  return 'M';
        if (j == UnitMission::Repair)    return 'R';
        if (j == UnitMission::Move)      return 'O';
        if (j == UnitMission::Scout)     return 'S';
        if (j == UnitMission::Proxy)     return 'P';
        return 'X';
    }

    const bool operator == (sc2::Unit & unit) const
    {
        return tag == unit.tag;
    }

    const bool operator == (const UnitInfo & rhs) const
    {
        return (tag == rhs.tag);
    }

    const bool operator < (const UnitInfo & rhs) const
    {
        return (tag < rhs.tag);
    }
};