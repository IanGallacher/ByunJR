#pragma once
#include "micro/MicroManager.h"

class ByunJRBot;

class RangedManager: public MicroManager
{
public:
    
    RangedManager(ByunJRBot & bot);
    void    ExecuteMicro(const std::set<const sc2::Unit*> & targets) override;
    void    AssignTargets(const std::set<const sc2::Unit*> & targets) const;
    int     GetAttackPriority(const sc2::Unit* ranged_unit, const sc2::Unit* target) const;
    const sc2::Unit* GetTarget(const sc2::Unit* ranged_unit, const std::vector<const sc2::Unit*> & targets) const;
};
