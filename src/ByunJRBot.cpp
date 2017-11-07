#include <sstream>
#include <sc2api/sc2_api.h>

#include "ByunJRBot.h"
#include "common/BotAssert.h"

ByunJRBot::ByunJRBot()
    : combat_commander_(*this)
    , information_manager_(*this)
    , strategy_(*this)
    , production_manager_(*this)
    , scout_manager_(*this)
    , proxy_manager_(*this)
    , debug_(*this)
    , workers_(*this)
    , is_willing_to_fight_(true)
    , frame_skip_(0)
{
}

void ByunJRBot::OnGameStart() 
{
    config_.ReadConfigFile();
    config_.MapName = Observation()->GetGameInfo().local_map_path;
    // Ignore file extension of the local_map_path.
    config_.MapName = config_.MapName.substr(0, config_.MapName.find('.'));

    information_manager_.OnStart();

    strategy_.OnStart();

    production_manager_.OnStart();
    scout_manager_.OnStart();
    proxy_manager_.OnStart();
    combat_commander_.OnStart();
}

void ByunJRBot::OnStep()
{
    frame_skip_++;
    if (frame_skip_ % 2) return;
    Control()->GetObservation();

    information_manager_.OnFrame();
    strategy_.OnFrame();

    strategy_.OnFrame();
    workers_.OnFrame();

    production_manager_.OnFrame();
    scout_manager_.OnFrame();
    proxy_manager_.OnFrame();
    combat_commander_.OnFrame(information_manager_.UnitInfo().GetCombatUnits());


    debug_.DrawAllUnitInformation();
    debug_.DrawResourceDebugInfo();
    debug_.DrawDebugInterface();


    //debug_.DrawEnemyDPSMap(information_manager_.GetDPSMap());

    if (config_.DrawWalkableSectors)
        debug_.DrawMapSectors();

    if (config_.DrawTileInfo)
        debug_.DrawMapWalkableTiles();

    if (config_.DrawBaseLocationInfo)
    {
        return;
    }

    Debug()->SendDebug();
}

void ByunJRBot::OnUnitCreated(const sc2::Unit* unit) {
    proxy_manager_.OnUnitCreated(unit);
    information_manager_.OnUnitCreated(unit);
    combat_commander_.OnUnitCreated(unit);
}

void ByunJRBot::OnUnitDestroyed(const sc2::Unit* unit)
{
    information_manager_.OnUnitDestroyed(unit);
    production_manager_.OnUnitDestroyed(unit);
}

void ByunJRBot::OnUnitEnterVision(const sc2::Unit* unit) {
    proxy_manager_.OnUnitEnterVision(unit);
}

void ByunJRBot::OnBuildingConstructionComplete(const sc2::Unit* unit) {
    production_manager_.OnBuildingConstructionComplete(unit);
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

StrategyManager & ByunJRBot::Strategy()
{
    return strategy_;
}

InformationManager & ByunJRBot::InformationManager()
{
    return information_manager_;
}

const ProductionManager & ByunJRBot::ProductionManager() const
{
    return production_manager_;
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