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
    }

    if (Util::IsWorker(unit))
    {
        workers_.insert(&unit_info_map_[unit->tag]);
    }
}

void UnitData::KillUnit(const sc2::Unit* unit)
{
    //m_mineralsLost += unit->unit_type.mineralPrice();
    //_gasLost += unit->getType().gasPrice();
    num_units_[unit->unit_type]--;
    num_dead_units_[unit->unit_type]++;

    // If a refinery or a base dies, no need to set the coresponding refinery_worker_count_ or base_worker_count_ to 0.
    // Those stats get changed when we call ClearPreviousJob(unit);

    // If the killed unit was a worker, go ahead and update some stats.
    ClearPreviousJob(unit);
    workers_.erase(&unit_info_map_[unit->tag]);
    // Erasing the unit must be last. Cleanup the pointers before deleting the object. 
    unit_info_map_.erase(unit->tag); 
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

int UnitData::GetNumAssignedWorkers(const sc2::Unit* depot) const
{
    if (Util::IsTownHall(depot))
    {
        // If the base does not yet exist in base_worker_count_, it gets set to 0. 
        // Only check the map once to see if the value exists. 
        // We can't use a simple .at() or bracket notation in order to keep this function const.
        std::map<sc2::Tag, int>::const_iterator iter = base_worker_count_.find(depot->tag);
        if (iter != base_worker_count_.end())
        {
            return iter->second;
        }
    }
    else if (Util::IsRefinery(depot))
    {
        // If the refinery does not yet exist in refinery_worker_count, it gets set to 0. 
        std::map<sc2::Tag, int>::const_iterator iter = refinery_worker_count_.find(depot->tag);
        if (iter != refinery_worker_count_.end())
        {
            return iter->second;
        }
    }

    // If there are no workers currently assigned to the depot, return 0.
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

// mission_target is optional.
void UnitData::SetJob(const sc2::Unit* unit, const UnitMission job, const sc2::Unit* mission_target)
{
    ClearPreviousJob(unit);

    UnitInfo & ui = unit_info_map_[unit->tag];

    // Update the information about the current job. 
    if (job == UnitMission::Minerals)
    {
        // We don't need to print an error. If we have lost all our bases, there is no point in assigning workers to minerals. 
        // Simply check to see if we have any bases to prevent crashes before assigning workers to mine at the given base. 
        if (!mission_target) return;

        // If we haven't assigned anything to this depot yet, set its worker count to 0.
        if (base_worker_count_.find(mission_target->tag) == base_worker_count_.end())
        {
            base_worker_count_[mission_target->tag] = 0;
        }

        // Add the depot to our set of depots.
        depots_.insert(&unit_info_map_[unit->tag]);

        // Increase the worker count of this depot.
        base_worker_count_[mission_target->tag]++;
        worker_depot_map_[unit->tag] = unit;
    }
    else if (job == UnitMission::Gas)
    {
        if (!mission_target) // mission_target is the refinery to gather resources from. 
        {
            std::cout << "WARNING: Attempted assigning worker to refinery when there are no available refineries.";
            // Avoid repeating error messages by attempting to assign the worker we were given to minerals. 
            SetJob(unit, UnitMission::Minerals);
            return;
        }

        // If we haven't assigned any workers to this refinery yet set count to 0.
        if (refinery_worker_count_.find(mission_target->tag) == refinery_worker_count_.end())
        {
            refinery_worker_count_[mission_target->tag] = 0;
        }
        // Increase the count of workers assigned to this refinery.
        refinery_worker_count_[mission_target->tag] += 1;

        // Well, it looks like everything is alls et. Time to assign the worker to the refinery.
        worker_refinery_map_[unit->tag] = unit;
        ui.missionTarget = mission_target;
    }
    else if (job == UnitMission::Attack)
    {
        combat_units_.insert(&unit_info_map_[unit->tag]);
    }
    else if (job == UnitMission::Scout)
    {
        scout_units_.insert(&unit_info_map_[unit->tag]);
    }
    else if (job == UnitMission::Repair)
    {
        unit_repair_chart_[mission_target->tag]++;
        ui.missionTarget = mission_target;
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
        refinery_worker_count_[worker_refinery_map_[unit->tag]->tag]--;
        worker_refinery_map_.erase(unit->tag);
    }
    else if (unit_info_map_[unit->tag].mission == UnitMission::Repair)
        unit_repair_chart_[unit_info_map_[unit->tag].missionTarget->tag]--;

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

int UnitData::GetNumRepairWorkers(const sc2::Unit* unit) const
{
    return unit_repair_chart_[unit->tag];
}
