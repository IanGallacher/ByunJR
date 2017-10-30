#pragma once
#include "macro/BuildOrder.h"

typedef std::pair<sc2::UnitTypeID, size_t>  UnitPair;
typedef std::vector<UnitPair>               UnitPairVector;

class ByunJRBot;

enum class Strategy { ReaperRush, BattlecruiserMacro};

struct StrategyBuildOrder
{
    std::string name;
    sc2::Race   race;
    int         wins;
    int         losses;
    BuildOrder  buildOrder;

    StrategyBuildOrder();
    StrategyBuildOrder(const std::string & name, const sc2::Race & race, const BuildOrder & build_order);
};

class StrategyManager
{
    ByunJRBot & bot_;

    std::map<std::string, StrategyBuildOrder> strategies_;
    const BuildOrder                empty_build_order_;
    Strategy                        macro_goal_;
    // Have we sent the scout at the start of the game?
    bool                            initial_scout_set_;
    bool                            second_proxy_worker_set_;
    bool                            bases_safe_;
    mutable int                     times_expanded_ = 0;

    UnitPairVector GetProtossBuildOrderGoal() const;
    UnitPairVector GetTerranBuildOrderGoal() const;
    UnitPairVector GetZergBuildOrderGoal() const;

    // Functions called by HandleUnitAssignments.
    void SetScoutUnits();
    bool ShouldSendSecondProxyWorker() const;
    bool ShouldSendInitialScout() const;
    bool AreBasesSafe();

public:
    StrategyManager(ByunJRBot & bot);

    void OnStart();
    void OnFrame();
    void RecalculateMacroGoal();
    void AddStrategy(const std::string & name, const StrategyBuildOrder & strategy);
    UnitPairVector GetBuildOrderGoal() const;
    const BuildOrder & GetOpeningBookBuildOrder() const;
    void ReadStrategyFile(const std::string & str);

    void HandleUnitAssignments();
    bool ShouldExpandNow() const;
    Strategy MacroGoal() const;
};
