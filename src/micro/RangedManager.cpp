#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "micro/RangedManager.h"
#include "util/Util.h"

RangedManager::RangedManager(ByunJRBot & bot)
    : MicroManager(bot)
{

}

void RangedManager::ExecuteMicro(const std::vector<sc2::Tag> & targets)
{
    AssignTargets(targets);
}

void RangedManager::AssignTargets(const std::vector<sc2::Tag> & targets)
{
    const std::vector<sc2::Tag> & ranged_units = GetUnits();

    // figure out targets
    std::vector<sc2::Tag> ranged_unit_targets;


    // Zerg eggs are a pain in the butt to kill. Don't bother.
    for (auto & target_tag : targets)
    {
        const auto target = bot_.GetUnit(target_tag);

        if (!target) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

        ranged_unit_targets.push_back(target_tag);
    }

    // for each meleeUnit
    for (auto & ranged_unit_tag : ranged_units)
    {
        auto ranged_unit = bot_.GetUnit(ranged_unit_tag);
        BOT_ASSERT(ranged_unit, "melee unit is null");

        // if the order is to attack or defend
        if (order_.GetType() == SquadOrderTypes::Attack || order_.GetType() == SquadOrderTypes::Defend)
        {
            if (!ranged_unit_targets.empty() && GetTarget(ranged_unit_tag, ranged_unit_targets) != 0)
            {
                // find the best target for this meleeUnit
                const sc2::Unit* target = bot_.GetUnit(GetTarget(ranged_unit_tag, ranged_unit_targets));

                // attack it
                if (bot_.Config().KiteWithRangedUnits)
                {
                    Micro::SmartKiteTarget(ranged_unit, target, bot_);
                }
                else
                {
                    Micro::SmartAttackUnit(ranged_unit, target, bot_);
                }
            }
            // if there are no targets
            else
            {
                // if we're not near the order position
                if (Util::Dist(ranged_unit->pos, order_.GetPosition()) > 4)
                {
                    // move to it
                    Micro::SmartMove(ranged_unit, order_.GetPosition(), bot_);
                }
            }
        }

        if (bot_.Config().DrawUnitTargetInfo)
        {
            // TODO: draw the line to the unit's target
        }
    }
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
sc2::Tag RangedManager::GetTarget(const sc2::Tag & ranged_unit_tag, const std::vector<sc2::Tag> & targets)
{
    auto ranged_unit = bot_.GetUnit(ranged_unit_tag);
    BOT_ASSERT(ranged_unit, "null ranged unit in getTarget");

    int high_priority = 0;
    double closest_dist = std::numeric_limits<double>::max();
    sc2::Tag closest_target = 0;

    // for each target possiblity
    for (auto & target_tag : targets)
    {
        auto target_unit = bot_.GetUnit(target_tag);
        BOT_ASSERT(target_unit, "null target unit in getTarget");

        const int priority = GetAttackPriority(ranged_unit_tag, target_tag);
        const float distance = Util::Dist(ranged_unit->pos, target_unit->pos);

        // Don't bother attacking units that we can not hit. 
        if (target_unit->is_flying && !Util::CanAttackAir(bot_.Observation()->GetUnitTypeData()[ranged_unit->unit_type].weapons))
        {
            continue;
        }
        // If there are ranged units on high ground we can't see, we can't attack them back.
        if(!bot_.Map().IsVisible(target_unit->pos) && Util::IsCombatUnit(target_unit))
        {
            continue;
        }

        if (!closest_target || (priority > high_priority) || (priority == high_priority && distance < closest_dist))
        {
            closest_dist = distance;
            high_priority = priority;
            closest_target = target_tag;
        }
    }

    return closest_target;
}

// get the attack priority of a type in relation to a zergling
int RangedManager::GetAttackPriority(const sc2::Tag & attacker, const sc2::Tag & unit_tag) const
{
    auto unit = bot_.GetUnit(unit_tag);
    BOT_ASSERT(unit, "null unit in getAttackPriority");

    if (Util::IsCombatUnit(unit))
    {
        return 10;
    }

    if (Util::IsWorker(unit))
    {
        return 9;
    }

    return 1;
}

