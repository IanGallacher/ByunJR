#include <sstream>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "common/Common.h"
#include "macro/Building.h"
#include "macro/BuildingManager.h"
#include "micro/Micro.h"
#include "util/Util.h"

BuildingManager::BuildingManager(ByunJRBot & bot)
    : bot_(bot)
{

}

void BuildingManager::OnStart()
{

}

void BuildingManager::OnFrame()
{
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        // Filter out units which aren't buildings under construction.
        if (Util::IsBuilding(unit->unit_type))
        {
            std::stringstream ss;
            ss << unit->tag;
            bot_.DebugHelper().DrawText(unit->pos, ss.str());
        }
    }

    StopConstructingDeadBuildings();        // Check to see if assigned workers have died en route or while constructing.
    AssignWorkersToUnassignedBuildings();   // Assign workers to the unassigned buildings and label them 'planned'
    CheckForDeadBuilders();                 // If we are terran and a building is under construction without a worker, assign a new one.
    ConstructAssignedBuildings();           // For each planned building, if the worker isn't constructing, send the command.
    CheckForStartedConstruction();          // Check to see if any buildings have started construction and update data structures.
    CheckForCompletedBuildings();           // Check to see if any buildings have completed and update data structures.

    DrawBuildingInformation();
}

bool BuildingManager::IsBeingBuilt(const sc2::UnitTypeID type)
{
    for (auto & b : buildings_)
    {
        if (b.type == type)
        {
            return true;
        }
    }

    return false;
}

#pragma region The six steps for constructing a building. 
// STEP 1: If a building has dies during construction, do not attempt to build it again.
void BuildingManager::StopConstructingDeadBuildings()
{
    std::vector<Building> to_remove;

    // Look through all our buildings for ones that died during construction.
    for (auto & b : buildings_)
    {
        // The building MUST be under construction. If it is not, look for the next one. 
        if (b.status != BuildingStatus::UnderConstruction) continue;

        const auto building_unit = b.buildingUnit;

        BOT_ASSERT(Util::IsBuilding(b.buildingUnit->unit_type), "Error: Tried to assign a builder to a building that already had one ");

        if (!building_unit || (building_unit->health <= 0))
        {
            to_remove.push_back(b);
        }
    }

    RemoveBuildings(to_remove);
}

// STEP 2: Assign workers to buildings without them. 
void BuildingManager::AssignWorkersToUnassignedBuildings()
{
    // For each building that doesn't have a builder, assign one.
    for (Building & b : buildings_)
    {
        // If the building does not yet have a worker assigned to it, go assign one. 
        if (b.status != BuildingStatus::Unassigned) continue;

        // Only assign a worker to the building if it does not yet have one, or the worker died en route. 
        BOT_ASSERT(b.builderUnit == nullptr || !b.builderUnit->is_alive, "Error: Tried to assign a builder to a building that already had one ");

        // Grab a worker unit from WorkerManager which is closest to this final position.
        const sc2::Point2DI test_location = GetBuildingLocation(b);
        if (!bot_.Map().IsOnMap(sc2::Point2D(test_location.x,test_location.y)))
            continue;

        b.finalPosition = test_location;

        // Grab the worker unit from WorkerManager which is closest to this final position.
        const sc2::Unit* builder_unit_tag = bot_.InformationManager().GetBuilder(b);
        b.builderUnit = builder_unit_tag;
        if (!b.builderUnit) 
            continue;

        // Reserve this building's space.
        bot_.InformationManager().BuildingPlacer().ReserveTiles(b.type, b.finalPosition);

        if (b.type == sc2::UNIT_TYPEID::TERRAN_BARRACKS)
        {
            std::cout << "finalplacementlocation" << b.desiredPosition.x << "x " << b.desiredPosition.y << "y " << std::endl;
        }

        b.status = BuildingStatus::Assigned;
    }
}

// STEP 3: If a worker while trying to build a building, find another worker to use to build the building.
//         This is different from step 2 because we don't need to look for a building placement location a second time. 
void BuildingManager::CheckForDeadBuilders()
{   
    // For each building that doesn't have a builder, assign one.
    for (Building & b : buildings_)
    {
        if (b.status != BuildingStatus::Unassigned && b.builderUnit->is_alive) continue;

        // grab the worker unit from WorkerManager which is closest to this final position
        const sc2::Unit* builder_unit = bot_.InformationManager().GetBuilder(b);
        b.builderUnit = builder_unit;
    }
}

// STEP 4: Issue construction orders. 
void BuildingManager::ConstructAssignedBuildings()
{
    for (auto & b : buildings_)
    {
        if (b.status != BuildingStatus::Assigned) continue;

        // TODO: not sure if this is the correct way to tell if the building is constructing
        const sc2::AbilityID build_ability = Util::UnitTypeIDToAbilityID(b.type);
        const sc2::Unit* builder_unit = b.builderUnit;


        // is_construction_in_progress checks and proves that the building construction is actually in progress.
        // Sometimes a worker will fail to build a building, even if it was previously issued a command to build the building. 
        // Example: a unit will accidently block placement of a building, preventing the building from ever being built. 
        bool is_construction_in_progress = false;

        // If we're zerg and the builder unit is null, we assume it morphed into the building.
        if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Zerg)
        {
            if (!builder_unit)
            {
                is_construction_in_progress = true;
            }
        }
        else
        {
            BOT_ASSERT(builder_unit, "null builder unit");
            is_construction_in_progress = (builder_unit->orders.size() > 0) && (builder_unit->orders[0].ability_id == build_ability);
        }

        // If the building is not under construction, attempt to begin construction. 
        if (!is_construction_in_progress)
        {
            // If we haven't explored the build position, go there.
            // For all current ladder maps, this will always be true. 
            // We are leaving this in here to insure future compatability (campaign maps, broodwar, etc)
            if (!IsBuildingPositionExplored(b))
            {
                Micro::SmartMove(builder_unit, sc2::Point2D(b.finalPosition.x,b.finalPosition.y), bot_);
            }
            // If this is not the first time we've sent this guy to build this.
            // It must be the case that something was in the way of building.
            else if (b.buildCommandGiven)
            {
                Micro::SmartBuild(b.builderUnit, b.type, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), bot_);
                // TODO: in here is where we would check to see if the builder died on the way
                //       or if things are taking too long, or the build location is no longer valid
            }
            // If is_construction_in_progress is not true AND we have already sent a command to build a building, something must have gone wrong. 
            // Resend the command to build the building.
            else
            {
                // If it's a refinery, we have to build on a geyser. 
                if (Util::IsRefineryType(b.type))
                {
                    // First we find the geyser at the desired location.
                    const sc2::Unit* geyser = nullptr;
                    for (auto & unit : bot_.Observation()->GetUnits())
                    {
                        if (Util::IsGeyser(unit) && Util::Dist(b.finalPosition, unit->pos) < 3)
                        {
                            geyser = unit;
                            break;
                        }
                    }
                    
                    if (geyser)
                    {
                        Micro::SmartBuildTag(b.builderUnit, b.type, geyser, bot_);
                    }
                    else
                    {
                        std::cout << "WARNING: NO VALID GEYSER UNIT FOUND TO BUILD ON, SKIPPING REFINERY" << std::endl;
                    }
                }
                // If it's not a refinery, we build right on the position.
                else
                {
                    Micro::SmartBuild(b.builderUnit, b.type, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), bot_);
                }

                // Don't spam build commands. 
                b.buildCommandGiven = true;
            }
        }
    }
}

// STEP 5: Update data structures for buildings starting construction.
void BuildingManager::CheckForStartedConstruction()
{
    // For each building unit which is being constructed.
    for (auto & building_started : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        // Filter out units which aren't buildings under construction.
        if (!Util::IsBuilding(building_started->unit_type) || building_started->build_progress == 0.0f || building_started->build_progress == 1.0f)
        {
            continue;
        }

        // Check all our building status objects to see if we have a match and if we do, update it.

        for (auto & b : buildings_)
        {
            if (b.status != BuildingStatus::Assigned) continue;

            // Check if the positions match.
            const float dx = b.finalPosition.x - building_started->pos.x;
            const float dy = b.finalPosition.y - building_started->pos.y;

            if (dx*dx + dy*dy < 1)
            {
                if (b.buildingUnit != nullptr)
                {
                    std::cout << "Starting construction on a building that was already assigned a partially completed building." << std::endl;
                }
                
                // Flag it as started and set the buildingUnit.
                b.underConstruction = true;
                b.buildingUnit = building_started;

                // If we are zerg, the buildingUnit now becomes nullptr since it's destroyed.
                if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Zerg)
                {
                    b.builderUnit = nullptr;
                }
                else if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Protoss)
                {
                    // Protoss does not need to keep the worker around after starting construction.
                    bot_.InformationManager().UnitInfo().SetJob(b.builderUnit, UnitMission::Idle);
                    b.builderUnit = nullptr;
                }

                // Mark the building as "under construction"
                b.status = BuildingStatus::UnderConstruction;

                // Once we find a building that matches, stop looking for additional matches. 
                break;
            }
        }
    }
}

// STEP 6: Check for completed buildings.
void BuildingManager::CheckForCompletedBuildings()
{
    std::vector<Building> to_remove;

    // For each of our buildings under construction.
    for (auto & b : buildings_)
    {
        if (b.status != BuildingStatus::UnderConstruction || !b.buildingUnit) continue; 

        // If the building has completed.
        if (b.buildingUnit->build_progress == 1.0f)
        {
            // If we are Terran, give the worker back to worker manager.
            if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Terran)
            {
                bot_.InformationManager().UnitInfo().SetJob(b.builderUnit, UnitMission::Idle);
            }

            // This building is completed, no need to ever attempt construction again.
            to_remove.push_back(b);
        }
    }

    RemoveBuildings(to_remove);
}
#pragma endregion

// Add a new building to be constructed.
void BuildingManager::AddBuildingTask(const sc2::UnitTypeID & type, const sc2::Point2DI& desired_position)
{
    Building b(type, desired_position);
    b.status = BuildingStatus::Unassigned;
    buildings_.push_back(b);
}

// TODO: may need to iterate over all tiles of the building footprint.
bool BuildingManager::IsBuildingPositionExplored(const Building & b) const
{
    return bot_.Map().IsExplored( sc2::Point2D(b.finalPosition.x,b.finalPosition.y) );
}

void BuildingManager::DrawBuildingInformation()
{
    bot_.InformationManager().BuildingPlacer().DrawReservedTiles();

    if (!bot_.Config().DrawBuildingInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Building Information " << buildings_.size() << "\n\n\n";

    int yspace = 0;

    for (const auto & b : buildings_)
    {
        std::stringstream dss;

        if (b.builderUnit)
        {
            dss << "\n\nBuilder: " << b.builderUnit << std::endl;
        }

        if (b.buildingUnit)
        {
            dss << "Building: " << b.buildingUnit << std::endl << b.buildingUnit->build_progress;
            bot_.DebugHelper().DrawText(b.buildingUnit->pos, dss.str());
        }

        const std::string job_code = bot_.InformationManager().UnitInfo().GetUnitInfo(b.builderUnit)->GetJobCode();
        if (b.status == BuildingStatus::Unassigned)
        {
            ss << "Unassigned " << sc2::UnitTypeToName(b.type) << "    " << job_code << std::endl;
        }
        else if (b.status == BuildingStatus::Assigned)
        {
            ss << "Assigned " << sc2::UnitTypeToName(b.type) << "    " << b.builderUnit << " " << job_code << " (" << b.finalPosition.x << "," << b.finalPosition.y << ")\n";

            bot_.DebugHelper().DrawBoxAroundUnit(b.type, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), sc2::Colors::Red);
            //bot_.Map().drawLine(b.finalPosition, bot_.GetUnit(b.builderUnitTag)->pos, sc2::Colors::Yellow);
        }
        else if (b.status == BuildingStatus::UnderConstruction)
        {
            ss << "Constructing " << sc2::UnitTypeToName(b.type) << "    " << b.builderUnit << " " << b.buildingUnit << " " << job_code << std::endl;
        }
    }

    bot_.DebugHelper().DrawTextScreen(sc2::Point2D(0.05f, 0.05f), ss.str());
}

std::vector<sc2::UnitTypeID> BuildingManager::BuildingsQueued() const
{
    std::vector<sc2::UnitTypeID> buildings_queued;

    for (const auto & b : buildings_)
    {
        if (b.status == BuildingStatus::Unassigned || b.status == BuildingStatus::Assigned)
        {
            buildings_queued.push_back(b.type);
        }
    }

    return buildings_queued;
}

sc2::Point2DI BuildingManager::GetBuildingLocation(const Building & b) const
{
    // TODO: if requires psi and we have no pylons return 0

    if (Util::IsRefineryType(b.type))
    {
        return bot_.InformationManager().BuildingPlacer().GetRefineryPosition();
    }

    if (Util::IsTownHallType(b.type))
    {
        // TODO: fix this so we can actually expand
        //return bot_.Bases().getNextExpansion(Players::Self);
    }

    // get a position within our region
    // TODO: put back in special pylon / cannon spacing
    return bot_.InformationManager().BuildingPlacer().GetBuildLocationNear(b, bot_.Config().BuildingSpacing);
}

void BuildingManager::RemoveBuildings(const std::vector<Building> & to_remove)
{
    for (auto & b : to_remove)
    {
        const auto & it = std::find(buildings_.begin(), buildings_.end(), b);

        if (it != buildings_.end())
        {
            buildings_.erase(it);
        }
    }
}

bool BuildingManager::IsValidBuildLocation(const int x, const int y, const sc2::UnitTypeID type) const
{
    return bot_.InformationManager().BuildingPlacer().CanBuildHereWithSpace(x, y, type, 0);
}
