#include "information/UnitData.h"
#include "util/Util.h"

UnitData::UnitData()
    : m_mineralsLost(0)
    , m_gasLost(0)
{
    const int maxTypeID = 1024;
    m_numDeadUnits	    = std::vector<int>(maxTypeID + 1, 0);
    m_numUnits		    = std::vector<int>(maxTypeID + 1, 0);
}

void UnitData::updateUnit(const sc2::Unit* unit)
{
    bool firstSeen = false;
    const auto & it = m_unitInfoMap.find((int)unit->tag);
    if (it == m_unitInfoMap.end())
    {
        firstSeen = true;
        m_unitInfoMap[(int)unit->tag] = UnitInfo();
    }

    UnitInfo & ui   = m_unitInfoMap[(int)unit->tag];
    ui.unit         = unit;
    ui.player       = Util::GetPlayer(unit);
    ui.lastPosition = unit->pos;
    ui.lastHealth   = unit->health;
    ui.lastShields  = unit->shield;
    ui.tag          = (int)unit->tag;
    ui.type         = unit->unit_type;
    ui.progress     = unit->build_progress;

    if (firstSeen)
    {
        m_numUnits[ui.type]++;
    }
}

void UnitData::killUnit(const sc2::Unit* unit)
{
    //_mineralsLost += unit->getType().mineralPrice();
    //_gasLost += unit->getType().gasPrice();
    m_numUnits[unit->unit_type]--;
    m_numDeadUnits[unit->unit_type]++;

    m_unitInfoMap.erase((int)unit->tag);
}

void UnitData::removeBadUnits()
{
    for (auto iter = m_unitInfoMap.begin(); iter != m_unitInfoMap.end();)
    {
        if (badUnitInfo(iter->second))
        {
            m_numUnits[iter->second.type]--;
            iter = m_unitInfoMap.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

bool UnitData::badUnitInfo(const UnitInfo & ui) const
{
    return false;
}

int UnitData::getGasLost() const
{
    return m_gasLost;
}

int UnitData::getMineralsLost() const
{
    return m_mineralsLost;
}

int UnitData::getNumUnits(sc2::UnitTypeID t) const
{
    return m_numUnits[t];
}

int UnitData::getNumDeadUnits(sc2::UnitTypeID t) const
{
    return m_numDeadUnits[t];
}

const std::map<int, UnitInfo> & UnitData::getUnitInfoMap() const
{
    return m_unitInfoMap;
}

void UnitData::setJob(const sc2::Unit* unit, UnitMission job)
{
    UnitInfo & ui = m_unitInfoMap[(int)unit->tag];
    ui.mission = job;
}
