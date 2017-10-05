#pragma once
#include <sc2api/sc2_api.h>

#include "information/WorkerData.h"

class Building;
class ByunJRBot;

class WorkerManager
{
    ByunJRBot & m_bot;

    mutable WorkerData  m_workerData;
    sc2::Tag     m_previousClosestWorker;

    void        setMineralWorker(const sc2::Unit & unit);
    
    void        handleIdleWorkers();
    void        handleGasWorkers();
    void        handleRepairWorkers();

public:

    WorkerManager(ByunJRBot & bot);

    void        onStart();
    void        onFrame();

    void        finishedWithWorker(const sc2::Tag & tag);
    void        drawResourceDebugInfo();
    void        drawWorkerInformation();
    void        setScoutWorker(const sc2::Tag & worker);
    void        setCombatWorker(const sc2::Tag & worker);
    void        setProxyWorker(const sc2::Tag & workerTag);
    void        setBuildingWorker(const sc2::Unit & worker, Building & b);
    void        setRepairWorker(const sc2::Unit & worker,const sc2::Unit & unitToRepair);
    void        stopRepairing(const sc2::Unit & worker);

    bool        isWorkerScout(const sc2::Unit & worker) const;
    bool        isFree(const sc2::Unit & worker) const;
    bool        isBuilder(const sc2::Unit & worker) const;

    sc2::Tag     getBuilder(Building & b,bool setJobAsBuilder = true) const;
    sc2::Tag     getClosestCC(const sc2::Unit & worker) const;
    sc2::Tag     getGasWorker(const sc2::Unit & refinery) const;
    sc2::Tag     getClosestMineralWorkerTo(const sc2::Point2D & pos) const;
};

