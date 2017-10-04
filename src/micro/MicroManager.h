#pragma once
#include "micro/SquadOrder.h"
#include "micro/Micro.h"

struct AirThreat
{
    sc2::Tag unit;
    double weight;
};

struct GroundThreat
{
    sc2::Tag unit;
    double weight;
};

class ByunJRBot;

class MicroManager
{
    std::vector<sc2::Tag> m_units;

protected:

    ByunJRBot & m_bot;
    SquadOrder order;

    virtual void executeMicro(const std::vector<sc2::Tag> & targets) = 0;

public:

    MicroManager(ByunJRBot & bot);

    const std::vector<sc2::Tag> & getUnits() const;

    void setUnits(const std::vector<sc2::Tag> & u);
    void execute(const SquadOrder & order);
    void regroup(const sc2::Point2D & regroupPosition) const;

};