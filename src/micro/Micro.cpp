// Huge thanks to UAlbertaBot for laying the framework and designing most of these functions!

#include "ByunJRBot.h"
#include "TechLab/InformationManager.h"
#include "TechLab/util/Util.h"
#include "AI/pathfinding.h"
#include "micro/Micro.h"

void Micro::SmartAttackUnit(const sc2::Unit* attacker, const sc2::Unit* target, sc2::Agent & bot)
{
    assert(attacker, "SmartAttackUnit: Attacker not valid");
    assert(target, "SmartAttackUnit: Target not valid");

    // Prevent sending duplicate commands to prevent strange errors and give an accurate APM measurement in replays
    bool sent_command_already = false;
    for (const sc2::UnitOrder the_order : attacker->orders)
    {
        if (the_order.ability_id == sc2::ABILITY_ID::MOVE && the_order.target_unit_tag == target->tag)
        {
            sent_command_already = true;
        }
    }
    if (sent_command_already == false)
        bot.Actions()->UnitCommand(attacker, sc2::ABILITY_ID::ATTACK_ATTACK, target);
}

void Micro::SmartAttackMove(const sc2::Unit* attacker, const sc2::Point2D & target_position, sc2::Agent & bot)
{
    bot.Actions()->UnitCommand(attacker, sc2::ABILITY_ID::ATTACK_ATTACK, target_position);
}

void Micro::SmartMove(const sc2::Unit* unit, const sc2::Point2D & target_position, sc2::Agent & bot, bool queued_command)
{
    // Prevent sending duplicate commands to give an accurate APM measurement in replays.
    // Spamming every frame also causes bugs in the sc2 engine. 
    bool sent_command_already = false;
    if (sc2::Point2D(unit->pos.x, unit->pos.y) == target_position)
        sent_command_already = true;

    for (sc2::UnitOrder the_order : unit->orders)
    {
        if (the_order.ability_id == sc2::ABILITY_ID::MOVE && the_order.target_pos == target_position)
        {
            sent_command_already = true;
        }
    }
    if (sent_command_already == false)
        bot.Actions()->UnitCommand(unit, sc2::ABILITY_ID::MOVE, target_position, queued_command);
}

 void Micro::SmartRightClick(const sc2::Unit* unit, const sc2::Unit* target, sc2::Agent & bot)
{
     // Prevent sending duplicate commands to give an accurate APM measurement in replays.
     // Spamming every frame also causes bugs in the sc2 engine. 
    bool sent_command_already = false;
    for (sc2::UnitOrder the_order : unit->orders)
    {
        if (the_order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN || the_order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER) return;
        if (the_order.ability_id == sc2::ABILITY_ID::SMART && the_order.target_unit_tag == target->tag)
        {
            sent_command_already = true;
        }
    }
    if (sent_command_already == false)
        bot.Actions()->UnitCommand(unit, sc2::ABILITY_ID::SMART, target);
}

void Micro::SmartRepair(const sc2::Unit* scv, const sc2::Unit* target, sc2::Agent & bot)
{
    bool sent_command_already = false;
    for (const sc2::UnitOrder the_order : scv->orders)
    {
        if (the_order.ability_id == sc2::ABILITY_ID::EFFECT_REPAIR && the_order.target_unit_tag == target->tag)
        {
            sent_command_already = true;
        }
    }
    if (sent_command_already == false)
    bot.Actions()->UnitCommand(scv, sc2::ABILITY_ID::EFFECT_REPAIR, target);
}

void Micro::SmartRepairWithSCVCount(const sc2::Unit* unit_to_repair, const int num_repair_workers, InformationManager & info)
{    
    const int current_repairing_workers = info.UnitInfo().GetNumRepairWorkers(unit_to_repair);
    if(current_repairing_workers < num_repair_workers)
    {
        // If we are not repairing with enough scv's, send some more to repair.
        for (int i = 0; i < num_repair_workers - current_repairing_workers; i++)
        {
            const sc2::Unit* scv = info.GetClosestUnitWithJob(unit_to_repair->pos, UnitMission::Minerals);
            if(scv)
                info.UnitInfo().SetJob(scv, UnitMission::Repair, unit_to_repair);
        }
    }
}


// Warning: This funcition has no discrestion in what it kites. Be careful to not attack overlords with reapers!
void Micro::SmartKiteTarget(const sc2::Unit* ranged_unit, const sc2::Unit* target, ByunJRBot & bot)
{
    assert(ranged_unit, "SmartKiteTarget: Unit not valid");
    assert(target, "SmartKiteTarget: Target not valid");
    assert(target->tag);

    const float range = Util::GetAttackRange(ranged_unit->unit_type, bot);

    bool should_flee(true);

    // When passing a unit into PathingDistance, how the unit moves is taken into account.
    // EXAMPLE:: Reapers can cliffjump, Void Rays can fly over everything.
    const double dist(bot.Query()->PathingDistance(ranged_unit, target->pos));
    const double speed(bot.Observation()->GetUnitTypeData()[ranged_unit->unit_type].movement_speed);

    const double time_to_enter = (dist - range) / speed;

    // If we start moving back to attack, will our weapon be off cooldown?
    if ((time_to_enter >= ranged_unit->weapon_cooldown))
    {
        should_flee = false;
    }

    // Don't kite workers and buildings. 
    if (Util::IsBuilding(target->unit_type) || Util::IsWorker(target))
    {
        should_flee = false;
    }

    sc2::Point2D flee_position;
    
    float delta_x = ranged_unit->pos.x - target->pos.x;
    float delta_y = ranged_unit->pos.y - target->pos.y;
    
    float dist2 = Util::Dist(ranged_unit->pos, target->pos);

    float new_x = delta_x * range / dist2 + target->pos.x;
    float new_y = delta_y * range / dist2 + target->pos.y;

    // If we are in danger of dieing, run back to home base!
    if (ranged_unit->health < Util::DPSAtPoint(ranged_unit->pos,
         bot.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy), bot) + 5.0
      || ranged_unit->health < Util::DPSAtPoint(sc2::Point2D(new_x, new_y),
         bot.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy), bot) + 5.0)
    {
        // No matter what the other logic above says to do, RUN!
        should_flee = true;
        flee_position = sc2::Point2D(bot.Config().ProxyLocationX, bot.Config().ProxyLocationY);
        bot.DebugHelper().DrawLine(ranged_unit->pos, sc2::Point2D(new_x, new_y), sc2::Colors::Red);
        Pathfinding p;
        p.SmartRunAway(ranged_unit, 20, bot);
        return;
    }
    // Otherwise, kite if we are not close to death.
    else
    {
        flee_position = ranged_unit->pos - target->pos + ranged_unit->pos;
        bot.DebugHelper().DrawLine(ranged_unit->pos, sc2::Point2D(new_x, new_y), sc2::Colors::Green);
    }

    // If we are on cooldown, run away.
    if (should_flee)
    {
        bot.DebugHelper().DrawLine(ranged_unit->pos, flee_position);
        flee_position = ranged_unit->pos - target->pos + ranged_unit->pos;
        SmartMove(ranged_unit, flee_position, bot);
    }
    // Otherwise go attack!
    else
    {
       // bot.DebugHelper().DrawLine(ranged_unit->pos, target->pos, sc2::Colors::Red);
        SmartAttackUnit(ranged_unit, target, bot);
    }
}

void Micro::SmartBuild(const sc2::Unit* builder, const sc2::UnitTypeID & building_type, const sc2::Point2D pos, sc2::Agent & bot)
{
    // Prevent sending duplicate commands to give an accurate APM measurement in replays.
    // Spamming every frame also causes bugs in the sc2 engine. 
    bool sent_command_already = false;
    for (sc2::UnitOrder the_order : builder->orders)
    {
        if (the_order.ability_id == Util::UnitTypeIDToAbilityID(building_type))
        {
            sent_command_already = true;
        }
    }
    if (sent_command_already == false && Util::UnitCanBuildTypeNow(builder, building_type, bot))
        bot.Actions()->UnitCommand(builder, Util::UnitTypeIDToAbilityID(building_type), pos);
    else 
        SmartMove(builder, pos, bot);
}

void Micro::SmartBuildGeyser(const sc2::Unit* builder, const sc2::UnitTypeID & building_type, const sc2::Unit* target, sc2::Agent & bot)
{
    // Prevent sending duplicate commands to give an accurate APM measurement in replays.
    // Spamming every frame also causes bugs in the sc2 engine. 
    bool sent_command_already = false;
    for (sc2::UnitOrder the_order : builder->orders)
    {
        if (the_order.ability_id == Util::UnitTypeIDToAbilityID(building_type))
        {
            sent_command_already = true;
        }
    }
    if (sent_command_already == false)
        bot.Actions()->UnitCommand(builder, Util::UnitTypeIDToAbilityID(building_type), target);
}

void Micro::SmartTrain(const sc2::Unit* production_building, const sc2::UnitTypeID & type_to_train, sc2::Agent & bot)
{
    bot.Actions()->UnitCommand(production_building, Util::UnitTypeIDToAbilityID(type_to_train));
}