#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "macro/Building.h"
#include "macro/WorkerManager.h"
#include "util/Util.h"
#include "common/BotAssert.h"

WorkerManager::WorkerManager(ByunJRBot & bot)
    : m_bot         (bot)
{
    m_previousClosestWorker = nullptr;
}

void WorkerManager::onFrame()
{
    assignIdleWorkers();
    assignGasWorkers();
	handleWorkers();
}

void WorkerManager::assignIdleWorkers() const
{
	// for each of our workers
	for (auto & workerInfo : m_bot.InformationManager().UnitInfo().getWorkers())
	{
		const sc2::Unit* worker = workerInfo->unit;
		if (!worker) { continue; }

		// if it's a scout or creating a proxy building, don't handle it here
		if (m_bot.InformationManager().UnitInfo().getUnitInfoMap(PlayerArrayIndex::Self).at(workerInfo->unit->tag).mission == UnitMission::Scout
			|| m_bot.InformationManager().UnitInfo().getUnitInfoMap(PlayerArrayIndex::Self).at(workerInfo->unit->tag).mission == UnitMission::Proxy)
		{
			continue;
		}

		// if it is idle
		if (Util::IsIdle(worker) || workerInfo->mission == UnitMission::Idle)
		{
		// Uncomment to worker rush. 
		//	m_bot.InformationManager().assignUnit(workerInfo->unit->tag, UnitMission::Attack);
			setMineralWorker(workerInfo->unit);
		}
	}
}

void WorkerManager::assignGasWorkers() const
{
    // for each unit we have
    for (auto refinery : m_bot.InformationManager().UnitInfo().getUnits(PlayerArrayIndex::Self))
    {
        // if that unit is a refinery
        if (Util::IsRefinery(refinery) && Util::IsCompleted(refinery))
        {
            // get the number of workers currently assigned to it
            const int numAssigned = m_bot.InformationManager().UnitInfo().getNumAssignedWorkers(refinery);

            // if it's less than we want it to be, fill 'er up
			// As a side effect of the following hack ( read next paragraph of comments )
			// (3-numAssigned) has been changed to (2-numAssigned)
			// Otherwise there will be four workers on gas when playing terran. 
            for (int i=0; i<(2-numAssigned); ++i)
            {
                sc2::Tag gasWorker = getGasWorker(refinery);
                if (gasWorker)
                {
                    m_bot.InformationManager().UnitInfo().setJob(m_bot.GetUnit(gasWorker), UnitMission::Gas, refinery->tag);
					// Once a unit starts gathering resources, we don't send another command to gather resources. 
					// If a unit is already gathering minerals, he won't start mining gas. 
					// As a temporary work around, we send the gather command once they get assigned to a geyser. 
					// A side effect of this system is the workers will never look for a more optimal mineral field to gather from. 
					// TODO: Come up with a better system. 
					m_bot.Actions()->UnitCommand(m_bot.GetUnit(gasWorker), sc2::ABILITY_ID::SMART, refinery);
					Micro::SmartRightClick(m_bot.GetUnit(gasWorker), refinery, m_bot);
                }
            }
        }
    }
}

// Make the workers go do the things that they were assigned to do. 
void WorkerManager::handleWorkers() const
{
	for (auto & workerInfo : m_bot.InformationManager().UnitInfo().getWorkers())
	{
		const UnitMission job = workerInfo->mission;
		if (job == UnitMission::Minerals)
		{
			// find the mineral to mine and mine it
			const sc2::Unit* cc = m_bot.InformationManager().getClosestUnitOfType(workerInfo->unit, sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER);
			const sc2::Tag mineralToMine = getMineralToMine(cc);
			Micro::SmartRightClick(workerInfo->unit, m_bot.GetUnit(mineralToMine), m_bot);
		}
		else if (job == UnitMission::Gas)
		{
			// right click the refinery to start harvesting
			Micro::SmartRightClick(workerInfo->unit, m_bot.GetUnit(workerInfo->workerDepotTag), m_bot);
		}
	}
}

sc2::Tag WorkerManager::getMineralToMine(const sc2::Unit* unit) const
{
	sc2::Tag bestMineral = -1;
	double bestDist = std::numeric_limits<double>::max();;

	for (auto & mineral : m_bot.Observation()->GetUnits())
	{
		if (!Util::IsMineral(mineral)) continue;

		const double dist = Util::Dist(mineral->pos, unit->pos);

		if (dist < bestDist)
		{
			bestMineral = mineral->tag;
			bestDist = dist;
		}
	}

	return bestMineral;
}

// Unlike the GetCosestUnit functions inside InformationManager, this only iterates through workers.
sc2::Tag WorkerManager::getClosestMineralWorkerTo(const sc2::Point2D & pos) const
{
    sc2::Tag closestMineralWorker = 0;
    double closestDist = std::numeric_limits<double>::max();

    // for each of our workers
    for (auto & workerInfo : m_bot.InformationManager().UnitInfo().getWorkers())
    {
		if (!workerInfo) { std::cout << "Warning: a workerInfo pointer is invalid." << std::endl; continue; }

        // if it is a mineral worker
        if (workerInfo->mission == UnitMission::Minerals
        ||  workerInfo->mission == UnitMission::Proxy)
        {
            const double dist = Util::DistSq(workerInfo->unit->pos, pos);

            if (!closestMineralWorker || dist < closestDist)
            {
                closestMineralWorker = workerInfo->unit->tag;
                closestDist = dist;
            }
        }
    }

    return closestMineralWorker;
}

// set a worker to mine minerals
void WorkerManager::setMineralWorker(const sc2::Unit* unit) const
{
    // check if there is a mineral available to send the worker to
	const sc2::Unit* base = m_bot.InformationManager().getClosestBase(unit);
	if (!base) return; 

    const sc2::Tag baseTag = base->tag; 

    // if there is a valid mineral
    if (baseTag)
    {
        // update m_workerData with the new job
        m_bot.InformationManager().UnitInfo().setJob(unit, UnitMission::Minerals, baseTag);
    }
}

sc2::Tag WorkerManager::getGasWorker(const sc2::Unit* refinery) const
{
    return getClosestMineralWorkerTo(refinery->pos);
}

// gets a builder for BuildingManager to use
// if setJobAsBuilder is true (default), it will be flagged as a builder unit
// set 'setJobAsBuilder' to false if we just want to see which worker will build a building
sc2::Tag WorkerManager::getBuilder(Building & b, const bool setJobAsBuilder) const
{
    const sc2::Tag builderWorker = getClosestMineralWorkerTo(b.finalPosition);

    // if the worker exists (one may not have been found in rare cases)
    if (builderWorker && setJobAsBuilder)
    {
		m_bot.InformationManager().UnitInfo().setJob(m_bot.GetUnit(builderWorker), UnitMission::Build, b.type);
    }

    return builderWorker;
}

bool WorkerManager::isFree(const sc2::Unit* worker) const
{
	const UnitMission job = m_bot.InformationManager().UnitInfo().getUnitInfo(worker)->mission;
    return job == UnitMission::Minerals || job == UnitMission::Idle;
}

bool WorkerManager::isWorkerScout(const sc2::Unit* worker) const
{
    return (m_bot.InformationManager().UnitInfo().getUnitInfo(worker)->mission == UnitMission::Scout);
}

bool WorkerManager::isBuilder(const sc2::Unit* worker) const
{
    return (m_bot.InformationManager().UnitInfo().getUnitInfo(worker)->mission == UnitMission::Build);
}
