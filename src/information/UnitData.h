#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitInfo.h"
#include "macro/Building.h"

typedef std::vector<UnitInfo> UnitInfoVector;

// The difference between UnitData and UnitInfo is that UnitData is specific to a player. UnitInfo is everything.
class UnitData
{
    std::map<sc2::Tag, UnitInfo>           unit_info_map_;

    // Pointers to all the workers to allow for faster iteration. 
    std::set<const UnitInfo*>              workers_;
    // Pointers to all the combat units to allow for faster iteration.
    std::set<const UnitInfo*>              combat_units_;
    // Pointers to all the scouting units to allow for faster iteration.
    std::set<const UnitInfo*>              scout_units_;
    std::set<const UnitInfo*>              depots_;
    // sc2::Tag is a player's base. int is the number of workers at that base. 
    std::map<sc2::Tag, int>                base_worker_count_;
    // sc2::Tag is a player's base. int is the number of workers at that refinery. 
    std::map<sc2::Tag, int>                refinery_worker_count_;
    // sc2::Tag is the refinery. const sc2::Unit* is the workers at that refinery.
    std::map<sc2::Tag, const sc2::Unit*>   worker_refinery_map_;
    // sc2::Tag is the base. const sc2::Unit* is the workers at that base.
    std::map<sc2::Tag, const sc2::Unit*>   worker_depot_map_;
    // Keep track of how many units are currenlty repairing a given unit.
    // Marked as mutable. If we are checking if an element in the map exists, create one if it does not. 
    mutable std::map<sc2::Tag, int >       unit_repair_chart_;

    // These vectors are effectively maps. 
    // They have every UnitTypeID. 
    std::vector<int>                       num_dead_units_;
    std::vector<int>                       num_units_;
    int                                    minerals_lost_;
    int                                    gas_lost_;

    bool BadUnitInfo(const UnitInfo & ui) const;

public:

    UnitData();

    void UpdateUnit(const sc2::Unit* unit);
    void KillUnit(const sc2::Unit* unit);
    void RemoveBadUnits();

    int GetGasLost() const;
    int GetMineralsLost() const;
    int GetNumUnits(sc2::UnitTypeID t) const;
    int GetNumDeadUnits(sc2::UnitTypeID t) const;
    int GetNumAssignedWorkers(const sc2::Unit* depot) const;
    const std::map<sc2::Tag, UnitInfo>& GetUnitInfoMap() const;
    std::set<const UnitInfo*> GetCombatUnits() const;

    // mission_target is the additional information that some jobs require. Minerals requires a base, gas requires a geyser, etc. 
    void SetJob(const sc2::Unit* unit, const UnitMission job, const sc2::Unit* mission_target=nullptr);
    size_t GetNumWorkers() const;
    void ClearPreviousJob(const sc2::Unit* unit);
    std::set<const UnitInfo*> GetWorkers() const;
    std::set<const UnitInfo*> GetScouts() const;
    int GetNumRepairWorkers(const sc2::Unit* unit) const;
};
