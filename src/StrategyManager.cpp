#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "StrategyManager.h"
#include "util/JSONTools.h"
#include "util/Util.h"

Strategy::Strategy()
{

}

Strategy::Strategy(const std::string & name, const sc2::Race & race, const BuildOrder & buildOrder)
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
{

}

void StrategyManager::OnStart()
{
    ReadStrategyFile(bot_.Config().ConfigFileLocation);
}

void StrategyManager::OnFrame()
{

}

// assigns units to various managers
void StrategyManager::HandleUnitAssignments()
{
    bot_.InformationManager().HandleUnitAssignments();
}

bool StrategyManager::ShouldSendInitialScout() const
{
    return true;

    switch (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self))
    {
    case sc2::Race::Terran:  return bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true) > 0;
    case sc2::Race::Protoss: return bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::PROTOSS_PYLON, true) > 0;
    case sc2::Race::Zerg:    return bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, true) > 0;
    default: return false;
    }
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
        BOT_ASSERT(false, "Strategy not found: %s, returning empty initial build order", bot_.Config().StrategyName.c_str());
        return empty_build_order_;
    }
}

bool StrategyManager::ShouldExpandNow() const
{
    return false;
}

void StrategyManager::AddStrategy(const std::string & name, const Strategy & strategy)
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
    const sc2::Race race = bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self);
    std::string our_race = Util::GetStringFromRace(race);
    std::string config = bot_.Config().RawConfigString;
    rapidjson::Document doc;
    const bool parsing_failed = doc.Parse(config.c_str()).HasParseError();
    if (parsing_failed)
    {
        std::cerr << "ParseStrategy could not find file: " << filename << ", shutting down.\n";
        return;
    }

    // Parse the Strategy Options
    if (doc.HasMember("Strategy") && doc["Strategy"].IsObject())
    {
        const rapidjson::Value & strategy = doc["Strategy"];

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
                    BOT_ASSERT(false, "Strategy must have a Race string: %s", name.c_str());
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

                AddStrategy(name, Strategy(name, strategy_race, build_order));
            }
        }
    }
}