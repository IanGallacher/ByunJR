#include <sstream>

#include "ByunJRBot.h"
#include "TechLab/util/Util.h"

#include "common/Common.h"
#include "macro/ProductionManager.h"
#include "micro/Micro.h"

ProductionManager::ProductionManager(ByunJRBot & bot)
    : bot_             (bot)
    , building_manager_ (bot)
    , queue_           (bot)
{

}

void ProductionManager::OnStart()
{
    building_manager_.OnStart();
    queue_.SetBuildOrder(bot_.Strategy().GetOpeningBookBuildOrder());
}

void ProductionManager::OnFrame()
{
    // Dynamically spend our money based on our current needs. 
    PreventSupplyBlock();

    // check the _queue for stuff we can build
    // Do this before MacroUp to allow OrbitalCommands to build. (otherwise scvs will get trained instead)
    ManageBuildOrderQueue();

    MacroUp();


    // TODO: if nothing is currently building, get a new goal from the strategy manager
    // TODO: detect if there's a build order deadlock once per second
    // TODO: triggers for game things like cloaked units etc

    building_manager_.OnFrame();
}

void ProductionManager::OnUnitDestroyed(const sc2::Unit* building)
{
    // The building is dead! We can build where it used to be!
    if(Util::IsBuilding(building->unit_type))
        bot_.Strategy().BuildingPlacer().FreeTiles(building->unit_type, sc2::Point2DI(building->pos.x, building->pos.y));
}

// Called every frame.
void ProductionManager::ManageBuildOrderQueue()
{
    if (queue_.IsEmpty()) return;

    // the current item to be used
    BuildOrderItem & current_item = queue_.GetHighestPriorityItem();

    // while there is still something left in the queue
    while (!queue_.IsEmpty())
    {
        // this is the unit which can produce the current_item
        const sc2::Unit* producer = GetProducer(current_item.type);

        // check to see if we can make it right now
        const bool can_make = CanMakeNow(producer, current_item.type);

        // TODO: if it's a building and we can't make it yet, predict the worker movement to the location

        // if we can make the current item
        if (producer && can_make)
        {
            // create it and remove it from the _queue
            Create(producer, current_item);
            queue_.RemoveCurrentHighestPriorityItem();

            // don't actually loop around in here
            break;
        }
        // otherwise, if we can skip the current item
        else if (queue_.CanSkipItem())
        {
            // skip it
            queue_.SkipItem();

            // and get the next one
            current_item = queue_.GetNextHighestPriorityItem();
        }
        else
        {
            // so break out
            break;
        }

        // Rebuild the tech tree if we have to. 
        AddPrerequisitesToQueue(current_item.type);
    }
}

// If our base gets wiped, we need to know the tech tree well enough to rebuild.
void ProductionManager::AddPrerequisitesToQueue(sc2::UnitTypeID unit_type)
{
    sc2::UnitTypeID tech_requirement;
    if (Util::IsBuilding(unit_type))
    {
        tech_requirement = Util::GetUnitTypeData(unit_type, bot_).tech_requirement;
    }
    else
    {
        tech_requirement = Util::WhatBuilds(unit_type);
    }

    if (bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, tech_requirement)
        + NumberOfBuildingsQueued(tech_requirement) == 0)
    {
        queue_.QueueAsHighestPriority(tech_requirement, true);
    }
}

size_t ProductionManager::NumberOfBuildingsQueued(sc2::UnitTypeID unit_type) const
{
    return building_manager_.NumberOfBuildingTypeInProduction(unit_type);
}

// Buildings scvs have been sent to make plus the number of buildings in the queue. 
// Useful for counting how many depots have been planned to prevent a supply block. 
size_t ProductionManager::NumberOfBuildingsPlanned(sc2::UnitTypeID unit_type) const
{
    return building_manager_.NumberOfBuildingTypePlanned(unit_type)
        + queue_.GetItemsInQueueOfType(unit_type);
}

int ProductionManager::TrueUnitCount(sc2::UnitTypeID unit_type)
{
    return bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, unit_type)
        + queue_.GetItemsInQueueOfType(unit_type)
        + NumberOfBuildingsQueued(unit_type);
}

bool has_completed_wall = false;
// Every frame, see if more depots are required. 
void ProductionManager::PreventSupplyBlock() {
    // If the current supply that we have plus the total amount of things that could be made 
    if ( 
        // If we are at max supply, there is no point in building more depots. 
         bot_.Observation()->GetFoodCap() < 200
        && (bot_.Observation()->GetFoodUsed() + ProductionCapacity())  // We used to compare only against things that are planned on being made
                                                            // Is greater than 
        >=
        // the player supply capacity, including pylons in production. 
        // The depots in production is key, otherwise you will build hundreds of pylons while suppsuly blocked.
        ( bot_.Observation()->GetFoodCap() 
            + (TrueUnitCount(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT) * 8)) // Not sure how to get supply provided by a depot, lets just go with 8.
       )
    {
        queue_.QueueAsHighestPriority(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true);
    }

    // Build wall if needed.
    if (bot_.Info().GetPlayerRace(sc2::Unit::Alliance::Enemy) == sc2::Zerg
        && Util::GetGameTimeInSeconds(bot_) > 50 && !has_completed_wall)
    {
        has_completed_wall = true;
        queue_.QueueAsHighestPriority(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true);
        queue_.QueueAsHighestPriority(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true);
    }
}

// A set of rules to dictate what we should build based on our current strategy.
void ProductionManager::MacroUp() {
    const int scv_count = bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_SCV);
    const int base_count = bot_.Info().UnitInfo().GetNumberOfCompletedTownHalls(sc2::Unit::Alliance::Self);
    const int barracks_count = bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_BARRACKS);
    const int starport_count = bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_STARPORT);

    if (bot_.Strategy().ShouldExpandNow()
        // Don't queue more bases than you have minerals for.
     && queue_.GetItemsInQueueOfType(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER)
        + NumberOfBuildingsQueued(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER) 
        < bot_.Observation()->GetMinerals() / 400)
    {
        queue_.QueueItem(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER, 2);
    }
    // Once we are done following a build order, let's go on to do some other stuff!
    if(base_count > 1)
    {
        if(TrueUnitCount(sc2::UNIT_TYPEID::TERRAN_REFINERY) < bot_.Info().Bases().NumberOfControlledGeysers())
            queue_.QueueItem(sc2::UNIT_TYPEID::TERRAN_REFINERY, 2);

        // Upgrade to Orbital Commands!
        for (const auto & base : bot_.Info().Bases().GetOccupiedBaseLocations(sc2::Unit::Alliance::Self))
        {
            if (base->GetTownHall())
            {
                const sc2::Unit* drop_mineral = bot_.Info().GetClosestMineralField(base->GetTownHall());
                bot_.Actions()->UnitCommand(base->GetTownHall(), sc2::ABILITY_ID::EFFECT_CALLDOWNMULE, drop_mineral);
                Micro::SmartTrain(base->GetTownHall(), sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND, bot_);
            }
        }
    }

    if(bot_.Strategy().MacroGoal() == Strategy::ReaperRush)
    {
        for (const auto & unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
        {
            // Constantly make SCV's. At this level of play, no reason not to.
            // Skip one scv to get the proxy barracks up faster. 
            if (Util::IsTownHall(unit) && unit->orders.size() == 0 && (scv_count < 15 || barracks_count > 1) && scv_count < base_count * 23 && scv_count < 70)
            {
                Micro::SmartTrain(unit, sc2::UNIT_TYPEID::TERRAN_SCV, bot_);
            }

            // Get ready to make CattleBruisers
            if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_BARRACKS && unit->orders.size() == 0)
            {
                Micro::SmartTrain(unit, sc2::UNIT_TYPEID::TERRAN_REAPER, bot_);
                //queue_.QueueItem(sc2::UNIT_TYPEID::TERRAN_REAPER, 5);
            }
        }
    }
    else if(bot_.Strategy().MacroGoal() == Strategy::BattlecruiserMacro)
    {
        for (const auto & unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
        {
            // Constantly make SCV's. At this level of play, no reason not to.
            if (Util::IsTownHall(unit) && unit->orders.size() == 0 && scv_count < base_count * 22 && scv_count < 70)
            {
                Micro::SmartTrain(unit, sc2::UNIT_TYPEID::TERRAN_SCV, bot_);
            }

            // Get ready to make CattleBruisers
            if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_STARPORT && unit->orders.size() == 0)
            {
                Micro::SmartTrain(unit, sc2::UNIT_TYPEID::TERRAN_TECHLAB, bot_);
                Micro::SmartTrain(unit, sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER, bot_);
            }
            if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_ARMORY && unit->orders.size() == 0)
            {
                bot_.Actions()->UnitCommand(unit, sc2::ABILITY_ID::RESEARCH_TERRANSHIPWEAPONS);
            }
            if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_ARMORY && unit->orders.size() == 0)
            {
                bot_.Actions()->UnitCommand(unit, sc2::ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATING);
            }
            if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_FUSIONCORE && unit->orders.size() == 0)
            {
                bot_.Actions()->UnitCommand(unit, sc2::ABILITY_ID::RESEARCH_BATTLECRUISERWEAPONREFIT);
            }

            if (base_count > 1 && TrueUnitCount(sc2::UNIT_TYPEID::TERRAN_STARPORT) < base_count - 1)
            {
                queue_.QueueItem(sc2::UNIT_TYPEID::TERRAN_STARPORT, 2);
            } 
        }
    }
}

int ProductionManager::ProductionCapacity() const
{
    const  size_t command_centers = bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER)
                                  + bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND)
                                  + bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS);

    const size_t barracks = bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_BARRACKS);
    const size_t factory = bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_FACTORY);
    const size_t starport = bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, sc2::UNIT_TYPEID::TERRAN_STARPORT);
    // Factories and starports can build really supply intensive units. Make sure we have enough supply. 
    return static_cast<int>(command_centers + barracks) * 2 + factory * 4 + starport * 12;
}

const sc2::Unit* ProductionManager::GetProducer(const sc2::UnitTypeID t, const sc2::Point2D closest_to) const
{
    // TODO: get the type of unit that builds this
    const sc2::UnitTypeID producer_type = Util::WhatBuilds(t);

    // make a set of all candidate producers
    std::vector<const sc2::Unit*> candidate_producers;
    for (auto & unit : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
    {
        // reasons a unit can not train the desired type
        if (unit->unit_type != producer_type) { continue; }
        if (unit->build_progress < 1.0f) { continue; }
        if (Util::IsBuilding(producer_type) && unit->orders.size() > 0) { continue; }
        // TODO: if unit is not powered continue
        if (unit->is_flying) { continue; }

        // TODO: if the type is an addon, some special cases
        // TODO: if the type requires an addon and the producer doesn't have one

        // if we haven't cut it, add it to the set of candidates
        candidate_producers.push_back(unit);
    }

    return GetClosestUnitToPosition(candidate_producers, closest_to);
}

const sc2::Unit* ProductionManager::GetClosestUnitToPosition(const std::vector<const sc2::Unit*> & units, const sc2::Point2D closest_to) const
{
    if (units.size() == 0)
    {
        return nullptr;
    }

    // if we don't care where the unit is return the first one we have
    if (closest_to.x == 0 && closest_to.y == 0)
    {
        return units[0];
    }

    const sc2::Unit* closest_unit = nullptr;
    double min_dist = std::numeric_limits<double>::max();

    for (auto & unit : units)
    {
        const double distance = Util::Dist(unit->pos, closest_to);
        if (!closest_unit || distance < min_dist)
        {
            closest_unit = unit;
            min_dist = distance;
        }
    }

    return closest_unit;
}

// this function will check to see if all preconditions are met and then create a unit
void ProductionManager::Create(const sc2::Unit* producer, BuildOrderItem & item)
{
    if (!producer) return;

    const sc2::UnitTypeID item_type = item.type;

    // Make the unit using whatever command is necessary
    if (Util::IsMorphCommand(Util::UnitTypeIDToAbilityID(item.type)))
        Micro::SmartTrain(producer, item_type, bot_);
    else if (Util::IsBuilding(item_type))
        building_manager_.AddBuildingTask(item_type);
    else
        Micro::SmartTrain(producer, item_type, bot_);
}

bool ProductionManager::CanMakeNow(const sc2::Unit* producer_unit, const sc2::UnitTypeID type) const
{
    const sc2::Point2DI point = bot_.Strategy().BuildingPlacer().GetBuildLocationForType(type);
    const int dist = producer_unit ? bot_.Query()->PathingDistance(producer_unit, sc2::Point2D(point.x, point.y)) : 0;
    if (!MeetsReservedResources(type, dist))
    {
        return false;
    }
    if(producer_unit==nullptr)
        return false;

    //sc2::AvailableAbilities available_abilities = bot_.Query()->GetAbilitiesForUnit(producer_unit,true );

    //// quick check if the unit can't do anything it certainly can't build the thing we want
    //if (available_abilities.abilities.empty())
    //{
    //    return false;
    //}
    //else
    //{
    //    // check to see if one of the unit's available abilities matches the build ability type
    //    const sc2::AbilityID build_type_ability = Util::UnitTypeIDToAbilityID(type);
    //    for (const sc2::AvailableAbility & available_ability : available_abilities.abilities)
    //    {
    //        if (available_ability.ability_id == build_type_ability)
    //        {
    //            return true;
    //        }
    //    }
    //}
    return true;
    //return false;
}

// Return whether or not we meet resources.
// distance is optional. If it is greater or equal to 0, 
// take into account income earned while the scv is traveling to the desired location.
bool ProductionManager::MeetsReservedResources(const sc2::UnitTypeID type, int distance) const
{
    int minerals_en_route = 0;
    int gas_en_route = 0;
    if (distance >= 0)
    {
        minerals_en_route = bot_.Info().Bases().MineralIncomePerSecond() * (distance / 2.813); // 2.813 is worker speed. 
        gas_en_route = bot_.Info().Bases().GasIncomePerSecond() * (distance / 2.813);
    }

    // Can we afford the unit?
    if( (Util::GetUnitTypeMineralPrice(type, bot_) + building_manager_.PlannedMinerals() 
           <= bot_.Observation()->GetMinerals() + minerals_en_route)
     && (Util::GetUnitTypeGasPrice(type, bot_) <= bot_.Observation()->GetVespene() + gas_en_route))
        return true;
    return false; // break on rax three, make sure scv gets there accounting for travel dist
}

std::string ProductionManager::ToString() const
{
    std::stringstream ss;
    ss << "Production Information" << std::endl << std::endl;
    ss << queue_.ToString();
    return ss.str();
}

std::string ProductionManager::BuildingInfoString() const
{
    return building_manager_.ToString();
}