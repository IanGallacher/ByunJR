#include "ByunJRBot.h"
#include "information/UnitData.h"
#include "util/Util.h"

UnitData::UnitData()
    : m_mineralsLost(0)
    , m_gasLost(0)
{
    const int maxTypeID = 1024;
    m_numDeadUnits	    = std::vector<int>(maxTypeID + 1, 0);
    m_numUnits		    = std::vector<int>(maxTypeID + 1, 0);
}

void UnitData::updateUnit(const sc2::Unit* unit)
{
    bool firstSeen = false;
    const auto & it = m_unitInfoMap.find(unit->tag);
    if (it == m_unitInfoMap.end())
    {
        firstSeen = true;
        m_unitInfoMap[unit->tag] = UnitInfo();
    }

    UnitInfo & ui   = m_unitInfoMap[unit->tag];
    ui.unit         = unit;
    ui.player       = Util::GetPlayer(unit);
    ui.lastPosition = unit->pos;
    ui.lastHealth   = unit->health;
    ui.lastShields  = unit->shield;
    ui.type         = unit->unit_type;
    ui.progress     = unit->build_progress;

    if (firstSeen)
    {
        m_numUnits[ui.type]++;
		ui.mission = UnitMission::Idle;
    }

	if (Util::IsWorker(unit))
	{
		m_workers.insert(&m_unitInfoMap[unit->tag]);
	}
    else if(Util::IsCombatUnit(unit))
    {
        m_combatUnits.insert(&m_unitInfoMap[unit->tag]);
    }
}

void UnitData::killUnit(const sc2::Unit* unit)
{
    //m_mineralsLost += unit->unit_type.mineralPrice();
    //_gasLost += unit->getType().gasPrice();
    m_numUnits[unit->unit_type]--;
    m_numDeadUnits[unit->unit_type]++;

	// If the previous unit was a worker, go ahead and update some stats.
	clearPreviousJob(unit);
	m_workers.erase(&m_unitInfoMap[unit->tag]);
	// Erasing the unit must be last. Cleanup the pointers before deleting the object. 
    m_unitInfoMap.erase(unit->tag); 
}

void UnitData::removeBadUnits()
{
    for (auto iter = m_unitInfoMap.begin(); iter != m_unitInfoMap.end();)
    {
        if (badUnitInfo(iter->second))
        {
            m_numUnits[iter->second.type]--;
            iter = m_unitInfoMap.erase(iter);
			m_combatUnits.erase(&iter->second);
			m_workers.erase(&iter->second);
        }
        else
        {
            iter++;
        }
    }
}

bool UnitData::badUnitInfo(const UnitInfo & ui) const
{
    return false;
}

size_t UnitData::getNumWorkers() const
{
	return m_workers.size();
}

int UnitData::getGasLost() const
{
    return m_gasLost;
}

int UnitData::getMineralsLost() const
{
    return m_mineralsLost;
}

int UnitData::getNumUnits(sc2::UnitTypeID t) const
{
    return m_numUnits[t];
}

int UnitData::getNumDeadUnits(sc2::UnitTypeID t) const
{
    return m_numDeadUnits[t];
}

int UnitData::getNumAssignedWorkers(const sc2::Unit* depot)
{
	if (Util::IsTownHall(depot))
	{
		const auto it = m_baseWorkerCount.find(depot->tag);

		// if there is an entry, return it
		if (it != m_baseWorkerCount.end())
		{
			return it->second;
		}
	}
	else if (Util::IsRefinery(depot))
	{
        return depot->assigned_harvesters;
	}

	// when all else fails, return 0
	return 0;
}

const std::map<sc2::Tag, UnitInfo>& UnitData::getUnitInfoMap() const
{
    return m_unitInfoMap;
}

std::set<const UnitInfo*> UnitData::GetCombatUnits() const
{
	return m_combatUnits;
}

// jobUnitTag is optional.
void UnitData::setJob(const sc2::Unit* unit, const UnitMission job, const sc2::Tag jobUnitTag)
{
	clearPreviousJob(unit);

	UnitInfo & ui = m_unitInfoMap[unit->tag];

	// Update the information about the current job. 
	if (job == UnitMission::Minerals)
	{
		// if we haven't assigned anything to this depot yet, set its worker count to 0
		if (m_baseWorkerCount.find(jobUnitTag) == m_baseWorkerCount.end())
		{
			m_baseWorkerCount[jobUnitTag] = 0;
		}

		// add the depot to our set of depots
		m_depots.insert(&m_unitInfoMap[unit->tag]);

		// increase the worker count of this depot
		m_baseWorkerCount[jobUnitTag]++;
		m_workerDepotMap[unit->tag] = unit;
	}
	else if (job == UnitMission::Gas)
	{
		m_workerRefineryMap[unit->tag] = unit;
		// If the jobUnitTag is actually valid, set the worker depot to that value.
		if (jobUnitTag != 0)
		{
			ui.workerDepotTag = jobUnitTag;
		}
	}
	else if (job == UnitMission::Attack)
	{
		m_combatUnits.insert(&m_unitInfoMap[unit->tag]);
	}

    ui.mission = job;
}

void UnitData::setBuildingWorker(const sc2::Unit* worker, Building & b)
{
	UnitInfo & ui = m_unitInfoMap[worker->tag];
	ui.mission = UnitMission::Build;
	setJob(worker, UnitMission::Build, b.type);
}

void UnitData::clearPreviousJob(const sc2::Unit* unit)
{
	// Remove the entry from the previous job, if there is one. 
	if (m_unitInfoMap[unit->tag].mission == UnitMission::Minerals)
	{
		// Remove one worker from the count of the depot this worker was assigned to.
		m_baseWorkerCount[m_workerDepotMap[unit->tag]->tag]--;
		m_workerDepotMap.erase(unit->tag);
	}
	else if (m_unitInfoMap[unit->tag].mission == UnitMission::Gas)
	{
		m_workerRefineryMap.erase(unit->tag);
	}

	m_combatUnits.erase(&m_unitInfoMap[unit->tag]);
	// No need to remove workers. m_workers keep track of scv's and mules.
	// Workers will always be workers. Only remove them from m_workers when they die. 
	// m_workers.erase(&m_unitInfoMap[unit->tag]);
}

std::set<const UnitInfo*> UnitData::getWorkers() const
{
	return m_workers;
}