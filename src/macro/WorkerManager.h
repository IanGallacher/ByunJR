#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitInfo.h"

class Building;
class ByunJRBot;

class WorkerManager
{
    ByunJRBot & m_bot;
    const sc2::Unit* m_previousClosestWorker;


    void assignIdleWorkers() const;
	void assignGasWorkers() const;
	void handleWorkers() const;
	sc2::Tag getMineralToMine(const sc2::Unit* unit) const;

	void setMineralWorker(const sc2::Unit* unit) const;

public:

    WorkerManager(ByunJRBot & bot);

    void        onFrame();

	void        setWorkerJob(const sc2::Tag & tag, UnitMission mission);
	void        setBuildingWorker(const sc2::Unit* worker, Building & b);

    bool        isWorkerScout(const sc2::Unit* worker) const;
    bool        isFree(const sc2::Unit* worker) const;
    bool        isBuilder(const sc2::Unit* worker) const;

    sc2::Tag    getClosestMineralWorkerTo(const sc2::Point2D & pos) const;
    sc2::Tag    getBuilder(Building & b,bool setJobAsBuilder = true) const;
    sc2::Tag    getGasWorker(const sc2::Unit* refinery) const;
};

