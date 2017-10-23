#pragma once
#include "information/ProxyTrainingData.h"

class ByunJRBot;

class ProxyManager
{
    ByunJRBot &          bot_;
    const sc2::Unit*     proxy_worker_;
    bool                 proxy_under_attack_;
    bool                 first_reaper_created_;
    ProxyTrainingData    ptd_;
    // bool              loggedResult;


public:
    ProxyManager(ByunJRBot & bot);
    void OnStart();
    void OnFrame();
    void OnUnitCreated(const sc2::Unit* unit);
    void OnUnitEnterVision(const sc2::Unit* unit);
    bool ProxyBuildingAtChosenRandomLocation();

    sc2::Point2DI GetProxyLocation();
    ProxyTrainingData& GetProxyTrainingData();
};