#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "micro/MeleeManager.h"
#include "util/Util.h"


MeleeManager::MeleeManager(ByunJRBot & bot)
    : MicroManager(bot)
{

}

void MeleeManager::ExecuteMicro(const std::vector<sc2::Tag> & targets)
{
    AssignTargets(targets);
}

void MeleeManager::AssignTargets(const std::vector<sc2::Tag> & targets)
{
    const std::vector<sc2::Tag> & melee_units = GetUnits();

    // figure out targets
    std::vector<sc2::Tag> melee_unit_targets;
    for (auto & target_tag : targets)
    {
        const auto target = bot_.GetUnit(target_tag);

        if (!target) { continue; }
        if (target->is_flying) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

        melee_unit_targets.push_back(target_tag);
    }

    // for each meleeUnit
    for (auto & melee_unit_tag : melee_units)
    {
        auto melee_unit = bot_.GetUnit(melee_unit_tag);
        BOT_ASSERT(melee_unit, "melee unit is null");

        // if the order is to attack or defend
        if (order_.GetType() == SquadOrderTypes::Attack || order_.GetType() == SquadOrderTypes::Defend)
        {
            // run away if we meet the retreat critereon
            if (MeleeUnitShouldRetreat(melee_unit_tag, targets))
            {
                const sc2::Point2D flee_to(bot_.GetStartLocation());

                Micro::SmartMove(melee_unit, flee_to, bot_);
                bot_.InformationManager().UnitInfo().SetJob(melee_unit, UnitMission::Minerals);
            }
            // if there are targets
            else if (!melee_unit_targets.empty())
            {
                // find the best target for this meleeUnit
                sc2::Tag target_tag = GetTarget(melee_unit_tag, melee_unit_targets);

                // attack it
                Micro::SmartAttackUnit(melee_unit, bot_.GetUnit(target_tag), bot_);
            }
            // if there are no targets
            else
            {
                // if we're not near the order position
                if (Util::Dist(melee_unit->pos, order_.GetPosition()) > 4)
                {
                    // move to it
                    Micro::SmartMove(melee_unit, order_.GetPosition(), bot_);
                }
            }
        }

        if (bot_.Config().DrawUnitTargetInfo)
        {
            // TODO: draw the line to the unit's target
        }
    }
}

// get a target for the meleeUnit to attack
sc2::Tag MeleeManager::GetTarget(const sc2::Tag & melee_unit_tag, const std::vector<sc2::Tag> & targets)
{
    auto melee_unit = bot_.GetUnit(melee_unit_tag);
    BOT_ASSERT(melee_unit, "null melee unit in getTarget");

    int high_priority = 0;
    double closest_dist = std::numeric_limits<double>::max();
    sc2::Tag closest_target = 0;

    // for each target possiblity
    for (auto & target_tag : targets)
    {
        auto target_unit = bot_.GetUnit(target_tag);
        BOT_ASSERT(target_unit, "null target unit in getTarget");

        const int priority = GetAttackPriority(melee_unit_tag, target_tag);
        const float distance = Util::Dist(melee_unit->pos, target_unit->pos);

        // if it's a higher priority, or it's closer, set it
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
int MeleeManager::GetAttackPriority(const sc2::Tag & attacker, const sc2::Tag & unit_tag) const
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

bool MeleeManager::MeleeUnitShouldRetreat(const sc2::Tag& melee_unit, const std::vector<sc2::Tag>& targets) const
{
    // TODO: should melee units ever retreat?
    if (bot_.GetUnit(melee_unit)->health <= 10)
        return true;
    return false;
}
