#pragma once

#include "sc2api/sc2_api.h"

#include "MapTools.h"
#include "BaseLocationManager.h"
#include "UnitInfoManager.h"
#include "WorkerManager.h"
#include "BotConfig.h"
#include "GameCommander.h"
#include "BuildingManager.h"
#include "StrategyManager.h"

class ByunJRBot : public sc2::Agent 
{
    sc2::Race               m_playerRace[2];

    MapTools                m_map;
    BaseLocationManager     m_bases;
    UnitInfoManager         m_unitInfo;
    WorkerManager           m_workers;
    StrategyManager         m_strategy;
    BotConfig               m_config;

    GameCommander           m_gameCommander;
    bool                    m_isWillingToFight;

    void OnError(const std::vector<sc2::ClientError> & client_errors,
                 const std::vector<std::string> & protocol_errors = {});

public:

    ByunJRBot();
    void OnGameStart() override;
    void OnStep() override;
    void OnUnitCreated(const sc2::Unit*) override;
    void OnUnitEnterVision(const sc2::Unit*) override;
    void OnBuildingConstructionComplete(const sc2::Unit*) override;
    bool IsWillingToFight();
    void Resign();
    sc2::Point2D GetProxyLocation();

          BotConfig & Config();
          WorkerManager & Workers();
    const BaseLocationManager & Bases() const;
    const MapTools & Map() const;
    const UnitInfoManager & UnitInfo() const;
    const StrategyManager & Strategy() const;
    const sc2::Race & GetPlayerRace(int player) const;
    sc2::Point2D GetStartLocation() const;
    const sc2::Unit * GetUnit(const UnitTag & tag) const;
};