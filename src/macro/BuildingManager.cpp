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
    , building_placer_(bot)
    , debug_mode_(false)
    , reserved_minerals_(0)
    , reserved_gas_(0)
{

}

void BuildingManager::OnStart()
{
    building_placer_.OnStart();
}

// gets called every frame from GameCommander
void BuildingManager::OnFrame()
{
    for (auto & unit : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        // filter out units which aren't buildings under construction
        if (Util::IsBuilding(unit->unit_type))
        {
            std::stringstream ss;
            ss << unit->tag;
            bot_.DebugHelper().DrawText(unit->pos, ss.str());
        }
    }

    ValidateWorkersAndBuildings();          // check to see if assigned workers have died en route or while constructing
    AssignWorkersToUnassignedBuildings();   // assign workers to the unassigned buildings and label them 'planned'    
    CheckForDeadTerranBuilders();           // if we are terran and a building is under construction without a worker, assign a new one    
    ConstructAssignedBuildings();           // for each planned building, if the worker isn't constructing, send the command    
    CheckForStartedConstruction();          // check to see if any buildings have started construction and update data structures    
    CheckForCompletedBuildings();           // check to see if any buildings have completed and update data structures

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

// STEP 1: DO BOOK KEEPING ON WORKERS WHICH MAY HAVE DIED
void BuildingManager::ValidateWorkersAndBuildings()
{
    // TODO: if a terran worker dies while constructing and its building
    //       is under construction, place unit back into buildingsNeedingBuilders

    std::vector<Building> to_remove;

    // find any buildings which have become obsolete
    for (auto & b : buildings_)
    {
        if (b.status != BuildingStatus::UnderConstruction)
        {
            continue;
        }

        const auto building_unit = bot_.GetUnit(b.buildingUnitTag);

        // TODO: || !b.buildingUnit->getType().isBuilding()
        if (!building_unit || (building_unit->health <= 0))
        {
            to_remove.push_back(b);
        }
    }

    RemoveBuildings(to_remove);
}

// STEP 2: ASSIGN WORKERS TO BUILDINGS WITHOUT THEM
void BuildingManager::AssignWorkersToUnassignedBuildings()
{
    // for each building that doesn't have a builder, assign one
    for (Building & b : buildings_)
    {
        if (b.status != BuildingStatus::Unassigned)
        {
            continue;
        }

        BOT_ASSERT(b.builderUnitTag == 0, "Error: Tried to assign a builder to a building that already had one ");

        if (debug_mode_) { printf("Assigning Worker To: %s", sc2::UnitTypeToName(b.type)); }

        // grab a worker unit from WorkerManager which is closest to this final position
        const sc2::Point2DI test_location = GetBuildingLocation(b);
        if (!bot_.Map().IsOnMap(sc2::Point2D(test_location.x,test_location.y)))
        {
            continue;
        }

        b.finalPosition = test_location;

        // grab the worker unit from WorkerManager which is closest to this final position
        const sc2::Tag builder_unit_tag = bot_.InformationManager().GetBuilder(b);
        b.builderUnitTag = builder_unit_tag;
        if (!b.builderUnitTag)
        {
            continue;
        }

        // reserve this building's space
        building_placer_.ReserveTiles(b.finalPosition.x, b.finalPosition.y, Util::GetUnitTypeWidth(b.type, bot_), Util::GetUnitTypeHeight(b.type, bot_));

        if (b.type == sc2::UNIT_TYPEID::TERRAN_BARRACKS)
        {
            const sc2::Point2DI proxy_location = b.desiredPosition;
            std::cout << "finalplacementlocation" << proxy_location.x << "x " << proxy_location.y << "y " << std::endl;
        }

        b.status = BuildingStatus::Assigned;
    }
}

// STEP 3: IF OUR WORKERS DIED, ASSIGN THEM AGAIN.
void BuildingManager::CheckForDeadTerranBuilders()
{   // for each building that doesn't have a builder, assign one
    for (Building & b : buildings_)
    {
        if (b.status != BuildingStatus::Unassigned && bot_.GetUnit(b.builderUnitTag))
        {
            continue;
        }

        if (debug_mode_) { printf("Assigning Worker To: %s", sc2::UnitTypeToName(b.type)); }

        // grab the worker unit from WorkerManager which is closest to this final position
        const sc2::Tag builder_unit_tag = bot_.InformationManager().GetBuilder(b);
        b.builderUnitTag = builder_unit_tag;
    }
}

// STEP 4: ISSUE CONSTRUCTION ORDERS TO ASSIGN BUILDINGS AS NEEDED
void BuildingManager::ConstructAssignedBuildings()
{
    for (auto & b : buildings_)
    {
        if (b.status != BuildingStatus::Assigned)
        {
            continue;
        }

        // TODO: not sure if this is the correct way to tell if the building is constructing
        const sc2::AbilityID build_ability = Util::UnitTypeIDToAbilityID(b.type);
        const sc2::Unit* builder_unit = bot_.GetUnit(b.builderUnitTag);

        bool is_constructing = false;

        // if we're zerg and the builder unit is null, we assume it morphed into the building
        if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Zerg)
        {
            if (!builder_unit)
            {
                is_constructing = true;
            }
        }
        else
        {
            BOT_ASSERT(builder_unit, "null builder unit");
            is_constructing = (builder_unit->orders.size() > 0) && (builder_unit->orders[0].ability_id == build_ability);
        }

        // if that worker is not currently constructing
        if (!is_constructing)
        {
            // if we haven't explored the build position, go there
            if (!IsBuildingPositionExplored(b))
            {
                Micro::SmartMove(builder_unit, sc2::Point2D(b.finalPosition.x,b.finalPosition.y), bot_);
            }
            // if this is not the first time we've sent this guy to build this
            // it must be the case that something was in the way of building
            else if (b.buildCommandGiven)
            {
                // TODO: in here is where we would check to see if the builder died on the way
                //       or if things are taking too long, or the build location is no longer valid
            }
            else
            {
                // if it's a refinery, the build command has to be on the geyser unit tag
                if (Util::IsRefineryType(b.type))
                {
                    // first we find the geyser at the desired location
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
                        Micro::SmartBuildTag(bot_.GetUnit(b.builderUnitTag), b.type, geyser, bot_);
                    }
                    else
                    {
                        std::cout << "WARNING: NO VALID GEYSER UNIT FOUND TO BUILD ON, SKIPPING REFINERY\n";
                    }
                }
                // if it's not a refinery, we build right on the position
                else
                {
                    Micro::SmartBuild(bot_.GetUnit(b.builderUnitTag), b.type, sc2::Point2D(b.finalPosition.x, b.finalPosition.y), bot_);
                }

                // set the flag to true
                b.buildCommandGiven = true;
            }
        }
    }
}

// STEP 5: UPDATE DATA STRUCTURES FOR BUILDINGS STARTING CONSTRUCTION
void BuildingManager::CheckForStartedConstruction()
{
    // for each building unit which is being constructed
    for (auto & building_started : bot_.InformationManager().UnitInfo().GetUnits(PlayerArrayIndex::Self))
    {
        // filter out units which aren't buildings under construction
        if (!Util::IsBuilding(building_started->unit_type) || building_started->build_progress == 0.0f || building_started->build_progress == 1.0f)
        {
            continue;
        }

        // check all our building status objects to see if we have a match and if we do, update it

        for (auto & b : buildings_)
        {
            if (b.status != BuildingStatus::Assigned)
            {
                continue;
            }

            // check if the positions match
            const float dx = b.finalPosition.x - building_started->pos.x;
            const float dy = b.finalPosition.y - building_started->pos.y;

            if (dx*dx + dy*dy < 1)
            {
                if (b.buildingUnitTag != 0)
                {
                    std::cout << "Building mis-match somehow\n";
                }

                // the resources should now be spent, so unreserve them
                reserved_minerals_ -= Util::GetUnitTypeMineralPrice(building_started->unit_type, bot_);
                reserved_gas_      -= Util::GetUnitTypeGasPrice(building_started->unit_type, bot_);
                
                // flag it as started and set the buildingUnit
                b.underConstruction = true;
                b.buildingUnitTag = building_started->tag;

                // if we are zerg, the buildingUnit now becomes nullptr since it's destroyed
                if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Zerg)
                {
                    b.builderUnitTag = 0;
                }
                else if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Protoss)
                {
                    bot_.InformationManager().finishedWithUnit(b.builderUnitTag);
                    b.builderUnitTag = 0;
                }

                // put it in the under construction vector
                b.status = BuildingStatus::UnderConstruction;

                // free this space
                building_placer_.FreeTiles(b.finalPosition.x, b.finalPosition.y, Util::GetUnitTypeWidth(b.type, bot_), Util::GetUnitTypeHeight(b.type, bot_));

                // only one building will match
                break;
            }
        }
    }
}

// STEP 6: CHECK FOR COMPLETED BUILDINGS
void BuildingManager::CheckForCompletedBuildings()
{
    std::vector<Building> to_remove;

    // for each of our buildings under construction
    for (auto & b : buildings_)
    {
        if (b.status != BuildingStatus::UnderConstruction || !bot_.GetUnit(b.buildingUnitTag))
        {
            continue;
        }

        // if the unit has completed
        if (bot_.GetUnit(b.buildingUnitTag)->build_progress == 1.0f)
        {
            // if we are terran, give the worker back to worker manager
            if (bot_.InformationManager().GetPlayerRace(PlayerArrayIndex::Self) == sc2::Race::Terran)
            {
                bot_.InformationManager().finishedWithUnit(b.builderUnitTag);
            }

            // remove this unit from the under construction vector
            to_remove.push_back(b);
        }
    }

    RemoveBuildings(to_remove);
}

// add a new building to be constructed
void BuildingManager::AddBuildingTask(const sc2::UnitTypeID & type, const sc2::Point2DI& desired_position)
{
    reserved_minerals_  += Util::GetUnitTypeMineralPrice(type, bot_);
    reserved_gas_       += Util::GetUnitTypeGasPrice(type, bot_);

    Building b(type, desired_position);
    b.status = BuildingStatus::Unassigned;

    buildings_.push_back(b);
}

// TODO: may need to iterate over all tiles of the building footprint
bool BuildingManager::IsBuildingPositionExplored(const Building & b) const
{
    return bot_.Map().IsExplored( sc2::Point2D(b.finalPosition.x,b.finalPosition.y) );
}

int BuildingManager::GetReservedMinerals() const
{
    return reserved_minerals_;
}

int BuildingManager::GetReservedGas() const
{
    return reserved_gas_;
}

void BuildingManager::DrawBuildingInformation()
{
    building_placer_.DrawReservedTiles();

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

        if (b.builderUnitTag)
        {
            dss << "\n\nBuilder: " << b.builderUnitTag << std::endl;
        }

        if (b.buildingUnitTag)
        {
            dss << "Building: " << b.buildingUnitTag << std::endl << bot_.GetUnit(b.buildingUnitTag)->build_progress;
            bot_.DebugHelper().DrawText(bot_.GetUnit(b.buildingUnitTag)->pos, dss.str());
        }


        const std::string job_code = bot_.InformationManager().UnitInfo().GetUnitInfo(bot_.GetUnit(b.builderUnitTag))->GetJobCode();
        if (b.status == BuildingStatus::Unassigned)
        {
            ss << "Unassigned " << sc2::UnitTypeToName(b.type) << "    " << job_code << std::endl;
        }
        else if (b.status == BuildingStatus::Assigned)
        {
            ss << "Assigned " << sc2::UnitTypeToName(b.type) << "    " << b.builderUnitTag << " " << job_code << " (" << b.finalPosition.x << "," << b.finalPosition.y << ")\n";

            const float x1 = b.finalPosition.x;
            const float y1 = b.finalPosition.y;
            const float x2 = b.finalPosition.x + Util::GetUnitTypeWidth(b.type, bot_);
            const float y2 = b.finalPosition.y + Util::GetUnitTypeHeight(b.type, bot_);

            bot_.DebugHelper().DrawSquare(x1, y1, x2, y2, sc2::Colors::Red);
            //bot_.Map().drawLine(b.finalPosition, bot_.GetUnit(b.builderUnitTag)->pos, sc2::Colors::Yellow);
        }
        else if (b.status == BuildingStatus::UnderConstruction)
        {
            ss << "Constructing " << sc2::UnitTypeToName(b.type) << "    " << b.builderUnitTag << " " << b.buildingUnitTag << " " << job_code << std::endl;
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
        return building_placer_.GetRefineryPosition();
    }

    if (Util::IsTownHallType(b.type))
    {
        // TODO: fix this so we can actually expand
        //return bot_.Bases().getNextExpansion(Players::Self);
    }

    // get a position within our region
    // TODO: put back in special pylon / cannon spacing
    return building_placer_.GetBuildLocationNear(b, bot_.Config().BuildingSpacing);
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