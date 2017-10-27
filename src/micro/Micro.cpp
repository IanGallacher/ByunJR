#include "ByunJRBot.h"
#include "common/Common.h"
#include "micro/Micro.h"
#include "util/Util.h"
#include "ai/Pathfinding.h"

const float dot_radius = 0.1f;

void Micro::SmartAttackUnit(const sc2::Unit* attacker, const sc2::Unit* target, ByunJRBot & bot)
{
    //UAB_ASSERT(attacker, "SmartAttackUnit: Attacker not valid");
    //UAB_ASSERT(target, "SmartAttackUnit: Target not valid");

    //if (!attacker || !target)
    //{
    //    return;
    //}

    //// if we have issued a command to this unit already this frame, ignore this one
    //if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
    //{
    //    return;
    //}

    //// get the unit's current command
    //BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

    //// if we've already told this unit to attack this target, ignore this command
    //if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&    currentCommand.getTarget() == target)
    //{
    //    return;
    //}

    //// if nothing prevents it, attack the target
    //attacker->attack(target);
    //TotalCommands++;

    //if (Config::Debug::DrawUnitTargetInfo)
    //{
    //    BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::Red, true);
    //    BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Red, true);
    //    BWAPI::Broosmdwar->drawLineMap(attacker->getPosition(), target->getPosition(), BWAPI::Colors::Red);
    //}

    // Prevent sending duplicate commands to give an accurate APM measurement in replays
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

void Micro::SmartAttackMove(const sc2::Unit* attacker, const sc2::Point2D & target_position, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(attacker, sc2::ABILITY_ID::ATTACK_ATTACK, target_position);
}

void Micro::SmartPathfind(const sc2::Unit* unit, const sc2::Point2D & target_position, ByunJRBot & bot)
{
    // Sometimes after we remove the floating points, it will turn out we are trying to move to is almost the same as our current position.
    // No need to run the pathfinding algorithm in that case. 
    if (sc2::Point2DI(unit->pos.x, unit->pos.y)
     == sc2::Point2DI(target_position.x, target_position.y))
    {
        SmartMove(unit, target_position, bot);
        return;
    }
    Pathfinding p;
    std::vector<sc2::Point2D> move_path = p.Djikstra(sc2::Point2DI(unit->pos.x, unit->pos.y),
        sc2::Point2DI(target_position.x, target_position.y),
        bot.InformationManager().GetDPSMap());
    SmartMove(unit, move_path[0], bot);
}

void Micro::SmartRunAway(const sc2::Unit* unit, const int run_distance, ByunJRBot & bot)
{
    Pathfinding p;
    std::vector<sc2::Point2D> move_path = p.DjikstraLimit(sc2::Point2DI(unit->pos.x, unit->pos.y),
        run_distance,
        bot.InformationManager().GetDPSMap());
    //SmartMove(unit, move_path[0], bot, false);
    //SmartMove(unit, move_path[1], bot, true);
    //SmartMove(unit, move_path[2], bot, true);
    SmartMove(unit, move_path[3], bot, false);
    //for (const auto & j : move_path)
    //{
    //    SmartMove(unit, j, bot, true);
    //}
}

void Micro::SmartMove(const sc2::Unit* unit, const sc2::Point2D & target_position, ByunJRBot & bot, bool queued_command)
{
    // Prevent sending duplicate commands to give an accurate APM measurement in replays
    bool sent_command_already = false;
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

void Micro::SmartRightClick(const sc2::Unit* unit, const sc2::Unit* target, ByunJRBot & bot)
{
    // Prevent sending duplicate commands to give an accurate APM measurement in replays
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

void Micro::SmartRepair(const sc2::Unit* unit, const sc2::Unit* & target, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(unit, sc2::ABILITY_ID::SMART, target);
}

void Micro::SmartKiteTarget(const sc2::Unit* ranged_unit, const sc2::Unit* target, ByunJRBot & bot)
{
    //UAB_ASSERT(rangedUnit, "SmartKiteTarget: Unit not valid");
    //UAB_ASSERT(target, "SmartKiteTarget: Target not valid");

    //if (!rangedUnit || !target)
    //{
    //    return;
    //}
    const float range = Util::GetAttackRange(ranged_unit->unit_type, bot);

    //// determine whether the target can be kited
    //bool kiteLonger = Config::Micro::KiteLongerRangedUnits.find(rangedUnit->getType()) != Config::Micro::KiteLongerRangedUnits.end();
    //if (!kiteLonger && (range <= target->getType().groundWeapon().maxRange()))
    //{
    //    // if we can't kite it, there's no point
    //    Micro::SmartAttackUnit(rangedUnit, target);
    //    return;
    //}

    bool kite(true);
    const double dist(bot.Map().GetGroundDistance(ranged_unit->pos, target->pos));
    const double speed(bot.Observation()->GetUnitTypeData()[ranged_unit->unit_type].movement_speed);


    // if the unit can't attack back don't kite
    //if (bot.GetUnit(target)->is_flying /*&& !UnitUtil::CanAttackAir(target)) || (!rangedUnit->isFlying() && !UnitUtil::CanAttackGround(target))*/)
    //{
    //    kite = false;
    //}

    const double time_to_enter = (dist - range) / speed;
    // If we start moving back to attack, will our weapon be off cooldown?
    if ((time_to_enter >= ranged_unit->weapon_cooldown))
    {
        kite = false;
    }

    if (Util::IsBuilding(target->unit_type))
    {
        kite = false;
    }

    sc2::Point2D flee_position;
    if (ranged_unit->health < Util::EnemyDPSInRange(ranged_unit->pos, bot) + 5.0)
    {
        kite = true;
        flee_position = sc2::Point2D(bot.Config().ProxyLocationX, bot.Config().ProxyLocationY);
        SmartRunAway(ranged_unit, 20, bot);
        return;
    }
    else
    {
        // kite if we are not close to death.
        flee_position = ranged_unit->pos - target->pos + ranged_unit->pos;
    }

    //// if we can't shoot, run away
    if (kite)
    {
        //fleePosition = rangedUnit->pos - target->pos + rangedUnit->pos;
        bot.DebugHelper().DrawLine(ranged_unit->pos, flee_position);
        flee_position = ranged_unit->pos - target->pos + ranged_unit->pos;
        SmartMove(ranged_unit, flee_position, bot);
        //SmartRunAway(ranged_unit, 20, bot);
    }
    //// otherwise shoot
    else
    {
        //bot.Actions()->UnitCommand(rangedUnit, sc2::ABILITY_ID::EFFECT_KD8CHARGE, target);
        bot.DebugHelper().DrawLine(ranged_unit->pos, target->pos, sc2::Colors::Red);
        SmartAttackUnit(ranged_unit, target, bot);
    }
}

void Micro::SmartBuild(const sc2::Unit* builder, const sc2::UnitTypeID & building_type, const sc2::Point2D pos, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(builder, Util::UnitTypeIDToAbilityID(building_type), pos);
}

void Micro::SmartBuildTag(const sc2::Unit* builder, const sc2::UnitTypeID & building_type, const sc2::Unit* target, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(builder, Util::UnitTypeIDToAbilityID(building_type), target);
}

void Micro::SmartTrain(const sc2::Unit* production_building, const sc2::UnitTypeID & type_to_train, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(production_building, Util::UnitTypeIDToAbilityID(type_to_train));
}