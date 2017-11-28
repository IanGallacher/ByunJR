#include "ByunJRBot.h"
#include "TechLab/util/Util.h"

#include "AI/pathfinding.h"
#include "common/BotAssert.h"
#include "micro/CombatMicroManager.h"

CombatMicroManager::CombatMicroManager(ByunJRBot & bot)
    : bot_(bot)
{
}

void CombatMicroManager::SetUnits(const std::vector<const sc2::Unit*> & u)
{
    combat_units_ = u;
}

void CombatMicroManager::Execute(const SquadOrder & input_order)
{
    // Nothing to do if we have no units
    if (combat_units_.empty() || !(input_order.GetType() == SquadOrderTypes::Attack || input_order.GetType() == SquadOrderTypes::Defend))
    {
        return;
    }

    order_ = input_order;

    // Discover enemies within region of interest
    std::set<const sc2::Unit*> nearby_enemies;

    // Get all relavant units that are close to our combat unit.
    for (auto & enemy_unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy))
    {
        if (Util::Dist(enemy_unit->pos, order_.GetPosition()) < order_.GetRadius())
        {
            nearby_enemies.insert(enemy_unit);
        }
    }

    // otherwise we want to see everything on the way as well
    //if (order.getType() == SquadOrderTypes::Attack)
    //{
    //    for (auto & unitTag : units)
    //    {
    //        auto unit = bot_.unitTag;
    //        BOT_ASSERT(unit, "null unit in attack");

    //        for (auto & enemyUnit : bot_.Info().UnitInfo().getUnits(sc2::Unit::Alliance::Enemy))
    //        {
    //            if (Util::Dist(enemyUnit.pos, unit->pos) < order.getRadius())
    //            {
    //                nearbyEnemies.insert(enemyUnit.tag);
    //            }
    //        }
    //    }
    //}

    if (order_.GetType() == SquadOrderTypes::Attack || order_.GetType() == SquadOrderTypes::Defend)
    {
        AttackTargets(nearby_enemies);
    }
}

void CombatMicroManager::Regroup(const sc2::Point2D & regroup_position) const
{
    const sc2::Point2D our_base_position = bot_.GetStartLocation();
    const int regroup_distance_from_base = bot_.Info().Map().GetGroundDistance(regroup_position, our_base_position);

    // for each of the units we have
    for (auto & unit : combat_units_)
    {
        BOT_ASSERT(unit, "null unit in CombatMicroManager regroup");

        const int unit_distance_from_base = bot_.Info().Map().GetGroundDistance(unit->pos, our_base_position);

        // if the unit is outside the regroup area
        if (unit_distance_from_base > regroup_distance_from_base)
        {
            Micro::SmartMove(unit, our_base_position, bot_);
        }
        else if (Util::Dist(unit->pos, regroup_position) > 4)
        {
            // regroup it
            Micro::SmartMove(unit, regroup_position, bot_);
        }
        else
        {
            Micro::SmartAttackMove(unit, unit->pos, bot_);
        }
    }
}

void CombatMicroManager::AttackTargets(const std::set<const sc2::Unit*> & targets)
{
    for (auto & combat_unit : combat_units_)
    {
        // Run away with low health units.
        if (ShouldUnitRetreat(combat_unit))
        {
            if(Util::IsWorker(combat_unit))
                bot_.Info().UnitInfo().SetJob(combat_unit, UnitMission::Minerals);
            
            if (Util::IsUnitOfType(combat_unit, sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER, bot_))
            {
                bot_.Actions()->UnitCommand(combat_unit, sc2::ABILITY_ID::EFFECT_TACTICALJUMP,
                    bot_.Info().Bases().GetPlayerStartingBaseLocation(sc2::Unit::Alliance::Self)->GetPosition());
                bot_.Info().UnitInfo().SetJob(combat_unit, UnitMission::Wait);
            }
            // The unit is running away. We don't have to fight anything, so lets move on to the next unit. 
            continue;
        }


		const sc2::Unit* target = GetTarget(combat_unit, targets);

        // If there are no targets, no need to try to micro our units. 
        if (!target)
            // No targets? Go find the enemy base!
        {
            // if we're not near the order position
            if (Util::Dist(combat_unit->pos, order_.GetPosition()) > 4)
            {
                // move to it
                Micro::SmartMove(combat_unit, order_.GetPosition(), bot_);
            }
        }

        if (order_.GetType() == SquadOrderTypes::Attack || order_.GetType() == SquadOrderTypes::Defend)
        {
            // Sometimes we won't find a unit to attack. 
            if (!target) continue;

            // Special logic to attack with battlecruisers.
            if (combat_unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER)
            {
                Micro::SmartAttackUnit(combat_unit, target, bot_);

                const sc2::Unit* yamato_target = GetYamatoTarget(combat_unit, targets);

                if (yamato_target && planned_damage_[target->tag] < target->health)
                {
                    bot_.Actions()->UnitCommand(combat_unit, sc2::ABILITY_ID::EFFECT_YAMATOGUN, target);
                    planned_damage_[target->tag] += 300;
                }
            }

            // attack it
            else if (bot_.Config().KiteWithRangedUnits)
            {
                SmartKiteTarget(combat_unit, target);
            }
            else
            {
                Micro::SmartAttackUnit(combat_unit, target, bot_);
            }
        }

        if (bot_.Config().DrawUnitTargetInfo)
        {
            // TODO: draw the line to the unit's target
        }
    }
}

// Out of all possible targets, which is the best one for our unit to attack.
const sc2::Unit* CombatMicroManager::GetTarget(const sc2::Unit* combat_unit, const std::set<const sc2::Unit*> & targets) const
{
    BOT_ASSERT(combat_unit, "null combat unit in GetTarget");

    int lowest_health = std::numeric_limits<int>::max();
    int high_priority = std::numeric_limits<int>::max();
    double closest_dist = std::numeric_limits<double>::max();
    const sc2::Unit* best_target = nullptr;

    // Look through all possible targets. Find the best one for the given unit. 
    for (auto & target_unit : targets)
    {
        if (target_unit->unit_type == sc2::UNIT_TYPEID::ZERG_EGG)   continue;
        if (target_unit->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) continue;


        // If our reaper is currently safe, go kill some workers.
        if (bot_.Info().GetDPSMap()[combat_unit->pos.y][combat_unit->pos.x] < 12.0f)
        {
            if (Util::IsWorker(target_unit))
            {
                const float distance = Util::Dist(combat_unit->pos, target_unit->pos);

                // Only look for workers that are close to the reaper. 
                if (distance > 7) continue;
                if (!best_target || target_unit->health < lowest_health)
                {
                    lowest_health = static_cast<int>(target_unit->health);
                    best_target = target_unit;
                }
            }
        }

        // If our unit flies, kill everything that can shoot us. 
        if (combat_unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER)
        {
            const float distance = Util::Dist(combat_unit->pos, target_unit->pos);

            // Only look for workers that are close to the battlecruiser. 
            if (distance > 7) continue;
            // Prioritise the units that can attack us. Otherwise we can simply use the generic code to find a target. 
            if (!Util::CanAttackAir(bot_.Observation()->GetUnitTypeData()[target_unit->unit_type].weapons))
                continue;
            if (!best_target || target_unit->health + target_unit->shield < lowest_health)
            {
                lowest_health = static_cast<int>(target_unit->health + target_unit->shield);
                best_target = target_unit;
            }
        }
    }

    // The reaper is safe and has a low health worker it wants to attack. 
    if (best_target) return best_target;

    // If no special case handles the situation, use a standard way of getting a target. 
    for (auto & target_unit : targets)
    {
        const int priority = GetAttackPriority(combat_unit, target_unit);
        const float distance = Util::Dist(combat_unit->pos, target_unit->pos);
        int f = bot_.Observation()->GetGameLoop();

        // Don't waste time killing buildings until we have a good chance of winning the game from all the workers we have killed
        if (Util::IsBuilding(target_unit->unit_type) && Util::GetGameTimeInSeconds(bot_) < 400)
            continue;

        // Don't bother attacking units that we can not hit. 
        if (target_unit->is_flying && !Util::CanAttackAir(bot_.Observation()->GetUnitTypeData()[combat_unit->unit_type].weapons))
            continue;

        // If there are ranged units on high ground we can't see, we can't attack them back.
        if (!bot_.Info().Map().IsVisible(target_unit->pos) && Util::IsCombatUnit(target_unit, bot_))
            continue;

        if (!best_target || (priority > high_priority) || (priority == high_priority && distance < closest_dist))
        {
            closest_dist = distance;
            high_priority = priority;
            best_target = target_unit;
        }
    }

    return best_target;
}


// Out of all possible targets, which is the best one for our unit to attack.
const sc2::Unit* CombatMicroManager::GetYamatoTarget(const sc2::Unit* combat_unit, const std::set<const sc2::Unit*> & targets) const
{
    BOT_ASSERT(combat_unit, "null combat unit in GetTarget");

    int lowest_health = std::numeric_limits<int>::max();
    int high_priority = std::numeric_limits<int>::max();
    double closest_dist = std::numeric_limits<double>::max();
    const sc2::Unit* best_target = nullptr;

    bool attack_air_target = false;
    // Look through all possible targets. Find the best one for the given unit. 
    for (auto & target_unit : targets)
    {
        // Always yamato medivacs. They may have units in them, and they are healing the marines. 
        if (target_unit->unit_type == sc2::UNIT_TYPEID::TERRAN_MEDIVAC
         || target_unit->unit_type == sc2::UNIT_TYPEID::TERRAN_VIKINGFIGHTER)
            return target_unit;

        if (target_unit->unit_type == sc2::UNIT_TYPEID::ZERG_EGG)   continue;
        if (target_unit->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA) continue;

        // It is faster to kill marines without yamato
        if (target_unit->unit_type == sc2::UNIT_TYPEID::TERRAN_MARINE) continue;
        
        // Only look for workers that are close to the battlecruiser.
        const float distance = Util::Dist(combat_unit->pos, target_unit->pos);
        if (distance > 10) continue;

        // If we have found a target that can shoot us, don't bother trying to shoot anything on the ground.
        if (attack_air_target && !Util::CanAttackAir(bot_.Observation()->GetUnitTypeData()[target_unit->unit_type].weapons))
            continue;
        if (!best_target || target_unit->health + target_unit->shield < lowest_health)
        {
            lowest_health = static_cast<int>(target_unit->health + target_unit->shield);
            best_target = target_unit;
            attack_air_target = Util::CanAttackAir(bot_.Observation()->GetUnitTypeData()[target_unit->unit_type].weapons);
        }
    }
    return best_target;
}


bool CombatMicroManager::ShouldUnitRetreat(const sc2::Unit* unit) const
{
    if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER
        && unit->health < unit->health_max / 5)
        return true;

    if (unit->health <= 10)
        return true;
    return false;
}

// get the attack priority of a type in relation to a zergling
int CombatMicroManager::GetAttackPriority(const sc2::Unit* attacker, const sc2::Unit* unit_tag) const
{
    auto unit = unit_tag;
    BOT_ASSERT(unit, "null unit in getAttackPriority");

    if (Util::IsCombatUnit(unit, bot_))
    {
        return 10;
    }

    if (Util::IsWorker(unit))
    {
        return 9;
    }

    return 1;
}



#pragma region Advanced micro functionality
float CombatMicroManager::TimeToFaceEnemy(const sc2::Unit* unit, const sc2::Unit* target) const
{
    const float x_segment = abs(unit->pos.x - target->pos.x);
    const float y_segment = abs(unit->pos.y - target->pos.y);
    const float angle = atan(y_segment / x_segment) * 57.29f; // 57.29 = 180/pi

    return angle/999; // 999 is the turning spead of reapers. 
}

// Warning: This funcition has no discrestion in what it kites. Be careful to not attack overlords with reapers!
void CombatMicroManager::SmartKiteTarget(const sc2::Unit* unit, const sc2::Unit* target) const
{
    assert(unit);
    assert(target);

    const float range = Util::GetAttackRange(unit->unit_type, bot_);

    bool should_flee(true);

    // When passing a unit into PathingDistance, how the unit moves is taken into account.
    // EXAMPLE:: Reapers can cliffjump, Void Rays can fly over everything.
    const float dist(bot_.Query()->PathingDistance(unit, target->pos));
    const float speed(bot_.Observation()->GetUnitTypeData()[unit->unit_type].movement_speed);

    const float time_to_enter = (dist - range) / speed;

    // If we start moving back to attack, will our weapon be off cooldown?
    if ((time_to_enter >= unit->weapon_cooldown))
    {
        should_flee = false;
    }

    // Don't kite workers and buildings. 
    if (Util::IsBuilding(target->unit_type))
    {
        should_flee = false;
    }

    sc2::Point2D flee_position;

    // find the new coordinates.
    const float delta_x = unit->pos.x - target->pos.x;
    const float delta_y = unit->pos.y - target->pos.y;

    const float dist2 = Util::Dist(unit->pos, target->pos);

    const float new_x = delta_x * range / dist2 + target->pos.x;
    const float new_y = delta_y * range / dist2 + target->pos.y;

    const float fire_time = TimeToFaceEnemy(unit, target) + Util::GetAttackRate(unit->unit_type,bot_) + 0.05f;

    // If we are in danger of dieing, run back to home base!
        // If we are danger of dieing while attacking
    if (unit->health <= Util::PredictFutureDPSAtPoint(unit->pos, fire_time, bot_)
        // If we are danger of dieing while moving to attack a point.
     || unit->health <= Util::DPSAtPoint(sc2::Point2D{new_x, new_y}, bot_))
    {
        // No matter what the other logic above says to do, RUN!
        should_flee = true;
        flee_position = sc2::Point2D{static_cast<float>(bot_.Config().ProxyLocationX), 
									 static_cast<float>(bot_.Config().ProxyLocationY)};
        bot_.DebugHelper().DrawLine(unit->pos, sc2::Point2D{new_x, new_y}, sc2::Colors::Red);
        Pathfinding p;
        p.SmartRunAway(unit, 15, bot_);
        return;
    }
    // Otherwise, kite if we are not close to death.
    else
    {
        flee_position = unit->pos - target->pos + unit->pos;
        bot_.DebugHelper().DrawLine(unit->pos, sc2::Point2D{new_x, new_y}, sc2::Colors::Green);
    }

    // If we are on cooldown, run away.
    if (should_flee)
    {
        bot_.DebugHelper().DrawLine(unit->pos, flee_position);
        flee_position = unit->pos - target->pos + unit->pos;
        Micro::SmartMove(unit, flee_position, bot_);
    }
    // Otherwise go attack!
    else
    {
        // bot.DebugHelper().DrawLine(ranged_unit->pos, target->pos, sc2::Colors::Red);
        Micro::SmartAttackUnit(unit, target, bot_);
    }
}
#pragma endregion