#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitInfo.h"
#include "macro/Building.h"

typedef std::vector<UnitInfo> UnitInfoVector;

// The difference between UnitData and UnitInfo is that UnitData is specific to a player. UnitInfo is everything.
class UnitData
{
    std::map<sc2::Tag, UnitInfo>           m_unitInfoMap;

	// Pointers to all the workers to allow for faster iteration. 
	std::set<const UnitInfo*>              m_workers;
	// Pointers to all the combat units to allow for faster iteration.
	std::set<const UnitInfo*>              m_combatUnits;
	std::set<const UnitInfo*>              m_depots;
	// sc2::Tag is a player's base. int is the number of workers at that base. 
	std::map<sc2::Tag, int>                m_baseWorkerCount;
	// sc2::Tag is the refinery. const sc2::Unit* is the workers at that refinery.
	std::map<sc2::Tag, const sc2::Unit*>   m_workerRefineryMap;
	// sc2::Tag is the base. const sc2::Unit* is the workers at that base.
	std::map<sc2::Tag, const sc2::Unit*>   m_workerDepotMap;

	// These vectors are effectively maps. 
	// They have every UnitTypeID. 
    std::vector<int>                       m_numDeadUnits;
    std::vector<int>                       m_numUnits;
    int                                    m_mineralsLost;
    int                                    m_gasLost;

    bool badUnitInfo(const UnitInfo & ui) const;

public:

    UnitData();

    void updateUnit(const sc2::Unit* unit);
    void killUnit(const sc2::Unit* unit);
    void removeBadUnits();

    int getGasLost() const;
    int getMineralsLost() const;
    int getNumUnits(sc2::UnitTypeID t) const;
    int getNumDeadUnits(sc2::UnitTypeID t) const;
	int getNumAssignedWorkers(const sc2::Unit* depot);
	const std::map<sc2::Tag, UnitInfo>& getUnitInfoMap() const;
	std::set<const UnitInfo*> GetCombatUnits() const;

	// jobUnitTag is the tag that some jobs require. Minerals requires a base, gas requires a geyser, etc. 
    void setJob(const sc2::Unit* unit, const UnitMission job, const sc2::Tag jobUnitTag=0);
	void setBuildingWorker(const sc2::Unit* worker, Building& b);
	size_t getNumWorkers() const;
	void clearPreviousJob(const sc2::Unit* unit);
	std::set<const UnitInfo*> getWorkers() const;
};
