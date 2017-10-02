#pragma once
#include "micro/MicroManager.h"

class ByunJRBot;

class MeleeManager: public MicroManager
{

public:

    MeleeManager(ByunJRBot & bot);
    void    executeMicro(const std::vector<sc2::Tag> & targets);
    void    assignTargets(const std::vector<sc2::Tag> & targets);
    int     getAttackPriority(const sc2::Tag & attacker, const sc2::Tag & unit);
    sc2::Tag getTarget(const sc2::Tag & meleeUnit, const std::vector<sc2::Tag> & targets);
    bool    meleeUnitShouldRetreat(const sc2::Tag & meleeUnit, const std::vector<sc2::Tag> & targets);
};
