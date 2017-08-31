#pragma once

#include "Common.h"

class CCBot;

class ProxyManager
{
    CCBot &   m_bot;
    UnitTag         m_proxyUnitTag;
    bool            m_proxyUnderAttack;

    // Proxy training
    bool            loadProxyTrainingData();
    bool            writeProxyTrainingData(int x, int y, int fitness);
    bool            isProxyLocationValid(int x, int y);
    bool            setupProxyLocation();
    bool            proxyBuildingAtRandomLocation();

public:
    ProxyManager(CCBot & bot);
    void onStart();
    void onFrame();

    sc2::Point2D    getProxyLocation();
};