#pragma once
#include "micro/Squad.h"
#include "micro/SquadData.h"

class ByunJRBot;

class CombatCommander
{
    ByunJRBot &               bot_;

    SquadData                 squad_data_;
    std::set<const UnitInfo*> combat_units_;
    bool                      initialized_;
    bool                      attack_started_;

    //void             updateScoutDefenseSquad();
    //void             updateDefenseSquads();
    //void             updateAttackSquads();
    //void             updateIdleSquad();
    //bool             isSquadUpdateFrame() const;

    const sc2::Unit* FindClosestDefender(const Squad & defense_squad, const sc2::Point2D & pos);

    sc2::Point2D     GetMainAttackLocation() const;

    //void             updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded);
    bool             ShouldWeStartAttacking() const;

public:
    CombatCommander(ByunJRBot & bot);

    void OnStart();
    void OnFrame(const std::set<const UnitInfo*>& combat_units);
    void OnUnitCreated(const sc2::Unit* unit);

    void DrawSquadInformation();
};

