#include <sc2api/sc2_api.h>

#include "InformationManager.h"
#include "information/unitInfo.h"
#include "util/Util.h"

InformationManager::InformationManager(sc2::Agent & bot)
    : bot_(bot)
    // map_ must be be before bases_ in order to satisfy dependancies.
    , map_(bot, *this)
    , bases_(bot, map_)
    , unit_info_(bot)
{

}

void InformationManager::OnStart()
{
    map_.OnStart();
    unit_info_.OnStart();
    bases_.OnStart();

    // WARNING: Will only support two player games. 
    const auto player_id = bot_.Observation()->GetPlayerID();
    for (auto & player_info : bot_.Observation()->GetGameInfo().player_info)
    {
        if (player_info.player_id == player_id)
        {
            player_race_[sc2::Unit::Alliance::Self] = player_info.race_actual;
        }
        else
        {
            player_race_[sc2::Unit::Alliance::Enemy] = player_info.race_requested;
        }
    }
    dps_map_ = vvi{};
    dps_map_.clear();
    for (int y = 0; y < map_.TrueMapHeight(); ++y)
    {
        dps_map_.push_back(std::vector<int>());
        for (int x = 0; x < map_.TrueMapWidth(); ++x)
        {
            // There is an inherit "danger" for traveling through any square. 
            // Don't use 0, otherwise we won't find the "shortest and safest path"
            dps_map_[y].push_back(1);
        }
    }
}

void InformationManager::OnUnitCreated(const sc2::Unit* unit)
{
}

void InformationManager::OnUnitDestroyed(const sc2::Unit* unit)
{
    unit_info_.OnUnitDestroyed(unit);
}

void InformationManager::OnFrame()
{
    map_.OnFrame();
    unit_info_.OnFrame();
    bases_.OnFrame(*this);

    // Reset dps_map_
    for (const auto & unit : unit_info_.GetUnits(sc2::Unit::Alliance::Enemy))
    {
        for (int y = 0; y < dps_map_.size(); ++y)
        {
            for (int x = 0; x < dps_map_[y].size(); ++x)
            {
                dps_map_[y][x] = 1;
            }
        }
    }

    // Update dps_map_
    for(const auto & unit : unit_info_.GetUnits(sc2::Unit::Alliance::Enemy))
    {
        const int damage = Util::GetAttackDamage(unit->unit_type, bot_);
        if (damage == 0) continue;
        int range = Util::GetAttackRange(unit->unit_type, bot_);
        //  Melee units are dangerous too.
        if (range == 0 && !Util::IsBuilding(unit->unit_type)) range = 2;

        for (int y = 0; y < dps_map_.size(); ++y)
        {
            for (int x = 0; x < dps_map_[y].size(); ++x)
            {
                if( Util::DistSq(sc2::Point2D(x,y),unit->pos) <= (range*range) )
                {
                    dps_map_[y][x] += damage;
                }
            }
        }
    }

    for (int y = 0; y < map_.TrueMapHeight(); ++y)
    {
        for (int x = 0; x < map_.TrueMapWidth(); ++x)
        {
            if (!map_.IsWalkable(x, y))
                dps_map_[y][x] = 999;
        }
    }
}

const BaseLocationManager & InformationManager::Bases() const
{
    return bases_;
}

const MapTools & InformationManager::Map() const
{
    return map_;
}

UnitInfoManager & InformationManager::UnitInfo()
{
    return unit_info_;
}

const sc2::Race & InformationManager::GetPlayerRace(sc2::Unit::Alliance player) const
{
    assert(player < 1 || player_race_.size(), "invalid player for GetPlayerRace");
    return player_race_.at(player);
}

const ::UnitInfo * InformationManager::GetClosestUnitInfoWithJob(const sc2::Point2D reference_point, const UnitMission unit_mission) const
{
    const ::UnitInfo * closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto & unit_info_pair : unit_info_.GetUnitInfoMap(sc2::Unit::Alliance::Self))
    {
        const ::UnitInfo & unit_info = unit_info_pair.second;
        if (unit_info.mission == unit_mission)
        {
            const double distance = Util::DistSq(reference_point, unit_info.unit->pos);
            if (!closest_unit || distance < closest_distance)
            {
                closest_unit = &unit_info;
                closest_distance = distance;
            }
        }
    }

    return closest_unit;
}

const sc2::Unit * InformationManager::GetClosestUnitWithJob(const sc2::Point2D reference_point, const UnitMission unit_mission) const
{
    const sc2::Unit * closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto & unit_info_pair : unit_info_.GetUnitInfoMap(sc2::Unit::Alliance::Self))
    {
        const ::UnitInfo & unit_info = unit_info_pair.second;
        if (unit_info.mission == unit_mission)
        {
            const double distance = Util::DistSq(reference_point, unit_info.unit->pos);
            if (distance < closest_distance)
            {
                closest_unit = unit_info.unit;
                closest_distance = distance;
            }
        }
    }

    return closest_unit;
}

const UnitInfo* InformationManager::GetClosestUnitInfoWithJob(const sc2::Point2D point,
                                                          const std::vector<UnitMission> mission_vector) const
{
    const ::UnitInfo * closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto & unit_info_pair : unit_info_.GetUnitInfoMap(sc2::Unit::Alliance::Self))
    {
        if (std::find(mission_vector.begin(), mission_vector.end(), unit_info_pair.second.mission) != mission_vector.end())
        {
            const double distance = Util::DistSq(unit_info_pair.second.unit->pos, point);
            if (distance < closest_distance)
            {
                closest_unit = &unit_info_pair.second;
                closest_distance = distance;
            }
        }
    }

    return closest_unit;
}


const sc2::Unit* InformationManager::GetClosestUnitWithJob(const sc2::Point2D point,
    const std::vector<UnitMission> mission_vector) const
{
    const sc2::Unit * closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto & unit_info_pair : unit_info_.GetUnitInfoMap(sc2::Unit::Alliance::Self))
    {
        const ::UnitInfo & unit_info = unit_info_pair.second;
        // Buildings are part of the unit info map, but they do not have jobs. 
        if (!Util::IsWorkerType(unit_info.type)) continue;
        if (std::find(mission_vector.begin(), mission_vector.end(), unit_info.mission) != mission_vector.end())
        {
            const double distance = Util::DistSq(unit_info_pair.second.unit->pos, point);
            if (distance < closest_distance)
            {
                closest_unit = unit_info.unit;
                closest_distance = distance;
            }
        }
    }

    return closest_unit;
}

const sc2::Unit* InformationManager::GetClosestUnitOfType(const sc2::Unit* reference_unit,
    const sc2::UnitTypeID reference_type_id) const
{
    const sc2::Unit* closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto unit : unit_info_.GetUnits(sc2::Unit::Alliance::Self))
    {
        if (unit->unit_type == reference_type_id)
        {
            const double distance = Util::DistSq(unit->pos, reference_unit->pos);
            if (!closest_unit || distance < closest_distance)
            {
                closest_unit = unit;
                closest_distance = distance;
            }
        }
    }

    return closest_unit;
}

vvi InformationManager::GetDPSMap() const
{
    return dps_map_;
}
