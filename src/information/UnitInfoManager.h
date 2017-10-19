#pragma once
#include <sc2api/sc2_api.h>

#include "macro/WorkerManager.h"
#include "information/UnitData.h"

class ByunJRBot;
class UnitInfoManager 
{
    ByunJRBot &              m_bot;

    std::map<PlayerArrayIndex, UnitData> m_unitData;

    std::map<PlayerArrayIndex, std::vector<const sc2::Unit*>> m_units;

    void                    updateUnit(const sc2::Unit* unit);
    void                    updateUnitInfo();
    bool                    isValidUnit(const sc2::Unit* unit);
    
    const UnitData &        getUnitData(PlayerArrayIndex player) const;

	void drawSelectedUnitDebugInfo() const;

public:

    UnitInfoManager(ByunJRBot & bot);

    void                    onStart();
    void                    onFrame();
    void                    onUnitDestroyed(const sc2::Unit* unit);

	const std::vector<const sc2::Unit*>& getUnits(PlayerArrayIndex player) const;

    size_t                  getUnitTypeCount(PlayerArrayIndex player, sc2::UnitTypeID type, bool completed = true) const;

    void                    getNearbyForce(std::vector<UnitInfo>& unitInfo, sc2::Point2D p, PlayerArrayIndex player, float radius) const;

	const std::map<sc2::Tag, UnitInfo>& getUnitInfoMap(PlayerArrayIndex player) const;

    //bool                  enemyHasCloakedUnits() const;
    void                    drawUnitInformation(float x, float y) const;
	int getNumAssignedWorkers(const sc2::Unit* depot);
	void setJob(const sc2::Unit* unit, const UnitMission job, const sc2::Tag jobUnitTag=0);
	void setBuildingWorker(const sc2::Unit* worker, Building& b);
	std::set<const UnitInfo*> getWorkers();
	const UnitInfo* getUnitInfo(const sc2::Unit* tag);
	std::set<const UnitInfo*> getCombatUnits() const;
};