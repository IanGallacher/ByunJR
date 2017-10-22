#pragma once
#include "micro/MeleeManager.h"
#include "micro/RangedManager.h"
#include "micro/SquadOrder.h"

class ByunJRBot;

class Squad
{
    ByunJRBot &         bot_;

    std::string         name_;
    std::set<sc2::Tag>  units_;
    std::string         regroup_status_;
    int                 last_retreat_switch_;
    bool                last_retreat_switch_val_;
    size_t              priority_;

    SquadOrder          order_;
    MeleeManager        melee_manager_;
    RangedManager       ranged_manager_;

    std::map<sc2::Tag, bool>    near_enemy_;

    sc2::Tag UnitClosestToEnemy() const;

    void UpdateUnits();
    void AddUnitsToMicroManagers();
    void SetNearEnemyUnits();
    void SetAllUnits();

    bool IsUnitNearEnemy(const sc2::Tag & unit) const;
    bool NeedsToRegroup() const;
    int  SquadUnitsNear(const sc2::Point2D & pos) const;

public:

    Squad(const std::string & name, const SquadOrder & order, size_t priority, ByunJRBot & bot);
    Squad(ByunJRBot & bot);

    void OnFrame();
    void SetSquadOrder(const SquadOrder & so);
    void AddUnit(const sc2::Tag & u);
    void RemoveUnit(const sc2::Tag & u);
    void Clear();

    bool ContainsUnit(const sc2::Tag & u) const;
    bool IsEmpty() const;
    size_t GetPriority() const;
    void SetPriority(const size_t & priority);
    const std::string & GetName() const;

    sc2::Point2D CalcCenter() const;
    sc2::Point2D CalcRegroupPosition() const;

    const std::set<sc2::Tag>& GetUnits() const;
    const SquadOrder& GetSquadOrder() const;
};
