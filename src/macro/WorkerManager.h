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
};

