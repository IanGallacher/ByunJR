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
    previousClosestWorker = nullptr;
}

void WorkerManager::OnFrame()
{
    assignIdleWorkers();
    assignGasWorkers();
    handleWorkers();
}

void WorkerManager::assignIdleWorkers() const
{
    // for each of our workers
    for (auto & workerInfo : bot_.InformationManager().UnitInfo().GetWorkers())
    {
        const sc2::Unit* worker = workerInfo->unit;
        if (!worker) { continue; }

        // if it's a scout or creating a proxy building, don't handle it here
        if (bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self).at(workerInfo->unit->tag).mission == UnitMission::Scout
            || bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self).at(workerInfo->unit->tag).mission == UnitMission::Proxy)
        {
            continue;
        }

        // if it is idle
        if (Util::IsIdle(worker) || workerInfo->mission == UnitMission::Idle)
        {
        // Uncomment to worker rush. 
        //    bot_.InformationManager().assignUnit(workerInfo->unit->tag, UnitMission::Attack);
            setMineralWorker(workerInfo->unit);
        }
    }
}

void WorkerManager::assignGasWorkers() const
{
    // for each unit we have
    for (auto refinery : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        // if that unit is a refinery
        if (Util::IsRefinery(refinery) && Util::IsCompleted(refinery))
        {
            // get the number of workers currently assigned to it
            const int numAssigned = bot_.InformationManager().UnitInfo().GetNumAssignedWorkers(refinery);

            // if it's less than we want it to be, fill 'er up
            // As a side effect of the following hack ( read next paragraph of comments )
            // (3-numAssigned) has been changed to (2-numAssigned)
            // Otherwise there will be four workers on gas when playing terran. 
            for (int i=0; i<(2-numAssigned); ++i)
            {
                sc2::Tag gasWorker = getGasWorker(refinery);
                if (gasWorker)
                {
                    bot_.InformationManager().UnitInfo().SetJob(bot_.GetUnit(gasWorker), UnitMission::Gas, refinery->tag);
                    // Once a unit starts gathering resources, we don't send another command to gather resources. 
                    // If a unit is already gathering minerals, he won't start mining gas. 
                    // As a temporary work around, we send the gather command once they get assigned to a geyser. 
                    // A side effect of this system is the workers will never look for a more optimal mineral field to gather from. 
                    // TODO: Come up with a better system. 
                    bot_.Actions()->UnitCommand(bot_.GetUnit(gasWorker), sc2::ABILITY_ID::SMART, refinery);
                    Micro::SmartRightClick(bot_.GetUnit(gasWorker), refinery, bot_);
                }
            }
        }
    }
}

// Make the workers go do the things that they were assigned to do. 
void WorkerManager::handleWorkers() const
{
    for (auto & workerInfo : bot_.InformationManager().UnitInfo().GetWorkers())
    {
        const UnitMission job = workerInfo->mission;
        if (job == UnitMission::Minerals)
        {
            // find the mineral to mine and mine it
            const sc2::Unit* cc = bot_.InformationManager().GetClosestUnitOfType(workerInfo->unit, sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER);
            if (!cc) { return; }
            const sc2::Tag mineralToMine = getMineralToMine(cc);
            Micro::SmartRightClick(workerInfo->unit, bot_.GetUnit(mineralToMine), bot_);
        }
        else if (job == UnitMission::Gas)
        {
            // right click the refinery to start harvesting
            Micro::SmartRightClick(workerInfo->unit, bot_.GetUnit(workerInfo->workerDepotTag), bot_);
        }
    }
}

sc2::Tag WorkerManager::getMineralToMine(const sc2::Unit* unit) const
{
    sc2::Tag bestMineral = -1;
    double bestDist = std::numeric_limits<double>::max();;

    for (auto & mineral : bot_.Observation()->GetUnits())
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
    for (auto & workerInfo : bot_.InformationManager().UnitInfo().GetWorkers())
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
    const sc2::Unit* base = bot_.InformationManager().GetClosestBase(unit);
    if (!base) return; 

    const sc2::Tag baseTag = base->tag; 

    // if there is a valid mineral
    if (baseTag)
    {
        // update workerData with the new job
        bot_.InformationManager().UnitInfo().SetJob(unit, UnitMission::Minerals, baseTag);
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
    const sc2::Tag builderWorker = getClosestMineralWorkerTo(sc2::Point2D(b.finalPosition.x, b.finalPosition.y));

    // if the worker exists (one may not have been found in rare cases)
    if (builderWorker && setJobAsBuilder)
    {
        bot_.InformationManager().UnitInfo().SetJob(bot_.GetUnit(builderWorker), UnitMission::Build, b.type);
    }

    return builderWorker;
}

bool WorkerManager::isFree(const sc2::Unit* worker) const
{
    const UnitMission job = bot_.InformationManager().UnitInfo().GetUnitInfo(worker)->mission;
    return job == UnitMission::Minerals || job == UnitMission::Idle;
}

bool WorkerManager::isWorkerScout(const sc2::Unit* worker) const
{
    return (bot_.InformationManager().UnitInfo().GetUnitInfo(worker)->mission == UnitMission::Scout);
}

bool WorkerManager::isBuilder(const sc2::Unit* worker) const
{
    return (bot_.InformationManager().UnitInfo().GetUnitInfo(worker)->mission == UnitMission::Build);
}
