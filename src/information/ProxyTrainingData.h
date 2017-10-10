#pragma once
#include <sc2api/sc2_api.h>

#include "common/Common.h"

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
    TilePos m_proxy_loc;
    TilePos m_best_proxy_loc;
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
    
    TilePos ProxyTrainingData::flipCoordinatesIfNecessary(int x, int y);

    std::string     getTrainingDataFileName();
    bool            loadProxyTrainingData();
    void            testAllPointsOnMap();
    void            reduceSearchSpace(int reductionFactor);

public:
    void            InitAllValues(ByunJRBot & bot);
    bool            setupProxyLocation();
    bool            proxyLocationReady() const;

    // Proxy training
    void            upadateViableLocationsList();
    bool            isProxyLocationValid(int x, int y) const;
    void            recordResult(int fitness);
    void            writeAllTrainingData(std::string filename);

    sc2::Point2D    getProxyLocation();
    sc2::Point2D    getBestProxyLocation();
    int             getReward();
    sc2::Point2D    getNearestUntestedProxyLocation(int x, int y);
    sc2::Point2D    getRandomViableProxyLocation();
};
