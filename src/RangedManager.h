#pragma once

#include "Common.h"
#include "MicroManager.h"

class ByunJRBot;

class RangedManager: public MicroManager
{
public:

    RangedManager(ByunJRBot & bot);
    void    executeMicro(const std::vector<sc2::Tag> & targets);
    void    assignTargets(const std::vector<sc2::Tag> & targets);
    int     getAttackPriority(const sc2::Tag & rangedUnit, const sc2::Tag & target);
    sc2::Tag getTarget(const sc2::Tag & rangedUnit, const std::vector<sc2::Tag> & targets);
};
