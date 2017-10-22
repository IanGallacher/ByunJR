#pragma once
#include "micro/MicroManager.h"

class ByunJRBot;

class MeleeManager: public MicroManager
{

public:

    MeleeManager(ByunJRBot & bot);
    void              ExecuteMicro(const std::set<const sc2::Unit*> & targets) override;
    void              AssignTargets(const std::set<const sc2::Unit*> & targets);
    int               GetAttackPriority(const sc2::Unit* attacker, const sc2::Unit* unit) const;
    const sc2::Unit*  GetTarget(const sc2::Unit* melee_unit, const std::vector<const sc2::Unit*> & targets);
    bool              MeleeUnitShouldRetreat(const sc2::Unit* melee_unit, const std::set<const sc2::Unit*>& targets) const;
};
