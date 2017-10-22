#include <sc2api/sc2_api.h>

#include "ByunJRBot.h"
#include "InformationManager.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "information/unitInfo.h"
#include "util/Util.h"

InformationManager::InformationManager(ByunJRBot & bot)
    : bot_(bot)
    , unit_info_(bot)
    , initial_scout_set_(false)
{

}

void InformationManager::OnStart()
{
    unit_info_.OnStart();
}

void InformationManager::OnUnitCreated(const sc2::Unit* unit)
{
}

void InformationManager::OnUnitDestroyed(const sc2::Unit* unit)
{
    unit_info_.OnUnitDestroyed(unit);
}

void InformationManager::OnFrame()
{
    unit_info_.OnFrame();
}

UnitInfoManager & InformationManager::UnitInfo()
{
    return unit_info_;
}

sc2::Point2DI InformationManager::GetProxyLocation() const
{
    return bot_.GetProxyManager().GetProxyLocation();
}

void InformationManager::assignUnit(const sc2::Tag & unit, UnitMission job)
{
    // Remove unit from any existing jobs. 
    finishedWithUnit(unit);
    if (job == UnitMission::Scout)
    {
        unit_info_.SetJob(bot_.GetUnit(unit), UnitMission::Scout);
        scout_units_.push_back(unit);
    }
    else
    {
        unit_info_.SetJob(bot_.GetUnit(unit), job);
    }
}

void InformationManager::finishedWithUnit(const sc2::Tag & unit)
{
    if (std::find(scout_units_.begin(), scout_units_.end(), unit) != scout_units_.end())
    {
        scout_units_.erase(std::remove(scout_units_.begin(), scout_units_.end(), unit), scout_units_.end());
    }
}

void InformationManager::SetScoutUnits(const bool shouldSendInitialScout)
{
    // if we haven't set a scout unit, do it
    if (scout_units_.empty() && !initial_scout_set_)
    {
        // if it exists
        if (shouldSendInitialScout)
        {
            // grab the closest worker to the supply provider to send to scout
            const ::UnitInfo * workerScout = GetClosestUnitWithJob(bot_.GetStartLocation(), UnitMission::Minerals);

            // if we find a worker (which we should) add it to the scout units
            if (workerScout)
            {
                bot_.Scout().SetWorkerScout(workerScout->unit->tag);

                assignUnit(workerScout->unit->tag, UnitMission::Scout);
                initial_scout_set_ = true;
            }
            else
            {

            }
        }
    }
}

sc2::Tag InformationManager::GetBuilder(Building & b, bool setJobAsBuilder)
{
    const std::vector<UnitMission> acceptableMissions{ UnitMission::Minerals, UnitMission::Proxy };
    const sc2::Tag builderWorker = GetClosestUnitTagWithJob(sc2::Point2D(b.finalPosition.x, b.finalPosition.y), acceptableMissions );

    // if the worker exists (one may not have been found in rare cases)
    if (builderWorker && setJobAsBuilder)
    {
        unit_info_.SetJob(bot_.GetUnit(builderWorker), UnitMission::Build);
    }

    return builderWorker;
}

const sc2::Unit* InformationManager::GetClosestUnitOfType(const sc2::Unit* referenceUnit,
                                                          const sc2::UnitTypeID referenceTypeID) const
{
    const sc2::Unit* closestUnit = nullptr;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        if (unit->unit_type == referenceTypeID)
        {
            const double distance = Util::DistSq(unit->pos, referenceUnit->pos);
            if (!closestUnit || distance < closestDistance)
            {
                closestUnit = unit;
                closestDistance = distance;
            }
        }
    }

    return closestUnit;
}

// Does not look for flying bases. Only landed bases. 
const sc2::Unit* InformationManager::GetClosestBase(const sc2::Unit* referenceUnit) const
{
    const sc2::Unit* closestUnit = nullptr;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER
        ||  unit->unit_type == sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND
        ||  unit->unit_type == sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS
        ||  unit->unit_type == sc2::UNIT_TYPEID::PROTOSS_NEXUS
        ||  unit->unit_type == sc2::UNIT_TYPEID::ZERG_HATCHERY
        ||  unit->unit_type == sc2::UNIT_TYPEID::ZERG_LAIR
        ||  unit->unit_type == sc2::UNIT_TYPEID::ZERG_HIVE)
        {
            const double distance = Util::DistSq(unit->pos, referenceUnit->pos);
            if (!closestUnit || distance < closestDistance)
            {
                closestUnit = unit;
                closestDistance = distance;
            }
        }
    }

    return closestUnit;
}

const ::UnitInfo * InformationManager::GetClosestUnitWithJob(const sc2::Point2D referencePoint, const UnitMission unitMission) const
{
    const ::UnitInfo * closestUnit = nullptr;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto & unitInfoPair : bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self))
    {
        const ::UnitInfo & unitInfo = unitInfoPair.second;
        if (unitInfo.mission == unitMission)
        {
            const double distance = Util::DistSq(referencePoint, unitInfo.unit->pos);
            if (!closestUnit || distance < closestDistance)
            {
                closestUnit = &unitInfo;
                closestDistance = distance;
            }
        }
    }

    return closestUnit;
}

sc2::Tag InformationManager::GetClosestUnitTagWithJob(const sc2::Point2D point, const UnitMission mission) const
{
    sc2::Tag closestUnit;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto & unitInfo : bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self))
    {
        if (unitInfo.second.mission == mission)
        {
            const double distance = Util::DistSq(unitInfo.second.unit->pos, point);
            if (distance < closestDistance)
            {
                closestUnit = unitInfo.second.unit->tag;
                closestDistance = distance;
            }
        }
    }

    return closestUnit;
}


const sc2::Tag InformationManager::GetClosestUnitTagWithJob(const sc2::Point2D point, const std::vector<UnitMission> missionVector) const
{
    sc2::Tag closestUnit;
    double closestDistance = std::numeric_limits<double>::max();

    for (auto & unitInfo : bot_.InformationManager().UnitInfo().GetUnitInfoMap(PlayerArrayIndex::Self))
    {
        if (std::find(missionVector.begin(), missionVector.end(), unitInfo.second.mission) != missionVector.end())
        {
            const double distance = Util::DistSq(unitInfo.second.unit->pos, point);
            if (distance < closestDistance)
            {
                closestUnit = unitInfo.second.unit->tag;
                closestDistance = distance;
            }
        }
    }

    return closestUnit;
}

void InformationManager::HandleUnitAssignments()
{
    // set each type of unit
    SetScoutUnits(bot_.Strategy().shouldSendInitialScout());
}
