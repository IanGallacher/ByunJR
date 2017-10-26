#include <sc2api/sc2_api.h>

#include "ByunJRBot.h"
#include "InformationManager.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "information/unitInfo.h"
#include "util/Util.h"

InformationManager::InformationManager(ByunJRBot & bot)
    : bot_(bot)
    , building_placer_(bot)
    , unit_info_(bot)
{

}

void InformationManager::OnStart()
{
    building_placer_.OnStart();
    unit_info_.OnStart();

    // get my race
    const auto player_id = bot_.Observation()->GetPlayerID();
    for (auto & player_info : bot_.Observation()->GetGameInfo().player_info)
    {
        if (player_info.player_id == player_id)
        {
            player_race_[static_cast<int>(PlayerArrayIndex::Self)] = player_info.race_actual;
        }
        else
        {
            player_race_[static_cast<int>(PlayerArrayIndex::Enemy)] = player_info.race_requested;
        }
    }

    for (int y = 0; y < bot_.Map().TrueMapHeight(); ++y)
    {
        dps_map_.push_back(std::vector<int>());
        for (int x = 0; x < bot_.Map().TrueMapWidth(); ++x)
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
    unit_info_.OnFrame();

    // Reset dps_map_
    for (const auto & unit : unit_info_.GetUnits(PlayerArrayIndex::Enemy))
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
    for(const auto & unit : unit_info_.GetUnits(PlayerArrayIndex::Enemy))
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

    for (int y = 0; y < bot_.Map().TrueMapHeight(); ++y)
    {
        for (int x = 0; x < bot_.Map().TrueMapWidth(); ++x)
        {
            if (!bot_.Map().IsWalkable(x, y))
                dps_map_[y][x] = 999;
        }
    }
}

BuildingPlacer & InformationManager::BuildingPlacer()
{
    return building_placer_;
}

UnitInfoManager & InformationManager::UnitInfo()
{
    return unit_info_;
}

sc2::Point2DI InformationManager::GetProxyLocation() const
{
    return bot_.GetProxyManager().GetProxyLocation();
}

// TODO: Figure out my race
const sc2::Race & InformationManager::GetPlayerRace(PlayerArrayIndex player) const
{
    BOT_ASSERT(player == PlayerArrayIndex::Self || player == PlayerArrayIndex::Enemy, "invalid player for GetPlayerRace");
    return player_race_[static_cast<int>(player)];
}

const sc2::Unit* InformationManager::GetBuilder(Building& b, const bool set_job_as_builder)
{
    const std::vector<UnitMission> acceptable_missions{ UnitMission::Minerals, UnitMission::Proxy };
    const sc2::Unit* builder_worker = GetClosestUnitInfoWithJob(sc2::Point2D(b.finalPosition.x, b.finalPosition.y), acceptable_missions)->unit;

    // if the worker exists (one may not have been found in rare cases)
    if (builder_worker && set_job_as_builder)
    {
        unit_info_.SetJob(builder_worker, UnitMission::Build);
    }

    return builder_worker;
}

// Does not look for flying bases. Only landed bases. 
const sc2::Unit* InformationManager::GetClosestBase(const sc2::Unit* reference_unit) const
{
    const sc2::Unit* closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER
        ||  unit->unit_type == sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND
        ||  unit->unit_type == sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS
        ||  unit->unit_type == sc2::UNIT_TYPEID::PROTOSS_NEXUS
        ||  unit->unit_type == sc2::UNIT_TYPEID::ZERG_HATCHERY
        ||  unit->unit_type == sc2::UNIT_TYPEID::ZERG_LAIR
        ||  unit->unit_type == sc2::UNIT_TYPEID::ZERG_HIVE)
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

const ::UnitInfo * InformationManager::GetClosestUnitInfoWithJob(const sc2::Point2D reference_point, const UnitMission unit_mission) const
{
    const ::UnitInfo * closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto & unit_info_pair : bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self))
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

const UnitInfo* InformationManager::GetClosestUnitInfoWithJob(const sc2::Point2D point,
                                                          const std::vector<UnitMission> mission_vector) const
{
    const ::UnitInfo * closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto & unit_info_pair : bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self))
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

const sc2::Unit* InformationManager::GetClosestUnitOfType(const sc2::Unit* reference_unit,
    const sc2::UnitTypeID reference_type_id) const
{
    const sc2::Unit* closest_unit = nullptr;
    double closest_distance = std::numeric_limits<double>::max();

    for (auto unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
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

const sc2::Unit* InformationManager::GetClosestNotOptimalRefinery(const sc2::Unit* reference_unit) const
{
    const sc2::Unit* closest_refinery = nullptr;
    double closest_distance = std::numeric_limits<double>::max();
    // Find all our refineries. If they not full, fill em up.
    for (auto refinery : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        if (Util::IsRefinery(refinery) && Util::IsCompleted(refinery))
        {
            const int num_assigned = bot_.InformationManager().UnitInfo().GetNumAssignedWorkers(refinery);

            if (0 < (refinery->ideal_harvesters - num_assigned))
            {
                const double distance = Util::DistSq(refinery->pos, reference_unit->pos);
                if (!closest_refinery || distance < closest_distance)
                {
                    closest_refinery = refinery;
                    closest_distance = distance;
                }
            }
        }
    }

    return closest_refinery;
}
vvi InformationManager::GetDPSMap() const
{
    return dps_map_;
}
