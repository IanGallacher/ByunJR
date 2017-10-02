#pragma once

class ByunJRBot;

namespace WorkerJobs
{
    enum { Minerals, Gas, Build, Combat, Idle, Repair, Move, Scout, Proxy, None, Num };
}

class WorkerData
{
    ByunJRBot & m_bot;

    std::set<sc2::Tag>           m_workers;
    std::set<sc2::Tag>           m_depots;
    std::map<int, int>          m_workerJobCount;
    std::map<sc2::Tag, int>      m_workerJobMap;
    std::map<sc2::Tag, int>      m_refineryWorkerCount;
    std::map<sc2::Tag, int>      m_depotWorkerCount;
    std::map<sc2::Tag, sc2::Tag>  m_workerRefineryMap;
    std::map<sc2::Tag, sc2::Tag>  m_workerDepotMap;

    void clearPreviousJob(const sc2::Tag & unit);

public:

    WorkerData(ByunJRBot & bot);

    void    workerDestroyed(const sc2::Tag & unit);
    void    updateAllWorkerData();
    void    updateWorker(const sc2::Tag & unit);
    void    setWorkerJob(const sc2::Tag & unit, int job, sc2::Tag jobUnitTag = 0);
    void    drawDepotDebugInfo();
    size_t  getNumWorkers() const;
    int     getWorkerJobCount(int job) const;
    int     getNumAssignedWorkers(const sc2::Tag & unit);
    int     getWorkerJob(const sc2::Tag & unit) const;
    sc2::Tag getMineralToMine(const sc2::Tag & unit) const;
    sc2::Tag getWorkerDepot(const sc2::Tag & unit) const;
    const char * getJobCode(const sc2::Tag & unit);
    const std::set<sc2::Tag> & getWorkers() const;
};