#include <iostream>

#include "ByunJRBot.h"
#include "TechLab/util/JSONTools.h"
#include "TechLab/util/Util.h"

#include "common/BotAssert.h"
#include "StrategyManager.h"

StrategyBuildOrder::StrategyBuildOrder()
{

}

StrategyBuildOrder::StrategyBuildOrder(const std::string & name, const sc2::Race & race, const BuildOrder & buildOrder)
    : name(name)
    , race(race)
    , buildOrder(buildOrder)
    , wins(0)
    , losses(0)
{

}

// constructor
StrategyManager::StrategyManager(ByunJRBot & bot)
    : bot_(bot)
    , building_placer_(bot)
    , macro_goal_(Strategy::ReaperRush)
    , initial_scout_set_(false)
    , second_proxy_worker_set_(false)
    , bases_safe_(false)
{
}

void StrategyManager::OnStart()
{
    ReadStrategyFile(bot_.Config().ConfigFileLocation);
    building_placer_.OnStart();
}

// This strategy code is only for Terran. 
// This code will not function correctly if playing other races.
void StrategyManager::OnFrame()
{
    // Update variables that we will need later. 
    bases_safe_ = AreBasesSafe();
    RecalculateMacroGoal();
    HandleUnitAssignments();


    // At various times we will want to use special abilities of a unit. 
    // Loop through all our units and see if it is time to use one yet. 
    for (const auto & unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
    {
        // Emergency repair units and depots.
        if (Util::IsBuilding(unit->unit_type))
        {
            // If a depot may die, go repair it. 
            if(unit->unit_type == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT || unit->unit_type == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)
            {
                if (unit->health != unit->health_max)
                Micro::SmartRepairWithSCVCount(unit, 2, bot_.Info());
            }

            if (unit->health < unit->health_max/3+100)
            {
                bot_.Actions()->UnitCommand(unit, sc2::ABILITY_ID::LIFT);
            }
            else
            {
            //    bot_.Actions()->UnitCommand(unit, sc2::ABILITY_ID::LAND,unit->pos);
            }
        }
        // Repair battlecruisers that have tactical jumped back to our base. 
        if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER 
         && unit->health != unit->health_max
            // Square 10 to avoid taking the square root as part of the distance formula. 
         && Util::DistSq(unit->pos,bot_.Info().Bases().GetPlayerStartingBaseLocation(sc2::Unit::Alliance::Self)->GetPosition()) < 10*10)
        {
            if(bases_safe_)
            // If we repair with too many workers, the battlecruiser will get sent back into battle before Tactical Jump is back online. 
                Micro::SmartRepairWithSCVCount(unit, 2, bot_.Info());
            if (!bases_safe_)
            // If we are in critical danger, pull all the boys!
                Micro::SmartRepairWithSCVCount(unit, 10, bot_.Info());
        }
        // Once we are done repairing, send that battlecruiser back to the field!
        else if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER
            && unit->health == unit->health_max)
        {
            bot_.Info().UnitInfo().SetJob(unit, UnitMission::Attack);
        }
    }
}

BuildingPlacer & StrategyManager::BuildingPlacer()
{
    return building_placer_;
}

void StrategyManager::RecalculateMacroGoal()
{
    if (bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Enemy, sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON)
     || bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Enemy, sc2::UNIT_TYPEID::TERRAN_SIEGETANK)
     || bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Enemy, sc2::UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)
    // || bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Enemy, sc2::UNIT_TYPEID::PROTOSS_VOIDRAY)
     || bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Enemy, sc2::UNIT_TYPEID::TERRAN_BANSHEE)
     || (bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_REAPER) < 2
        && Util::GetGameTimeInSeconds(bot_) > 240 )
     || Util::GetGameTimeInSeconds(bot_) > 420)
    {
        macro_goal_ = Strategy::BattlecruiserMacro;
    }
}

// assigns units to various managers
void StrategyManager::HandleUnitAssignments()
{
    SetScoutUnits();

    // Repair any damaged supply depots. If our base is safe, lower the wall. Otherwise, raise the wall. 
    for (const auto & unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
    {
        // Find all the depots and perform some actions on them. 
        if (Util::IsSupplyProvider(unit))
        {
            // If the depot may die, go repair it. 
            if (unit->health != unit->health_max)
                Micro::SmartRepairWithSCVCount(unit, 2, bot_.Info());

            if (bases_safe_)
            {
                bot_.Actions()->UnitCommand(unit, sc2::ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER);
            }
            else
            {
                bot_.Actions()->UnitCommand(unit, sc2::ABILITY_ID::MORPH_SUPPLYDEPOT_RAISE);
            }
        }
    }
}

void StrategyManager::SetScoutUnits()
{
    // if we haven't set a scout unit, do it
    if (bot_.Info().UnitInfo().GetScouts().empty() && !initial_scout_set_)
    {
        // Should we send the initial scout?
        if (ShouldSendInitialScout())
        {
            const ::UnitInfo * worker_scout = bot_.Info().GetClosestUnitInfoWithJob(bot_.GetStartLocation(), UnitMission::Minerals);

            // If we find a worker (which we should) add it to the Scouting units.
            if (worker_scout)
            {
                bot_.Info().UnitInfo().SetJob(worker_scout->unit, UnitMission::Scout);
                initial_scout_set_ = true;
            }
        }
    }
    // Is it time to send the worker to go build the second barracks?
    if (ShouldSendSecondProxyWorker())
    {
        // Grab the closest worker to our base.
        const ::UnitInfo * proxy_worker = bot_.Info().GetClosestUnitInfoWithJob(bot_.GetStartLocation(), UnitMission::Minerals);

        //// If we find a worker (which we should) go send it out to proxy.
        //if (proxy_worker)
        //{
        //    bot_.Info().UnitInfo().SetJob(proxy_worker->unit, UnitMission::Proxy);
        //    second_proxy_worker_set_ = true;
        //}
    }
}

bool StrategyManager::ShouldSendSecondProxyWorker() const
{
    if (Util::GetGameTimeInSeconds(bot_) > 20 && !second_proxy_worker_set_)
        return true;
    return false;
}

bool StrategyManager::ShouldSendInitialScout() const
{
    return true;
    switch (bot_.Info().GetPlayerRace(sc2::Unit::Alliance::Self))
    {
        case sc2::Race::Terran:  return bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true) > 0;
        case sc2::Race::Protoss: return bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::PROTOSS_PYLON, true) > 0;
        case sc2::Race::Zerg:    return bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, true) > 0;
        default: return false;
    }
}

bool StrategyManager::AreBasesSafe()
{
    for (const auto & enemy_unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy))
    {
        for (const auto & potential_base : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
        {
            if( Util::IsTownHall(potential_base)
             && Util::DistSq(potential_base->pos, enemy_unit->pos) < (30*30))
            {
                return false;
            }
        }
    }
    return true;
}

const BuildOrder & StrategyManager::GetOpeningBookBuildOrder() const
{
    const auto build_order_it = strategies_.find(bot_.Config().StrategyName);

    // look for the build order in the build order map
    if (build_order_it != std::end(strategies_))
    {
        return (*build_order_it).second.buildOrder;
    }
    else
    {
        BOT_ASSERT(false, "StrategyBuildOrder not found: %s, returning empty initial build order", bot_.Config().StrategyName.c_str());
        return empty_build_order_;
    }
}


bool StrategyManager::ShouldExpandNow() const
{
    if (bot_.Observation()->GetMinerals() > 400 && bases_safe_)
    {
        return true;
    }
    return false;
}

Strategy StrategyManager::MacroGoal() const
{
    return macro_goal_;
}

void StrategyManager::AddStrategy(const std::string & name, const StrategyBuildOrder & strategy)
{
    strategies_[name] = strategy;
}

UnitPairVector StrategyManager::GetBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

UnitPairVector StrategyManager::GetProtossBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

UnitPairVector StrategyManager::GetTerranBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

UnitPairVector StrategyManager::GetZergBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

void StrategyManager::ReadStrategyFile(const std::string & filename)
{
    const sc2::Race race = bot_.Info().GetPlayerRace(sc2::Unit::Alliance::Self);
    std::string our_race = Util::GetStringFromRace(race);
    std::string config = bot_.Config().RawConfigString;
    rapidjson::Document doc;
    const bool parsing_failed = doc.Parse(config.c_str()).HasParseError();
    if (parsing_failed)
    {
        std::cerr << "ParseStrategy could not find file: " << filename << ", shutting down.\n";
        return;
    }

    // Parse the StrategyBuildOrder Options
    if (doc.HasMember("StrategyBuildOrder") && doc["StrategyBuildOrder"].IsObject())
    {
        const rapidjson::Value & strategy = doc["StrategyBuildOrder"];

        // read in the various strategic elements
        JSONTools::ReadBool("ScoutHarassEnemy", strategy, bot_.Config().ScoutHarassEnemy);
        JSONTools::ReadString("ReadDirectory", strategy, bot_.Config().ReadDir);
        JSONTools::ReadString("WriteDirectory", strategy, bot_.Config().WriteDir);

        // if we have set a strategy for the current race, use it
        if (strategy.HasMember(our_race.c_str()) && strategy[our_race.c_str()].IsString())
        {
            bot_.Config().StrategyName = strategy[our_race.c_str()].GetString();
        }

        // check if we are using an enemy specific strategy
        JSONTools::ReadBool("UseEnemySpecificStrategy", strategy, bot_.Config().UseEnemySpecificStrategy);
        if (bot_.Config().UseEnemySpecificStrategy && strategy.HasMember("EnemySpecificStrategy") && strategy["EnemySpecificStrategy"].IsObject())
        {
            // TODO: Figure out enemy name
            const std::string enemy_name = "ENEMY NAME";
            const rapidjson::Value & specific = strategy["EnemySpecificStrategy"];

            // check to see if our current enemy name is listed anywhere in the specific strategies
            if (specific.HasMember(enemy_name.c_str()) && specific[enemy_name.c_str()].IsObject())
            {
                const rapidjson::Value & enemy_strategies = specific[enemy_name.c_str()];

                // if that enemy has a strategy listed for our current race, use it
                if (enemy_strategies.HasMember(our_race.c_str()) && enemy_strategies[our_race.c_str()].IsString())
                {
                    bot_.Config().StrategyName = enemy_strategies[our_race.c_str()].GetString();
                    bot_.Config().FoundEnemySpecificStrategy = true;
                }
            }
        }

        // Parse all the Strategies
        if (strategy.HasMember("Strategies") && strategy["Strategies"].IsObject())
        {
            const rapidjson::Value & strategies = strategy["Strategies"];
            for (auto itr = strategies.MemberBegin(); itr != strategies.MemberEnd(); ++itr)
            {
                const std::string &         name = itr->name.GetString();
                const rapidjson::Value &    val  = itr->value;

                sc2::Race strategy_race;
                if (val.HasMember("Race") && val["Race"].IsString())
                {
                    strategy_race = Util::GetRaceFromString(val["Race"].GetString());
                }
                else
                {
                    BOT_ASSERT(false, "StrategyBuildOrder must have a Race string: %s", name.c_str());
                    continue;
                }

                BuildOrder build_order(strategy_race);
                if (val.HasMember("OpeningBuildOrder") && val["OpeningBuildOrder"].IsArray())
                {
                    const rapidjson::Value & build = val["OpeningBuildOrder"];

                    for (rapidjson::SizeType b(0); b < build.Size(); ++b)
                    {
                        if (build[b].IsString())
                        {
                            const sc2::UnitTypeID type_id = Util::GetUnitTypeIDFromName(bot_.Observation(), build[b].GetString());

                            build_order.Add(type_id);
                        }
                        else
                        {
                            BOT_ASSERT(false, "Build order item must be a string %s", name.c_str());
                            continue;
                        }
                    }
                }

                AddStrategy(name, StrategyBuildOrder(name, strategy_race, build_order));
            }
        }
    }
}