#include <sstream>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "micro/CombatCommander.h"
#include "util/Util.h"

const size_t IdlePriority = 0;
const size_t AttackPriority = 1;
const size_t BaseDefensePriority = 2;
const size_t ScoutDefensePriority = 3;
const size_t DropPriority = 4;

CombatCommander::CombatCommander(ByunJRBot & bot)
    : bot_(bot)
    , squad_data_(bot)
    , initialized_(false)
    , attack_started_(false)
{

}

void CombatCommander::OnStart()
{
    squad_data_.ClearSquadData();

    SquadOrder idle_order(SquadOrderTypes::Idle, bot_.GetStartLocation(), 5, "Chill Out");
    squad_data_.AddSquad("Idle", Squad("Idle", idle_order, IdlePriority, bot_));

    // the main attack squad that will pressure the enemy's closest base location
    SquadOrder mainAttackOrder(SquadOrderTypes::Attack, sc2::Point2D(0.0f, 0.0f), 1000, "Attack Enemy Base");
    squad_data_.AddSquad("MainAttack", Squad("MainAttack", mainAttackOrder, AttackPriority, bot_));

    // the scout defense squad will handle chasing the enemy worker scout
    SquadOrder enemyScoutDefense(SquadOrderTypes::Defend, bot_.GetStartLocation(), 25, "Get the scout");
    squad_data_.AddSquad("ScoutDefense", Squad("ScoutDefense", enemyScoutDefense, ScoutDefensePriority, bot_));
}

void CombatCommander::OnFrame(const std::set<const UnitInfo*> & combat_units)
{
    if (!attack_started_)
    {
        attack_started_ = ShouldWeStartAttacking();
    }

    combat_units_ = combat_units;

    //if (isSquadUpdateFrame())
    //{
    //    updateIdleSquad();
    //    updateScoutDefenseSquad();
    //    updateDefenseSquads();
    //    updateAttackSquads();
    //}

    Squad & main_attack_squad = squad_data_.GetSquad("MainAttack");
    main_attack_squad.Clear();
    
    for (auto & unit_info : combat_units)
    {
        auto unit = unit_info->unit;
        BOT_ASSERT(unit, "null unit in combat units");
    
        // get every unit of a lower priority and put it into the attack squad
        if (squad_data_.CanAssignUnitToSquad(unit, main_attack_squad))
        {
            squad_data_.AssignUnitToSquad(unit, main_attack_squad);
        }
    }
    
    const SquadOrder main_attack_order(SquadOrderTypes::Attack, GetMainAttackLocation(), 1000, "Attack Enemy Base");
    main_attack_squad.SetSquadOrder(main_attack_order);

    squad_data_.OnFrame();
}

void CombatCommander::OnUnitCreated(const sc2::Unit* unit)
{
    if (Util::IsCombatUnit(unit))
    {
        bot_.InformationManager().UnitInfo().SetJob(unit, UnitMission::Attack);;
    }
}

bool CombatCommander::ShouldWeStartAttacking() const
{
    return combat_units_.size() >= bot_.Config().CombatUnitsForAttack;
}

sc2::Point2D CombatCommander::GetMainAttackLocation() const
{
    const BaseLocation* enemy_base_location = bot_.InformationManager().Bases().GetPlayerStartingBaseLocation(sc2::Unit::Alliance::Enemy);

    // First choice: Attack an enemy region if we can see units inside it
    if (enemy_base_location)
    {
        const sc2::Point2D enemy_base_position = enemy_base_location->GetPosition();
        //const sc2::Point2D enemyBasePosition = bot_.Observation()->GetGameInfo().enemy_start_locations[0];//enemyBaseLocation->getPosition();

        // If the enemy base hasn't been seen yet, go there.
        if (!bot_.InformationManager().Map().IsExplored(enemy_base_position))
        {
            return enemy_base_position;
        }

        // First choice: attack the known enemy base location. 
        // if it has been explored, go there if there are any visible enemy units there
        for (auto & enemy_unit : bot_.InformationManager().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy))
        {
            if (Util::Dist(enemy_unit->pos, enemy_base_position) < 25)
            {
                return enemy_base_position;
            }
        }

    }

    // Second choice: Attack known enemy buildings
    for (const auto & kv : bot_.InformationManager().UnitInfo().GetUnitInfoMap(sc2::Unit::Alliance::Enemy))
    {
        const UnitInfo & ui = kv.second;

        if (Util::IsBuilding(ui.type) && !(ui.lastPosition.x == 0.0f && ui.lastPosition.y == 0.0f))
        {
            return ui.lastPosition;
        }
    }

    // Third choice: Attack visible enemy units that aren't overlords
    for (auto & enemy_unit : bot_.InformationManager().UnitInfo().GetUnits(sc2::Unit::Alliance::Enemy))
    {
        if (enemy_unit->unit_type != sc2::UNIT_TYPEID::ZERG_OVERLORD)
        {
            return enemy_unit->pos;
        }
    }

    std::cout << "WARNING: ENEMY BASE LOCATION NOT FOUND, RETURNING 0,0" << std::endl;
    return sc2::Point2D(0, 0);

    // Fourth choice: We can't see anything so explore the map attacking along the way
    //return bot_.InformationManager().Map().getLeastRecentlySeenPosition();
}

//
//void CombatCommander::updateIdleSquad()
//{
//    Squad & idleSquad = squadData.getSquad("Idle");
//    for (auto & unit : combatUnits)
//    {
//        // if it hasn't been assigned to a squad yet, put it in the low priority idle squad
//        if (squadData.canAssignUnitToSquad(unit, idleSquad))
//        {
//            idleSquad.addUnit(unit);
//        }
//    }
//}
//
//// Called every frame.
//void CombatCommander::updateAttackSquads()
//{
//    if (!m_attackStarted)
//    {
//        return;
//    }
//
//    Squad & mainAttackSquad = squadData.getSquad("MainAttack");
//
//    for (auto & unitTag : combatUnits)
//    {
//        auto unit = bot_.GetUnit(unitTag);
//        BOT_ASSERT(unit, "null unit in combat units");
//
//        // get every unit of a lower priority and put it into the attack squad
//        if (!Util::IsWorker(*unit) && squadData.canAssignUnitToSquad(unitTag, mainAttackSquad))
//        {
//            squadData.assignUnitToSquad(unitTag, mainAttackSquad);
//        }
//    }
//
//    const SquadOrder mainAttackOrder(SquadOrderTypes::Attack, getMainAttackLocation(), 25, "Attack Enemy Base");
//    mainAttackSquad.setSquadOrder(mainAttackOrder);
//}
//
//void CombatCommander::updateScoutDefenseSquad()
//{
//    if (combatUnits.empty())
//    {
//        return;
//    }
//
//    // if the current squad has units in it then we can ignore this
//    Squad & scoutDefenseSquad = squadData.getSquad("ScoutDefense");
//
//    // get the region that our base is located in
//    const BaseLocation* myBaseLocation = bot_.InformationManager().Bases().getPlayerStartingBaseLocation(Players::Self);
//    BOT_ASSERT(myBaseLocation, "null self base location");
//
//    // get all of the enemy units in this region
//    std::vector<sc2::Tag> enemyUnitsInRegion;
//    for (auto & unit : bot_.UnitInfo().getUnits(Players::Enemy))
//    {
//        if (myBaseLocation->containsPosition(unit->pos))
//        {
//            enemyUnitsInRegion.push_back(unit->tag);
//        }
//    }
//
//    // if there's an enemy worker in our region then assign someone to chase him
//    bool assignScoutDefender = (enemyUnitsInRegion.size() == 1) && Util::IsWorker(*bot_.GetUnit(enemyUnitsInRegion[0]));
//
//    // if our current squad is empty and we should assign a worker, do it
//    if (scoutDefenseSquad.isEmpty() && assignScoutDefender)
//    {
//        // the enemy worker that is attacking us
//        const sc2::Tag enemyWorkerTag = *enemyUnitsInRegion.begin();
//        auto enemyWorkerUnit = bot_.GetUnit(enemyWorkerTag);
//        BOT_ASSERT(enemyWorkerUnit, "null enemy worker unit");
//
//        // get our worker unit that is mining that is closest to it
//        const sc2::Tag workerDefenderTag = bot_.Workers().findClosestWorkerTo(enemyWorkerUnit->pos);
//
//        if (enemyWorkerTag && workerDefenderTag)
//        {
//            // grab it from the worker manager and put it in the squad
//            if (squadData.canAssignUnitToSquad(workerDefenderTag, scoutDefenseSquad))
//            {
//                bot_.Workers().setCombatWorker(workerDefenderTag);
//                squadData.assignUnitToSquad(workerDefenderTag, scoutDefenseSquad);
//            }
//        }
//    }
//    // if our squad is not empty and we shouldn't have a worker chasing then take him out of the squad
//    else if (!scoutDefenseSquad.isEmpty() && !assignScoutDefender)
//    {
//        for (auto & unitTag : scoutDefenseSquad.getUnits())
//        {
//            auto unit = bot_.GetUnit(unitTag);
//            BOT_ASSERT(unit, "null unit in scoutDefenseSquad");
//
//            if (Util::IsWorker(*unit))
//            {
//                bot_.Workers().finishedWithWorker(unitTag);
//            }
//        }
//
//        scoutDefenseSquad.clear();
//    }
//}
//
//void CombatCommander::updateDefenseSquads()
//{
//    if (combatUnits.empty())
//    {
//        return;
//    }
//
//    // for each of our occupied regions
//    const BaseLocation* enemyBaseLocation = bot_.InformationManager().Bases().getPlayerStartingBaseLocation(Players::Enemy);
//    for (const BaseLocation* myBaseLocation : bot_.InformationManager().Bases().getOccupiedBaseLocations(Players::Self))
//    {
//        // don't defend inside the enemy region, this will end badly when we are stealing gas or cannon rushing
//        if (myBaseLocation == enemyBaseLocation)
//        {
//            continue;
//        }
//
//        const sc2::Point2D basePosition = myBaseLocation->getPosition();
//
//        // start off assuming all enemy units in region are just workers
//        const int numDefendersPerEnemyUnit = 2;
//
//        // all of the enemy units in this region
//        std::vector<sc2::Tag> enemyUnitsInRegion;
//        for (auto & unit : bot_.UnitInfo().getUnits(Players::Enemy))
//        {
//            // if it's an overlord, don't worry about it for defense, we don't care what they see
//            if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_OVERLORD)
//            {
//                continue;
//            }
//
//            if (myBaseLocation->containsPosition(unit->pos))
//            {
//                enemyUnitsInRegion.push_back(unit->tag);
//            }
//        }
//
//        // we can ignore the first enemy worker in our region since we assume it is a scout
//        for (auto & unitTag : enemyUnitsInRegion)
//        {
//            auto unit = bot_.GetUnit(unitTag);
//            BOT_ASSERT(unit, "null enemyt unit in region");
//
//            if (Util::IsWorker(*unit))
//            {
//                enemyUnitsInRegion.erase(std::remove(enemyUnitsInRegion.begin(), enemyUnitsInRegion.end(), unitTag), enemyUnitsInRegion.end());
//                break;
//            }
//        }
//
//        // calculate how many units are flying / ground units
//        int numEnemyFlyingInRegion = 0;
//        int numEnemyGroundInRegion = 0;
//        for (auto & unitTag : enemyUnitsInRegion)
//        {
//            auto unit = bot_.GetUnit(unitTag);
//            BOT_ASSERT(unit, "null enemyt unit in region");
//
//            if (unit->is_flying)
//            {
//                numEnemyFlyingInRegion++;
//            }
//            else
//            {
//                numEnemyGroundInRegion++;
//            }
//        }
//
//
//        std::stringstream squadName;
//        squadName << "Base Defense " << basePosition.x << " " << basePosition.y;
//
//        // if there's nothing in this region to worry about
//        if (enemyUnitsInRegion.empty())
//        {
//            // if a defense squad for this region exists, remove it
//            if (squadData.squadExists(squadName.str()))
//            {
//                squadData.getSquad(squadName.str()).clear();
//            }
//
//            // and return, nothing to defend here
//            continue;
//        }
//        else
//        {
//            // if we don't have a squad assigned to this region already, create one
//            if (!m_squadData.squadExists(squadName.str()))
//            {
//                SquadOrder defendRegion(SquadOrderTypes::Defend, basePosition, 32 * 25, "Defend Region!");
//                squadData.addSquad(squadName.str(), Squad(squadName.str(), defendRegion, BaseDefensePriority, bot_));
//            }
//        }
//
//        // assign units to the squad
//        if (squadData.squadExists(squadName.str()))
//        {
//            Squad & defenseSquad = squadData.getSquad(squadName.str());
//
//            // figure out how many units we need on defense
//            const int flyingDefendersNeeded = numDefendersPerEnemyUnit * numEnemyFlyingInRegion;
//            const int groundDefensersNeeded = numDefendersPerEnemyUnit * numEnemyGroundInRegion;
//
//            updateDefenseSquadUnits(defenseSquad, flyingDefendersNeeded, groundDefensersNeeded);
//        }
//        else
//        {
//            BOT_ASSERT(false, "Squad should have existed: %s", squadName.str().c_str());
//        }
//    }
//
//    // for each of our defense squads, if there aren't any enemy units near the position, remove the squad
//    std::set<std::string> uselessDefenseSquads;
//    for (const auto & kv : squadData.getSquads())
//    {
//        const Squad & squad = kv.second;
//        const SquadOrder & order = squad.getSquadOrder();
//
//        if (order.getType() != SquadOrderTypes::Defend)
//        {
//            continue;
//        }
//
//        bool enemyUnitInRange = false;
//        for (auto & unit : bot_.UnitInfo().getUnits(Players::Enemy))
//        {
//            if (Util::Dist(unit->pos, order.getPosition()) < order.getRadius())
//            {
//                enemyUnitInRange = true;
//                break;
//            }
//        }
//
//        if (!enemyUnitInRange)
//        {
//            squadData.getSquad(squad.getName()).clear();
//        }
//    }
//}
//
//void CombatCommander::updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded)
//{
//    auto & squadUnits = defenseSquad.getUnits();
//
//    // TODO: right now this will assign arbitrary defenders, change this so that we make sure they can attack air/ground
//
//    // if there's nothing left to defend, clear the squad
//    if (flyingDefendersNeeded == 0 && groundDefendersNeeded == 0)
//    {
//        defenseSquad.clear();
//        return;
//    }
//
//    size_t defendersNeeded = flyingDefendersNeeded + groundDefendersNeeded;
//    size_t defendersAdded = 0;
//
//    while (defendersNeeded > defendersAdded)
//    {
//        sc2::Tag defenderToAdd = findClosestDefender(defenseSquad, defenseSquad.getSquadOrder().getPosition());
//
//        if (defenderToAdd)
//        {
//            squadData.assignUnitToSquad(defenderToAdd, defenseSquad);
//            defendersAdded++;
//        }
//        else
//        {
//            break;
//        }
//    }
//}

const sc2::Unit* CombatCommander::FindClosestDefender(const Squad & defense_squad, const sc2::Point2D & pos)
{
    const sc2::Unit* closest_defender = nullptr;
    float min_distance = std::numeric_limits<float>::max();

    // TODO: add back special case of zergling rush defense

    for (auto & unit_info : combat_units_)
    {
        auto unit = unit_info->unit;
        BOT_ASSERT(unit, "null combat unit");

        if (!squad_data_.CanAssignUnitToSquad(unit, defense_squad))
        {
            continue;
        }

        const float dist = Util::Dist(unit->pos, pos);
        if (!closest_defender || (dist < min_distance))
        {
            closest_defender = unit;
            min_distance = dist;
        }
    }

    return closest_defender;
}


void CombatCommander::DrawSquadInformation()
{
    squad_data_.DrawSquadInformation();
}

