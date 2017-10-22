#pragma once
#include "micro/SquadOrder.h"
#include "micro/Micro.h"

struct AirThreat
{
    const sc2::Unit* unit;
    double weight;
};

struct GroundThreat
{
    const sc2::Unit* unit;
    double weight;
};

class ByunJRBot;

class MicroManager
{
    std::vector<const sc2::Unit*> units_;

protected:
    ByunJRBot & bot_;
    SquadOrder order_;

    virtual void ExecuteMicro(const std::set<const sc2::Unit*> & targets) = 0;

public:
    MicroManager(ByunJRBot & bot);

    const std::vector<const sc2::Unit*> & GetUnits() const;

    void SetUnits(const std::vector<const sc2::Unit*> & u);
    void Execute(const SquadOrder & order);
    void Regroup(const sc2::Point2D & regroup_position) const;

};