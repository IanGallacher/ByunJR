#include <sstream>
#include <sc2api/sc2_api.h>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"

ByunJRBot::ByunJRBot()
    : m_map(*this)
    , m_bases(*this)
    , m_unitInfo(*this)
    , m_workers(*this)
    , m_productionManager(*this)
    , m_scoutManager(*this)
    , m_proxyManager(*this)
    , m_combatCommander(*this)
    , m_strategy(*this)
    , m_informationManager(*this)
    , m_debug(*this)
    , m_isWillingToFight(true)
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
            m_playerRace[(int)PlayerArrayIndex::Self] = playerInfo.race_actual;
        }
        else
        {
            m_playerRace[(int)PlayerArrayIndex::Enemy] = playerInfo.race_requested;
        }
    }

    m_strategy.onStart();
    m_map.onStart();
    m_unitInfo.onStart();
    m_bases.onStart();
    m_workers.onStart();

    m_productionManager.onStart();
    m_scoutManager.onStart();
    m_proxyManager.onStart();
    m_combatCommander.onStart();
}

void ByunJRBot::OnStep()
{
    Control()->GetObservation();

    m_map.onFrame();
    m_unitInfo.onFrame();
    m_bases.onFrame();
    m_workers.onFrame();
    m_strategy.onFrame();

    m_strategy.handleUnitAssignments();

    m_productionManager.onFrame();
    m_scoutManager.onFrame();
    m_proxyManager.onFrame();
    m_combatCommander.onFrame(m_informationManager.GetCombatUnits());


    m_debug.drawAllUnitInformation();

    m_debug.drawDebugInterface();

    Debug()->SendDebug();
}

void ByunJRBot::OnUnitCreated(const sc2::Unit* unit) {
    m_proxyManager.onUnitCreated(*unit);
}

//void ByunJRBot::onUnitDestroy(const sc2::Unit & unit)
//{
//    //_productionManager.onUnitDestroy(unit);
//}

void ByunJRBot::OnUnitEnterVision(const sc2::Unit* unit) {
    m_proxyManager.onUnitEnterVision(*unit);
}

void ByunJRBot::OnBuildingConstructionComplete(const sc2::Unit* unit) {
    m_productionManager.onBuildingConstructionComplete(*unit);
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
const sc2::Race & ByunJRBot::GetPlayerRace(PlayerArrayIndex player) const
{
    BOT_ASSERT(player == PlayerArrayIndex::Self || player == PlayerArrayIndex::Enemy, "invalid player for GetPlayerRace");
    return m_playerRace[(int) player];
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

InformationManager & ByunJRBot::InformationManager()
{
    return m_informationManager;
}

const BaseLocationManager & ByunJRBot::Bases() const
{
    return m_bases;
}

ScoutManager & ByunJRBot::Scout()
{
    return m_scoutManager;
}

const UnitInfoManager & ByunJRBot::UnitInfoManager() const
{
    return m_unitInfo;
}

WorkerManager & ByunJRBot::Workers()
{
    return m_workers;
}

ProxyManager & ByunJRBot::GetProxyManager() {
    return m_proxyManager;
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