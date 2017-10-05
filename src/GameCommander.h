#pragma once
#include "macro/ProductionManager.h"
#include "micro/ProxyManager.h"
#include "micro/ScoutManager.h"
#include "micro/CombatCommander.h"
#include "util/Timer.hpp"

class ByunJRBot;

class GameCommander
{
    ByunJRBot &              m_bot;
    Timer                    m_timer;

    std::vector<sc2::Tag>    m_validUnits;
    std::vector<sc2::Tag>    m_combatUnits;
    std::vector<sc2::Tag>    m_scoutUnits;

    bool                     m_initialScoutSet;

    void assignUnit(const sc2::Tag & unit, std::vector<sc2::Tag> & units);
    bool isAssigned(const sc2::Tag & unit) const;

public:

    GameCommander(ByunJRBot & bot);
    sc2::Point2D GetProxyLocation();
    ProxyManager & GetProxyManager();

    void onStart();
    void onFrame();
    void onUnitCreated(const sc2::Unit & unit);
    void onUnitDestroy(const sc2::Unit & unit);
    void onUnitEnterVision(const sc2::Unit & unit);
    void onBuildingConstructionComplete(const sc2::Unit& unit);

    void handleUnitAssignments();
    void setValidUnits();
    void setScoutUnits();
    void setCombatUnits();

    void drawDebugInterface();
    void drawGameInformation(int x, int y);

    bool shouldSendInitialScout();
};
