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


// All the unit combat controllers currently inherit from CombatMicroManager.
class CombatMicroManager
{
    std::vector<const sc2::Unit*> combat_units_;
    std::map<sc2::Tag, int> planned_damage_;

protected:
    ByunJRBot & bot_;
    SquadOrder order_;

public:
    CombatMicroManager(ByunJRBot & bot);

    void SetUnits(const std::vector<const sc2::Unit*> & u);
    void Execute(const SquadOrder & order);
    void Regroup(const sc2::Point2D & regroup_position) const;


    void AttackTargets(const std::set<const sc2::Unit*> & targets);
    int  GetAttackPriority(const sc2::Unit* ranged_unit, const sc2::Unit* target) const;
    const sc2::Unit* GetTarget(const sc2::Unit* ranged_unit, const std::set<const sc2::Unit*> & targets) const;

    const sc2::Unit * GetYamatoTarget(const sc2::Unit * combat_unit, const std::set<const sc2::Unit*>& targets) const;

    bool ShouldUnitRetreat(const sc2::Unit * unit) const;


#pragma region Advanced micro functionality
    float TimeToFaceEnemy(const sc2::Unit * unit, const sc2::Unit * target) const;
    void SmartKiteTarget(const sc2::Unit* ranged_unit, const sc2::Unit* target) const;
#pragma endregion
};