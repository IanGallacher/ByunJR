#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "micro/ScoutManager.h"
#include "micro/Micro.h"
#include "util/Util.h"

ScoutManager::ScoutManager(ByunJRBot & bot)
    : bot_             (bot)
    , scout_unit_tag_    (0)
    , num_scouts_       (0)
    , scout_under_attack_(false)
    , scout_status_     ("None")
    , previous_scout_hp_ (0.0f)
{
}

void ScoutManager::OnStart()
{

}

void ScoutManager::OnFrame()
{
    MoveScouts();
    DrawScoutInformation();
}

void ScoutManager::SetWorkerScout(const sc2::Tag & tag)
{
    // if we have a previous worker scout, release it back to the worker manager
    if (scout_unit_tag_)
    {
        bot_.InformationManager().finishedWithUnit(scout_unit_tag_);
    }

    scout_unit_tag_ = tag;
    bot_.InformationManager().assignUnit(scout_unit_tag_, UnitMission::Scout);
}

void ScoutManager::DrawScoutInformation() const
{
    if (!bot_.Config().DrawScoutInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Scout Info: " << scout_status_;

    bot_.DebugHelper().DrawTextScreen(sc2::Point2D(0.1f, 0.6f), ss.str());
}

void ScoutManager::MoveScouts()
{
    if (!scout_unit_tag_) { return; }
    const sc2::Unit* worker_scout = bot_.GetUnit(scout_unit_tag_);
    if (!worker_scout || worker_scout->health <= 0) { return; }

    const float scout_hp = worker_scout->health + worker_scout->shield;

    if(scout_hp <= 10)
    {
        Micro::SmartMove(worker_scout, bot_.Bases().GetPlayerStartingBaseLocation(PlayerArrayIndex::Self)->GetPosition(), bot_);
        bot_.InformationManager().finishedWithUnit(scout_unit_tag_);
        scout_unit_tag_ = -1;
        return;
    }

    // get the enemy base location, if we have one
    const BaseLocation* enemy_base_location = bot_.Bases().GetPlayerStartingBaseLocation(PlayerArrayIndex::Enemy);

    int scout_distance_threshold = 20;

    // if we know where the enemy region is and where our scout is
    if (enemy_base_location)
    {
        int scout_distance_to_enemy = bot_.Map().GetGroundDistance(worker_scout->pos, enemy_base_location->GetPosition());
        const bool scout_in_range_ofenemy = enemy_base_location->ContainsPosition(worker_scout->pos);

        // we only care if the scout is under attack within the enemy region
        // this ignores if their scout worker attacks it on the way to their base
        if (scout_hp < previous_scout_hp_)
        {
            scout_under_attack_ = true;
        }

        if (scout_hp == previous_scout_hp_ && !EnemyWorkerInRadiusOf(worker_scout->pos))
        {
            scout_under_attack_ = false;
        }

        // if the scout is in the enemy region
        if (scout_in_range_ofenemy)
        {
            // get the closest enemy worker
            const sc2::Unit* closest_enemy_worker_unit = ClosestEnemyWorkerTo(worker_scout->pos);

            // if the worker scout is not under attack
            if (!scout_under_attack_)
            {
                // if there is a worker nearby, harass it
                if (bot_.Config().ScoutHarassEnemy && closest_enemy_worker_unit && (Util::Dist(worker_scout->pos, closest_enemy_worker_unit->pos) < 12))
                {
                    scout_status_ = "Harass enemy worker";
                    Micro::SmartAttackUnit(worker_scout, closest_enemy_worker_unit, bot_);
                }
                // otherwise keep moving to the enemy base location
                else
                {
                    scout_status_ = "Moving to enemy base location";
                    Micro::SmartMove(worker_scout, enemy_base_location->GetPosition(), bot_);
                }
            }
            // if the worker scout is under attack
            else
            {
                scout_status_ = "Under attack inside, fleeing";
                Micro::SmartMove(worker_scout, GetFleePosition(), bot_);
            }
        }
        // if the scout is not in the enemy region
        else if (scout_under_attack_)
        {
            scout_status_ = "Under attack outside, fleeing";

            Micro::SmartMove(worker_scout, GetFleePosition(), bot_);
        }
        else
        {
            scout_status_ = "Enemy region known, going there";

            // move to the enemy region
            Micro::SmartMove(worker_scout, enemy_base_location->GetPosition(), bot_);
        }

    }

    // for each start location on the map
    if (!enemy_base_location)
    {
        scout_status_ = "Enemy base unknown, exploring";
        
        //for (const BaseLocation* startLocation : bot_.Bases().getStartingBaseLocations())
        for (const sc2::Point2D start_location : bot_.Observation()->GetGameInfo().enemy_start_locations)
        {
            // if we haven't explored it yet then scout it out
            // TODO: this is where we could change the order of the base scouting, since right now it's iterator order
            if (!bot_.Map().IsExplored(start_location))
            {
                Micro::SmartMove(bot_.GetUnit(scout_unit_tag_), start_location, bot_);
                return;
            }
        }
    }

    previous_scout_hp_ = scout_hp;
}

const sc2::Unit* ScoutManager::ClosestEnemyWorkerTo(const sc2::Point2D & pos) const
{
    const sc2::Unit* our_scout = bot_.GetUnit(scout_unit_tag_);

    if (!our_scout) { return nullptr; }

    sc2::Tag enemy_worker_tag = 0;
    float min_dist = std::numeric_limits<float>::max();

    // for each enemy worker
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Enemy))
    {
        if (Util::IsWorker(unit))
        {
            const float dist = Util::Dist(unit->pos, our_scout->pos);

            if (dist < min_dist)
            {
                min_dist = dist;
                enemy_worker_tag = unit->tag;
            }
        }
    }

    return bot_.GetUnit(enemy_worker_tag);
}
bool ScoutManager::EnemyWorkerInRadiusOf(const sc2::Point2D & pos) const
{
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Enemy))
    {
        if (Util::IsWorker(unit) && Util::Dist(unit->pos, pos) < 10)
        {
            return true;
        }
    }

    return false;
}

sc2::Point2D ScoutManager::GetFleePosition() const
{
    // TODO: make this follow the perimeter of the enemy base again, but for now just use home base as flee direction
    return bot_.GetStartLocation();
}