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

void StrategyManager::onStart()
{
    readStrategyFile(bot_.Config().ConfigFileLocation);
}

void StrategyManager::OnFrame()
{

}

// assigns units to various managers
void StrategyManager::handleUnitAssignments()
{
    bot_.InformationManager().HandleUnitAssignments();
}

bool StrategyManager::shouldSendInitialScout() const
{
    return true;

    switch (bot_.GetPlayerRace(PlayerArrayIndex::Self))
    {
    case sc2::Race::Terran:  return bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true) > 0;
    case sc2::Race::Protoss: return bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::PROTOSS_PYLON, true) > 0;
    case sc2::Race::Zerg:    return bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, true) > 0;
    default: return false;
    }
}

const BuildOrder & StrategyManager::getOpeningBookBuildOrder() const
{
    const auto buildOrderIt = strategies.find(bot_.Config().StrategyName);

    // look for the build order in the build order map
    if (buildOrderIt != std::end(strategies))
    {
        return (*buildOrderIt).second.buildOrder;
    }
    else
    {
        BOT_ASSERT(false, "Strategy not found: %s, returning empty initial build order", bot_.Config().StrategyName.c_str());
        return emptyBuildOrder;
    }
}

bool StrategyManager::shouldExpandNow() const
{
    return false;
}

void StrategyManager::addStrategy(const std::string & name, const Strategy & strategy)
{
    strategies[name] = strategy;
}

UnitPairVector StrategyManager::getBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

UnitPairVector StrategyManager::getProtossBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

UnitPairVector StrategyManager::getTerranBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}

UnitPairVector StrategyManager::getZergBuildOrderGoal() const
{
    return std::vector<UnitPair>();
}


void StrategyManager::onEnd(const bool isWinner)
{

}

void StrategyManager::readStrategyFile(const std::string & filename)
{
    sc2::Race race = bot_.GetPlayerRace(PlayerArrayIndex::Self);
    std::string ourRace = Util::GetStringFromRace(race);
    std::string config = bot_.Config().RawConfigString;
    rapidjson::Document doc;
    const bool parsingFailed = doc.Parse(config.c_str()).HasParseError();
    if (parsingFailed)
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
        if (strategy.HasMember(ourRace.c_str()) && strategy[ourRace.c_str()].IsString())
        {
            bot_.Config().StrategyName = strategy[ourRace.c_str()].GetString();
        }

        // check if we are using an enemy specific strategy
        JSONTools::ReadBool("UseEnemySpecificStrategy", strategy, bot_.Config().UseEnemySpecificStrategy);
        if (bot_.Config().UseEnemySpecificStrategy && strategy.HasMember("EnemySpecificStrategy") && strategy["EnemySpecificStrategy"].IsObject())
        {
            // TODO: Figure out enemy name
            const std::string enemyName = "ENEMY NAME";
            const rapidjson::Value & specific = strategy["EnemySpecificStrategy"];

            // check to see if our current enemy name is listed anywhere in the specific strategies
            if (specific.HasMember(enemyName.c_str()) && specific[enemyName.c_str()].IsObject())
            {
                const rapidjson::Value & enemyStrategies = specific[enemyName.c_str()];

                // if that enemy has a strategy listed for our current race, use it
                if (enemyStrategies.HasMember(ourRace.c_str()) && enemyStrategies[ourRace.c_str()].IsString())
                {
                    bot_.Config().StrategyName = enemyStrategies[ourRace.c_str()].GetString();
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

                sc2::Race strategyRace;
                if (val.HasMember("Race") && val["Race"].IsString())
                {
                    strategyRace = Util::GetRaceFromString(val["Race"].GetString());
                }
                else
                {
                    BOT_ASSERT(false, "Strategy must have a Race string: %s", name.c_str());
                    continue;
                }

                BuildOrder buildOrder(strategyRace);
                if (val.HasMember("OpeningBuildOrder") && val["OpeningBuildOrder"].IsArray())
                {
                    const rapidjson::Value & build = val["OpeningBuildOrder"];

                    for (rapidjson::SizeType b(0); b < build.Size(); ++b)
                    {
                        if (build[b].IsString())
                        {
                            const sc2::UnitTypeID typeID = Util::GetUnitTypeIDFromName(bot_.Observation(), build[b].GetString());

                            buildOrder.Add(typeID);
                        }
                        else
                        {
                            BOT_ASSERT(false, "Build order item must be a string %s", name.c_str());
                            continue;
                        }
                    }
                }

                addStrategy(name, Strategy(name, strategyRace, buildOrder));
            }
        }
    }
}