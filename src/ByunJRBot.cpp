#include <sc2api/sc2_api.h>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"

ByunJRBot::ByunJRBot()
    : m_map(*this)
    , m_bases(*this)
    , m_unitInfo(*this)
    , m_workers(*this)
    , m_gameCommander(*this)
    , m_strategy(*this),
    m_isWillingToFight(true)
{
    
}

void ByunJRBot::OnGameStart() 
{
    m_config.readConfigFile();

    // get my race
    auto playerID = Observation()->GetPlayerID();
    for (auto & playerInfo : Observation()->GetGameInfo().player_info)
    {
        if (playerInfo.player_id == playerID)
        {
            m_playerRace[Players::Self] = playerInfo.race_actual;
        }
        else
        {
            m_playerRace[Players::Enemy] = playerInfo.race_requested;
        }
    }

    m_strategy.onStart();
    m_map.onStart();
    m_unitInfo.onStart();
    m_bases.onStart();
    m_workers.onStart();

    m_gameCommander.onStart();
}

void ByunJRBot::OnStep()
{
    Control()->GetObservation();

    m_map.onFrame();
    m_unitInfo.onFrame();
    m_bases.onFrame();
    m_workers.onFrame();
    m_strategy.onFrame();

    m_gameCommander.onFrame();

    Debug()->SendDebug();
}

void ByunJRBot::OnUnitCreated(const sc2::Unit* unit) {
    m_gameCommander.onUnitCreated(*unit);
}

void ByunJRBot::OnUnitEnterVision(const sc2::Unit* unit) {
    m_gameCommander.onUnitEnterVision(*unit);
}

void ByunJRBot::OnBuildingConstructionComplete(const sc2::Unit* unit) {
    m_gameCommander.onBuildingConstructionComplete(*unit);
}

// Returns true if the bot thinks it still has a chance.
// Return false if there is no point continuing the simulation.
bool ByunJRBot::IsWillingToFight()
{
    return m_isWillingToFight;
}

void ByunJRBot::Resign()
{
    m_isWillingToFight = false;
}


// TODO: Figure out my race
const sc2::Race & ByunJRBot::GetPlayerRace(int player) const
{
    BOT_ASSERT(player == Players::Self || player == Players::Enemy, "invalid player for GetPlayerRace");
    return m_playerRace[player];
}

BotConfig & ByunJRBot::Config()
{
     return m_config;
}

const MapTools & ByunJRBot::Map() const
{
    return m_map;
}

const StrategyManager & ByunJRBot::Strategy() const
{
    return m_strategy;
}

const BaseLocationManager & ByunJRBot::Bases() const
{
    return m_bases;
}

const UnitInfoManager & ByunJRBot::UnitInfo() const
{
    return m_unitInfo;
}

GameCommander & ByunJRBot::GameCommander()
{
    return m_gameCommander;
}

WorkerManager & ByunJRBot::Workers()
{
    return m_workers;
}

const sc2::Unit * ByunJRBot::GetUnit(const sc2::Tag & tag) const
{
    return Observation()->GetUnit(tag);
}

sc2::Point2D ByunJRBot::GetStartLocation() const
{
    return Observation()->GetStartLocation();
}

void ByunJRBot::OnError(const std::vector<sc2::ClientError> & client_errors, const std::vector<std::string> & protocol_errors)
{
    
}