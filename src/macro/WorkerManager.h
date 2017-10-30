#pragma once
#include <sc2api/sc2_api.h>
#include "macro/Building.h"

class ByunJRBot;

class WorkerManager
{
    ByunJRBot & bot_;

    void AssignIdleWorkers() const;
    void AssignGasWorkers() const;
    void HandleWorkers() const;
    const sc2::Unit* GetMineralToMine(const sc2::Unit* unit) const;

public:
    WorkerManager(ByunJRBot & bot);

    void                OnFrame();

    bool                IsWorkerScout(const sc2::Unit* worker) const;
    bool                IsFree(const sc2::Unit* worker) const;
    bool                IsBuilder(const sc2::Unit* worker) const;

    const sc2::Unit*    GetClosestMineralWorkerTo(const sc2::Point2D & pos) const;
    const sc2::Unit*    GetBuilder(Building & b,bool set_job_as_builder = true) const;
    const sc2::Unit*    GetGasWorker(const sc2::Unit* refinery) const;
};

