#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "micro/MeleeManager.h"
#include "util/Util.h"


MeleeManager::MeleeManager(ByunJRBot & bot)
    : MicroManager(bot)
{

}

void MeleeManager::executeMicro(const std::vector<sc2::Tag> & targets)
{
    assignTargets(targets);
}

void MeleeManager::assignTargets(const std::vector<sc2::Tag> & targets)
{
    const std::vector<sc2::Tag> & meleeUnits = getUnits();

    // figure out targets
    std::vector<sc2::Tag> meleeUnitTargets;
    for (auto & targetTag : targets)
    {
        auto target = m_bot.GetUnit(targetTag);

        if (!target) { continue; }
        if (target->is_flying) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_EGG) { continue; }
        if (target->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) { continue; }

        meleeUnitTargets.push_back(targetTag);
    }

    // for each meleeUnit
    for (auto & meleeUnitTag : meleeUnits)
    {
        auto meleeUnit = m_bot.GetUnit(meleeUnitTag);
        BOT_ASSERT(meleeUnit, "melee unit is null");

        // if the order is to attack or defend
        if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
        {
            // run away if we meet the retreat critereon
            if (meleeUnitShouldRetreat(meleeUnitTag, targets))
            {
                sc2::Point2D fleeTo(m_bot.GetStartLocation());

                Micro::SmartMove(meleeUnitTag, fleeTo, m_bot);
            }
            // if there are targets
            else if (!meleeUnitTargets.empty())
            {
                // find the best target for this meleeUnit
                sc2::Tag targetTag = getTarget(meleeUnitTag, meleeUnitTargets);

                // attack it
                Micro::SmartAttackUnit(meleeUnitTag, targetTag, m_bot);
            }
            // if there are no targets
            else
            {
                // if we're not near the order position
                if (Util::Dist(meleeUnit->pos, order.getPosition()) > 4)
                {
                    // move to it
                    Micro::SmartMove(meleeUnitTag, order.getPosition(), m_bot);
                }
            }
        }

        if (m_bot.Config().DrawUnitTargetInfo)
        {
            // TODO: draw the line to the unit's target
        }
    }
}

// get a target for the meleeUnit to attack
sc2::Tag MeleeManager::getTarget(const sc2::Tag & meleeUnitTag, const std::vector<sc2::Tag> & targets)
{
    auto meleeUnit = m_bot.GetUnit(meleeUnitTag);
    BOT_ASSERT(meleeUnit, "null melee unit in getTarget");

    int highPriority = 0;
    double closestDist = std::numeric_limits<double>::max();
    sc2::Tag closestTarget = 0;

    // for each target possiblity
    for (auto & targetTag : targets)
    {
        auto targetUnit = m_bot.GetUnit(targetTag);
        BOT_ASSERT(targetUnit, "null target unit in getTarget");

        int priority = getAttackPriority(meleeUnitTag, targetTag);
        float distance = Util::Dist(meleeUnit->pos, targetUnit->pos);

        // if it's a higher priority, or it's closer, set it
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
int MeleeManager::getAttackPriority(const sc2::Tag & attacker, const sc2::Tag & unitTag)
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

bool MeleeManager::meleeUnitShouldRetreat(const sc2::Tag & meleeUnit, const std::vector<sc2::Tag> & targets)
{
    // TODO: should melee units ever retreat?
    return false;
}
