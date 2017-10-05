#pragma once
#include <sc2api/sc2_api.h>

class ByunJRBot;

class InformationManager
{
    ByunJRBot &              m_bot;

    std::vector<sc2::Tag>    m_validUnits;
    std::vector<sc2::Tag>    m_combatUnits;
    std::vector<sc2::Tag>    m_scoutUnits;

    bool                     m_initialScoutSet;

    void assignUnit(const sc2::Tag & unit, std::vector<sc2::Tag> & units);
    bool isAssigned(const sc2::Tag & unit) const;

public:
    InformationManager(ByunJRBot & bot);
    sc2::Point2D GetProxyLocation() const;


    void setValidUnits();
    void setScoutUnits(bool shouldSendInitialScout);
    void setCombatUnits();

    void handleUnitAssignments();
    std::vector<sc2::Tag> GetCombatUnits() const;
};
