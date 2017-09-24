#pragma once

#include "Common.h"
#include "MapTools.h"

class ByunJRBot;


class ProxyLocation {
public:
    sc2::Point2D m_loc;
    int m_fitness;

    ProxyLocation(sc2::Point2D loc, int fitness)
    {
        m_loc = loc;
        m_fitness = fitness;
    }
};

// ProxyTrainingData is for a specific map ONLY.
class ProxyTrainingData
{
    int m_proxy_x;
    int m_proxy_y;

    int m_playerStart_y;
    int m_enemyStart_y;

    int m_arena_width;
    int m_arena_height;

    const MapTools* m_map;

    sc2::Point2D m_playable_min;
    sc2::Point2D m_playable_max;

    // There is a subtle difference between result and ViableLocations.
    // Result is a vector of vectors that represent ALL points on the map. 
    // ViableLocations is an UNINDEXED list that does not include places that get scouted easily or are impossible to build on.
    // When picking a random proxy location, ViableLocations is used to make sure that the location we pick is always viable. 
    std::vector<std::vector<int>> m_result;  // stored in the format result[y][x]
    std::vector<ProxyLocation> ViableLocations;



    bool            loadProxyTrainingData();
    void            testAllPointsOnMap();
    bool            setupProxyLocation();

public:
    void InitAllValues(ByunJRBot & bot);

    // Proxy training
    void            upadateViableLocationsList();
    bool            isProxyLocationValid(int x, int y);
    void            recordResult(int fitness);
    void            writeAllTrainingData(std::string filename);

    sc2::Point2D    getProxyLocation();
};

class ProxyManager
{
    ByunJRBot &     m_bot;
    UnitTag         m_proxyUnitTag;
    bool            m_proxyUnderAttack;
    bool            m_firstReaperCreated;
    ProxyTrainingData        m_ptd;
    // bool            loggedResult;


public:
    ProxyManager(ByunJRBot & bot);
    void onStart();
    void onFrame();
    void onUnitCreated(const sc2::Unit & unit);
    void onUnitEnterVision(const sc2::Unit & unit);
    void writeAllTrainingData();
    bool proxyBuildingAtChosenRandomLocation();

    sc2::Point2D    getProxyLocation();
};