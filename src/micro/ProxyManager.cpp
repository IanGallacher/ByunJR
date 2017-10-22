#include <sc2lib/sc2_lib.h>
#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "macro/Building.h"
#include "micro/ProxyManager.h"
#include "micro/Micro.h"

ProxyManager::ProxyManager(ByunJRBot & bot)
    : bot_(bot)
    , proxy_worker_(nullptr)
    , proxy_under_attack_(false)
{

}

void ProxyManager::OnStart()
{
    first_reaper_created_ = false;
    ptd_.InitAllValues(bot_);
}

void ProxyManager::OnFrame()
{
    ProxyBuildingAtChosenRandomLocation();
}

void ProxyManager::OnUnitCreated(const sc2::Unit* unit)
{
    if (bot_.Config().TrainingMode && unit->unit_type == sc2::UNIT_TYPEID::TERRAN_REAPER && !first_reaper_created_)
    {
        const BaseLocation* enemy_base_location = bot_.Bases().GetPlayerStartingBaseLocation(PlayerArrayIndex::Enemy);

        bot_.Resign();
        ptd_.RecordResult(static_cast<int>(bot_.Query()->PathingDistance(unit, enemy_base_location->GetPosition())));
        first_reaper_created_ = true;
    }
}

void ProxyManager::OnUnitEnterVision(const sc2::Unit* enemy_unit)
{
    if (!proxy_worker_) return;
    // TODO: Optimize this code to only search buildings, not every single unit a player owns.
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BARRACKS || unit->tag == proxy_worker_->tag)
        {
            const double dist(sqrt((enemy_unit->pos.x - unit->pos.x)*(enemy_unit->pos.x - unit
                ->pos.x) + (enemy_unit->pos.y - unit->pos.y)*(enemy_unit->pos.y - unit->pos.y)));

            if (bot_.Config().TrainingMode && dist < 10 && !first_reaper_created_)
            {
                bot_.Resign();
                ptd_.RecordResult(-9);
                std::cout << "THERE IS NO POINT IN CONTINUING" << std::endl;
            }
        }
    }
}

// YOU MUST CALL ptd.InitAllValues() before this.
bool ProxyManager::ProxyBuildingAtChosenRandomLocation()
{
    if (!ptd_.ProxyLocationReady())
        return false;

    //if (proxyWorker->pos.x > myVec.x - 1 && proxyWorker->pos.x < myVec.x + 1)
    //{
    //    bot_.Workers().finishedWithWorker(proxyWorker);
    //}
    //else
    //{
    if (!proxy_worker_)
    {
        const sc2::Point2DI my_vec(ptd_.GetProxyLocation());
        Building b(sc2::UNIT_TYPEID::TERRAN_BARRACKS, my_vec);
        proxy_worker_ = bot_.GetUnit(bot_.InformationManager().GetBuilder(b, false));
        if(!proxy_worker_)
        {
            std::cout << "WARNING: PROXY WORKER WAS NOT FOUND." << std::endl;
            return false;
        }
        bot_.InformationManager().UnitInfo().SetJob(proxy_worker_, UnitMission::Proxy);
        Micro::SmartMove(proxy_worker_, sc2::Point2D(my_vec.x, my_vec.y), bot_);
    }
    //}

    return true;
}

sc2::Point2DI ProxyManager::GetProxyLocation()
{
    return ptd_.GetProxyLocation();
}

ProxyTrainingData & ProxyManager::GetProxyTrainingData()
{
    return ptd_;
}