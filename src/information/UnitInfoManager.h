#pragma once
#include <sc2api/sc2_api.h>

#include "macro/WorkerManager.h"
#include "information/UnitData.h"

class ByunJRBot;
class UnitInfoManager 
{
    ByunJRBot &              bot_;

    std::map<PlayerArrayIndex, UnitData> unit_data_;

    std::map<PlayerArrayIndex, std::vector<const sc2::Unit*>> units_;

    void                    UpdateUnit(const sc2::Unit* unit);
    void                    UpdateUnitInfo();
    bool                    IsValidUnit(const sc2::Unit* unit);
    
    const UnitData &        GetUnitData(PlayerArrayIndex player) const;

    void DrawSelectedUnitDebugInfo() const;

public:

    UnitInfoManager(ByunJRBot & bot);

    void                    OnStart();
    void                    OnFrame();
    void                    OnUnitDestroyed(const sc2::Unit* unit);

    const std::vector<const sc2::Unit*>& GetUnits(PlayerArrayIndex player) const;

    size_t                  GetUnitTypeCount(PlayerArrayIndex player, sc2::UnitTypeID type, bool completed = true) const;

    void                    GetNearbyForce(std::vector<UnitInfo>& unit_info, sc2::Point2D p, PlayerArrayIndex player, float radius) const;

    const std::map<sc2::Tag, UnitInfo>& GetUnitInfoMap(PlayerArrayIndex player) const;

    //bool                  enemyHasCloakedUnits() const;
    void                    DrawUnitInformation() const;
    int GetNumAssignedWorkers(const sc2::Unit* depot);
    void SetJob(const sc2::Unit* unit, const UnitMission job, const sc2::Unit* mission_target=nullptr);
    std::set<const UnitInfo*> GetWorkers();
    std::set<const UnitInfo*> GetScouts();
    const UnitInfo* GetUnitInfo(const sc2::Unit* unit);
    std::set<const UnitInfo*> GetCombatUnits() const;
    int GetNumRepairWorkers(const sc2::Unit* unit) const;
};