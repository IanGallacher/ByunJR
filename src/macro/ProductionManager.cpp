#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "macro/ProductionManager.h"
#include "micro/Micro.h"
#include "util/Util.h"

ProductionManager::ProductionManager(ByunJRBot & bot)
    : bot_             (bot)
    , buildingManager (bot)
    , queue           (bot)
{

}

void ProductionManager::setBuildOrder(const BuildOrder & buildOrder)
{
    queue.ClearAll();

    for (size_t i(0); i<buildOrder.Size(); ++i)
    {
        queue.QueueAsLowestPriority(buildOrder[i], true);
    }
}


void ProductionManager::onStart()
{
    planned_supply_depots = 0;
    buildingManager.OnStart();
    setBuildOrder(bot_.Strategy().GetOpeningBookBuildOrder());
}

void ProductionManager::OnFrame()
{
    // check the _queue for stuff we can build
    manageBuildOrderQueue();

    // TODO: if nothing is currently building, get a new goal from the strategy manager
    // TODO: detect if there's a build order deadlock once per second
    // TODO: triggers for game things like cloaked units etc

    buildingManager.OnFrame();
    drawProductionInformation();
}

void ProductionManager::onBuildingConstructionComplete(const sc2::Unit* unit) {
    if (unit->unit_type == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT)
    {
        planned_supply_depots--;
    }
}

// on unit destroy
void ProductionManager::onUnitDestroy(const sc2::Unit* unit)
{
    // TODO: might have to re-do build order if a vital unit died
}

// Called every frame.
void ProductionManager::manageBuildOrderQueue()
{
    // if there is nothing in the queue, oh well
    if (queue.IsEmpty())
    {
        return;
    }

    preventSupplyBlock();

    // the current item to be used
    BuildOrderItem & currentItem = queue.GetHighestPriorityItem();

    // while there is still something left in the queue
    while (!queue.IsEmpty())
    {
        // this is the unit which can produce the currentItem
        const sc2::Tag producer = getProducer(currentItem.type);

        // check to see if we can make it right now
        const bool canMake = canMakeNow(producer, currentItem.type);

        // TODO: if it's a building and we can't make it yet, predict the worker movement to the location

        // if we can make the current item
        if (producer && canMake)
        {
            // create it and remove it from the _queue
            create(producer, currentItem);
            queue.RemoveCurrentHighestPriorityItem();

            // don't actually loop around in here
            break;
        }
        // otherwise, if we can skip the current item
        else if (queue.CanSkipItem())
        {
            // skip it
            queue.SkipItem();

            // and get the next one
            currentItem = queue.GetNextHighestPriorityItem();
        }
        else
        {
            // so break out
            break;
        }
    }
}

// Every frame, see if more depots are required. 
void ProductionManager::preventSupplyBlock() {
    // If the current supply that we have plus the total amount of things that could be made 
    if (
        (bot_.Observation()->GetFoodUsed() + productionCapacity())  // We used to compare only against things that are planned on being made // _planned_production)
                                                            // Is greater than 
        >=
        // the player supply capacity, including pylons in production. 
        // The depots in production is key, otherwise you will build hundreds of pylons while supply blocked.
        (bot_.Observation()->GetFoodCap() + (planned_supply_depots * 8)) // Not sure how to get supply provided by a depot, lets just go with 8.
        )
    {
        planned_supply_depots++;
        queue.QueueAsHighestPriority(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true);
    }
}

int ProductionManager::productionCapacity() const
{
    // Probes take take up twice as much supply as usual because two can finish before a pylon is done.
    const  size_t commandCenters = bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER)
                                 + bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND)
                                 + bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS);

    const size_t barracks = bot_.InformationManager().UnitInfo().GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_BARRACKS);
    return (int) (commandCenters + barracks) * 2;
}

sc2::Tag ProductionManager::getProducer(sc2::UnitTypeID t, sc2::Point2D closestTo)
{
    // TODO: get the type of unit that builds this
    const sc2::UnitTypeID producerType = Util::WhatBuilds(t);

    // make a set of all candidate producers
    std::vector<sc2::Tag> candidateProducers;
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        // reasons a unit can not train the desired type
        if (unit->unit_type != producerType) { continue; }
        if (unit->build_progress < 1.0f) { continue; }
        if (Util::IsBuilding(producerType) && unit->orders.size() > 0) { continue; }
        // TODO: if unit is not powered continue
        if (unit->is_flying) { continue; }

        // TODO: if the type is an addon, some special cases
        // TODO: if the type requires an addon and the producer doesn't have one

        // if we haven't cut it, add it to the set of candidates
        candidateProducers.push_back(unit->tag);
    }

    return getClosestUnitToPosition(candidateProducers, closestTo);
}

sc2::Tag ProductionManager::getClosestUnitToPosition(const std::vector<sc2::Tag> & units, const sc2::Point2D closestTo) const
{
    if (units.size() == 0)
    {
        return 0;
    }

    // if we don't care where the unit is return the first one we have
    if (closestTo.x == 0 && closestTo.y == 0)
    {
        return units[0];
    }

    sc2::Tag closestUnit = 0;
    double minDist = std::numeric_limits<double>::max();

    for (auto & unit : units)
    {
        const double distance = Util::Dist(bot_.GetUnit(unit)->pos, closestTo);
        if (!closestUnit || distance < minDist)
        {
            closestUnit = unit;
            minDist = distance;
        }
    }

    return closestUnit;
}

// this function will check to see if all preconditions are met and then create a unit
void ProductionManager::create(const sc2::Tag producer, BuildOrderItem & item)
{
    if (!producer)
    {
        return;
    }

    sc2::UnitTypeID t = item.type;

    // if we're dealing with a building
    // TODO: deal with morphed buildings & addons
    if (Util::IsBuilding(t))
    {
        // send the building task to the building manager
        if (t == sc2::UNIT_TYPEID::TERRAN_BARRACKS)
        {
            const sc2::Point2DI proxyLocation = bot_.InformationManager().GetProxyLocation();
            buildingManager.AddBuildingTask(t, proxyLocation);
        }
        else
        {
            buildingManager.AddBuildingTask(t, sc2::Point2DI(bot_.GetStartLocation().x, bot_.GetStartLocation().y));
        }
    }
    // if we're dealing with a non-building unit
    else
    {
        Micro::SmartTrain(bot_.GetUnit(producer), t, bot_);
    }
}

bool ProductionManager::canMakeNow(const sc2::Tag producerTag, const sc2::UnitTypeID type)
{
    if (!meetsReservedResources(type))
    {
        return false;
    }
    if(producerTag==0)
        return false;

    sc2::AvailableAbilities available_abilities = bot_.Query()->GetAbilitiesForUnit(bot_.GetUnit(producerTag));

    // quick check if the unit can't do anything it certainly can't build the thing we want
    if (available_abilities.abilities.empty())
    {
        return false;
    }
    else
    {
        // check to see if one of the unit's available abilities matches the build ability type
        const sc2::AbilityID buildTypeAbility = Util::UnitTypeIDToAbilityID(type);
        for (const sc2::AvailableAbility & available_ability : available_abilities.abilities)
        {
            if (available_ability.ability_id == buildTypeAbility)
            {
                return true;
            }
        }
    }

    return false;
}

bool ProductionManager::detectBuildOrderDeadlock() const
{
    // TODO: detect build order deadlocks here
    return false;
}

int ProductionManager::getFreeMinerals()
{
    return bot_.Observation()->GetMinerals() - buildingManager.GetReservedMinerals();
}

int ProductionManager::getFreeGas()
{
    return bot_.Observation()->GetVespene() - buildingManager.GetReservedGas();
}

// return whether or not we meet resources, including building reserves
bool ProductionManager::meetsReservedResources(sc2::UnitTypeID type)
{
    // return whether or not we meet the resources
    return (Util::GetUnitTypeMineralPrice(type, bot_) <= getFreeMinerals()) && (Util::GetUnitTypeGasPrice(type, bot_) <= getFreeGas());
}

void ProductionManager::drawProductionInformation() const
{
    if (!bot_.Config().DrawProductionInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Production Information\n\n";

    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        if (unit->build_progress < 1.0f)
        {
            //ss << sc2::UnitTypeToName(unit->unit_type) << " " << unit->build_progress << std::endl;
        }
    }

    ss << queue.GetQueueInformation();

    bot_.DebugHelper().DrawTextScreen(sc2::Point2D(0.01f, 0.01f), ss.str(), sc2::Colors::Yellow);
}
