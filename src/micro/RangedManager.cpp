#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "micro/RangedManager.h"
#include "util/Util.h"

RangedManager::RangedManager(ByunJRBot & bot)
    : CombatMicroManager(bot)
{

}

void RangedManager::ExecuteMicro(const std::set<const sc2::Unit*> & targets)
{
    AssignTargets(targets);
}

void RangedManager::AssignTargets(const std::set<const sc2::Unit*> & targets) const
{
    const std::vector<const sc2::Unit*> & ranged_units = GetUnits();

    // figure out targets
    std::vector<const sc2::Unit*> ranged_unit_targets;


    // Zerg eggs are a pain in the butt to kill. Don't bother.
    for (auto & target : targets)
    {
        if (!target) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

        ranged_unit_targets.push_back(target);
    }

    // for each ranged_unit
    for (auto & ranged_unit : ranged_units)
    {
        // Run away with the cattlebruisers!
        if (ranged_unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER
            && ranged_unit->health < ranged_unit->health_max / 5)
        {
            bot_.Actions()->UnitCommand(ranged_unit, sc2::ABILITY_ID::EFFECT_TACTICALJUMP,
                bot_.Bases().GetPlayerStartingBaseLocation(PlayerArrayIndex::Self)->GetPosition());
            bot_.InformationManager().UnitInfo().SetJob(ranged_unit, UnitMission::Wait);
            continue;
        }

        // if the order is to attack or defend
        if (order_.GetType() == SquadOrderTypes::Attack || order_.GetType() == SquadOrderTypes::Defend)
        {
            if (!ranged_unit_targets.empty() && GetTarget(ranged_unit, ranged_unit_targets))
            {
                // find the best target for this meleeUnit
                const sc2::Unit* target = GetTarget(ranged_unit, ranged_unit_targets);

                // Sometimes we won't find a unit to attack. 
                if (!target)
                    continue;

                if(ranged_unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER)
                {
                    Micro::SmartAttackUnit(ranged_unit, target, bot_);
                }
                // attack it
                else if (bot_.Config().KiteWithRangedUnits)
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
const sc2::Unit* RangedManager::GetTarget(const sc2::Unit* ranged_unit_tag, const std::vector<const sc2::Unit*> & targets) const
{
    auto ranged_unit = ranged_unit_tag;
    BOT_ASSERT(ranged_unit, "null ranged unit in getTarget");

    int lowest_health = std::numeric_limits<int>::max();
    int high_priority = std::numeric_limits<int>::max();
    double closest_dist = std::numeric_limits<double>::max();
    const sc2::Unit* best_target = nullptr;

    // If our reaper is currently safe, go kill some workers.
    if (bot_.InformationManager().GetDPSMap()[ranged_unit->pos.y][ranged_unit->pos.x] < 12)
    {
        for(auto & target_unit : targets)
        {
            if(Util::IsWorker(target_unit))
            {
                const float distance = Util::Dist(ranged_unit->pos, target_unit->pos);

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
    // The reaper is safe and has a low health worker it wants to attack. 
    if (best_target) return best_target;

    // for each target possiblity
    for (auto & target_unit : targets)
    {
        BOT_ASSERT(target_unit, "null target unit in getTarget");

        const int priority = GetAttackPriority(ranged_unit_tag, target_unit);
        const float distance = Util::Dist(ranged_unit->pos, target_unit->pos);
        int f = bot_.Observation()->GetGameLoop();

        // Don't waste time killing buildings until we have a good chance of winning the game from all the workers we have killed
        if(Util::IsBuilding(target_unit->unit_type) && Util::GetGameTimeInSeconds(bot_) < 400)
            continue;

        // Don't bother attacking units that we can not hit. 
        if (target_unit->is_flying && !Util::CanAttackAir(bot_.Observation()->GetUnitTypeData()[ranged_unit->unit_type].weapons))
            continue;

        // If there are ranged units on high ground we can't see, we can't attack them back.
        if(!bot_.Map().IsVisible(target_unit->pos) && Util::IsCombatUnit(target_unit))
            continue;
        
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
int RangedManager::GetAttackPriority(const sc2::Unit* attacker, const sc2::Unit* unit_tag) const
{
    auto unit = unit_tag;
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

