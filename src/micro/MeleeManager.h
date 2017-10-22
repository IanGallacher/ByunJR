#pragma once
#include "micro/MicroManager.h"

class ByunJRBot;

class MeleeManager: public MicroManager
{

public:

    MeleeManager(ByunJRBot & bot);
    void    ExecuteMicro(const std::vector<sc2::Tag> & targets) override;
    void    AssignTargets(const std::vector<sc2::Tag> & targets);
    int     GetAttackPriority(const sc2::Tag & attacker, const sc2::Tag & unit) const;
    sc2::Tag GetTarget(const sc2::Tag & melee_unit, const std::vector<sc2::Tag> & targets);
    bool MeleeUnitShouldRetreat(const sc2::Tag& melee_unit, const std::vector<sc2::Tag>& targets) const;
};
