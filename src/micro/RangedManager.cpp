#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "micro/RangedManager.h"
#include "util/Util.h"

RangedManager::RangedManager(ByunJRBot & bot)
    : MicroManager(bot)
{

}

void RangedManager::executeMicro(const std::vector<sc2::Tag> & targets)
{
    assignTargets(targets);
}

void RangedManager::assignTargets(const std::vector<sc2::Tag> & targets)
{
    const std::vector<sc2::Tag> & rangedUnits = getUnits();

    // figure out targets
    std::vector<sc2::Tag> rangedUnitTargets;
    for (auto & targetTag : targets)
    {
        const auto target = m_bot.GetUnit(targetTag);

        if (!target) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

        rangedUnitTargets.push_back(targetTag);
    }

    // for each meleeUnit
    for (auto & rangedUnitTag : rangedUnits)
    {
        auto rangedUnit = m_bot.GetUnit(rangedUnitTag);
        BOT_ASSERT(rangedUnit, "melee unit is null");

        // if the order is to attack or defend
        if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
        {
            if (!rangedUnitTargets.empty())
            {
                // find the best target for this meleeUnit
                sc2::Tag targetTag = getTarget(rangedUnitTag, rangedUnitTargets);

                // attack it
                if (m_bot.Config().KiteWithRangedUnits)
                {
                    Micro::SmartKiteTarget(rangedUnitTag, targetTag, m_bot);
                }
                else
                {
                    Micro::SmartAttackUnit(rangedUnitTag, targetTag, m_bot);
                }
            }
            // if there are no targets
            else
            {
                // if we're not near the order position
                if (Util::Dist(rangedUnit->pos, order.getPosition()) > 4)
                {
                    // move to it
                    Micro::SmartMove(rangedUnitTag, order.getPosition(), m_bot);
                }
            }
        }

        if (m_bot.Config().DrawUnitTargetInfo)
        {
            // TODO: draw the line to the unit's target
        }
    }
}

// get a target for the ranged unit to attack
// TODO: this is the melee targeting code, replace it with something better for ranged units
sc2::Tag RangedManager::getTarget(const sc2::Tag & rangedUnitTag, const std::vector<sc2::Tag> & targets)
{
    auto rangedUnit = m_bot.GetUnit(rangedUnitTag);
    BOT_ASSERT(rangedUnit, "null ranged unit in getTarget");

    int highPriority = 0;
    double closestDist = std::numeric_limits<double>::max();
    sc2::Tag closestTarget = 0;

    // for each target possiblity
    for (auto & targetTag : targets)
    {
        auto targetUnit = m_bot.GetUnit(targetTag);
        BOT_ASSERT(targetUnit, "null target unit in getTarget");

        const int priority = getAttackPriority(rangedUnitTag, targetTag);
        const float distance = Util::Dist(rangedUnit->pos, targetUnit->pos);

        // Don't bother attacking units that we can not hit. 
        if (targetUnit->is_flying && !Util::CanAttackAir(m_bot.Observation()->GetUnitTypeData()[rangedUnit->unit_type].weapons))
        {
            continue;
        }

        if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist))
        {
            closestDist = distance;
            highPriority = priority;
            closestTarget = targetTag;
        }
    }

    return closestTarget;
}

// get the attack priority of a type in relation to a zergling
int RangedManager::getAttackPriority(const sc2::Tag & attacker, const sc2::Tag & unitTag)
{
    auto unit = m_bot.GetUnit(unitTag);
    BOT_ASSERT(unit, "null unit in getAttackPriority");

    if (Util::IsCombatUnit(*unit))
    {
        return 10;
    }

    if (Util::IsWorker(*unit))
    {
        return 9;
    }

    return 1;
}

