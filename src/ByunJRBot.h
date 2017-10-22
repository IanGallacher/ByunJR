#pragma once

#include <sc2api/sc2_api.h>

#include "InformationManager.h"
#include "global/BotConfig.h"
#include "global/Debug.h"
#include "StrategyManager.h"
#include "macro/BuildingManager.h"
#include "macro/ProductionManager.h"
#include "micro/ProxyManager.h"
#include "macro/WorkerManager.h"
#include "micro/ScoutManager.h"
#include "micro/CombatCommander.h"
#include "information/BaseLocationManager.h"
#include "util/MapTools.h"

#define DllExport   __declspec( dllexport )  

class ByunJRBot : public sc2::Agent 
{
    CombatCommander          combat_commander_;
    InformationManager       information_manager_;

    MapTools                 map_;
    BaseLocationManager      bases_;
    StrategyManager          strategy_;
    BotConfig                config_;

    ProductionManager        production_manager_;
    ScoutManager             scout_manager_;
    ProxyManager             proxy_manager_;
    DebugManager             debug_;
    WorkerManager            workers_;

    bool                     is_willing_to_fight_;

    void OnError(const std::vector<sc2::ClientError> & client_errors,
                 const std::vector<std::string> & protocol_errors = {});

public:

    ByunJRBot();
    void OnGameStart() override;
    void OnStep() override;
    void OnUnitCreated(const sc2::Unit*) override;
    void OnUnitDestroyed(const sc2::Unit*) override;
    void OnUnitEnterVision(const sc2::Unit*) override;
    void OnBuildingConstructionComplete(const sc2::Unit*) override;
    bool IsWillingToFight() const;
    void Resign();

          BotConfig & Config();
    const BaseLocationManager & Bases() const;
    ScoutManager & Scout();
    DebugManager& DebugHelper();
    InformationManager & InformationManager();
    const MapTools & Map() const;
    ProxyManager & GetProxyManager();
    const StrategyManager & Strategy() const;
    sc2::Point2D GetStartLocation() const;
};

void DllExport *CreateNewAgent();

int DllExport GetAgentRace();

const char DllExport *GetAgentName();