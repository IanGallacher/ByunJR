#pragma once
#include "micro/MeleeManager.h"
#include "micro/RangedManager.h"
#include "micro/SquadOrder.h"

class ByunJRBot;

class Squad
{
    ByunJRBot &         m_bot;

    std::string         m_name;
    std::set<sc2::Tag>  m_units;
    std::string         m_regroupStatus;
    int                 m_lastRetreatSwitch;
    bool                m_lastRetreatSwitchVal;
    size_t              m_priority;

    SquadOrder          m_order;
    MeleeManager        m_meleeManager;
    RangedManager       m_rangedManager;

    std::map<sc2::Tag, bool>	m_nearEnemy;

    sc2::Tag unitClosestToEnemy() const;

    void updateUnits();
    void addUnitsToMicroManagers();
    void setNearEnemyUnits();
    void setAllUnits();

    bool isUnitNearEnemy(const sc2::Tag & unit) const;
    bool needsToRegroup() const;
    int  squadUnitsNear(const sc2::Point2D & pos) const;

public:

    Squad(const std::string & name, const SquadOrder & order, size_t priority, ByunJRBot & bot);
    Squad(ByunJRBot & bot);

    void onFrame();
    void setSquadOrder(const SquadOrder & so);
    void addUnit(const sc2::Tag & u);
    void removeUnit(const sc2::Tag & u);
    void clear();

    bool containsUnit(const sc2::Tag & u) const;
    bool isEmpty() const;
    size_t getPriority() const;
    void setPriority(const size_t & priority);
    const std::string & getName() const;

    sc2::Point2D calcCenter() const;
    sc2::Point2D calcRegroupPosition() const;

    const std::set<sc2::Tag> & getUnits() const;
    const SquadOrder & getSquadOrder() const;
};
