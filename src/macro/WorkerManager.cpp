#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "macro/Building.h"
#include "macro/WorkerManager.h"
#include "util/Util.h"
#include "common/BotAssert.h"

WorkerManager::WorkerManager(ByunJRBot & bot)
    : bot_ (bot)
{

}

void WorkerManager::OnFrame()
{
    // If all our units are dead, calling GetClosestUnitInfoWithJob(refinery->pos, UnitMission::Minerals)->unit will cause a crash.
    // Don't bother doing anything if all our workers are dead.
    if (bot_.InformationManager().UnitInfo().GetWorkers().size() == 0)
        return;

    AssignIdleWorkers();
    AssignGasWorkers();
    HandleWorkers();
}

void WorkerManager::AssignIdleWorkers() const
{
    // for each of our workers
    for (auto & worker_info : bot_.InformationManager().UnitInfo().GetWorkers())
    {
        const sc2::Unit* worker = worker_info->unit;
        if (!worker) { continue; }

        // if it's a scout or creating a proxy building, don't handle it here
        if (bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self).at(worker_info->unit->tag).mission == UnitMission::Scout
            || bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self).at(worker_info->unit->tag).mission == UnitMission::Proxy)
        {
            continue;
        }

        // if it is idle
        if (Util::IsIdle(worker) || worker_info->mission == UnitMission::Idle)
        {
        // Uncomment to worker rush. 
        //    bot_.InformationManager().assignUnit(workerInfo->unit->tag, UnitMission::Attack);
            bot_.InformationManager().UnitInfo().SetJob(worker_info->unit, UnitMission::Minerals);
        }
    }
}

void WorkerManager::AssignGasWorkers() const
{
    // for each unit we have
    for (auto refinery : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        // if that unit is a refinery
        if (Util::IsRefinery(refinery) && Util::IsCompleted(refinery))
        {
            // get the number of workers currently assigned to it
            const int num_assigned = bot_.InformationManager().UnitInfo().GetNumAssignedWorkers(refinery);

            // if it's less than we want it to be, fill 'er up
            for (int i=0; i<(refinery->ideal_harvesters - num_assigned); ++i)
            {
                // Find the closest mining scv to the refinery.
                const sc2::Unit* gas_worker = bot_.InformationManager().GetClosestUnitWithJob(refinery->pos, UnitMission::Minerals);
                if (!gas_worker) continue;
                bot_.InformationManager().UnitInfo().SetJob(gas_worker, UnitMission::Gas);
                // Once a unit starts gathering resources, we don't send another command to gather resources. 
                // If a unit is already gathering minerals, he won't start mining gas. 
                // As a temporary work around, we send the gather command once they get assigned to a geyser. 
                // A side effect of this system is the workers will never look for a more optimal mineral field to gather from. 
                // TODO: Come up with a better system. 
                bot_.Actions()->UnitCommand(gas_worker, sc2::ABILITY_ID::SMART, refinery);
                Micro::SmartRightClick(gas_worker, refinery, bot_);
            }
        }
    }
}

// Make the workers go do the things that they were assigned to do. 
void WorkerManager::HandleWorkers() const
{
    for (auto & worker_info : bot_.InformationManager().UnitInfo().GetWorkers())
    {
        const UnitMission job = worker_info->mission;
        if (job == UnitMission::Minerals)
        {
            // find the mineral to mine and mine it
            const sc2::Unit* cc = bot_.InformationManager().GetClosestUnitOfType(worker_info->unit, sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER);
            if (!cc) { return; }
            const sc2::Unit* mineral_to_mine = GetMineralToMine(cc);
            Micro::SmartRightClick(worker_info->unit, mineral_to_mine, bot_);
        }
        else if (job == UnitMission::Gas)
        {
            // right click the refinery to start harvesting
            Micro::SmartRightClick(worker_info->unit, worker_info->missionTarget, bot_);

            // If the geyser is out of gas, the worker is free to do other things. 
            if(worker_info->missionTarget->vespene_contents == 0)
            {
                bot_.InformationManager().UnitInfo().SetJob(worker_info->unit, UnitMission::Minerals);
            }
        }
        else if (job == UnitMission::Repair)
        {
            // right click the refinery to start harvesting
            Micro::SmartRepair(worker_info->unit, worker_info->missionTarget, bot_);

            // If we are done repairing, time to send the worker back to work. 
            if(worker_info->missionTarget->health == worker_info->missionTarget->health_max)
            {
                bot_.InformationManager().UnitInfo().SetJob(worker_info->unit, UnitMission::Minerals);
            }
        }
    }
}

const sc2::Unit* WorkerManager::GetMineralToMine(const sc2::Unit* unit) const
{
    const sc2::Unit* best_mineral = nullptr;
    double best_dist = std::numeric_limits<double>::max();;

    for (auto & mineral : bot_.Observation()->GetUnits())
    {
        if (!Util::IsMineral(mineral)) continue;

        const double dist = Util::Dist(mineral->pos, unit->pos);

        if (dist < best_dist)
        {
            best_mineral = mineral;
            best_dist = dist;
        }
    }

    return best_mineral;
}

// Unlike the GetCosestUnit functions inside InformationManager, this only iterates through workers.
const sc2::Unit* WorkerManager::GetClosestMineralWorkerTo(const sc2::Point2D & pos) const
{
    const sc2::Unit* closest_mineral_worker = nullptr;
    double closest_dist = std::numeric_limits<double>::max();

    // for each of our workers
    for (auto & worker_info : bot_.InformationManager().UnitInfo().GetWorkers())
    {
        if (!worker_info) { std::cout << "Warning: a workerInfo pointer is invalid." << std::endl; continue; }

        // if it is a mineral worker
        if (worker_info->mission == UnitMission::Minerals
        ||  worker_info->mission == UnitMission::Proxy)
        {
            const double dist = Util::DistSq(worker_info->unit->pos, pos);

            if (!closest_mineral_worker || dist < closest_dist)
            {
                closest_mineral_worker = worker_info->unit;
                closest_dist = dist;
            }
        }
    }

    return closest_mineral_worker;
}

const sc2::Unit* WorkerManager::GetGasWorker(const sc2::Unit* refinery) const
{
    return GetClosestMineralWorkerTo(refinery->pos);
}

// gets a builder for BuildingManager to use
// if set_job_as_builder is true (default), it will be flagged as a builder unit
// set 'set_job_as_builder' to false if we just want to see which worker will build a building
const sc2::Unit* WorkerManager::GetBuilder(Building & b, const bool set_job_as_builder) const
{
    const sc2::Unit* builder_worker = GetClosestMineralWorkerTo(sc2::Point2D(b.finalPosition.x, b.finalPosition.y));

    // if the worker exists (one may not have been found in rare cases)
    if (builder_worker && set_job_as_builder)
    {
        bot_.InformationManager().UnitInfo().SetJob(builder_worker, UnitMission::Build);
    }

    return builder_worker;
}

bool WorkerManager::IsFree(const sc2::Unit* worker) const
{
    const UnitMission job = bot_.InformationManager().UnitInfo().GetUnitInfo(worker)->mission;
    return job == UnitMission::Minerals || job == UnitMission::Idle;
}

bool WorkerManager::IsWorkerScout(const sc2::Unit* worker) const
{
    return (bot_.InformationManager().UnitInfo().GetUnitInfo(worker)->mission == UnitMission::Scout);
}

bool WorkerManager::IsBuilder(const sc2::Unit* worker) const
{
    return (bot_.InformationManager().UnitInfo().GetUnitInfo(worker)->mission == UnitMission::Build);
}
