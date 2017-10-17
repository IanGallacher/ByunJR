#pragma once
#include "information/ProxyTrainingData.h"

class ByunJRBot;

class ProxyManager
{
    ByunJRBot &     m_bot;
    sc2::Tag        m_proxyUnitTag;
    bool            m_proxyUnderAttack;
    bool            m_firstReaperCreated;
    ProxyTrainingData        m_ptd;
    // bool            loggedResult;


public:
    ProxyManager(ByunJRBot & bot);
    void onStart();
    void onFrame();
    void onUnitCreated(const sc2::Unit* unit);
    void onUnitEnterVision(const sc2::Unit* unit);
    bool proxyBuildingAtChosenRandomLocation();

    sc2::Point2D getProxyLocation();
    ProxyTrainingData& getProxyTrainingData();
};