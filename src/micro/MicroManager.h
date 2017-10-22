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
    std::vector<sc2::Tag> units_;

protected:

    ByunJRBot & bot_;
    SquadOrder order_;

    virtual void ExecuteMicro(const std::vector<sc2::Tag> & targets) = 0;

public:

    MicroManager(ByunJRBot & bot);

    const std::vector<sc2::Tag> & GetUnits() const;

    void SetUnits(const std::vector<sc2::Tag> & u);
    void Execute(const SquadOrder & order);
    void Regroup(const sc2::Point2D & regroup_position) const;

};