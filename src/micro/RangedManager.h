#pragma once
#include "micro/MicroManager.h"

class ByunJRBot;

class RangedManager: public MicroManager
{
public:

    RangedManager(ByunJRBot & bot);
    void    ExecuteMicro(const std::vector<sc2::Tag> & targets) override;
    void    AssignTargets(const std::vector<sc2::Tag> & targets);
    int     GetAttackPriority(const sc2::Tag & ranged_unit, const sc2::Tag & target) const;
    sc2::Tag GetTarget(const sc2::Tag & ranged_unit, const std::vector<sc2::Tag> & targets);
};
