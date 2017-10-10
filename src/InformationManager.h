#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitInfoManager.h"

class ByunJRBot;

class InformationManager
{
    ByunJRBot &              m_bot;

    std::vector<sc2::Tag>    m_validUnits;
    std::vector<sc2::Tag>    m_combatUnits;
    std::vector<sc2::Tag>    m_scoutUnits;
    UnitInfoManager          m_unitInfo;

    bool                     m_initialScoutSet;

    void assignUnit(const sc2::Tag & unit, UnitMission job);
    bool isAssigned(const sc2::Tag & unit) const;

public:
    InformationManager(ByunJRBot & bot);
    void onStart();
    void onUnitCreated(const sc2::Unit& unit);
    void onUnitDestroyed(const sc2::Unit& unit);
    void onFrame();

    sc2::Point2D GetProxyLocation() const;
    UnitInfoManager & UnitInfo();


    void setValidUnits();
    void setScoutUnits(bool shouldSendInitialScout);
    void setCombatUnits();

    void handleUnitAssignments();
    std::vector<sc2::Tag> GetCombatUnits() const;
};
