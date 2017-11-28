// Huge thanks to UAlbertaBot for laying the framework and designing most of these functions!

#include "ByunJRBot.h"
#include "TechLab/InformationManager.h"
#include "TechLab/util/Util.h"
#include "micro/Micro.h"

void Micro::SmartAttackUnit(const sc2::Unit* attacker, const sc2::Unit* target, sc2::Agent & bot)
{
    assert(attacker); assert(target);

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
	assert(attacker);
    bot.Actions()->UnitCommand(attacker, sc2::ABILITY_ID::ATTACK_ATTACK, target_position);
}

void Micro::SmartMove(const sc2::Unit* unit, const sc2::Point2D & target_position, sc2::Agent & bot, bool queued_command)
{
	assert(unit);
    // Prevent sending duplicate commands to give an accurate APM measurement in replays.
    // Spamming every frame also causes bugs in the sc2 engine. 
    bool sent_command_already = false;
    if (sc2::Point2D{unit->pos.x, unit->pos.y} == target_position)
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
	assert(unit);
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
            if(scv) info.UnitInfo().SetJob(scv, UnitMission::Repair, unit_to_repair);
        }
    }
}

void Micro::SmartBuild(const Building b, const sc2::Point2D pos, sc2::Agent & bot)
{
    // Prevent sending duplicate commands to give an accurate APM measurement in replays.
    // Spamming every frame also causes bugs in the sc2 engine. 
    bool sent_command_already = false;
    for (sc2::UnitOrder the_order : b.builderUnit->orders)
    {
        if (the_order.ability_id == Util::UnitTypeIDToAbilityID(b.type))
        {
            sent_command_already = true;
        }
    }
    if (sent_command_already == false && Util::UnitCanBuildTypeNow(b.builderUnit, b.type, bot))
        bot.Actions()->UnitCommand(b.builderUnit, Util::UnitTypeIDToAbilityID(b.type), pos);
 /*   else if(b.buildingUnit)
        Micro::SmartRightClick(b.builderUnit, b.buildingUnit, bot);*/
    else 
        SmartMove(b.builderUnit, pos, bot);

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