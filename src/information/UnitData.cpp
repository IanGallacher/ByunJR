#include "ByunJRBot.h"
#include "information/UnitData.h"
#include "util/Util.h"

UnitData::UnitData()
    : minerals_lost_(0)
    , gas_lost_(0)
{
    const int max_type_id = 1024;
    num_dead_units_       = std::vector<int>(max_type_id + 1, 0);
    num_units_            = std::vector<int>(max_type_id + 1, 0);
}

void UnitData::UpdateUnit(const sc2::Unit* unit)
{
    bool first_seen = false;
    const auto & it = unit_info_map_.find(unit->tag);
    if (it == unit_info_map_.end())
    {
        first_seen = true;
        unit_info_map_[unit->tag] = UnitInfo();
    }

    UnitInfo & ui   = unit_info_map_[unit->tag];
    ui.unit         = unit;
    ui.player       = Util::GetPlayer(unit);
    ui.lastPosition = unit->pos;
    ui.lastHealth   = unit->health;
    ui.lastShields  = unit->shield;
    ui.type         = unit->unit_type;
    ui.progress     = unit->build_progress;

    if (first_seen)
    {
        num_units_[ui.type]++;
        ui.mission = UnitMission::Idle;
    }

    if (Util::IsWorker(unit))
    {
        workers_.insert(&unit_info_map_[unit->tag]);
    }
    else if(Util::IsCombatUnit(unit))
    {
        combat_units_.insert(&unit_info_map_[unit->tag]);
    }
}

void UnitData::KillUnit(const sc2::Unit* unit)
{
    //m_mineralsLost += unit->unit_type.mineralPrice();
    //_gasLost += unit->getType().gasPrice();
    num_units_[unit->unit_type]--;
    num_dead_units_[unit->unit_type]++;

    // If the previous unit was a worker, go ahead and update some stats.
    ClearPreviousJob(unit);
    workers_.erase(&unit_info_map_[unit->tag]);
    // Erasing the unit must be last. Cleanup the pointers before deleting the object. 
    unit_info_map_.erase(unit->tag); 
}

void UnitData::RemoveBadUnits()
{
    for (auto iter = unit_info_map_.begin(); iter != unit_info_map_.end();)
    {
        if (BadUnitInfo(iter->second))
        {
            num_units_[iter->second.type]--;
            iter = unit_info_map_.erase(iter);
            combat_units_.erase(&iter->second);
            workers_.erase(&iter->second);
        }
        else
        {
            iter++;
        }
    }
}

bool UnitData::BadUnitInfo(const UnitInfo & ui) const
{
    return false;
}

size_t UnitData::GetNumWorkers() const
{
    return workers_.size();
}

int UnitData::GetGasLost() const
{
    return gas_lost_;
}

int UnitData::GetMineralsLost() const
{
    return minerals_lost_;
}

int UnitData::GetNumUnits(const sc2::UnitTypeID t) const
{
    return num_units_[t];
}

int UnitData::GetNumDeadUnits(const sc2::UnitTypeID t) const
{
    return num_dead_units_[t];
}

int UnitData::GetNumAssignedWorkers(const sc2::Unit* depot)
{
    if (Util::IsTownHall(depot))
    {
        const auto it = base_worker_count_.find(depot->tag);

        // if there is an entry, return it
        if (it != base_worker_count_.end())
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

const std::map<sc2::Tag, UnitInfo>& UnitData::GetUnitInfoMap() const
{
    return unit_info_map_;
}

std::set<const UnitInfo*> UnitData::GetCombatUnits() const
{
    return combat_units_;
}

// jobUnitTag is optional.
void UnitData::SetJob(const sc2::Unit* unit, const UnitMission job, ByunJRBot & bot)
{
    ClearPreviousJob(unit);

    UnitInfo & ui = unit_info_map_[unit->tag];

    // Update the information about the current job. 
    if (job == UnitMission::Minerals)
    {
        // Check if there is a mineral available to send the worker to.
        const sc2::Unit* base = bot.InformationManager().GetClosestBase(unit);

        // We don't need to print an error. If we have lost all our bases, there is no point in assigning workers to minerals. 
        // Simply check to see if we have any bases to prevent crashes before assigning workers to mine at the given base. 
        if (!base) return;

        // if we haven't assigned anything to this depot yet, set its worker count to 0
        if (base_worker_count_.find(base->tag) == base_worker_count_.end())
        {
            base_worker_count_[base->tag] = 0;
        }

        // add the depot to our set of depots
        depots_.insert(&unit_info_map_[unit->tag]);

        // increase the worker count of this depot
        base_worker_count_[base->tag]++;
        worker_depot_map_[unit->tag] = unit;
    }
    else if (job == UnitMission::Gas)
    {
        // The Gas worker we are assigning should already 
        const sc2::Unit* refinery = bot.InformationManager().GetClosestNotOptimalRefinery(unit);
        if (!refinery)
        {
            std::cout << "WARNING: Attempted assigning worker to refinery when there are no available refineries.";
            // Avoid repeating error messages by attempting to assign the worker we were given to minerals. 
            SetJob(unit, UnitMission::Minerals, bot);
            return;
        }

        // Well, it looks like everything is alls et. Time to assign the worker to the refinery.
        worker_refinery_map_[unit->tag] = unit;
        ui.workerDepot = refinery;
    }
    else if (job == UnitMission::Attack)
    {
        combat_units_.insert(&unit_info_map_[unit->tag]);
    }
    else if (job == UnitMission::Scout)
    {
        scout_units_.insert(&unit_info_map_[unit->tag]);
    }

    ui.mission = job;
}

void UnitData::ClearPreviousJob(const sc2::Unit* unit)
{
    // Remove the entry from the previous job, if there is one. 
    if (unit_info_map_[unit->tag].mission == UnitMission::Minerals)
    {
        // Remove one worker from the count of the depot this worker was assigned to.
        base_worker_count_[worker_depot_map_[unit->tag]->tag]--;
        worker_depot_map_.erase(unit->tag);
    }
    else if (unit_info_map_[unit->tag].mission == UnitMission::Gas)
    {
        worker_refinery_map_.erase(unit->tag);
    }

    scout_units_.erase(&unit_info_map_[unit->tag]);
    combat_units_.erase(&unit_info_map_[unit->tag]);
    // No need to remove workers. workers keep track of scv's and mules.
    // Workers will always be workers. Only remove them from workers when they die. 
    // workers.erase(&m_unitInfoMap[unit->tag]);
}

std::set<const UnitInfo*> UnitData::GetWorkers() const
{
    return workers_;
}

std::set<const UnitInfo*> UnitData::GetScouts() const
{
    return scout_units_;
}