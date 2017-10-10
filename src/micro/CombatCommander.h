#pragma once
#include "micro/Squad.h"
#include "micro/SquadData.h"

class ByunJRBot;

class CombatCommander
{
    ByunJRBot &      m_bot;

    SquadData        m_squadData;
    std::vector<sc2::Tag> m_combatUnits;
    bool             m_initialized;
    bool             m_attackStarted;

    //void             updateScoutDefenseSquad();
    //void             updateDefenseSquads();
    //void             updateAttackSquads();
    //void             updateIdleSquad();
    //bool             isSquadUpdateFrame() const;

    sc2::Tag         findClosestDefender(const Squad & defenseSquad, const sc2::Point2D & pos);

    sc2::Point2D     getMainAttackLocation() const;

    //void             updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded);
    bool             shouldWeStartAttacking() const;

public:

    CombatCommander(ByunJRBot & bot);


    void onStart();
    void onFrame(const std::vector<sc2::Tag> & combatUnits);

    void drawSquadInformation();
};

