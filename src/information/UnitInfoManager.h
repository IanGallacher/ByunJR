#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitData.h"

class ByunJRBot;
class UnitInfoManager 
{
    ByunJRBot &           m_bot;

    std::map<PlayerArrayIndex, UnitData> m_unitData;

    std::map<PlayerArrayIndex, std::vector<sc2::Unit>> m_units;

    void                    updateUnit(const sc2::Unit & unit);
    void                    updateUnitInfo();
    bool                    isValidUnit(const sc2::Unit & unit);
    
    const UnitData &        getUnitData(PlayerArrayIndex player) const;

    void drawSelectedUnitDebugInfo();

public:

    UnitInfoManager(ByunJRBot & bot);

    void                    onStart();
    void                    onFrame();
    void                    onUnitDestroyed(const sc2::Unit& unit);

    const std::vector<sc2::Unit> & getUnits(PlayerArrayIndex player) const;

    size_t                  getUnitTypeCount(PlayerArrayIndex player, sc2::UnitTypeID type, bool completed = true) const;

    void                    getNearbyForce(std::vector<UnitInfo>& unitInfo, sc2::Point2D p, PlayerArrayIndex player, float radius) const;

    const std::map<int, UnitInfo> & getUnitInfoMap(PlayerArrayIndex player) const;

    //bool                  enemyHasCloakedUnits() const;
    void                    drawUnitInformation(float x, float y) const;
    void                    setJob(const sc2::Unit & unit, UnitMission job);
};