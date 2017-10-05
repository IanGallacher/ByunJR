#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "macro/Building.h"
#include "macro/WorkerManager.h"
#include "util/Util.h"
#include "common/BotAssert.h"

WorkerManager::WorkerManager(ByunJRBot & bot)
    : m_bot         (bot)
    , m_workerData  (bot)
{
    m_previousClosestWorker = -1;
}

void WorkerManager::onStart()
{

}

void WorkerManager::onFrame()
{
    m_workerData.updateAllWorkerData();
    handleGasWorkers();
    handleIdleWorkers();

    drawResourceDebugInfo();

    m_workerData.drawDepotDebugInfo();

    handleRepairWorkers();
}

void WorkerManager::setRepairWorker(const sc2::Unit & worker, const sc2::Unit & unitToRepair)
{
    m_workerData.setWorkerJob(worker.tag, UnitMission::Repair, unitToRepair.tag);
}

void WorkerManager::stopRepairing(const sc2::Unit & worker)
{
    m_workerData.setWorkerJob(worker.tag, UnitMission::Idle);
}

void WorkerManager::handleGasWorkers()
{
    // for each unit we have
    for (auto & unit : m_bot.UnitInfoManager().getUnits(Players::Self))
    {
        // if that unit is a refinery
        if (Util::IsRefinery(unit) && Util::IsCompleted(unit))
        {
            // get the number of workers currently assigned to it
            int numAssigned = m_workerData.getNumAssignedWorkers(unit.tag);

            // if it's less than we want it to be, fill 'er up
            for (int i=0; i<(3-numAssigned); ++i)
            {
                sc2::Tag gasWorker = getGasWorker(unit);
                if (gasWorker)
                {
                    m_workerData.setWorkerJob(gasWorker, UnitMission::Gas, unit.tag);
                }
            }
        }
    }
}

void WorkerManager::handleIdleWorkers()
{
    // for each of our workers
    for (auto & workerTag : m_workerData.getWorkers())
    {
        auto worker = m_bot.GetUnit(workerTag);
        if (!worker) { continue; }

        // if it's a scout or creating a proxy building, don't handle it here
        if (m_workerData.getWorkerJob(workerTag) == UnitMission::Scout
        ||  m_workerData.getWorkerJob(workerTag) == UnitMission::Proxy)
        {
            continue;
        }

        // if it is idle
        if (Util::IsIdle(*worker) || m_workerData.getWorkerJob(workerTag) == UnitMission::Idle)
        {
            const sc2::Unit * workerUnit = m_bot.GetUnit(workerTag);

            // send it to the nearest mineral patch
            if (workerUnit)
            {
                setMineralWorker(*workerUnit);
            }
        }
    }
}

void WorkerManager::handleRepairWorkers()
{
    // TODO
}

sc2::Tag WorkerManager::getClosestMineralWorkerTo(const sc2::Point2D & pos) const
{
    sc2::Tag closestMineralWorker = 0;
    double closestDist = std::numeric_limits<double>::max();

    // for each of our workers
    for (auto & workerTag : m_workerData.getWorkers())
    {
        if (!m_bot.GetUnit(workerTag)) { continue; }

        // if it is a mineral worker
        if (m_workerData.getWorkerJob(workerTag) == UnitMission::Minerals
          || m_workerData.getWorkerJob(workerTag) == UnitMission::Proxy)
        {
            double dist = Util::DistSq(m_bot.GetUnit(workerTag)->pos, pos);

            if (!closestMineralWorker || dist < closestDist)
            {
                closestMineralWorker = workerTag;
                closestDist = dist;
            }
        }
    }

    return closestMineralWorker;
}

sc2::Tag WorkerManager::findClosestWorkerTo(const sc2::Point2D & target) const
{
    sc2::Tag closestMineralWorker = 0;
    float closestDist = std::numeric_limits<float>::max();


    // for each of our workers
    for (auto & workerTag : m_workerData.getWorkers())
    {
        if (!m_bot.GetUnit(workerTag)) { continue; }

        double dist = Util::DistSq(m_bot.GetUnit(workerTag)->pos, target);

        if (!closestMineralWorker || dist < closestDist)
        {
            closestMineralWorker = workerTag;
            closestDist = dist;
        }
    }

    return closestMineralWorker;
}


// set a worker to mine minerals
void WorkerManager::setMineralWorker(const sc2::Unit & unit)
{
    // check if there is a mineral available to send the worker to
    sc2::Tag depot = getClosestCC(unit);

    // if there is a valid mineral
    if (depot)
    {
        // update m_workerData with the new job
        m_workerData.setWorkerJob(unit.tag, UnitMission::Minerals, depot);
    }
}

sc2::Tag WorkerManager::getClosestCC(const sc2::Unit & worker) const
{
    sc2::Tag closestDepot = 0;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto & unit : m_bot.UnitInfoManager().getUnits(Players::Self))
    {
        //if (!m_bot.GetUnit(unit.tag)) { continue; }

        if (Util::IsTownHall(unit) && Util::IsCompleted(unit))
        {
            double distance = Util::DistSq(unit.pos, worker.pos);
            if (!closestDepot || distance < closestDistance)
            {
                closestDepot = unit.tag;
                closestDistance = distance;
            }
        }
    }

    return closestDepot;
}

// other managers that need workers call this when they're done with a unit
void WorkerManager::finishedWithWorker(const sc2::Tag & tag)
{
    m_workerData.setWorkerJob(tag, UnitMission::Idle);
}

sc2::Tag WorkerManager::getGasWorker(const sc2::Unit & refinery) const
{
    return getClosestMineralWorkerTo(refinery.pos);
}

void WorkerManager::setBuildingWorker(const sc2::Unit & worker, Building & b)
{
    m_workerData.setWorkerJob(worker.tag, UnitMission::Build, b.type);
}

// gets a builder for BuildingManager to use
// if setJobAsBuilder is true (default), it will be flagged as a builder unit
// set 'setJobAsBuilder' to false if we just want to see which worker will build a building
sc2::Tag WorkerManager::getBuilder(Building & b, bool setJobAsBuilder) const
{
    sc2::Tag builderWorker = getClosestMineralWorkerTo(b.finalPosition);

    // if the worker exists (one may not have been found in rare cases)
    if (builderWorker && setJobAsBuilder)
    {
        m_workerData.setWorkerJob(builderWorker, UnitMission::Build, b.type);
    }

    return builderWorker;
}

// sets a worker as a scout
void WorkerManager::setScoutWorker(const sc2::Tag & workerTag)
{
    m_workerData.setWorkerJob(workerTag, UnitMission::Scout);
}

void WorkerManager::setCombatWorker(const sc2::Tag & workerTag)
{
    m_workerData.setWorkerJob(workerTag, UnitMission::Attack);
}

void WorkerManager::setProxyWorker(const sc2::Tag & workerTag)
{
    m_workerData.setWorkerJob(workerTag, UnitMission::Proxy);
}

void WorkerManager::drawResourceDebugInfo()
{
    //if (!m_bot.Config().DrawResourceInfo)
    //{
    //    return;
    //}

    //for (auto & workerTag : m_workerData.getWorkers())
    //{
    //    if (!m_bot.GetUnit(workerTag)) { continue; }

    //    m_bot.Map().drawText(m_bot.GetUnit(workerTag)->pos, m_workerData.getJobCode(workerTag));

    //    auto depot = m_bot.GetUnit(m_workerData.getWorkerDepot(workerTag));
    //    if (depot)
    //    {
    //        m_bot.Map().drawLine(m_bot.GetUnit(workerTag)->pos, depot->pos);
    //    }
    //}
}

bool WorkerManager::isFree(const sc2::Unit & worker) const
{
    return m_workerData.getWorkerJob(worker.tag) == UnitMission::Minerals || m_workerData.getWorkerJob(worker.tag) == UnitMission::Idle;
}

bool WorkerManager::isWorkerScout(const sc2::Unit & worker) const
{
    return (m_workerData.getWorkerJob(worker.tag) == UnitMission::Scout);
}

bool WorkerManager::isBuilder(const sc2::Unit & worker) const
{
    return (m_workerData.getWorkerJob(worker.tag) == UnitMission::Build);
}
