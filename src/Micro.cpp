#include "Micro.h"
#include "Util.h"
#include "ByunJRBot.h"

const float dotRadius = 0.1f;

void Micro::SmartStop(const sc2::Tag & attacker, ByunJRBot & bot)
{
    //bot.Actions()->UnitCommand(attacker, sc2::ABILITY_ID::STOP);
}

void Micro::SmartAttackUnit(const sc2::Tag & attacker, const sc2::Tag & target, ByunJRBot & bot)
{

	//UAB_ASSERT(attacker, "SmartAttackUnit: Attacker not valid");
	//UAB_ASSERT(target, "SmartAttackUnit: Target not valid");

	//if (!attacker || !target)
	//{
	//	return;
	//}

	//// if we have issued a command to this unit already this frame, ignore this one
	//if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	//{
	//	return;
	//}

	//// get the unit's current command
	//BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	//// if we've already told this unit to attack this target, ignore this command
	//if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&	currentCommand.getTarget() == target)
	//{
	//	return;
	//}

	//// if nothing prevents it, attack the target
	//attacker->attack(target);
	//TotalCommands++;

	//if (Config::Debug::DrawUnitTargetInfo)
	//{
	//	BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::Red, true);
	//	BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Red, true);
	//	BWAPI::Broodwar->drawLineMap(attacker->getPosition(), target->getPosition(), BWAPI::Colors::Red);
	//}
    bot.Actions()->UnitCommand(bot.GetUnit(attacker), sc2::ABILITY_ID::ATTACK_ATTACK, bot.GetUnit(target));
}

void Micro::SmartAttackMove(const sc2::Tag & attacker, const sc2::Point2D & targetPosition, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(bot.GetUnit(attacker), sc2::ABILITY_ID::ATTACK_ATTACK, targetPosition);
}

void Micro::SmartMove(const sc2::Tag & unitToMove, const sc2::Point2D & targetPosition, ByunJRBot & bot)
{
    // Prevent sending duplicate commands to give an accurate APM measurement in replays
    bool sentCommandAlready = false;
    for (sc2::UnitOrder theOrder : bot.GetUnit(unitToMove)->orders)
    {
        if (theOrder.ability_id == sc2::ABILITY_ID::MOVE && theOrder.target_pos == targetPosition)
        {
            sentCommandAlready = true;
        }
    }
    if (sentCommandAlready == false)
        bot.Actions()->UnitCommand(bot.GetUnit(unitToMove), sc2::ABILITY_ID::MOVE, targetPosition);
}

void Micro::SmartRightClick(const sc2::Tag & unit, const sc2::Tag & target, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(bot.GetUnit(unit), sc2::ABILITY_ID::SMART, bot.GetUnit(target));
}

void Micro::SmartRepair(const sc2::Tag & unit, const sc2::Tag & target, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(bot.GetUnit(unit), sc2::ABILITY_ID::SMART, bot.GetUnit(target));
}

void Micro::SmartKiteTarget(const sc2::Tag & rangedUnit, const sc2::Tag & target, ByunJRBot & bot)
{
    //UAB_ASSERT(rangedUnit, "SmartKiteTarget: Unit not valid");
    //UAB_ASSERT(target, "SmartKiteTarget: Target not valid");

    //if (!rangedUnit || !target)
    //{
    //	return;
    //}

    float range = Util::GetAttackRange(bot.GetUnit(rangedUnit)->unit_type, bot);

    //// determine whether the target can be kited
    //bool kiteLonger = Config::Micro::KiteLongerRangedUnits.find(rangedUnit->getType()) != Config::Micro::KiteLongerRangedUnits.end();
    //if (!kiteLonger && (range <= target->getType().groundWeapon().maxRange()))
    //{
    //	// if we can't kite it, there's no point
    //	Micro::SmartAttackUnit(rangedUnit, target);
    //	return;
    //}

    bool kite(true);
    double dist(bot.Map().getGroundDistance(bot.GetUnit(rangedUnit)->pos, bot.GetUnit(target)->pos));
    double speed(bot.Observation()->GetUnitTypeData()[bot.GetUnit(rangedUnit)->unit_type].movement_speed);


    //// if the unit can't attack back don't kite
    //if ((rangedUnit->isFlying() && !UnitUtil::CanAttackAir(target)) || (!rangedUnit->isFlying() && !UnitUtil::CanAttackGround(target)))
    //{
    //	//kite = false;
    //}

    double timeToEnter = (dist - range) / speed;
    // If we start moving back to attack, will our weapon be off cooldown?
    if ((timeToEnter >= bot.GetUnit(rangedUnit)->weapon_cooldown))
    {
        kite = false;
    }

    if (Util::IsBuilding(bot.GetUnit(target)->unit_type))
    {
        kite = false;
    }
    sc2::Point2D fleePosition;
    if (bot.GetUnit(rangedUnit)->health < Util::EnemyDPSInRange(bot.GetUnit(rangedUnit)->pos, bot) + 5.0)
    {
        //std::cout << Util::EnemyDPSInRange(bot.GetUnit(rangedUnit)->pos, bot) << std::endl;
        kite = true;
        fleePosition = bot.Bases().getPlayerStartingBaseLocation(Players::Self)->getPosition();
    }
    else
    {
        // kite if we are not close to death.
        fleePosition = bot.GetUnit(rangedUnit)->pos - bot.GetUnit(target)->pos + bot.GetUnit(rangedUnit)->pos;
    }

    //// if we can't shoot, run away
    if (kite)
    {
        sc2::Point2D fleePosition(bot.GetUnit(rangedUnit)->pos - bot.GetUnit(target)->pos + bot.GetUnit(rangedUnit)->pos);
        //BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), fleePosition, BWAPI::Colors::Cyan);
        Micro::SmartMove(rangedUnit, fleePosition, bot);
    }
    //// otherwise shoot
    else
    {
        //bot.Actions()->UnitCommand(rangedUnit, sc2::ABILITY_ID::EFFECT_KD8CHARGE, target);
        SmartAttackUnit(rangedUnit, target, bot);
    }
}

void Micro::SmartBuild(const sc2::Tag & builder, const sc2::UnitTypeID & buildingType, sc2::Point2D pos, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(bot.GetUnit(builder), Util::UnitTypeIDToAbilityID(buildingType), pos);
}

void Micro::SmartBuildTag(const sc2::Tag & builder, const sc2::UnitTypeID & buildingType, sc2::Tag targetTag, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(bot.GetUnit(builder), Util::UnitTypeIDToAbilityID(buildingType), bot.GetUnit(targetTag));
}

void Micro::SmartTrain(const sc2::Tag & builder, const sc2::UnitTypeID & buildingType, ByunJRBot & bot)
{
    bot.Actions()->UnitCommand(bot.GetUnit(builder), Util::UnitTypeIDToAbilityID(buildingType));
}