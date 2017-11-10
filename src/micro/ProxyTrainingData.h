#pragma once
#include <sc2api/sc2_api.h>

class ByunJRBot;

struct ProxyLocation {
    sc2::Point2DI loc;
    int fitness;
};

// ProxyTrainingData is for a specific map ONLY.
class ProxyTrainingData
{
    ByunJRBot* bot_;
    sc2::Point2DI proxy_loc_;
    sc2::Point2DI best_proxy_loc_;

    sc2::Point2D playable_min_;
    sc2::Point2D playable_max_;
    int enemy_start_y_;
    int player_start_y_;



    enum MapDataValue : int {
        FailedLocation = -9,
        IgnoredLocationToSaveSearchSpace = -2,
        UnbuildableLocation = -1,
        LocationWithoutResultValue = 0
    };

    // There is a subtle difference between result_ and viable_locations_.
    // Result is a vector of vectors that represent ALL points on the map. 
    // viableLocations is an UNINDEXED list that does not include places that get scouted easily or are impossible to build on.
    // When picking a random proxy location, viableLocations is used to make sure that the location we pick is always viable. 
    std::vector<ProxyLocation> viable_locations_;

    // Using <MapDataValue> for result_ was considered, but results (the distance from the reaper spawn point to the enemy base)
    // do not implicity cast to MapDataValue. There is no MapDataValue that represents 149 tiles worth of distance for example. 
    // <int> is used instead to avoid strange typecasts, but please use a MapDataValue to store data whenever possible. 
    std::vector<std::vector<int>> result_;  // stored in the format result[y][x]

    sc2::Point2DI ProxyTrainingData::FlipCoordinatesIfNecessary(int x, int y);

    std::string     GetTrainingDataFileName();
    bool            LoadProxyTrainingData();
    void            TestAllPointsOnMap();
    void            ReduceSearchSpace(int reduction_factor);

public:
    void            InitAllValues(ByunJRBot & bot);
    bool            SetupProxyLocation();
    bool            ProxyLocationReady() const;

    // Proxy training
    void            UpadateViableLocationsList();
    bool            IsProxyLocationValid(int x, int y) const;
    void            RecordResult(int fitness);
    void            WriteAllTrainingData(std::string filename);

    sc2::Point2DI GetProxyLocation();
    sc2::Point2DI GetBestProxyLocation();
    int             GetReward();
    sc2::Point2D    GetNearestUntestedProxyLocation(int x, int y);
    sc2::Point2DI GetRandomViableProxyLocation();
};
