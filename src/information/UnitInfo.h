#pragma once
#include <sc2api/sc2_api.h>

enum class UnitMission { Idle, Wait, Move, Minerals, Gas, Build, Attack, Defend, Harass, Repair, Scout, Proxy };

struct UnitInfo
{
    // we need to store all of this data because if the unit is not visible, we
    // can't reference it from the unit pointer

    float             lastHealth;
    float             lastShields;
    sc2::Unit::Alliance  player;
    const sc2::Unit*  unit;
    sc2::Point3D      lastPosition;
    sc2::UnitTypeID   type;
    float             progress;
    UnitMission       mission;
    // If the unit mission is:
    //   Minerals: missionTarget is the base the worker is currently asigned to mine from.
    //   Repair: missionTarget is the unit the worker is currently repairing.
    //   Gas: missionTarget is the refinery the currently mining from.
    const sc2::Unit*  missionTarget;

    UnitInfo()
        : lastHealth(0)
        , player(sc2::Unit::Alliance (0))
        , lastPosition(sc2::Point3D(0, 0, 0))
        , type(0)
        , mission(UnitMission::Idle)
        , progress(1.0)
    {

    }

    std::string GetJobCode() const
    {
        const UnitMission j = mission;

        if (j == UnitMission::Build)     return "B";
        if (j == UnitMission::Attack)    return "A";
        if (j == UnitMission::Defend)    return "D";
        if (j == UnitMission::Wait)      return "W";
        if (j == UnitMission::Gas)       return "G";
        if (j == UnitMission::Minerals)  return "M";
        if (j == UnitMission::Repair)    return "R";
        if (j == UnitMission::Move)      return "O";
        if (j == UnitMission::Scout)     return "S";
        if (j == UnitMission::Proxy)     return "P";
        if (j == UnitMission::Idle)      return "I";
        return "X";
    }

    bool operator == (sc2::Unit & unit) const
    {
        return this->unit->tag == unit.tag;
    }

    bool operator == (const UnitInfo & rhs) const
    {
        return (this->unit->tag == rhs.unit->tag);
    }

    bool operator < (const UnitInfo & rhs) const
    {
        return (this->unit->tag < rhs.unit->tag);
    }
};