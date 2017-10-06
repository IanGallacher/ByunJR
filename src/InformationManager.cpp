#include <sc2api/sc2_api.h>

#include "ByunJRBot.h"
#include "InformationManager.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "util/Util.h"

InformationManager::InformationManager(ByunJRBot & bot)
    : m_bot(bot),
    m_initialScoutSet(false)
{

}

sc2::Point2D InformationManager::GetProxyLocation() const
{
    return m_bot.GetProxyManager().getProxyLocation();
}

void InformationManager::assignUnit(const sc2::Tag & unit, std::vector<sc2::Tag> & units)
{
    if (std::find(m_scoutUnits.begin(), m_scoutUnits.end(), unit) != m_scoutUnits.end())
    {
        m_bot.UnitInfoManager().setJob(*m_bot.GetUnit(unit), UnitMission::Scout);
        m_scoutUnits.erase(std::remove(m_scoutUnits.begin(), m_scoutUnits.end(), unit), m_scoutUnits.end());
    }
    else if (std::find(m_combatUnits.begin(), m_combatUnits.end(), unit) != m_combatUnits.end())
    {
        m_bot.UnitInfoManager().setJob(*m_bot.GetUnit(unit), UnitMission::Attack);
        m_combatUnits.erase(std::remove(m_combatUnits.begin(), m_combatUnits.end(), unit), m_combatUnits.end());
    }

    units.push_back(unit);
}

bool InformationManager::isAssigned(const sc2::Tag & unit) const
{
    return     (std::find(m_combatUnits.begin(), m_combatUnits.end(), unit) != m_combatUnits.end())
        || (std::find(m_scoutUnits.begin(), m_scoutUnits.end(), unit) != m_scoutUnits.end());
}



// validates units as usable for distribution to various managers
void InformationManager::setValidUnits()
{
    // make sure the unit is completed and alive and usable
    for (auto & unit : m_bot.UnitInfoManager().getUnits(PlayerArrayIndex::Self))
    {
        m_validUnits.push_back(unit.tag);
    }
}

void InformationManager::setScoutUnits(bool shouldSendInitialScout)
{
    // if we haven't set a scout unit, do it
    if (m_scoutUnits.empty() && !m_initialScoutSet)
    {
        // if it exists
        if (shouldSendInitialScout)
        {
            // grab the closest worker to the supply provider to send to scout
            sc2::Tag workerScoutTag = m_bot.Workers().getClosestMineralWorkerTo(m_bot.GetStartLocation());
            const sc2::Unit * workerScout = m_bot.GetUnit(workerScoutTag);

            // if we find a worker (which we should) add it to the scout units
            if (workerScout)
            {
                m_bot.Scout().setWorkerScout(workerScoutTag);

                assignUnit(workerScoutTag, m_scoutUnits);
                m_initialScoutSet = true;
            }
            else
            {

            }
        }
    }
}
// sets combat units to be passed to CombatCommander
void InformationManager::setCombatUnits()
{
    for (auto & unitTag : m_validUnits)
    {
        const sc2::Unit * unit = m_bot.GetUnit(unitTag);

        BOT_ASSERT(unit, "Have a null unit in our valid units\n");

        if (!isAssigned(unitTag) && Util::IsCombatUnitType(unit->unit_type))
        {
            assignUnit(unitTag, m_combatUnits);
        }
    }
   /* if (!m_attackStarted)
    {
        return;
    }*/
    
    //Squad & mainAttackSquad = m_squadData.getSquad("MainAttack");
    //
    //for (auto & unitTag : m_combatUnits)
    //{
    //    auto unit = m_bot.GetUnit(unitTag);
    //    BOT_ASSERT(unit, "null unit in combat units");
    //
    //    // get every unit of a lower priority and put it into the attack squad
    //    if (!Util::IsWorker(*unit) && m_squadData.canAssignUnitToSquad(unitTag, mainAttackSquad))
    //    {
    //        m_squadData.assignUnitToSquad(unitTag, mainAttackSquad);
    //    }
    //}
    //
    //const SquadOrder mainAttackOrder(SquadOrderTypes::Attack, getMainAttackLocation(), 25, "Attack Enemy Base");
    //mainAttackSquad.setSquadOrder(mainAttackOrder);
}

void InformationManager::handleUnitAssignments()
{
    m_validUnits.clear();
    m_combatUnits.clear();

    // filter our units for those which are valid and usable
    setValidUnits();

    // set each type of unit
    setScoutUnits(m_bot.Strategy().shouldSendInitialScout());
    setCombatUnits();
}

std::vector<sc2::Tag> InformationManager::GetCombatUnits() const
{
    return m_combatUnits;
}

