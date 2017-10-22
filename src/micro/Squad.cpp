#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "micro/Squad.h"
#include "util/Util.h"

Squad::Squad(ByunJRBot & bot)
    : bot_(bot)
    , last_retreat_switch_(0)
    , last_retreat_switch_val_(false)
    , priority_(0)
    , name_("Default")
    , melee_manager_(bot)
    , ranged_manager_(bot)
{

}

Squad::Squad(const std::string & name, const SquadOrder & order, const size_t priority, ByunJRBot & bot)
    : bot_(bot)
    , name_(name)
    , order_(order)
    , last_retreat_switch_(0)
    , last_retreat_switch_val_(false)
    , priority_(priority)
    , melee_manager_(bot)
    , ranged_manager_(bot)
{
}

void Squad::OnFrame()
{
    // update all necessary unit information within this squad
    UpdateUnits();

    // determine whether or not we should regroup
    const bool need_to_regroup = NeedsToRegroup();
    
    // if we do need to regroup, do it
    if (need_to_regroup)
    {
        const sc2::Point2D regroup_position = CalcRegroupPosition();

        bot_.DebugHelper().DrawSphere(regroup_position, 3, sc2::Colors::Purple);

        melee_manager_.Regroup(regroup_position);
        ranged_manager_.Regroup(regroup_position);
    }
    else // otherwise, execute micro
    {
        melee_manager_.Execute(order_);
        ranged_manager_.Execute(order_);

        //_detectorManager.setUnitClosestToEnemy(unitClosestToEnemy());
        //_detectorManager.execute(_order);
    }
}

bool Squad::IsEmpty() const
{
    return units_.empty();
}

size_t Squad::GetPriority() const
{
    return priority_;
}

void Squad::SetPriority(const size_t & priority)
{
    priority_ = priority;
}

void Squad::UpdateUnits()
{
    SetAllUnits();
    SetNearEnemyUnits();
    AddUnitsToMicroManagers();
}

void Squad::SetAllUnits()
{
    // clean up the _units vector just in case one of them died
    std::set<sc2::Tag> good_units;
    for (auto & unit_tag : units_)
    {
        const sc2::Unit* unit = bot_.GetUnit(unit_tag);
        if (!unit) { continue; }
        if (unit->build_progress < 1.0f) { continue; }
        if (unit->health <= 0) { continue; }
        
        good_units.insert(unit_tag);
    }

    units_ = good_units;
}

void Squad::SetNearEnemyUnits()
{
    near_enemy_.clear();
    for (auto & unit_tag : units_)
    {
        near_enemy_[unit_tag] = IsUnitNearEnemy(unit_tag);

        sc2::Color color = near_enemy_[unit_tag] ? bot_.Config().ColorUnitNearEnemy : bot_.Config().ColorUnitNotNearEnemy;
        //bot_.Map().drawSphereAroundUnit(unitTag, color);
    }
}

void Squad::AddUnitsToMicroManagers()
{
    std::vector<sc2::Tag> melee_units;
    std::vector<sc2::Tag> ranged_units;
    std::vector<sc2::Tag> detector_units;
    std::vector<sc2::Tag> transport_units;
    std::vector<sc2::Tag> tank_units;

    // add _units to micro managers
    for (auto & unit_tag : units_)
    {
        const sc2::Unit* unit = bot_.GetUnit(unit_tag);

        BOT_ASSERT(unit, "null unit in addUnitsToMicroManagers()");

        if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_SIEGETANK || unit->unit_type == sc2::UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)
        {
            tank_units.push_back(unit_tag);
        }
        // TODO: detectors
        else if (Util::IsDetector(unit) && !Util::IsBuilding(unit->unit_type))
        {
            detector_units.push_back(unit_tag);
        }
        // select ranged _units
        else if (Util::GetAttackRange(unit->unit_type, bot_) >= 1.5f)
        {
            ranged_units.push_back(unit_tag);
        }
        // select melee _units
        else if (Util::GetAttackRange(unit->unit_type, bot_) < 1.5f)
        {
            melee_units.push_back(unit_tag);
        }
    }

    melee_manager_.SetUnits(melee_units);
    ranged_manager_.SetUnits(ranged_units);
    //m_detectorManager.setUnits(detectorUnits);
    //m_tankManager.setUnits(tankUnits);
}

// TODO: calculates whether or not to regroup
bool Squad::NeedsToRegroup() const
{
    return false;
}

void Squad::SetSquadOrder(const SquadOrder & so)
{
    order_ = so;
}

bool Squad::ContainsUnit(const sc2::Tag & u) const
{
    return std::find(units_.begin(), units_.end(),u) != units_.end();
}

void Squad::Clear()
{
    units_.clear();
}

bool Squad::IsUnitNearEnemy(const sc2::Tag & unit_tag) const
{
    auto unit = bot_.GetUnit(unit_tag);
    BOT_ASSERT(unit, "null unit in squad");

    for (auto & unit : bot_.Observation()->GetUnits())
    {
        if ((Util::GetPlayer(unit) == PlayerArrayIndex::Enemy) && (Util::Dist(unit->pos, bot_.GetUnit(unit_tag)->pos) < 20))
        {
            return true;
        }
    }

    return false;
}

sc2::Point2D Squad::CalcCenter() const
{
    if (units_.empty())
    {
        return sc2::Point2D(0.0f,0.0f);
    }

    sc2::Point2D sum(0,0);
    for (auto & unit_tag : units_)
    {
        auto unit = bot_.GetUnit(unit_tag);
        BOT_ASSERT(unit, "null unit in squad calcCenter");

        sum += unit->pos;
    }

    return sc2::Point2D(sum.x / units_.size(), sum.y / units_.size());
}

sc2::Point2D Squad::CalcRegroupPosition() const
{
    sc2::Point2D regroup(0.0f,0.0f);

    float min_dist = std::numeric_limits<float>::max();

    for (auto & unit_tag : units_)
    {
        const auto unit = bot_.GetUnit(unit_tag);

        if (!near_enemy_.at(unit_tag))
        {
            const float dist = Util::Dist(order_.GetPosition(), unit->pos);
            if (dist < min_dist)
            {
                min_dist = dist;
                regroup = unit->pos;
            }
        }
    }

    if (regroup.x == 0.0f && regroup.y == 0.0f)
    {
        return bot_.GetStartLocation();
    }
    else
    {
        return regroup;
    }
}

sc2::Tag Squad::UnitClosestToEnemy() const
{
    sc2::Tag closest = 0;
    int closest_dist = std::numeric_limits<int>::max();

    for (auto & unit_tag : units_)
    {
        auto unit = bot_.GetUnit(unit_tag);
        BOT_ASSERT(unit, "null unit");

        // the distance to the order position
        const int dist = bot_.Map().GetGroundDistance(unit->pos, order_.GetPosition());

        if (dist != -1 && (!closest || dist < closest_dist))
        {
            closest = unit_tag;
            closest_dist = dist;
        }
    }

    return closest;
}

int Squad::SquadUnitsNear(const sc2::Point2D & p) const
{
    int num_units = 0;

    for (auto & unit_tag : units_)
    {
        auto unit = bot_.GetUnit(unit_tag);
        BOT_ASSERT(unit, "null unit");

        if (Util::Dist(unit->pos, p) < 20.0f)
        {
            num_units++;
        }
    }

    return num_units;
}

const std::set<sc2::Tag> & Squad::GetUnits() const
{
    return units_;
}

const SquadOrder & Squad::GetSquadOrder() const
{
    return order_;
}

void Squad::AddUnit(const sc2::Tag & u)
{
    units_.insert(u);
}

void Squad::RemoveUnit(const sc2::Tag & u)
{
    units_.erase(u);
}

const std::string & Squad::GetName() const
{
    return name_;
}