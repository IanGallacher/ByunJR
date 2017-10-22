#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "micro/MicroManager.h"
#include "util/Util.h"

MicroManager::MicroManager(ByunJRBot & bot)
    : bot_(bot)
{
}

void MicroManager::SetUnits(const std::vector<sc2::Tag> & u)
{
    units_ = u;
}

void MicroManager::Execute(const SquadOrder & input_order)
{
    // Nothing to do if we have no units
    if (units_.empty() || !(input_order.GetType() == SquadOrderTypes::Attack || input_order.GetType() == SquadOrderTypes::Defend))
    {
        return;
    }

    order_ = input_order;

    // Discover enemies within region of interest
    std::set<sc2::Tag> nearby_enemies;

    // Get all relavant units that are close to our combat unit.
    for (auto & enemy_unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Enemy))
    {
        if (Util::Dist(enemy_unit->pos, order_.GetPosition()) < order_.GetRadius())
        {
            nearby_enemies.insert(enemy_unit->tag);
        }
    }
    // otherwise we want to see everything on the way as well
    //if (order.getType() == SquadOrderTypes::Attack)
    //{
    //    for (auto & unitTag : units)
    //    {
    //        auto unit = bot_.GetUnit(unitTag);
    //        BOT_ASSERT(unit, "null unit in attack");

    //        for (auto & enemyUnit : bot_.InformationManager().UnitInfo().getUnits(PlayerArrayIndex::Enemy))
    //        {
    //            if (Util::Dist(enemyUnit.pos, unit->pos) < order.getRadius())
    //            {
    //                nearbyEnemies.insert(enemyUnit.tag);
    //            }
    //        }
    //    }
    //}

    std::vector<sc2::Tag> target_unit_tags;
    std::copy(nearby_enemies.begin(), nearby_enemies.end(), std::back_inserter(target_unit_tags));

    // the following block of code attacks all units on the way to the order position
    // we want to do this if the order is attack, defend, or harass
    if (order_.GetType() == SquadOrderTypes::Attack || order_.GetType() == SquadOrderTypes::Defend)
    {
        ExecuteMicro(target_unit_tags);
    }
}

const std::vector<sc2::Tag> & MicroManager::GetUnits() const
{
    return units_;
}

void MicroManager::Regroup(const sc2::Point2D & regroup_position) const
{
    const sc2::Point2D our_base_position = bot_.GetStartLocation();
    const int regroup_distance_from_base = bot_.Map().GetGroundDistance(regroup_position, our_base_position);

    // for each of the units we have
    for (auto & unit_tag : units_)
    {
        auto unit = bot_.GetUnit(unit_tag);
        BOT_ASSERT(unit, "null unit in MicroManager regroup");

        const int unit_distance_from_base = bot_.Map().GetGroundDistance(unit->pos, our_base_position);

        // if the unit is outside the regroup area
        if (unit_distance_from_base > regroup_distance_from_base)
        {
            Micro::SmartMove(bot_.GetUnit(unit_tag), our_base_position, bot_);
        }
        else if (Util::Dist(unit->pos, regroup_position) > 4)
        {
            // regroup it
            Micro::SmartMove(bot_.GetUnit(unit_tag), regroup_position, bot_);
        }
        else
        {
            Micro::SmartAttackMove(bot_.GetUnit(unit_tag), unit->pos, bot_);
        }
    }
}