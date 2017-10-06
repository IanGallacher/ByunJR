#include <sc2lib/sc2_lib.h>
#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "macro/Building.h"
#include "micro/ProxyManager.h"
#include "micro/Micro.h"

ProxyManager::ProxyManager(ByunJRBot & bot)
    : m_bot(bot)
    , m_proxyUnitTag(0)
    , m_proxyUnderAttack(false)
{

}

void ProxyManager::onStart()
{
    m_firstReaperCreated = false;
    m_ptd.InitAllValues(m_bot);
}

void ProxyManager::onFrame()
{
    proxyBuildingAtChosenRandomLocation();
}

void ProxyManager::onUnitCreated(const sc2::Unit& unit)
{
    if (m_bot.Config().TrainingMode && unit.unit_type == sc2::UNIT_TYPEID::TERRAN_REAPER && !m_firstReaperCreated)
    {
        const BaseLocation * enemyBaseLocation = m_bot.Bases().getPlayerStartingBaseLocation(PlayerArrayIndex::Enemy);

        m_bot.Resign();
        m_ptd.recordResult((int)m_bot.Query()->PathingDistance(&unit, enemyBaseLocation->getPosition()));
        m_firstReaperCreated = true;
    }
}

void ProxyManager::onUnitEnterVision(const sc2::Unit& enemyUnit)
{
    // TODO: Optimize this code to only search buildings, not every single unit a player owns.
    for (auto & unit : m_bot.InformationManager().UnitInfo().getUnits(PlayerArrayIndex::Self))
    {
        if (unit.unit_type == sc2::UNIT_TYPEID::TERRAN_BARRACKS || unit.tag == m_proxyUnitTag)
        {
            const double dist(sqrt((enemyUnit.pos.x - unit.pos.x)*(enemyUnit.pos.x - unit.pos.x) + (enemyUnit.pos.y - unit.pos.y)*(enemyUnit.pos.y - unit.pos.y)));

            if (m_bot.Config().TrainingMode && dist < 10 && !m_firstReaperCreated)
            {
                m_bot.Resign();
                m_ptd.recordResult(-9);
                std::cout << "THERE IS NO POINT IN CONTINUING";
            }
        }
    }
}

// YOU MUST CALL m_ptd.InitAllValues() before this.
bool ProxyManager::proxyBuildingAtChosenRandomLocation()
{
    if (!m_ptd.proxyLocationReady())
        return false;

    //if (m_bot.GetUnit(m_proxyUnitTag)->pos.x > myVec.x - 1 && m_bot.GetUnit(m_proxyUnitTag)->pos.x < myVec.x + 1)
    //{
    //    m_bot.Workers().finishedWithWorker(m_proxyUnitTag);
    //}
    //else
    //{
    if (m_proxyUnitTag == 0)
    {
        const sc2::Vector2D myVec(m_ptd.getProxyLocation());
        Building b(sc2::UNIT_TYPEID::TERRAN_BARRACKS, myVec);
        m_proxyUnitTag = m_bot.Workers().getBuilder(b, false);
        m_bot.Workers().setProxyWorker(m_proxyUnitTag);
        Micro::SmartMove(m_proxyUnitTag, myVec, m_bot);
    }
    //}

    return true;
}

sc2::Point2D ProxyManager::getProxyLocation()
{
    return m_ptd.getProxyLocation();
}

ProxyTrainingData & ProxyManager::getProxyTrainingData()
{
    return m_ptd;
}