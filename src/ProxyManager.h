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
    int m_best_proxy_x;
    int m_best_proxy_y;
    ByunJRBot* m_bot;

    sc2::Point2D m_playable_min;
    sc2::Point2D m_playable_max;
    int m_arena_height;
    int m_arena_width;
    int m_enemyStart_y;
    int m_playerStart_y;



    enum MapDataValue : int {
        FailedLocation = -9,
        IgnoredLocationToSaveSearchSpace = -2,
        UnbuildableLocation = -1,
        LocationWithoutResultValue = 0
    };

    // There is a subtle difference between m_result and m_viableLocations.
    // Result is a vector of vectors that represent ALL points on the map. 
    // m_viableLocations is an UNINDEXED list that does not include places that get scouted easily or are impossible to build on.
    // When picking a random proxy location, m_viableLocations is used to make sure that the location we pick is always viable. 
    std::vector<ProxyLocation> m_viableLocations;

    // Using <MapDataValue> for m_result was considered, but results (the distance from the reaper spawn point to the enemy base)
    // do not implicity cast to MapDataValue. There is no MapDataValue that represents 149 tiles worth of distance for example. 
    // <int> is used instead to avoid strange typecasts, but please use a MapDataValue to store data whenever possible. 
    std::vector<std::vector<int>> m_result;  // stored in the format result[y][x]




    std::string     getTrainingDataFileName();
    bool            loadProxyTrainingData();
    void            testAllPointsOnMap();
    void            reduceSearchSpace(int reductionFactor);

public:
    void            InitAllValues(ByunJRBot & bot);
    bool            setupProxyLocation();
    bool            proxyLocationReady();

    // Proxy training
    void            upadateViableLocationsList();
    bool            isProxyLocationValid(int x, int y);
    void            recordResult(int fitness);
    void            writeAllTrainingData(std::string filename);

    sc2::Point2D    getProxyLocation();
    sc2::Point2D    getBestProxyLocation();
    int             getReward();
    sc2::Point2D    getNearestUntestedProxyLocation(int x, int y);
    sc2::Point2D    getRandomViableProxyLocation();
};

class ProxyManager
{
    ByunJRBot &     m_bot;
    sc2::Tag         m_proxyUnitTag;
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
    bool proxyBuildingAtChosenRandomLocation();

    sc2::Point2D getProxyLocation();
    ProxyTrainingData& getProxyTrainingData();
};