#pragma once
#include "macro/BuildOrder.h"

typedef std::pair<sc2::UnitTypeID, size_t>  UnitPair;
typedef std::vector<UnitPair>               UnitPairVector;

class ByunJRBot;

struct Strategy
{
    std::string name;
    sc2::Race   race;
    int         wins;
    int         losses;
    BuildOrder  buildOrder;

    Strategy();
    Strategy(const std::string & name, const sc2::Race & race, const BuildOrder & build_order);
};

class StrategyManager
{
    ByunJRBot & bot_;

    sc2::Race                       self_race_;
    sc2::Race                       enemy_race_;
    std::map<std::string, Strategy> strategies_;
    int                             total_games_played_;
    const BuildOrder                empty_build_order_;

    bool  ShouldExpandNow() const;
    UnitPairVector GetProtossBuildOrderGoal() const;
    UnitPairVector GetTerranBuildOrderGoal() const;
    UnitPairVector GetZergBuildOrderGoal() const;

public:

    StrategyManager(ByunJRBot & bot);

    void OnStart();
    void OnFrame();
    void AddStrategy(const std::string & name, const Strategy & strategy);
    UnitPairVector GetBuildOrderGoal() const;
    const BuildOrder & GetOpeningBookBuildOrder() const;
    void ReadStrategyFile(const std::string & str);


    void HandleUnitAssignments();

    bool ShouldSendInitialScout() const;
};
