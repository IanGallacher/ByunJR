#pragma once

#include <sc2api/sc2_api.h>

#include "TechLab/InformationManager.h"
#include "TechLab/information/BaseLocationManager.h"
#include "TechLab/util/Debug.h"
#include "TechLab/information/MapTools.h"

#include "StrategyManager.h"
#include "global/BotConfig.h"
#include "macro/BuildingManager.h"
#include "macro/ProductionManager.h"
#include "micro/ProxyManager.h"
#include "macro/WorkerManager.h"
#include "micro/ScoutManager.h"
#include "micro/CombatCommander.h"

#define DllExport   __declspec( dllexport )  

class ByunJRBot : public sc2::Agent 
{
    CombatCommander          combat_commander_;
    InformationManager       information_manager_;
    DebugManager             debug_;

    StrategyManager          strategy_;
    BotConfig                config_;

    ProductionManager        production_manager_;
    ScoutManager             scout_manager_;
    ProxyManager             proxy_manager_;
    WorkerManager            workers_;

    bool                     is_willing_to_fight_;
    int                      frame_skip_;

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
    const ProductionManager & ProductionManager() const;
    ScoutManager & Scout();
    DebugManager& DebugHelper();
    InformationManager & InformationManager();
    ProxyManager & GetProxyManager();
    StrategyManager & Strategy();
    sc2::Point2D GetStartLocation() const;
};

void DllExport *CreateNewAgent();

int DllExport GetAgentRace();

const char DllExport *GetAgentName();