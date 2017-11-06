#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "micro/ScoutManager.h"
#include "micro/Micro.h"
#include "util/Util.h"

ScoutManager::ScoutManager(ByunJRBot & bot)
    : bot_                (bot)
    , num_scouts_         (0)
    , scout_under_attack_ (false)
    , scout_status_       ("None")
    , previous_scout_hp_  (0.0f)
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
    // Setup shorthand for bot_.InformationManager().UnitInfo().GetScouts(),
    const std::set<const UnitInfo*> & scout_units = bot_.InformationManager().UnitInfo().GetScouts();

    // No need to print an error message. Sometimes we don't need any scouts for our current strategy.
    if (scout_units.size() == 0) { return; }

    for(const auto & scout : scout_units)
    {
        if (!scout || scout->unit->health <= 0) { return; }

        // get the enemy base location, if we have one
        const BaseLocation* enemy_base_location = bot_.Bases().GetPlayerStartingBaseLocation(sc2::Unit::Alliance::Enemy);

        // If we know where the enemy region is, use the scouts to harass the enemy workers.
        if (enemy_base_location)
        {
            bot_.InformationManager().UnitInfo().SetJob(scout->unit, UnitMission::Attack);
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
                    Micro::SmartMove(scout->unit, start_location, bot_);
                    return;
                }
            }
        }
    }
}

const sc2::Unit* ScoutManager::ClosestEnemyWorkerTo(const sc2::Unit * scout) const
{
    if (!scout) { std::cout << "Looked for scout, could not find anything" << std::endl;  return nullptr; }

    const sc2::Unit* enemy_worker = nullptr;
    float min_dist = std::numeric_limits<float>::max();

    // for each enemy worker
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy))
    {
        if (Util::IsWorker(unit))
        {
            const float dist = Util::Dist(unit->pos, scout->pos);

            if (dist < min_dist)
            {
                min_dist = dist;
                enemy_worker = unit;
            }
        }
    }

    return enemy_worker;
}
bool ScoutManager::EnemyWorkerInRadiusOf(const sc2::Point2D & pos) const
{
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy))
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