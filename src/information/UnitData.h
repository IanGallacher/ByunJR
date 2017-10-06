#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitInfo.h"

typedef std::vector<UnitInfo> UnitInfoVector;

class UnitData
{
    std::map<int, UnitInfo> m_unitInfoMap;
    std::vector<int>        m_numDeadUnits;
    std::vector<int>        m_numUnits;
    int                     m_mineralsLost;
    int                     m_gasLost;

    bool badUnitInfo(const UnitInfo & ui) const;

public:

    UnitData();

    void updateUnit(const sc2::Unit & unit);
    void killUnit(const sc2::Unit & unit);
    void removeBadUnits();

    int getGasLost()                                const;
    int getMineralsLost()                           const;
    int getNumUnits(sc2::UnitTypeID t)              const;
    int getNumDeadUnits(sc2::UnitTypeID t)          const;
    const std::map<int, UnitInfo> & getUnitInfoMap()  const;
    void setJob(const sc2::Unit& unit, UnitMission job);
};
