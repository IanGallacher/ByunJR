#include <iostream>
#include <sc2api/sc2_api.h>
#include <sc2utils/sc2_manage_process.h>

#include "ByunJRBot.h"
#include "TechLab/util/Util.h"

#include "common/BotAssert.h"

Util::IsUnit::IsUnit(sc2::UNIT_TYPEID type) 
    : type(type) 
{
}

bool Util::IsTownHallType(const sc2::UnitTypeID & type)
{
    switch (type.ToType()) 
    {
        case sc2::UNIT_TYPEID::ZERG_HATCHERY                : return true;
        case sc2::UNIT_TYPEID::ZERG_LAIR                    : return true;
        case sc2::UNIT_TYPEID::ZERG_HIVE                    : return true;
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER         : return true;
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND        : return true;
        // There is no point in treating flying buildings like the building type they are supposed to be. 
        // You can't train units from a flying building. 
        // case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING  : return true;
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS     : return true;
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS                : return true;
        default: return false;
    }
}

bool Util::IsTownHall(const sc2::Unit* unit)
{
    return IsTownHallType(unit->unit_type);
}

bool Util::IsRefinery(const sc2::Unit* unit)
{
    return IsRefineryType(unit->unit_type);
}

bool Util::IsRefineryType(const sc2::UnitTypeID & type)
{
    switch (type.ToType()) 
    {
        case sc2::UNIT_TYPEID::TERRAN_REFINERY      : return true;
        case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR  : return true;
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR       : return true;
        default: return false;
    }
}

bool Util::IsGeyser(const sc2::Unit* unit)
{
    switch (unit->unit_type.ToType()) 
    {
        case sc2::UNIT_TYPEID::NEUTRAL_VESPENEGEYSER        : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER  : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER: return true;
        default: return false;
    }
}

bool Util::IsMineral(const sc2::Unit* unit)
{
    switch (unit->unit_type.ToType()) 
    {
        case sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD         : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD750      : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD     : return true;
        case sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_LABMINERALFIELD: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD: return true;
        case sc2::UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750: return true;
        default: return false;
    }
}

bool Util::IsWorker(const sc2::Unit* unit)
{
    return IsWorkerType(unit->unit_type);
}

bool Util::IsWorkerType(const sc2::UnitTypeID & unit)
{
    switch (unit.ToType()) 
    {
        case sc2::UNIT_TYPEID::TERRAN_SCV           : return true;
        case sc2::UNIT_TYPEID::PROTOSS_PROBE        : return true;
        case sc2::UNIT_TYPEID::ZERG_DRONE           : return true;
        case sc2::UNIT_TYPEID::ZERG_DRONEBURROWED   : return true;
        default: return false;
    }
}

sc2::UnitTypeID Util::GetSupplyProvider(const sc2::Race & race)
{
    switch (race) 
    {
        case sc2::Race::Terran: return sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
        case sc2::Race::Protoss: return sc2::UNIT_TYPEID::PROTOSS_PYLON;
        case sc2::Race::Zerg: return sc2::UNIT_TYPEID::ZERG_OVERLORD;
        default: return 0;
    }
}

sc2::UnitTypeID Util::GetTownHall(const sc2::Race & race)
{
    switch (race) 
    {
        case sc2::Race::Terran: return sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER;
        case sc2::Race::Protoss: return sc2::UNIT_TYPEID::PROTOSS_NEXUS;
        case sc2::Race::Zerg: return sc2::UNIT_TYPEID::ZERG_HATCHERY;
        default: return 0;
    }
}

bool Util::IsCompleted(const sc2::Unit* unit)
{
    return unit->build_progress == 1.0f;
}

bool Util::IsIdle(const sc2::Unit* unit)
{
    return unit->orders.empty();
}

int Util::GetUnitTypeMineralPrice(const sc2::UnitTypeID type, const sc2::Agent & bot)
{
    return bot.Observation()->GetUnitTypeData()[type].mineral_cost;
}

int Util::GetUnitTypeGasPrice(const sc2::UnitTypeID type, const sc2::Agent & bot)
{
    return bot.Observation()->GetUnitTypeData()[type].vespene_cost;
}

int Util::GetUnitTypeWidth(const sc2::UnitTypeID type, const sc2::Agent & bot)
{
    return static_cast<int>(2 * bot.Observation()->GetAbilityData()[UnitTypeIDToAbilityID(type)].footprint_radius);
}

int Util::GetUnitTypeHeight(const sc2::UnitTypeID type, const sc2::Agent & bot)
{
    return static_cast<int>(2 * bot.Observation()->GetAbilityData()[UnitTypeIDToAbilityID(type)].footprint_radius);
}


sc2::Point2D Util::CalcCenterOfUnitGroup(const std::vector<const sc2::Unit*>& units)
{
    if (units.empty())
    {
        return sc2::Point2D(0.0f,0.0f);
    }

    float cx = 0.0f;
    float cy = 0.0f;

    for (auto & unit : units)
    {
        cx += unit->pos.x;
        cy += unit->pos.y;
    }

    return sc2::Point2D(cx / units.size(), cy / units.size());
}

bool Util::IsDetector(const sc2::Unit* unit)
{
    return IsDetectorType(unit->unit_type);
}

float Util::GetAttackRange(const sc2::UnitTypeID & type, sc2::Agent & bot)
{
    auto & weapons = bot.Observation()->GetUnitTypeData()[type].weapons;
    
    if (weapons.empty())
    {
        return 0.0f;
    }

    float max_range = 0.0f;
    for (auto & weapon : weapons)
    {
        if (weapon.range > max_range)
        {
            max_range = weapon.range;
        }
    }

    return max_range;
}

float Util::GetAttackDamage(const sc2::UnitTypeID & type, sc2::Agent & bot)
{
    auto & weapons = bot.Observation()->GetUnitTypeData()[type].weapons;

    if (weapons.empty())
    {
        return 0.0f;
    }

    float max_damage = 0.0f;
    for (auto & weapon : weapons)
    {
        max_damage = weapon.damage_ * weapon.attacks;
    }

    return max_damage;
}

bool Util::IsDetectorType(const sc2::UnitTypeID & type)
{
    switch (type.ToType())
    {
    case sc2::UNIT_TYPEID::PROTOSS_OBSERVER: return true;
    case sc2::UNIT_TYPEID::TERRAN_RAVEN: return true;
    case sc2::UNIT_TYPEID::ZERG_OVERSEER: return true;
    case sc2::UNIT_TYPEID::TERRAN_MISSILETURRET: return true;
    case sc2::UNIT_TYPEID::ZERG_SPORECRAWLER: return true;
    case sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON: return true;
    default: return false;
    }
}

sc2::Unit::Alliance Util::GetPlayer(const sc2::Unit* unit)
{
    if (!unit) { return sc2::Unit::Alliance(0); }
    if (unit->alliance == sc2::Unit::Alliance::Self)
    {
        return sc2::Unit::Alliance::Self;
    }

    else if (unit->alliance == sc2::Unit::Alliance::Enemy)
    {
        return sc2::Unit::Alliance::Enemy;
    }

    else if (unit->alliance == sc2::Unit::Alliance::Neutral)
    {
        return sc2::Unit::Alliance::Neutral;
    }

    return sc2::Unit::Alliance(0);
}

bool Util::IsCombatUnitType(const sc2::UnitTypeID type)
{
    if (IsWorkerType(type)) { return false; }
    if (IsSupplyProviderType(type)) { return false; }
    if (IsBuilding(type)) { return false; }

    if (type == sc2::UNIT_TYPEID::ZERG_EGG) { return false; }
    if (type == sc2::UNIT_TYPEID::ZERG_LARVA) { return false; }

    return true;
}

bool Util::IsCombatUnit(const sc2::Unit* unit)
{
    return IsCombatUnitType(unit->unit_type);
}

bool Util::IsSupplyProviderType(const sc2::UnitTypeID type)
{
    switch (type.ToType()) 
    {
        case sc2::UNIT_TYPEID::ZERG_OVERLORD                : return true;
        case sc2::UNIT_TYPEID::PROTOSS_PYLON                : return true;
        case sc2::UNIT_TYPEID::PROTOSS_PYLONOVERCHARGED     : return true;
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT           : return true;
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED    : return true;
        default: return false;
    }
}

bool Util::IsSupplyProvider(const sc2::Unit* unit)
{
    return IsSupplyProviderType(unit->unit_type);
}

float Util::Dist(const int x1, const int y1, const int x2, const int y2)
{
    const float dx = x1 - x2;
    const float dy = y1 - y2;
    return dx*dx + dy*dy;
}

float Util::Dist(const sc2::Point2D & p1, const sc2::Point2D & p2)
{
    return sqrtf(Util::DistSq(p1,p2));
}

float Util::Dist(const sc2::Point2DI & p1, const sc2::Point2D & p2)
{
    return sqrtf(Util::DistSq(p1, p2));
}

float Util::DistSq(const sc2::Point2D & p1, const sc2::Point2D & p2)
{
    const float dx = p1.x - p2.x;
    const float dy = p1.y - p2.y;
    return dx*dx + dy*dy;
}

float Util::DistSq(const sc2::Point2DI & p1, const sc2::Point2D & p2)
{
    const float dx = p1.x - p2.x;
    const float dy = p1.y - p2.y;

    return dx*dx + dy*dy;
}

bool Util::Pathable(const sc2::GameInfo & info, const sc2::Point2D & point) 
{
    const sc2::Point2DI point_i(point.x, point.y);
    if (point_i.x < 0 || point_i.x >= info.width || point_i.y < 0 || point_i.y >= info.width)
    {
        return false;
    }

    assert(info.pathing_grid.data.size() == info.width * info.height);
    const unsigned char encoded_placement = info.pathing_grid.data[point_i.x + ((info.height - 1) - point_i.y) * info.width];
    const bool decoded_placement = encoded_placement == 255 ? false : true;
    return decoded_placement;
}

bool Util::Placement(const sc2::GameInfo & info, const sc2::Point2D & point) 
{
    const sc2::Point2DI point_i(static_cast<int>(point.x), static_cast<int>(point.y));
    if (point_i.x < 0 || point_i.x >= info.width || point_i.y < 0 || point_i.y >= info.width)
    {
        return false;
    }

    assert(info.placement_grid.data.size() == info.width * info.height);
    const unsigned char encoded_placement = info.placement_grid.data[point_i.x + ((info.height - 1) - point_i.y) * info.width];
    const bool decoded_placement = encoded_placement == 255 ? true : false;
    return decoded_placement;
}

std::string Util::GetStringFromRace(const sc2::Race & race)
{
    switch ( race )
    {
        case sc2::Race::Terran: return "Terran";
        case sc2::Race::Protoss: return "Protoss";
        case sc2::Race::Zerg: return "Zerg";
        default: return "Random";
    }
}

sc2::Race Util::GetRaceFromString(const std::string & race_in)
{
    std::string race(race_in);
    std::transform(race.begin(), race.end(), race.begin(), ::tolower);

    if (race == "terran")
    {
        return sc2::Race::Terran;
    }
    else if (race == "protoss")
    {
        return sc2::Race::Protoss;
    }
    else if (race == "zerg")
    {
        return sc2::Race::Zerg;
    }
    else if (race == "random")
    {
        return sc2::Race::Random;
    }

    BOT_ASSERT(false, "Unknown Race: ", race.c_str());
    return sc2::Race::Terran;
}

sc2::UnitTypeID Util::WhatBuilds(const sc2::UnitTypeID & type)
{
    switch (type.ToType()) 
    {
        case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR:         return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_CYBERNETICSCORE:     return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_DARKSHRINE:          return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_PYLON:               return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS:               return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_FLEETBEACON:         return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_FORGE:               return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_GATEWAY:             return sc2::UNIT_TYPEID::PROTOSS_PROBE;  
        case sc2::UNIT_TYPEID::PROTOSS_STARGATE:            return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON:        return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSBAY:         return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY:    return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE:      return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL:     return sc2::UNIT_TYPEID::PROTOSS_PROBE; 
        case sc2::UNIT_TYPEID::PROTOSS_PROBE:               return sc2::UNIT_TYPEID::PROTOSS_NEXUS; 
        case sc2::UNIT_TYPEID::PROTOSS_MOTHERSHIPCORE:      return sc2::UNIT_TYPEID::PROTOSS_NEXUS; 
        case sc2::UNIT_TYPEID::PROTOSS_ZEALOT:              return sc2::UNIT_TYPEID::PROTOSS_GATEWAY; 
        case sc2::UNIT_TYPEID::PROTOSS_SENTRY:              return sc2::UNIT_TYPEID::PROTOSS_GATEWAY;  
        case sc2::UNIT_TYPEID::PROTOSS_STALKER:             return sc2::UNIT_TYPEID::PROTOSS_GATEWAY; 
        case sc2::UNIT_TYPEID::PROTOSS_HIGHTEMPLAR:         return sc2::UNIT_TYPEID::PROTOSS_GATEWAY; 
        case sc2::UNIT_TYPEID::PROTOSS_DARKTEMPLAR:         return sc2::UNIT_TYPEID::PROTOSS_GATEWAY; 
        case sc2::UNIT_TYPEID::PROTOSS_ADEPT:               return sc2::UNIT_TYPEID::PROTOSS_GATEWAY; 
        case sc2::UNIT_TYPEID::PROTOSS_COLOSSUS:            return sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY; 
        case sc2::UNIT_TYPEID::PROTOSS_DISRUPTOR:           return sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY; 
        case sc2::UNIT_TYPEID::PROTOSS_WARPPRISM:           return sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY; 
        case sc2::UNIT_TYPEID::PROTOSS_OBSERVER:            return sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY;
        case sc2::UNIT_TYPEID::PROTOSS_IMMORTAL:            return sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY;
        case sc2::UNIT_TYPEID::PROTOSS_CARRIER:             return sc2::UNIT_TYPEID::PROTOSS_STARGATE;  
        case sc2::UNIT_TYPEID::PROTOSS_ORACLE:              return sc2::UNIT_TYPEID::PROTOSS_STARGATE; 
        case sc2::UNIT_TYPEID::PROTOSS_PHOENIX:             return sc2::UNIT_TYPEID::PROTOSS_STARGATE; 
        case sc2::UNIT_TYPEID::PROTOSS_VOIDRAY:             return sc2::UNIT_TYPEID::PROTOSS_STARGATE;  
        case sc2::UNIT_TYPEID::PROTOSS_TEMPEST:             return sc2::UNIT_TYPEID::PROTOSS_STARGATE;
        case sc2::UNIT_TYPEID::PROTOSS_INTERCEPTOR:         return sc2::UNIT_TYPEID::PROTOSS_CARRIER; 
        case sc2::UNIT_TYPEID::PROTOSS_ORACLESTASISTRAP:    return sc2::UNIT_TYPEID::PROTOSS_ORACLE; 
        case sc2::UNIT_TYPEID::TERRAN_ARMORY:               return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKS:             return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSREACTOR:      return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_REFINERY:             return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_SENSORTOWER:          return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORY:              return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_FUSIONCORE:           return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORT:             return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT:          return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_GHOSTACADEMY:         return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_BUNKER:               return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER:        return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_ENGINEERINGBAY:       return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_MISSILETURRET:        return sc2::UNIT_TYPEID::TERRAN_SCV; 
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND:       return sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER; 
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:    return sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER;
        case sc2::UNIT_TYPEID::TERRAN_SCV:                  return sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:      return sc2::UNIT_TYPEID::TERRAN_BARRACKS; 
        case sc2::UNIT_TYPEID::TERRAN_GHOST:                return sc2::UNIT_TYPEID::TERRAN_BARRACKS; 
        case sc2::UNIT_TYPEID::TERRAN_MARAUDER:             return sc2::UNIT_TYPEID::TERRAN_BARRACKS; 
        case sc2::UNIT_TYPEID::TERRAN_MARINE:               return sc2::UNIT_TYPEID::TERRAN_BARRACKS;
        case sc2::UNIT_TYPEID::TERRAN_REAPER:               return sc2::UNIT_TYPEID::TERRAN_BARRACKS;
        case sc2::UNIT_TYPEID::TERRAN_FACTORYREACTOR:       return sc2::UNIT_TYPEID::TERRAN_FACTORY; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORYTECHLAB:       return sc2::UNIT_TYPEID::TERRAN_FACTORY; 
        case sc2::UNIT_TYPEID::TERRAN_HELLION:              return sc2::UNIT_TYPEID::TERRAN_FACTORY; 
        case sc2::UNIT_TYPEID::TERRAN_CYCLONE:              return sc2::UNIT_TYPEID::TERRAN_FACTORY;  
        case sc2::UNIT_TYPEID::TERRAN_SIEGETANK:            return sc2::UNIT_TYPEID::TERRAN_FACTORY; 
        case sc2::UNIT_TYPEID::TERRAN_THOR:                 return sc2::UNIT_TYPEID::TERRAN_FACTORY;  
        case sc2::UNIT_TYPEID::TERRAN_WIDOWMINE:            return sc2::UNIT_TYPEID::TERRAN_FACTORY; 
        case sc2::UNIT_TYPEID::TERRAN_NUKE:                 return sc2::UNIT_TYPEID::TERRAN_GHOSTACADEMY; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR:      return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORTTECHLAB:      return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_BANSHEE:              return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER:        return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_LIBERATOR:            return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_VIKINGFIGHTER:        return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_RAVEN:                return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_MEDIVAC:              return sc2::UNIT_TYPEID::TERRAN_STARPORT; 
        case sc2::UNIT_TYPEID::ZERG_HATCHERY:               return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER:       return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR:              return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_BANELINGNEST:           return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_HYDRALISKDEN:           return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_INFESTATIONPIT:         return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_NYDUSCANAL:             return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_NYDUSNETWORK:           return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_ROACHWARREN:            return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL:           return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_SPINECRAWLER:           return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_SPIRE:                  return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_SPORECRAWLER:           return sc2::UNIT_TYPEID::ZERG_DRONE;
        case sc2::UNIT_TYPEID::ZERG_ULTRALISKCAVERN:        return sc2::UNIT_TYPEID::ZERG_DRONE; 
        case sc2::UNIT_TYPEID::ZERG_OVERLORD:               return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_BANELING:               return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_CORRUPTOR:              return sc2::UNIT_TYPEID::ZERG_LARVA;
        case sc2::UNIT_TYPEID::ZERG_DRONE:                  return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_HYDRALISK:              return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_INFESTOR:               return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_MUTALISK:               return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_QUEEN:                  return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_ROACH:                  return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_SWARMHOSTMP:            return sc2::UNIT_TYPEID::ZERG_LARVA;
        case sc2::UNIT_TYPEID::ZERG_ULTRALISK:              return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_VIPER:                  return sc2::UNIT_TYPEID::ZERG_LARVA; 
        case sc2::UNIT_TYPEID::ZERG_ZERGLING:               return sc2::UNIT_TYPEID::ZERG_LARVA;
        case sc2::UNIT_TYPEID::ZERG_LAIR:                   return sc2::UNIT_TYPEID::ZERG_HATCHERY;
        case sc2::UNIT_TYPEID::ZERG_HIVE:                   return sc2::UNIT_TYPEID::ZERG_LAIR;  
        case sc2::UNIT_TYPEID::ZERG_GREATERSPIRE:           return sc2::UNIT_TYPEID::ZERG_SPIRE; 
       
        default: return 0;
    }
}

int Util::EnemyDPSInRange(const sc2::Point3D unit_pos, ByunJRBot & bot)
{
    float total_dps = 0;
    for (auto & enemyunit : bot.InformationManager().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy))
    {
        double dist = Util::Dist(enemyunit->pos, unit_pos);
        double range = GetAttackRange(enemyunit->unit_type, bot);
        // if we are in range, the dps that is coming at us increases.
        if (dist < range+0.5f)
        {
            total_dps += GetAttackDamage(enemyunit->unit_type, bot);
        }
    }

    return total_dps;
}

sc2::UnitTypeID Util::GetUnitTypeIDFromName(const sc2::ObservationInterface * obs, const std::string & name)
{
    for (const sc2::UnitTypeData & data : obs->GetUnitTypeData())
    {
        if (name == data.name)
        {
            return data.unit_type_id;
        }
    }

    std::cerr << "Unit Type Not Found: " << name << std::endl;
    return 0;
}

sc2::Tag GetClosestEnemyUnitTo(const sc2::Unit* our_unit, const sc2::ObservationInterface * obs)
{
    sc2::Tag closest_tag = 0;
    double closest_dist = std::numeric_limits<double>::max();

    for (auto & unit : obs->GetUnits())
    {
        const double dist = Util::DistSq(unit->pos, our_unit->pos);

        if (!closest_tag || (dist < closest_dist))
        {
            closest_tag = unit->tag;
            closest_dist = dist;
        }
    }

    return closest_tag;
}

bool Util::IsMorphCommand(const sc2::AbilityID & ability) 
{
    switch (ability.ToType()) 
    {
        case sc2::ABILITY_ID::MORPH_HIVE:               return true;
        case sc2::ABILITY_ID::MORPH_LAIR:               return true;
        case sc2::ABILITY_ID::MORPH_GREATERSPIRE:       return true;
        case sc2::ABILITY_ID::MORPH_ORBITALCOMMAND:     return true;
        case sc2::ABILITY_ID::MORPH_PLANETARYFORTRESS:  return true;
        case sc2::ABILITY_ID::TRAIN_OVERLORD:           return true;
        default: return false;
    }
}

sc2::AbilityID Util::UnitTypeIDToAbilityID(const sc2::UnitTypeID & id)
{
    switch (id.ToType()) 
    {
        case sc2::UNIT_TYPEID::TERRAN_ARMORY: return sc2::ABILITY_ID::BUILD_ARMORY; 
        case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR: return sc2::ABILITY_ID::BUILD_ASSIMILATOR; 
        case sc2::UNIT_TYPEID::ZERG_BANELINGNEST: return sc2::ABILITY_ID::BUILD_BANELINGNEST; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKS: return sc2::ABILITY_ID::BUILD_BARRACKS; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSREACTOR: return sc2::ABILITY_ID::BUILD_REACTOR_BARRACKS; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSTECHLAB: return sc2::ABILITY_ID::BUILD_TECHLAB;
        case sc2::UNIT_TYPEID::TERRAN_TECHLAB: return sc2::ABILITY_ID::BUILD_TECHLAB;
        case sc2::UNIT_TYPEID::TERRAN_BUNKER: return sc2::ABILITY_ID::BUILD_BUNKER; 
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER: return sc2::ABILITY_ID::BUILD_COMMANDCENTER; 
        case sc2::UNIT_TYPEID::PROTOSS_CYBERNETICSCORE: return sc2::ABILITY_ID::BUILD_CYBERNETICSCORE; 
        case sc2::UNIT_TYPEID::PROTOSS_DARKSHRINE: return sc2::ABILITY_ID::BUILD_DARKSHRINE; 
        case sc2::UNIT_TYPEID::TERRAN_ENGINEERINGBAY: return sc2::ABILITY_ID::BUILD_ENGINEERINGBAY; 
        case sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER: return sc2::ABILITY_ID::BUILD_EVOLUTIONCHAMBER; 
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR: return sc2::ABILITY_ID::BUILD_EXTRACTOR; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORY: return sc2::ABILITY_ID::BUILD_FACTORY; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORYREACTOR: return sc2::ABILITY_ID::BUILD_REACTOR_FACTORY; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORYTECHLAB: return sc2::ABILITY_ID::BUILD_TECHLAB;
        case sc2::UNIT_TYPEID::PROTOSS_FLEETBEACON: return sc2::ABILITY_ID::BUILD_FLEETBEACON; 
        case sc2::UNIT_TYPEID::PROTOSS_FORGE: return sc2::ABILITY_ID::BUILD_FORGE; 
        case sc2::UNIT_TYPEID::TERRAN_FUSIONCORE: return sc2::ABILITY_ID::BUILD_FUSIONCORE; 
        case sc2::UNIT_TYPEID::PROTOSS_GATEWAY: return sc2::ABILITY_ID::BUILD_GATEWAY; 
        case sc2::UNIT_TYPEID::TERRAN_GHOSTACADEMY: return sc2::ABILITY_ID::BUILD_GHOSTACADEMY; 
        case sc2::UNIT_TYPEID::ZERG_HATCHERY: return sc2::ABILITY_ID::BUILD_HATCHERY; 
        case sc2::UNIT_TYPEID::ZERG_HYDRALISKDEN: return sc2::ABILITY_ID::BUILD_HYDRALISKDEN; 
        case sc2::UNIT_TYPEID::ZERG_INFESTATIONPIT: return sc2::ABILITY_ID::BUILD_INFESTATIONPIT; 
        case sc2::UNIT_TYPEID::PROTOSS_INTERCEPTOR: return sc2::ABILITY_ID::BUILD_INTERCEPTORS; 
        case sc2::UNIT_TYPEID::TERRAN_MISSILETURRET: return sc2::ABILITY_ID::BUILD_MISSILETURRET; 
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS: return sc2::ABILITY_ID::BUILD_NEXUS; 
        case sc2::UNIT_TYPEID::TERRAN_NUKE: return sc2::ABILITY_ID::BUILD_NUKE; 
        case sc2::UNIT_TYPEID::ZERG_NYDUSCANAL: return sc2::ABILITY_ID::BUILD_NYDUSWORM; 
        case sc2::UNIT_TYPEID::ZERG_NYDUSNETWORK: return sc2::ABILITY_ID::BUILD_NYDUSNETWORK; 
        case sc2::UNIT_TYPEID::PROTOSS_ORACLESTASISTRAP: return sc2::ABILITY_ID::BUILD_STASISTRAP; 
        case sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON: return sc2::ABILITY_ID::BUILD_PHOTONCANNON; 
        case sc2::UNIT_TYPEID::PROTOSS_PYLON: return sc2::ABILITY_ID::BUILD_PYLON; 
        //return sc2::ABILITY_ID::BUILD_QUEEN_CREEPTUMOR; case sc2::UNIT_TYPEID::ZERG_CREEPTUMORQUEEN: 
        case sc2::UNIT_TYPEID::TERRAN_REFINERY: return sc2::ABILITY_ID::BUILD_REFINERY; 
        case sc2::UNIT_TYPEID::ZERG_ROACHWARREN: return sc2::ABILITY_ID::BUILD_ROACHWARREN; 
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSBAY: return sc2::ABILITY_ID::BUILD_ROBOTICSBAY; 
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY: return sc2::ABILITY_ID::BUILD_ROBOTICSFACILITY; 
        case sc2::UNIT_TYPEID::TERRAN_SENSORTOWER: return sc2::ABILITY_ID::BUILD_SENSORTOWER; 
        case sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL: return sc2::ABILITY_ID::BUILD_SPAWNINGPOOL; 
        case sc2::UNIT_TYPEID::ZERG_SPINECRAWLER: return sc2::ABILITY_ID::BUILD_SPINECRAWLER; 
        case sc2::UNIT_TYPEID::ZERG_SPIRE: return sc2::ABILITY_ID::BUILD_SPIRE; 
        case sc2::UNIT_TYPEID::ZERG_SPORECRAWLER: return sc2::ABILITY_ID::BUILD_SPORECRAWLER; 
        case sc2::UNIT_TYPEID::PROTOSS_STARGATE: return sc2::ABILITY_ID::BUILD_STARGATE; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORT: return sc2::ABILITY_ID::BUILD_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR: return sc2::ABILITY_ID::BUILD_REACTOR_STARPORT; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORTTECHLAB: return sc2::ABILITY_ID::BUILD_TECHLAB;
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT: return sc2::ABILITY_ID::BUILD_SUPPLYDEPOT; 
        case sc2::UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE: return sc2::ABILITY_ID::BUILD_TEMPLARARCHIVE; 
        case sc2::UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL: return sc2::ABILITY_ID::BUILD_TWILIGHTCOUNCIL; 
        case sc2::UNIT_TYPEID::ZERG_ULTRALISKCAVERN: return sc2::ABILITY_ID::BUILD_ULTRALISKCAVERN; 
        case sc2::UNIT_TYPEID::ZERG_HIVE: return sc2::ABILITY_ID::MORPH_HIVE; 
        case sc2::UNIT_TYPEID::ZERG_LAIR: return sc2::ABILITY_ID::MORPH_LAIR; 
        case sc2::UNIT_TYPEID::ZERG_GREATERSPIRE: return sc2::ABILITY_ID::MORPH_GREATERSPIRE; 
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND: return sc2::ABILITY_ID::MORPH_ORBITALCOMMAND; 
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return sc2::ABILITY_ID::MORPH_PLANETARYFORTRESS; 
        case sc2::UNIT_TYPEID::ZERG_OVERLORD: return sc2::ABILITY_ID::TRAIN_OVERLORD; 
        /*case sc2::UNIT_TYPEID::PROTOSS_DARKTEMPLAR: return sc2::ABILITY_ID::TRAINWARP_DARKTEMPLAR; 
        case sc2::UNIT_TYPEID::PROTOSS_HIGHTEMPLAR: return sc2::ABILITY_ID::TRAINWARP_HIGHTEMPLAR; 
        case sc2::UNIT_TYPEID::PROTOSS_SENTRY: return sc2::ABILITY_ID::TRAINWARP_SENTRY; 
        case sc2::UNIT_TYPEID::PROTOSS_STALKER: return sc2::ABILITY_ID::TRAINWARP_STALKER; 
        case sc2::UNIT_TYPEID::PROTOSS_ADEPT: return sc2::ABILITY_ID::TRAINWARP_WARPINADEPT; 
        case sc2::UNIT_TYPEID::PROTOSS_ZEALOT: return sc2::ABILITY_ID::TRAINWARP_ZEALOT; */
        case sc2::UNIT_TYPEID::ZERG_BANELING: return sc2::ABILITY_ID::TRAIN_BANELING; 
        case sc2::UNIT_TYPEID::TERRAN_BANSHEE: return sc2::ABILITY_ID::TRAIN_BANSHEE; 
        case sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER: return sc2::ABILITY_ID::TRAIN_BATTLECRUISER; 
        case sc2::UNIT_TYPEID::TERRAN_CYCLONE: return sc2::ABILITY_ID::TRAIN_CYCLONE; 
        case sc2::UNIT_TYPEID::PROTOSS_CARRIER: return sc2::ABILITY_ID::TRAIN_CARRIER; 
        case sc2::UNIT_TYPEID::PROTOSS_COLOSSUS: return sc2::ABILITY_ID::TRAIN_COLOSSUS; 
        case sc2::UNIT_TYPEID::ZERG_CORRUPTOR: return sc2::ABILITY_ID::TRAIN_CORRUPTOR;
        case sc2::UNIT_TYPEID::PROTOSS_DARKTEMPLAR: return sc2::ABILITY_ID::TRAIN_DARKTEMPLAR; 
        case sc2::UNIT_TYPEID::ZERG_DRONE: return sc2::ABILITY_ID::TRAIN_DRONE; 
        case sc2::UNIT_TYPEID::TERRAN_GHOST: return sc2::ABILITY_ID::TRAIN_GHOST; 
        case sc2::UNIT_TYPEID::TERRAN_HELLION: return sc2::ABILITY_ID::TRAIN_HELLION; 
        //case sc2::UNIT_TYPEID::TERRAN_HELLION: return sc2::ABILITY_ID::TRAIN_HELLIONTANK; 
        case sc2::UNIT_TYPEID::PROTOSS_HIGHTEMPLAR: return sc2::ABILITY_ID::TRAIN_HIGHTEMPLAR; 
        case sc2::UNIT_TYPEID::ZERG_HYDRALISK: return sc2::ABILITY_ID::TRAIN_HYDRALISK; 
        case sc2::UNIT_TYPEID::PROTOSS_IMMORTAL: return sc2::ABILITY_ID::TRAIN_IMMORTAL;
        case sc2::UNIT_TYPEID::ZERG_INFESTOR: return sc2::ABILITY_ID::TRAIN_INFESTOR; 
        case sc2::UNIT_TYPEID::TERRAN_LIBERATOR: return sc2::ABILITY_ID::TRAIN_LIBERATOR; 
        case sc2::UNIT_TYPEID::TERRAN_MARAUDER: return sc2::ABILITY_ID::TRAIN_MARAUDER; 
        case sc2::UNIT_TYPEID::TERRAN_MARINE: return sc2::ABILITY_ID::TRAIN_MARINE; 
        case sc2::UNIT_TYPEID::TERRAN_MEDIVAC: return sc2::ABILITY_ID::TRAIN_MEDIVAC; 
        case sc2::UNIT_TYPEID::PROTOSS_MOTHERSHIPCORE: return sc2::ABILITY_ID::TRAIN_MOTHERSHIPCORE; 
        case sc2::UNIT_TYPEID::ZERG_MUTALISK: return sc2::ABILITY_ID::TRAIN_MUTALISK; 
        case sc2::UNIT_TYPEID::PROTOSS_OBSERVER: return sc2::ABILITY_ID::TRAIN_OBSERVER; 
        case sc2::UNIT_TYPEID::PROTOSS_ORACLE: return sc2::ABILITY_ID::TRAIN_ORACLE; 
        case sc2::UNIT_TYPEID::PROTOSS_PHOENIX: return sc2::ABILITY_ID::TRAIN_PHOENIX; 
        case sc2::UNIT_TYPEID::PROTOSS_PROBE: return sc2::ABILITY_ID::TRAIN_PROBE; 
        case sc2::UNIT_TYPEID::ZERG_QUEEN: return sc2::ABILITY_ID::TRAIN_QUEEN; 
        case sc2::UNIT_TYPEID::TERRAN_RAVEN: return sc2::ABILITY_ID::TRAIN_RAVEN; 
        case sc2::UNIT_TYPEID::TERRAN_REAPER: return sc2::ABILITY_ID::TRAIN_REAPER;
        case sc2::UNIT_TYPEID::ZERG_ROACH: return sc2::ABILITY_ID::TRAIN_ROACH; 
        case sc2::UNIT_TYPEID::TERRAN_SCV: return sc2::ABILITY_ID::TRAIN_SCV; 
        case sc2::UNIT_TYPEID::PROTOSS_SENTRY: return sc2::ABILITY_ID::TRAIN_SENTRY; 
        case sc2::UNIT_TYPEID::TERRAN_SIEGETANK: return sc2::ABILITY_ID::TRAIN_SIEGETANK; 
        case sc2::UNIT_TYPEID::PROTOSS_STALKER: return sc2::ABILITY_ID::TRAIN_STALKER; 
        case sc2::UNIT_TYPEID::ZERG_SWARMHOSTMP: return sc2::ABILITY_ID::TRAIN_SWARMHOST; 
        case sc2::UNIT_TYPEID::PROTOSS_TEMPEST: return sc2::ABILITY_ID::TRAIN_TEMPEST; 
        case sc2::UNIT_TYPEID::TERRAN_THOR: return sc2::ABILITY_ID::TRAIN_THOR; 
        case sc2::UNIT_TYPEID::ZERG_ULTRALISK: return sc2::ABILITY_ID::TRAIN_ULTRALISK; 
        case sc2::UNIT_TYPEID::TERRAN_VIKINGFIGHTER: return sc2::ABILITY_ID::TRAIN_VIKINGFIGHTER; 
        case sc2::UNIT_TYPEID::ZERG_VIPER: return sc2::ABILITY_ID::TRAIN_VIPER; 
        case sc2::UNIT_TYPEID::PROTOSS_VOIDRAY: return sc2::ABILITY_ID::TRAIN_VOIDRAY; 
        case sc2::UNIT_TYPEID::PROTOSS_ADEPT: return sc2::ABILITY_ID::TRAIN_ADEPT; 
        case sc2::UNIT_TYPEID::PROTOSS_DISRUPTOR: return sc2::ABILITY_ID::TRAIN_DISRUPTOR; 
        case sc2::UNIT_TYPEID::PROTOSS_WARPPRISM: return sc2::ABILITY_ID::TRAIN_WARPPRISM; 
        case sc2::UNIT_TYPEID::TERRAN_WIDOWMINE: return sc2::ABILITY_ID::TRAIN_WIDOWMINE; 
        case sc2::UNIT_TYPEID::PROTOSS_ZEALOT: return sc2::ABILITY_ID::TRAIN_ZEALOT; 
        case sc2::UNIT_TYPEID::ZERG_ZERGLING: return sc2::ABILITY_ID::TRAIN_ZERGLING; 

        default: return 0;
    }
}

bool Util::CanAttackAir(std::vector<sc2::Weapon> weapons)
{
    for(auto const &  w : weapons)
    {
        if(w.type == sc2::Weapon::TargetType::Air || w.type == sc2::Weapon::TargetType::Any)
            return true;
    }
    return false;
}
int Util::GetGameTimeInSeconds(const sc2::Agent& bot)
{
    return bot.Observation()->GetGameLoop() / 22;
}

bool Util::IsBuilding(const sc2::UnitTypeID & type)
{
    switch (type.ToType()) 
    {
        case sc2::UNIT_TYPEID::TERRAN_ARMORY:           return true; 
        case sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR:     return true; 
        case sc2::UNIT_TYPEID::ZERG_BANELINGNEST:       return true; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKS:         return true; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSREACTOR:  return true; 
        case sc2::UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:  return true; 
        case sc2::UNIT_TYPEID::TERRAN_BUNKER:           return true; 
        case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER:    return true; 
        case sc2::UNIT_TYPEID::PROTOSS_CYBERNETICSCORE: return true; 
        case sc2::UNIT_TYPEID::PROTOSS_DARKSHRINE:      return true; 
        case sc2::UNIT_TYPEID::TERRAN_ENGINEERINGBAY:   return true; 
        case sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER:   return true; 
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR:          return true; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORY:          return true; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORYREACTOR:   return true; 
        case sc2::UNIT_TYPEID::TERRAN_FACTORYTECHLAB:   return true; 
        case sc2::UNIT_TYPEID::PROTOSS_FLEETBEACON:     return true; 
        case sc2::UNIT_TYPEID::PROTOSS_FORGE:           return true; 
        case sc2::UNIT_TYPEID::TERRAN_FUSIONCORE:       return true; 
        case sc2::UNIT_TYPEID::PROTOSS_GATEWAY:         return true; 
        case sc2::UNIT_TYPEID::TERRAN_GHOSTACADEMY:     return true; 
        case sc2::UNIT_TYPEID::ZERG_HATCHERY:           return true; 
        case sc2::UNIT_TYPEID::ZERG_HYDRALISKDEN:       return true; 
        case sc2::UNIT_TYPEID::ZERG_INFESTATIONPIT:     return true; 
        case sc2::UNIT_TYPEID::TERRAN_MISSILETURRET:    return true; 
        case sc2::UNIT_TYPEID::PROTOSS_NEXUS:           return true; 
        case sc2::UNIT_TYPEID::ZERG_NYDUSCANAL:         return true; 
        case sc2::UNIT_TYPEID::ZERG_NYDUSNETWORK:       return true; 
        case sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON:    return true; 
        case sc2::UNIT_TYPEID::PROTOSS_PYLON:           return true; 
        case sc2::UNIT_TYPEID::TERRAN_REFINERY:         return true; 
        case sc2::UNIT_TYPEID::ZERG_ROACHWARREN:        return true; 
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSBAY:     return true; 
        case sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY: return true; 
        case sc2::UNIT_TYPEID::TERRAN_SENSORTOWER:      return true; 
        case sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL:       return true; 
        case sc2::UNIT_TYPEID::ZERG_SPINECRAWLER:       return true; 
        case sc2::UNIT_TYPEID::ZERG_SPIRE:              return true; 
        case sc2::UNIT_TYPEID::ZERG_SPORECRAWLER:       return true; 
        case sc2::UNIT_TYPEID::PROTOSS_STARGATE:        return true; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORT:         return true; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR:  return true; 
        case sc2::UNIT_TYPEID::TERRAN_STARPORTTECHLAB:  return true; 
        case sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT:      return true; 
        case sc2::UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE:  return true; 
        case sc2::UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL: return true; 
        case sc2::UNIT_TYPEID::ZERG_ULTRALISKCAVERN:    return true; 
        case sc2::UNIT_TYPEID::ZERG_HIVE:               return true; 
        case sc2::UNIT_TYPEID::ZERG_LAIR:               return true; 
        case sc2::UNIT_TYPEID::ZERG_GREATERSPIRE:       return true; 
        case sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND:   return true; 
        case sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return true;  

        default: return false;
    }
}

// checks where a given unit can make a given unit type now
// this is done by iterating its legal abilities for the build command to make the unit
bool Util::UnitCanBuildTypeNow(const sc2::Unit* unit, const sc2::UnitTypeID & type, sc2::Agent & bot)
{
    sc2::AvailableAbilities available_abilities = bot.Query()->GetAbilitiesForUnit(unit);
    
    // quick check if the unit can't do anything it certainly can't build the thing we want
    if (available_abilities.abilities.empty()) 
    {
        return false;
    }
    else 
    {
        // check to see if one of the unit's available abilities matches the build ability type
        const sc2::AbilityID build_type_ability = Util::UnitTypeIDToAbilityID(type);
        for (const sc2::AvailableAbility & available_ability : available_abilities.abilities) 
        {
            if (available_ability.ability_id == build_type_ability)
            {
                return true;
            }
        }
    }

    return false;
}