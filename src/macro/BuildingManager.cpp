#include <iostream>
#include <sstream>

#include "ByunJRBot.h"
#include "TechLab/util/Util.h"

#include "common/BotAssert.h"
#include "macro/Building.h"
#include "macro/BuildingManager.h"
#include "micro/Micro.h"

BuildingManager::BuildingManager(ByunJRBot & bot)
    : bot_(bot)
{ }

void BuildingManager::OnStart()
{

}

void BuildingManager::OnFrame()
{
    StopConstructingDeadBuildings();        // Check to see if assigned workers have died en route or while constructing.
    FindBuildingLocation();                 // Find a good place to build the building.
    AssignWorkersToUnassignedBuildings();   // If we are terran and a building is under construction without a worker, assign a new one.
    ConstructAssignedBuildings();           // For each planned building, if the worker isn't constructing, send the command.
    CheckForStartedConstruction();          // Check to see if any buildings have started construction and update data structures.
    CheckForCompletedBuildings();           // Check to see if any buildings have completed and update data structures.
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

// Returns the buildings that a worker has been sent to build, but has not yet started construction.
// Even if the "green preview building" is present, this function will not count it until the physical building is present. 
size_t BuildingManager::NumberOfBuildingTypePlanned(sc2::UnitTypeID unit_type) const
{
    size_t count = 0;
    for (const auto & b : buildings_)
    {
        if (b.type == unit_type && b.status != BuildingStatus::UnderConstruction) ++count;
    }
    return count;
}

// Returns the number of buildings that the building manager is currently in charge. 
// Regardless of if construction has started or not. 
size_t BuildingManager::NumberOfBuildingTypeInProduction(sc2::UnitTypeID unit_type) const
{
    size_t count = 0;
    for (const auto & b : buildings_)
    {
        if (b.type == unit_type) ++count;
    }
    return count;
}

int BuildingManager::PlannedMinerals() const
{
    int planned_minerals = 0;
    for (const auto & b : buildings_)
    {
        bool sent_command_already = false;
		if (!b.builderUnit) continue;

        for (sc2::UnitOrder the_order : b.builderUnit->orders)
        {
            if (the_order.ability_id == Util::UnitTypeIDToAbilityID(b.type))
            {
                sent_command_already = true;
                break;
            }
        }
        if (!sent_command_already)
        {
            // if we can't build the building, then don't bother reserving the resources. the unit is on the way!
            auto tech_requirement = Util::GetUnitTypeData(b.type, bot_).tech_requirement;
            if (bot_.Info().UnitInfo().GetUnitTypeCount(sc2::Unit::Alliance::Self, tech_requirement)
                + NumberOfBuildingTypeInProduction(tech_requirement) != 0)
            {
                planned_minerals += Util::GetMineralPrice(b.type, bot_);
            }
        }
    }

    return planned_minerals;
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

// STEP 2: Find a suitable location to build the requested building. 
void BuildingManager::FindBuildingLocation()
{
    // For each building that doesn't have a builder, assign one.
    for (Building & b : buildings_)
    {
        // If the building does not yet have a worker assigned to it, go assign one. 
        if (b.finalPosition != sc2::Point2DI(0,0)) continue;

        b.finalPosition = bot_.Strategy().BuildingPlacer().GetBuildLocationForType(b.type);
        BOT_ASSERT(bot_.Info().Map().IsOnMap(sc2::Point2D(b.finalPosition.x, b.finalPosition.y)), "Tried to build the building off of the map.");

        // Reserve this building's space.
        bot_.Strategy().BuildingPlacer().ReserveTiles(b.type, b.finalPosition);

        if (b.type == sc2::UNIT_TYPEID::TERRAN_BARRACKS)
        {
            std::cout << "finalplacementlocation" << b.finalPosition.x << "x " << b.finalPosition.y << "y " << std::endl;
        }
    }
}

// STEP 3: Assign workers to buildings without them. 
//         This also takes into account the case where workers die en route or constructing a building. 
void BuildingManager::AssignWorkersToUnassignedBuildings()
{   
    // For each building that doesn't have a builder, assign one.
    for (Building & b : buildings_)
    {
        if (b.status == BuildingStatus::Unassigned || !b.builderUnit || !b.builderUnit->is_alive)
        {
            // Grab the worker unit from WorkerManager which is closest to this final position.
            const std::vector<UnitMission> acceptable_missions{ UnitMission::Idle, UnitMission::Minerals, UnitMission::Proxy };
            b.builderUnit = bot_.Info().GetClosestUnitWithJob(sc2::Point2D(b.finalPosition.x, b.finalPosition.y), acceptable_missions);
            
            // if the worker exists (one may not have been found in rare cases)
            if (b.builderUnit)
            {
                bot_.Info().UnitInfo().SetJob(b.builderUnit, UnitMission::Build);
            }

            // If all our workers are dead or preocupied, no worries, we can try again next game loop.
            if (!b.builderUnit || !b.builderUnit->is_alive)
                continue;

            b.status = BuildingStatus::Assigned;
        }
    }
}

// STEP 4: Issue construction orders. 
void BuildingManager::ConstructAssignedBuildings()
{
    for (auto & b : buildings_)
    {
        if (b.status == BuildingStatus::Assigned)
        {
            // TODO: not sure if this is the correct way to tell if the building is constructing
            const sc2::AbilityID build_ability = Util::UnitTypeIDToAbilityID(b.type);
            const sc2::Unit* builder_unit = b.builderUnit;

            // We are unable to find a worker for the building. Don't try to attempt if we don't have a worker.
            // This handles the situation where a Zerg worker morphs into a building (destroys the worker)
            if (!builder_unit) return;

            // is_construction_in_progress checks and proves that the building construction is actually in progress.
            // Sometimes a worker will fail to build a building, even if it was previously issued a command to build the building. 
            // Example: a unit will accidently block placement of a building, preventing the building from ever being built. 
            bool is_construction_in_progress = (builder_unit->orders.size() > 0) && (builder_unit->orders[0].ability_id == build_ability);

            // If the building is not under construction, attempt to begin construction. 
            if (!is_construction_in_progress)
            {
                // If we haven't explored the build position, go there.
                // For all current ladder maps, this will always be true. 
                // We are leaving this in here to ensure future compatability (campaign maps, broodwar, etc)
                if (!IsBuildingPositionExplored(b))
                {
                    Micro::SmartMove(builder_unit, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), bot_);
                }
                // If this is not the first time we've sent this guy to build this.
                // It must be the case that something was in the way of building.
                else if (b.buildCommandGiven)
                {
                    if (b.type == sc2::UNIT_TYPEID::TERRAN_REFINERY)
                    {
                        bot_.Info().UnitInfo().SetJob(b.builderUnit, UnitMission::Build);
                        const sc2::Unit* refinery = bot_.Info().FindNeutralUnitAtPosition(b.finalPosition);
                        Micro::SmartBuildGeyser(b.builderUnit, b.type, refinery, bot_);
                    }
                    else
                    {
                        // If the build was interrupted, the worker will go back to gathering minerals. 
                        // Once we continue building, mark the unit as such.
                        bot_.Info().UnitInfo().SetJob(b.builderUnit, UnitMission::Build);
                        Micro::SmartBuild(b.builderUnit, b.type, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), bot_);
                    }
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
                            Micro::SmartBuildGeyser(b.builderUnit, b.type, geyser, bot_);
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

                    // If the build was interruptted, the worker will go back to gathering minerals. 
                    // Once we continue building, mark the unit as such.
                    bot_.Info().UnitInfo().SetJob(b.builderUnit, UnitMission::Build);
                    // Don't spam build commands. 
                    b.buildCommandGiven = true;
                }
            }
        }
    }
}

// STEP 5: Update data structures for buildings starting construction.
void BuildingManager::CheckForStartedConstruction()
{
    // For each building unit which is being constructed.
    for (auto & building_started : bot_.Info().UnitInfo().GetUnits(sc2::Unit::Alliance::Self))
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
                if (bot_.Info().GetPlayerRace(sc2::Unit::Alliance::Self) == sc2::Race::Zerg)
                {
                    b.builderUnit = nullptr;
                }
                else if (bot_.Info().GetPlayerRace(sc2::Unit::Alliance::Self) == sc2::Race::Protoss)
                {
                    // Protoss does not need to keep the worker around after starting construction.
                    bot_.Info().UnitInfo().SetJob(b.builderUnit, UnitMission::Idle);
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
            if (bot_.Info().GetPlayerRace(sc2::Unit::Alliance::Self) == sc2::Race::Terran)
            {
                bot_.Info().UnitInfo().SetJob(b.builderUnit, UnitMission::Idle);
            }

            // This building is completed, no need to ever attempt construction again.
            to_remove.push_back(b);
        }
    }

    RemoveBuildings(to_remove);
}
#pragma endregion

// Add a new building to be constructed.
void BuildingManager::AddBuildingTask(const sc2::UnitTypeID & type)
{
    Building b(type);
    b.status = BuildingStatus::Unassigned;
    buildings_.push_back(b);
}

// TODO: may need to iterate over all tiles of the building footprint.
bool BuildingManager::IsBuildingPositionExplored(const Building & b) const
{
    return bot_.Info().Map().IsExplored( sc2::Point2D(b.finalPosition.x,b.finalPosition.y) );
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
    return bot_.Strategy().BuildingPlacer().CanBuildHereWithSpace(x, y, type, 0);
}

void BuildingManager::DrawBuildingInfo() const
{
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

        const UnitInfo* u = b.builderUnit ? bot_.Info().UnitInfo().GetUnitInfo(b.builderUnit) : nullptr;
        const std::string job_code = u ? u->GetJobCode() : "NoWorkerFound";
        if (b.status == BuildingStatus::Assigned)
        {
            bot_.DebugHelper().DrawBoxAroundUnit(b.type, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), sc2::Colors::Red);
            bot_.DebugHelper().DrawLine(sc2::Point2D(b.finalPosition.x, b.finalPosition.y), b.builderUnit->pos, sc2::Colors::Yellow);
        }
    }
}

std::string BuildingManager::ToString() const
{
    std::stringstream ss;
    ss << "Building Information " << std::endl;

    for (const auto & b : buildings_)
    {
        std::stringstream dss;

        if (b.builderUnit)
        {
            dss << "\n\nBuilder: " << b.builderUnit << std::endl;
        } else return "null builder unit";

        if (b.buildingUnit)
        {
            dss << "Building: " << b.buildingUnit << std::endl << b.buildingUnit->build_progress;
            bot_.DebugHelper().DrawText(b.buildingUnit->pos, dss.str());
        }
        const UnitInfo* u = bot_.Info().UnitInfo().GetUnitInfo(b.builderUnit);
        const std::string job_code = u ? u->GetJobCode() : "NoWorkerFound";
        if (b.status == BuildingStatus::Unassigned)
        {
            ss << "Unassigned " << sc2::UnitTypeToName(b.type) << "    " << job_code << std::endl;
        }
        else if (b.status == BuildingStatus::Assigned)
        {
            ss << "Assigned " << sc2::UnitTypeToName(b.type) << "    " << b.builderUnit << " " << job_code << " (" << b.finalPosition.x << "," << b.finalPosition.y << ")\n";

            bot_.DebugHelper().DrawBoxAroundUnit(b.type, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), sc2::Colors::Red);
            bot_.DebugHelper().DrawLine(sc2::Point2D(b.finalPosition.x, b.finalPosition.y), b.builderUnit->pos, sc2::Colors::Yellow);
        }
        else if (b.status == BuildingStatus::UnderConstruction)
        {
            ss << "Constructing " << sc2::UnitTypeToName(b.type) << "    " << b.builderUnit << " " << b.buildingUnit << " " << job_code << std::endl;
        }
    }

    return ss.str();
}
