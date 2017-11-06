#pragma once
#include <sc2api/sc2_api.h>

#include "macro/WorkerManager.h"
#include "information/UnitData.h"

class ByunJRBot;
class UnitInfoManager 
{
    ByunJRBot &              bot_;

    std::map<sc2::Unit::Alliance, UnitData> unit_data_;

    std::map<sc2::Unit::Alliance, std::vector<const sc2::Unit*>> units_;

    void                    UpdateUnit(const sc2::Unit* unit);
    void                    UpdateUnitInfo();
    bool                    IsValidUnit(const sc2::Unit* unit);
    
    const UnitData &        GetUnitData(sc2::Unit::Alliance player) const;

    void DrawSelectedUnitDebugInfo() const;

public:

    UnitInfoManager(ByunJRBot & bot);

    void                    OnStart();
    void                    OnFrame();
    void                    OnUnitDestroyed(const sc2::Unit* unit);

    const std::vector<const sc2::Unit*>& GetUnits(sc2::Unit::Alliance player) const;

    size_t                  GetUnitTypeCount(sc2::Unit::Alliance player, sc2::UnitTypeID type, bool completed = true) const;

    void                    GetNearbyForce(std::vector<UnitInfo>& unit_info, sc2::Point2D p, sc2::Unit::Alliance player, float radius) const;

    const std::map<sc2::Tag, UnitInfo>& GetUnitInfoMap(sc2::Unit::Alliance player) const;

    //bool                  enemyHasCloakedUnits() const;
    void                    DrawUnitInformation() const;
    int GetNumAssignedWorkers(const sc2::Unit* depot);
    void SetJob(const sc2::Unit* unit, const UnitMission job, const sc2::Unit* mission_target=nullptr);
    std::set<const UnitInfo*> GetWorkers();
    std::set<const UnitInfo*> GetScouts();
    const UnitInfo* GetUnitInfo(const sc2::Unit* unit);
    std::set<const UnitInfo*> GetCombatUnits() const;
    int GetNumRepairWorkers(const sc2::Unit* unit) const;
    int GetNumDepots(sc2::Unit::Alliance self) const;
    size_t UnitsInProductionOfType(sc2::UnitTypeID unit_type) const;
};