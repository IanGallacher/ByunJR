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
    sc2::Race                m_playerRace[2];

    CombatCommander          m_combatCommander;
    InformationManager       m_informationManager;

    MapTools                 m_map;
    BaseLocationManager      m_bases;
    StrategyManager          m_strategy;
    BotConfig                m_config;

    ProductionManager        m_productionManager;
    ScoutManager             m_scoutManager;
    ProxyManager             m_proxyManager;
    DebugManager             m_debug;
	WorkerManager            m_workers;

    bool                     m_isWillingToFight;

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
    bool IsWillingToFight();
    void Resign();

          BotConfig & Config();
    const BaseLocationManager & Bases() const;
    ScoutManager & Scout();
	InformationManager & InformationManager();
    const MapTools & Map() const;
    ProxyManager & GetProxyManager();
    const StrategyManager & Strategy() const;
    const sc2::Race & GetPlayerRace(PlayerArrayIndex player) const;
    sc2::Point2D GetStartLocation() const;
    const sc2::Unit* GetUnit(const sc2::Tag & tag) const;
};

void DllExport *CreateNewAgent();

int DllExport GetAgentRace();

const char DllExport *GetAgentName();