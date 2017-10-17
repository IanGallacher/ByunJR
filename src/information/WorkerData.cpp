#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "information/WorkerData.h"
//#include "micro/Micro.h"
#include "util/Util.h"

WorkerData::WorkerData(ByunJRBot & bot)
    : m_bot(bot)
{

}

void WorkerData::updateAllWorkerData()
{
    // check all our units and add new workers if we find them
    for (auto & unit : m_bot.InformationManager().UnitInfo().getUnits(PlayerArrayIndex::Self))
    {
        if (Util::IsWorker(unit))
        {
            updateWorker(unit->tag);
        }
    }

    // for each of our Workers
    for (auto & workerTag : getWorkers())
    {
        const auto worker = m_bot.GetUnit(workerTag);
        if (worker == nullptr) { continue; }

        // TODO: If it's a gas worker whose refinery has been destroyed, set to minerals
    }

    // remove any worker units which no longer exist in the game
    std::vector<sc2::Tag> workersDestroyed;
    for (auto & workerTag : getWorkers())
    {
        const sc2::Unit* worker = m_bot.GetUnit(workerTag);

        // TODO: for now skip gas workers because they disappear inside refineries, this is annoying
        if (!worker && (getWorkerJob(workerTag) != UnitMission::Gas))
        {
            workersDestroyed.push_back(workerTag);
        }
    }

    for (auto tag : workersDestroyed)
    {
        workerDestroyed(tag);
    }
}

void WorkerData::workerDestroyed(const sc2::Tag & unit)
{
    clearPreviousJob(unit);
    m_workers.erase(unit);
}

void WorkerData::updateWorker(const sc2::Tag & unit)
{
    if (m_workers.find(unit) == m_workers.end())
    {
        m_workers.insert(unit);
        m_workerJobMap[unit] = UnitMission::Idle;
    }
}

void WorkerData::setWorkerJob(const sc2::Tag & unit, UnitMission job, sc2::Tag jobUnitTag)
{
    clearPreviousJob(unit);
    m_workerJobMap[unit] = job;
    m_bot.InformationManager().UnitInfo().setJob(m_bot.GetUnit(unit), job);

    if (job == UnitMission::Minerals)
    {
        // if we haven't assigned anything to this depot yet, set its worker count to 0
        if (m_depotWorkerCount.find(jobUnitTag) == m_depotWorkerCount.end())
        {
            m_depotWorkerCount[jobUnitTag] = 0;
        }

        // add the depot to our set of depots
        m_depots.insert(jobUnitTag);

        // increase the worker count of this depot
        m_workerDepotMap[unit] = jobUnitTag;
        m_depotWorkerCount[jobUnitTag]++;

        // find the mineral to mine and mine it
        sc2::Tag cc = m_bot.InformationManager().getClosestUnitOfType(m_bot.GetUnit(unit), sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER)->tag;
        const sc2::Tag mineralToMine = getMineralToMine(cc);
        Micro::SmartRightClick(unit, mineralToMine, m_bot);
    }
    else if (job == UnitMission::Gas)
    {
        // if we haven't assigned any workers to this refinery yet set count to 0
        if (m_refineryWorkerCount.find(jobUnitTag) == m_refineryWorkerCount.end())
        {
            m_refineryWorkerCount[jobUnitTag] = 0;
        }

        // increase the count of workers assigned to this refinery
        m_refineryWorkerCount[jobUnitTag] += 1;
        m_workerRefineryMap[unit] = jobUnitTag;

        // right click the refinery to start harvesting
        Micro::SmartRightClick(unit, jobUnitTag, m_bot);
    }
    else if (job == UnitMission::Repair)
    {
        Micro::SmartRepair(unit, jobUnitTag, m_bot);
    }
    else if (job == UnitMission::Scout)
    {

    }
    else if (job == UnitMission::Build)
    {

    }
    else if (job == UnitMission::Proxy)
    {

    }
}

void WorkerData::clearPreviousJob(const sc2::Tag & unit)
{
    const UnitMission previousJob = getWorkerJob(unit);

    if (previousJob == UnitMission::Minerals)
    {
        // remove one worker from the count of the depot this worker was assigned to
        m_depotWorkerCount[m_workerDepotMap[unit]]--;
        m_workerDepotMap.erase(unit);
    }
    else if (previousJob == UnitMission::Gas)
    {
        m_refineryWorkerCount[m_workerRefineryMap[unit]]--;
        m_workerRefineryMap.erase(unit);
    }
    else if (previousJob == UnitMission::Build)
    {

    }
    else if (previousJob == UnitMission::Repair)
    {

    }
    else if (previousJob == UnitMission::Move)
    {

    }

    m_workerJobMap.erase(unit);
}

size_t WorkerData::getNumWorkers() const
{
    return m_workers.size();
}

UnitMission WorkerData::getWorkerJob(const sc2::Tag & unit) const
{
    auto it = m_workerJobMap.find(unit);

    if (it != m_workerJobMap.end())
    {
        return it->second;
    }

    return UnitMission::Idle;
}

sc2::Tag WorkerData::getMineralToMine(const sc2::Tag & unit) const
{
    sc2::Tag bestMineral = -1;
    double bestDist = 100000;

    for (auto & mineral : m_bot.Observation()->GetUnits())
    {
        if (!Util::IsMineral(mineral)) continue;

        double dist = Util::Dist(mineral->pos, m_bot.GetUnit(unit)->pos);

        if (dist < bestDist)
        {
            bestMineral = mineral->tag;
            bestDist = dist;
        }
    }

    return bestMineral;
}

sc2::Tag WorkerData::getWorkerDepot(const sc2::Tag & unit) const
{
    auto it = m_workerDepotMap.find(unit);

    if (it != m_workerDepotMap.end())
    {
        return it->second;
    }

    return -1;
}

int WorkerData::getNumAssignedWorkers(const sc2::Tag & unit)
{
    if (Util::IsTownHall(m_bot.GetUnit(unit)))
    {
        const auto it = m_depotWorkerCount.find(unit);

        // if there is an entry, return it
        if (it != m_depotWorkerCount.end())
        {
            return it->second;
        }
    }
    else if (Util::IsRefinery(m_bot.GetUnit(unit)))
    {
        const auto it = m_refineryWorkerCount.find(unit);

        // if there is an entry, return it
        if (it != m_refineryWorkerCount.end())
        {
            return it->second;
        }
        // otherwise, we are only calling this on completed refineries, so set it
        else
        {
            m_refineryWorkerCount[unit] = 0;
        }
    }

    // when all else fails, return 0
    return 0;
}

// Used for printing debug text on the worker.

void WorkerData::drawDepotDebugInfo()
{
    for (auto & depotTag : m_depots)
    {
        const auto depot = m_bot.GetUnit(depotTag);

        if (!depot) continue;
        std::stringstream ss;
        ss << "Workers: " << getNumAssignedWorkers(depot->tag);

        m_bot.Map().drawText(depot->pos, ss.str());
    }
}

const std::set<sc2::Tag> & WorkerData::getWorkers() const
{
    return m_workers;
}