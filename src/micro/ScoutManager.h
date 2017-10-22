#pragma once
#include <sc2api/sc2_api.h>

class ByunJRBot;

class ScoutManager
{
    ByunJRBot &                       bot_;
    std::string                       scout_status_;
    int                               num_scouts_;
    bool                              scout_under_attack_;
    float                             previous_scout_hp_;

    bool                              EnemyWorkerInRadiusOf(const sc2::Point2D & pos) const;
    sc2::Point2D                      GetFleePosition() const;
    const sc2::Unit*                  ClosestEnemyWorkerTo(const sc2::Unit* scout) const;
    void                              MoveScouts();
    void                              DrawScoutInformation() const;

public:
    ScoutManager(ByunJRBot & bot);

    static void OnStart();
    void OnFrame();
};