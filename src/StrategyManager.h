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
    Strategy(const std::string & name, const sc2::Race & race, const BuildOrder & buildOrder);
};

class StrategyManager
{
    ByunJRBot & bot_;

    sc2::Race                       selfRace;
    sc2::Race                       enemyRace;
    std::map<std::string, Strategy> strategies;
    int                             totalGamesPlayed;
    const BuildOrder                emptyBuildOrder;

    bool  shouldExpandNow() const;
    UnitPairVector getProtossBuildOrderGoal() const;
    UnitPairVector getTerranBuildOrderGoal() const;
    UnitPairVector getZergBuildOrderGoal() const;

public:

    StrategyManager(ByunJRBot & bot);

    void onStart();
    void OnFrame();
    void onEnd(const bool isWinner);
    void addStrategy(const std::string & name, const Strategy & strategy);
    UnitPairVector getBuildOrderGoal() const;
    const BuildOrder & getOpeningBookBuildOrder() const;
    void readStrategyFile(const std::string & str);


    void handleUnitAssignments();

    bool shouldSendInitialScout() const;
};
