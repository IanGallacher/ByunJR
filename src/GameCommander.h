#pragma once

#include "Common.h"
#include "Timer.hpp"
#include "ProductionManager.h"
#include "ProxyManager.h"
#include "ScoutManager.h"
#include "CombatCommander.h"

class ByunJRBot;

class GameCommander
{
    ByunJRBot &                 m_bot;
    Timer                   m_timer;

    ProductionManager       m_productionManager;
    ScoutManager            m_scoutManager;
    ProxyManager            m_proxyManager;
    CombatCommander         m_combatCommander;

    std::vector<UnitTag>    m_validUnits;
    std::vector<UnitTag>    m_combatUnits;
    std::vector<UnitTag>    m_scoutUnits;

    bool                    m_initialScoutSet;

    void assignUnit(const UnitTag & unit, std::vector<UnitTag> & units);
    bool isAssigned(const UnitTag & unit) const;

public:

    GameCommander(ByunJRBot & bot);
    sc2::Point2D GetProxyLocation();

    void onStart();
    void onFrame();
    void OnUnitEnterVision(const sc2::Unit & unit);
    void onUnitCreated(const sc2::Unit & unit);
    void onUnitDestroy(const sc2::Unit & unit);
    void onBuildingConstructionComplete(const sc2::Unit& unit);

    void handleUnitAssignments();
    void setValidUnits();
    void setScoutUnits();
    void setCombatUnits();

    void drawDebugInterface();
    void drawGameInformation(int x, int y);

    bool shouldSendInitialScout();
};
