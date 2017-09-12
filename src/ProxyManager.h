#pragma once

#include "Common.h"

class CCBot;

class ProxyManager
{
    CCBot &   m_bot;
    UnitTag         m_proxyUnitTag;
    bool            m_proxyUnderAttack;
    // bool            loggedResult;

    // Proxy training
    bool            loadProxyTrainingData();
    void            upadateViableLocationsList();
    void            recordResult(int x, int y, int fitness);
    bool            isProxyLocationValid(int x, int y);
    void            testAllPointsOnMap();
    bool            setupProxyLocation();
    bool            proxyBuildingAtChosenRandomLocation();

public:
    ProxyManager(CCBot & bot);
    void onStart();
    void onFrame();
    void OnUnitEnterVision(const sc2::Unit & unit);
    bool writeAllTrainingData();

    sc2::Point2D    getProxyLocation();
};