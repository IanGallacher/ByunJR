#pragma once
#include <sc2api/sc2_api.h>

#include "information/UnitInfoManager.h"

class ByunJRBot;

class InformationManager
{
    ByunJRBot &              m_bot;

    std::vector<sc2::Tag>    m_scoutUnits;
    UnitInfoManager          m_unitInfo;

    bool                     m_initialScoutSet;

	void setScoutUnits(bool shouldSendInitialScout);
	void setCombatUnits();

public:
    InformationManager(ByunJRBot & bot);
    void onStart();
    void onUnitCreated(const sc2::Unit* unit);
    void onUnitDestroyed(const sc2::Unit* unit);
    void onFrame();

    sc2::Point2D GetProxyLocation() const;
    UnitInfoManager & UnitInfo();

	sc2::Tag getBuilder(Building& b, bool setJobAsBuilder = true);
	void assignUnit(const sc2::Tag & unit, UnitMission job);
	void finishedWithUnit(const sc2::Tag& unit);

	const sc2::Unit* getClosestUnitOfType(const sc2::Unit* unit, const sc2::UnitTypeID) const;
	const sc2::Unit* getClosestBase(const sc2::Unit* referenceUnit) const;
	const ::UnitInfo* getClosestUnitWithJob(const sc2::Point2D point, const UnitMission) const;
	sc2::Tag getClosestUnitTagWithJob(const sc2::Point2D point, const UnitMission mission) const;
	const sc2::Tag getClosestUnitTagWithJob(const sc2::Point2D point, const std::vector<UnitMission> mission) const;

	void handleUnitAssignments();
};
