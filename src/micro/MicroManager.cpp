#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "micro/MicroManager.h"
#include "util/Util.h"

MicroManager::MicroManager(ByunJRBot & bot)
    : m_bot(bot)
{
}

void MicroManager::setUnits(const std::vector<sc2::Tag> & u)
{
    m_units = u;
}

void MicroManager::execute(const SquadOrder & inputOrder)
{
    // Nothing to do if we have no units
    if (m_units.empty() || !(inputOrder.getType() == SquadOrderTypes::Attack || inputOrder.getType() == SquadOrderTypes::Defend))
    {
        return;
    }

    order = inputOrder;

    // Discover enemies within region of interest
    std::set<sc2::Tag> nearbyEnemies;

    // if the order is to defend, we only care about units in the radius of the defense
    if (order.getType() == SquadOrderTypes::Defend)
    {
        for (auto & enemyUnit : m_bot.InformationManager().UnitInfo().getUnits(PlayerArrayIndex::Enemy))
        {
            if (Util::Dist(enemyUnit.pos, order.getPosition()) < order.getRadius())
            {
                nearbyEnemies.insert(enemyUnit.tag);
            }
        }

    } // otherwise we want to see everything on the way as well
    else if (order.getType() == SquadOrderTypes::Attack)
    {
        for (auto & enemyUnit : m_bot.InformationManager().UnitInfo().getUnits(PlayerArrayIndex::Enemy))
        {
            if (Util::Dist(enemyUnit.pos, order.getPosition()) < order.getRadius())
            {
                nearbyEnemies.insert(enemyUnit.tag);
            }
        }

        for (auto & unitTag : m_units)
        {
            auto unit = m_bot.GetUnit(unitTag);
            BOT_ASSERT(unit, "null unit in attack");

            for (auto & enemyUnit : m_bot.InformationManager().UnitInfo().getUnits(PlayerArrayIndex::Enemy))
            {
                if (Util::Dist(enemyUnit.pos, unit->pos) < order.getRadius())
                {
                    nearbyEnemies.insert(enemyUnit.tag);
                }
            }
        }
    }

    std::vector<sc2::Tag> targetUnitTags;
    std::copy(nearbyEnemies.begin(), nearbyEnemies.end(), std::back_inserter(targetUnitTags));

    // the following block of code attacks all units on the way to the order position
    // we want to do this if the order is attack, defend, or harass
    if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend)
    {
        executeMicro(targetUnitTags);
    }
}

const std::vector<sc2::Tag> & MicroManager::getUnits() const
{
    return m_units;
}

void MicroManager::regroup(const sc2::Point2D & regroupPosition) const
{
    sc2::Point2D ourBasePosition = m_bot.GetStartLocation();
    int regroupDistanceFromBase = m_bot.Map().getGroundDistance(regroupPosition, ourBasePosition);

    // for each of the units we have
    for (auto & unitTag : m_units)
    {
        auto unit = m_bot.GetUnit(unitTag);
        BOT_ASSERT(unit, "null unit in MicroManager regroup");

        int unitDistanceFromBase = m_bot.Map().getGroundDistance(unit->pos, ourBasePosition);

        // if the unit is outside the regroup area
        if (unitDistanceFromBase > regroupDistanceFromBase)
        {
            Micro::SmartMove(unitTag, ourBasePosition, m_bot);
        }
        else if (Util::Dist(unit->pos, regroupPosition) > 4)
        {
            // regroup it
            Micro::SmartMove(unitTag, regroupPosition, m_bot);
        }
        else
        {
            Micro::SmartAttackMove(unitTag, unit->pos, m_bot);
        }
    }
}