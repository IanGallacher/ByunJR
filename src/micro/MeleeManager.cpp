#include "ByunJRBot.h"
#include "TechLab/util/Util.h"

#include "common/BotAssert.h"
#include "micro/MeleeManager.h"


MeleeManager::MeleeManager(ByunJRBot & bot)
    : CombatMicroManager(bot)
{

}

void MeleeManager::ExecuteMicro(const std::set<const sc2::Unit*> & targets)
{
    AssignTargets(targets);
}

void MeleeManager::AssignTargets(const std::set<const sc2::Unit*> & targets)
{
    const std::vector<const sc2::Unit*> & melee_units = GetUnits();

    // figure out targets
    std::vector<const sc2::Unit*> melee_unit_targets;
    for (auto & target : targets)
    {
        if (!target) { continue; }
        if (target->is_flying) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

        melee_unit_targets.push_back(target);
    }

    // for each meleeUnit
    for (auto & melee_unit_tag : melee_units)
    {
        auto melee_unit = melee_unit_tag;
        BOT_ASSERT(melee_unit, "melee unit is null");

        // if the order is to attack or defend
        if (order_.GetType() == SquadOrderTypes::Attack || order_.GetType() == SquadOrderTypes::Defend)
        {
            // run away if we meet the retreat critereon
            if (MeleeUnitShouldRetreat(melee_unit, targets))
            {
                const sc2::Point2D flee_to(bot_.GetStartLocation());

                Micro::SmartMove(melee_unit, flee_to, bot_);
                bot_.Info().UnitInfo().SetJob(melee_unit, UnitMission::Minerals);
            }
            // if there are targets
            else if (!melee_unit_targets.empty())
            {
                // find the best target for this meleeUnit
                const sc2::Unit* target = GetTarget(melee_unit_tag, melee_unit_targets);

                // Sometimes we won't find a unit to attack. 
                if (!target)
                    continue;

                // attack it
                Micro::SmartAttackUnit(melee_unit, target, bot_);
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
const sc2::Unit* MeleeManager::GetTarget(const sc2::Unit* melee_unit, const std::vector<const sc2::Unit*> & targets)
{
    BOT_ASSERT(melee_unit, "null melee unit in getTarget");

    int lowest_health = std::numeric_limits<int>::max();
    int high_priority = std::numeric_limits<int>::max();
    double closest_dist = std::numeric_limits<double>::max();
    const sc2::Unit* best_target = nullptr;

    // If our reaper is currently safe, go kill some workers.
    if (bot_.Info().GetDPSMap()[melee_unit->pos.y][melee_unit->pos.x] < 12)
    {
        for (auto & target_unit : targets)
        {
            if (Util::IsWorker(target_unit))
            {
                const float distance = Util::Dist(melee_unit->pos, target_unit->pos);

                // Only look for workers that are close to the reaper. 
                if (distance > 7) continue;
                if (!best_target || target_unit->health < lowest_health)
                {
                    lowest_health = target_unit->health;
                    best_target = target_unit;
                }
            }
        }
    }

    // for each target possiblity
    for (auto & target_unit : targets)
    {
        BOT_ASSERT(target_unit, "null target unit in getTarget");

        // Don't waste time killing buildings until we have a good chance of winning the game from all the workers we have killed
        if (Util::IsBuilding(target_unit->unit_type) && Util::GetGameTimeInSeconds(bot_) < 400)
            continue;

        const int priority = GetAttackPriority(melee_unit, target_unit);
        const float distance = Util::Dist(melee_unit->pos, target_unit->pos);

        // if it's a higher priority, or it's closer, set it
        if (!best_target || (priority > high_priority) || (priority == high_priority && distance < closest_dist))
        {
            closest_dist = distance;
            high_priority = priority;
            best_target = target_unit;
        }
    }


    return best_target;
}

// get the attack priority of a type in relation to a zergling
int MeleeManager::GetAttackPriority(const sc2::Unit* attacker, const sc2::Unit* unit) const
{
    BOT_ASSERT(unit, "null unit in getAttackPriority");

    if (Util::IsCombatUnit(unit, bot_))
    {
        return 10;
    }

    if (Util::IsWorker(unit))
    {
        return 9;
    }

    return 1;
}

bool MeleeManager::MeleeUnitShouldRetreat(const sc2::Unit* melee_unit, const std::set<const sc2::Unit*>& targets) const
{
    // TODO: should melee units ever retreat?
    if (melee_unit->health <= 10)
        return true;
    return false;
}
