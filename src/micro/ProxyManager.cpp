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

// The map must be set up before seting up the ProxyManager.
void ProxyManager::OnStart()
{
    first_reaper_created_ = false;
    ptd_.InitAllValues(bot_);
}

void ProxyManager::OnFrame()
{
    MoveProxyWorkers();
}

void ProxyManager::OnUnitCreated(const sc2::Unit* unit)
{
    if (bot_.Config().TrainingMode && unit->unit_type == sc2::UNIT_TYPEID::TERRAN_REAPER && !first_reaper_created_)
    {
        const BaseLocation* enemy_base_location = bot_.Info().Bases().GetPlayerStartingBaseLocation(sc2::Unit::Alliance::Enemy);

        bot_.Resign();
        ptd_.RecordResult(static_cast<int>(bot_.Query()->PathingDistance(unit, enemy_base_location->GetPosition())));
        first_reaper_created_ = true;
    }
}

void ProxyManager::OnUnitEnterVision(const sc2::Unit* enemy_unit)
{
    if (!proxy_worker_) return;
    // TODO: Optimize this code to only search buildings, not every single unit a player owns.
    for (auto & unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
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
bool ProxyManager::MoveProxyWorkers()
{
    if (!ptd_.ProxyLocationReady())
        return false;

    //const sc2::Point2DI my_vec(ptd_.GetProxyLocation());
    //if (!proxy_worker_)
    //{
    //    Building b(sc2::UNIT_TYPEID::TERRAN_BARRACKS);
    //    const std::vector<UnitMission> acceptable_missions{ UnitMission::Idle, UnitMission::Minerals, UnitMission::Proxy };
    //    proxy_worker_ = bot_.Info().GetClosestUnitWithJob(sc2::Point2D(my_vec.x, my_vec.y), acceptable_missions);

    //    if (proxy_worker_)
    //        bot_.Info().UnitInfo().SetJob(proxy_worker_, UnitMission::Proxy);
    //}

    //for (const auto & unit : bot_.Info().UnitInfo().GetWorkers())
    //{
    //    if(unit->mission == UnitMission::Proxy)
    //        Micro::SmartMove(unit->unit, sc2::Point2D(my_vec.x, my_vec.y), bot_);
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