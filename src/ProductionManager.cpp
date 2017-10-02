#include "ProductionManager.h"
#include "Util.h"
#include "ByunJRBot.h"
#include "Micro.h"

ProductionManager::ProductionManager(ByunJRBot & bot)
    : m_bot             (bot)
    , m_buildingManager (bot)
    , m_queue           (bot)
{

}

void ProductionManager::setBuildOrder(const BuildOrder & buildOrder)
{
    m_queue.clearAll();

    for (size_t i(0); i<buildOrder.size(); ++i)
    {
        m_queue.queueAsLowestPriority(buildOrder[i], true);
    }
}


void ProductionManager::onStart()
{
    m_planned_supply_depots = 0;
    m_buildingManager.onStart();
    setBuildOrder(m_bot.Strategy().getOpeningBookBuildOrder());
}

void ProductionManager::onFrame()
{
    // check the _queue for stuff we can build
    manageBuildOrderQueue();

    // TODO: if nothing is currently building, get a new goal from the strategy manager
    // TODO: detect if there's a build order deadlock once per second
    // TODO: triggers for game things like cloaked units etc

    m_buildingManager.onFrame();
    drawProductionInformation();
}

void ProductionManager::onBuildingConstructionComplete(const sc2::Unit unit) {
    if (unit.unit_type == sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT)
    {
        m_planned_supply_depots--;
    }
}

// on unit destroy
void ProductionManager::onUnitDestroy(const sc2::Unit & unit)
{
    // TODO: might have to re-do build order if a vital unit died
}

// Called every frame.
void ProductionManager::manageBuildOrderQueue()
{
    // if there is nothing in the queue, oh well
    if (m_queue.isEmpty())
    {
        return;
    }

    preventSupplyBlock();

    // the current item to be used
    BuildOrderItem & currentItem = m_queue.getHighestPriorityItem();

    // while there is still something left in the queue
    while (!m_queue.isEmpty())
    {
        // this is the unit which can produce the currentItem
        sc2::Tag producer = getProducer(currentItem.type);

        // check to see if we can make it right now
        bool canMake = canMakeNow(producer, currentItem.type);

        // TODO: if it's a building and we can't make it yet, predict the worker movement to the location

        // if we can make the current item
        if (producer && canMake)
        {
            // create it and remove it from the _queue
            create(producer, currentItem);
            m_queue.removeCurrentHighestPriorityItem();

            // don't actually loop around in here
            break;
        }
        // otherwise, if we can skip the current item
        else if (m_queue.canSkipItem())
        {
            // skip it
            m_queue.skipItem();

            // and get the next one
            currentItem = m_queue.getNextHighestPriorityItem();
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
        (m_bot.Observation()->GetFoodUsed() + productionCapacity())  // We used to compare only against things that are planned on being made // _planned_production)
                                                            // Is greater than 
        >=
        // the player supply capacity, including pylons in production. 
        // The depots in production is key, otherwise you will build hundreds of pylons while supply blocked.
        (m_bot.Observation()->GetFoodCap() + (m_planned_supply_depots * 8)) // Not sure how to get supply provided by a depot, lets just go with 8.
        )
    {
        m_planned_supply_depots++;
        m_queue.queueAsHighestPriority(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, true);
    }
}

int ProductionManager::productionCapacity() {
    // Probes take take up twice as much supply as usual because two can finish before a pylon is done.
    size_t commandCenters = m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER)
                          + m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND)
                          + m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS);

    size_t barracks = m_bot.UnitInfo().getUnitTypeCount(Players::Self, sc2::UNIT_TYPEID::TERRAN_BARRACKS);
    return (int) (commandCenters + barracks) * 2;
}

sc2::Tag ProductionManager::getProducer(sc2::UnitTypeID t, sc2::Point2D closestTo)
{
    // TODO: get the type of unit that builds this
    sc2::UnitTypeID producerType = Util::WhatBuilds(t);

    // make a set of all candidate producers
    std::vector<sc2::Tag> candidateProducers;
    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // reasons a unit can not train the desired type
        if (unit.unit_type != producerType) { continue; }
        if (unit.build_progress < 1.0f) { continue; }
        if (Util::IsBuilding(producerType) && unit.orders.size() > 0) { continue; }
        // TODO: if unit is not powered continue
        if (unit.is_flying) { continue; }

        // TODO: if the type is an addon, some special cases
        // TODO: if the type requires an addon and the producer doesn't have one

        // if we haven't cut it, add it to the set of candidates
        candidateProducers.push_back(unit.tag);
    }

    return getClosestUnitToPosition(candidateProducers, closestTo);
}

sc2::Tag ProductionManager::getClosestUnitToPosition(const std::vector<sc2::Tag> & units, sc2::Point2D closestTo)
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
        double distance = Util::Dist(m_bot.GetUnit(unit)->pos, closestTo);
        if (!closestUnit || distance < minDist)
        {
            closestUnit = unit;
            minDist = distance;
        }
    }

    return closestUnit;
}

// this function will check to see if all preconditions are met and then create a unit
void ProductionManager::create(sc2::Tag producer, BuildOrderItem & item)
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
            sc2::Point2D proxyLocation = m_bot.GameCommander().GetProxyLocation();
            m_buildingManager.addBuildingTask(t, proxyLocation);
        }
        else
        {
            m_buildingManager.addBuildingTask(t, m_bot.GetStartLocation());
        }
    }
    // if we're dealing with a non-building unit
    else
    {
        Micro::SmartTrain(producer, t, m_bot);
    }
}

bool ProductionManager::canMakeNow(sc2::Tag producerTag, sc2::UnitTypeID type)
{
    if (!meetsReservedResources(type))
    {
        return false;
    }
    if(producerTag==0)
        return false;

    sc2::AvailableAbilities available_abilities = m_bot.Query()->GetAbilitiesForUnit(m_bot.GetUnit(producerTag));

    // quick check if the unit can't do anything it certainly can't build the thing we want
    if (available_abilities.abilities.empty())
    {
        return false;
    }
    else
    {
        // check to see if one of the unit's available abilities matches the build ability type
        sc2::AbilityID buildTypeAbility = Util::UnitTypeIDToAbilityID(type);
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

bool ProductionManager::detectBuildOrderDeadlock()
{
    // TODO: detect build order deadlocks here
    return false;
}

int ProductionManager::getFreeMinerals()
{
    return m_bot.Observation()->GetMinerals() - m_buildingManager.getReservedMinerals();
}

int ProductionManager::getFreeGas()
{
    return m_bot.Observation()->GetVespene() - m_buildingManager.getReservedGas();
}

// return whether or not we meet resources, including building reserves
bool ProductionManager::meetsReservedResources(sc2::UnitTypeID type)
{
    // return whether or not we meet the resources
    return (Util::GetUnitTypeMineralPrice(type, m_bot) <= getFreeMinerals()) && (Util::GetUnitTypeGasPrice(type, m_bot) <= getFreeGas());
}

void ProductionManager::drawProductionInformation()
{
    if (!m_bot.Config().DrawProductionInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Production Information\n\n";

    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        if (unit.build_progress < 1.0f)
        {
            //ss << sc2::UnitTypeToName(unit.unit_type) << " " << unit.build_progress << "\n";
        }
    }

    ss << m_queue.getQueueInformation();

    m_bot.Map().drawTextScreen(sc2::Point2D(0.01f, 0.01f), ss.str(), sc2::Colors::Yellow);
}
