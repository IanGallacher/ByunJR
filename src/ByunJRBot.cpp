#include <sstream>
#include <sc2api/sc2_api.h>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"

ByunJRBot::ByunJRBot()
    : map_(*this)
    , bases_(*this)
    , production_manager_(*this)
    , scout_manager_(*this)
    , proxy_manager_(*this)
    , workers_(*this)
    , combat_commander_(*this)
    , strategy_(*this)
    , information_manager_(*this)
    , debug_(*this)
    , is_willing_to_fight_(true)
{
    
}

void ByunJRBot::OnGameStart() 
{
    config_.ReadConfigFile();
    config_.MapName = Observation()->GetGameInfo().local_map_path;
    // Ignore file extension of the local_map_path.
    config_.MapName = config_.MapName.substr(0, config_.MapName.find('.'));

    strategy_.OnStart();
    map_.OnStart();
    information_manager_.OnStart();
    bases_.OnStart();

    production_manager_.onStart();
    scout_manager_.OnStart();
    proxy_manager_.OnStart();
    combat_commander_.OnStart();
}

void ByunJRBot::OnStep()
{
    Control()->GetObservation();

    map_.OnFrame();
    information_manager_.OnFrame();
    strategy_.HandleUnitAssignments();

    bases_.OnFrame();
    strategy_.OnFrame();
    workers_.OnFrame();

    production_manager_.OnFrame();
    scout_manager_.OnFrame();
    proxy_manager_.OnFrame();
    combat_commander_.OnFrame(information_manager_.UnitInfo().GetCombatUnits());


    debug_.DrawAllUnitInformation();
    debug_.DrawResourceDebugInfo();
    debug_.DrawDebugInterface();

    Debug()->SendDebug();
}

void ByunJRBot::OnUnitCreated(const sc2::Unit* unit) {
    proxy_manager_.OnUnitCreated(unit);
    information_manager_.OnUnitCreated(unit);
}

void ByunJRBot::OnUnitDestroyed(const sc2::Unit* unit)
{
    information_manager_.OnUnitDestroyed(unit);
}

//void ByunJRBot::onUnitDestroy(const sc2::Unit* unit)
//{
//    //_productionManager.onUnitDestroy(unit);
//}

void ByunJRBot::OnUnitEnterVision(const sc2::Unit* unit) {
    proxy_manager_.OnUnitEnterVision(unit);
}

void ByunJRBot::OnBuildingConstructionComplete(const sc2::Unit* unit) {
    production_manager_.onBuildingConstructionComplete(unit);
}

// Returns true if the bot thinks it still has a chance.
// Return false if there is no point continuing the simulation.
bool ByunJRBot::IsWillingToFight() const
{
    return is_willing_to_fight_;
}

void ByunJRBot::Resign()
{
    is_willing_to_fight_ = false;
}

BotConfig & ByunJRBot::Config()
{
     return config_;
}

const MapTools & ByunJRBot::Map() const
{
    return map_;
}

const StrategyManager & ByunJRBot::Strategy() const
{
    return strategy_;
}

InformationManager & ByunJRBot::InformationManager()
{
    return information_manager_;
}

const BaseLocationManager & ByunJRBot::Bases() const
{
    return bases_;
}

ScoutManager & ByunJRBot::Scout()
{
    return scout_manager_;
}

DebugManager & ByunJRBot::DebugHelper()
{
    return debug_;
}

ProxyManager & ByunJRBot::GetProxyManager() {
    return proxy_manager_;
}

const sc2::Unit* ByunJRBot::GetUnit(const sc2::Tag & tag) const
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


#pragma region Functions for use on ladder
void *CreateNewAgent()
{
    return (void *) new ByunJRBot();
}

int GetAgentRace()
{
    return sc2::Race::Terran;
}

const char *GetAgentName()
{
    return "ByunJR";
}
#pragma endregion