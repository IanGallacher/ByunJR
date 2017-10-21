#pragma once
#include <sc2api/sc2_api.h>

class ByunJRBot;

class ScoutManager
{
    ByunJRBot &     m_bot;

    sc2::Tag        m_scoutUnitTag;
    std::string     m_scoutStatus;
    int             m_numScouts;
    bool            m_scoutUnderAttack;
    float           m_previousScoutHP;

    bool            enemyWorkerInRadiusOf(const sc2::Point2D & pos) const;
    sc2::Point2D    getFleePosition() const;
    const sc2::Unit* closestEnemyWorkerTo(const sc2::Point2D & pos) const;
    void            moveScouts();
    void            drawScoutInformation();

public:

    ScoutManager(ByunJRBot & bot);

    void onStart();
    void onFrame();
    void setWorkerScout(const sc2::Tag & tag);
};