#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "common/BotAssert.h"
#include "information/UnitInfoManager.h"
#include "util/Util.h"

UnitInfoManager::UnitInfoManager(ByunJRBot & bot)
    : bot_(bot)
{

}

void UnitInfoManager::OnStart()
{

}

void UnitInfoManager::OnFrame()
{
    // If units are created or modified, update the unitInfo object.
    UpdateUnitInfo();

    //m_workers.OnFrame();
    DrawUnitInformation();
    DrawSelectedUnitDebugInfo();
}

// If units die, update the unitInfo object.
void UnitInfoManager::OnUnitDestroyed(const sc2::Unit* unit)
{
    unit_data_[Util::GetPlayer(unit)].KillUnit(unit);
}


void UnitInfoManager::UpdateUnitInfo()
{
    units_[PlayerArrayIndex::Self].clear();
    units_[PlayerArrayIndex::Enemy].clear();

    for (auto & unit : bot_.Observation()->GetUnits())
    {
        if (Util::GetPlayer(unit) == PlayerArrayIndex::Self || Util::GetPlayer(unit) == PlayerArrayIndex::Enemy)
        {
            UpdateUnit(unit);
            units_[Util::GetPlayer(unit)].push_back(unit);
        }        
    }

    // remove bad enemy units
    unit_data_[PlayerArrayIndex::Self].RemoveBadUnits();
    unit_data_[PlayerArrayIndex::Enemy].RemoveBadUnits();
}

const std::map<sc2::Tag, UnitInfo>& UnitInfoManager::GetUnitInfoMap(const PlayerArrayIndex player) const
{
    return GetUnitData(player).GetUnitInfoMap();
}

const std::vector<const sc2::Unit*>& UnitInfoManager::GetUnits(PlayerArrayIndex player) const
{
    BOT_ASSERT(units_.find(player) != units_.end(), "Couldn't find player units: %d", player);

    return units_.at(player);
}

std::string GetAbilityText(const sc2::AbilityID ability_id) {
    std::string str;
    str += sc2::AbilityTypeToName(ability_id);
    str += " (";
    str += std::to_string(uint32_t(ability_id));
    str += ")";
    return str;
}

void UnitInfoManager::DrawSelectedUnitDebugInfo() const
{
    const sc2::Unit* unit = nullptr;
    for (const sc2::Unit* u : bot_.Observation()->GetUnits())
    {
        if (u->is_selected && u->alliance == sc2::Unit::Self) {
            unit = u;
            break;
        }
    }

    if (!unit) { return; }

    auto debug = bot_.Debug();
    auto query = bot_.Query();
    auto abilities = bot_.Observation()->GetAbilityData();

    std::string debug_txt = UnitTypeToName(unit->unit_type);
    if (debug_txt.length() < 1) 
    {
        debug_txt = "(Unknown name)";
        assert(0);
    }
    debug_txt += " (" + std::to_string(unit->unit_type) + ")";
        
    sc2::AvailableAbilities available_abilities = query->GetAbilitiesForUnit(unit);
    if (available_abilities.abilities.size() < 1) 
    {
        std::cout << "No abilities available for this unit" << std::endl;
    }
    else 
    {
        for (const sc2::AvailableAbility & available_ability : available_abilities.abilities) 
        {
            if (available_ability.ability_id >= abilities.size()) { continue; }

            const sc2::AbilityData & ability = abilities[available_ability.ability_id];

            debug_txt += GetAbilityText(ability.ability_id) + "\n";
        }
    }
    debug->DebugTextOut(debug_txt, unit->pos, sc2::Colors::Green);

    // Show the direction of the unit.
    sc2::Point3D p1; // Use this to show target distance.
    {
        const float length = 5.0f;
        sc2::Point3D p0 = unit->pos;
        p0.z += 0.1f; // Raise the line off the ground a bit so it renders more clearly.
        p1 = unit->pos;
        assert(unit->facing >= 0.0f && unit->facing < 6.29f);
        p1.x += length * std::cos(unit->facing);
        p1.y += length * std::sin(unit->facing);
        debug->DebugLineOut(p0, p1, sc2::Colors::Yellow);
    }

    // Box around the unit.
    {
        sc2::Point3D p_min = unit->pos;
        p_min.x -= 2.0f;
        p_min.y -= 2.0f;
        p_min.z -= 2.0f;
        sc2::Point3D p_max = unit->pos;
        p_max.x += 2.0f;
        p_max.y += 2.0f;
        p_max.z += 2.0f;
        debug->DebugBoxOut(p_min, p_max, sc2::Colors::Blue);
    }

    // Sphere around the unit.
    {
        sc2::Point3D p = unit->pos;
        p.z += 0.1f; // Raise the line off the ground a bit so it renders more clearly.
        debug->DebugSphereOut(p, 1.25f, sc2::Colors::Purple);
    }

    // Pathing query to get the target.
    bool has_target = false;
    sc2::Point3D target;
    std::string target_info;
    for (const sc2::UnitOrder& unit_order : unit->orders)
    {
        // TODO: Need to determine if there is a target point, no target point, or the target is a unit/snapshot.
        target.x = unit_order.target_pos.x;
        target.y = unit_order.target_pos.y;
        target.z = p1.z;
        has_target = true;

        target_info = "Target:\n";
        if (unit_order.target_unit_tag != 0x0LL) {
            target_info += "Tag: " + std::to_string(unit_order.target_unit_tag) + "\n";
        }
        if (unit_order.progress != 0.0f && unit_order.progress != 1.0f) {
            target_info += "Progress: " + std::to_string(unit_order.progress) + "\n";
        }

        // Perform the pathing query.
        {
            const float distance = query->PathingDistance(unit->pos, target);
            target_info += "\nPathing dist: " + std::to_string(distance);
        }

        break;
    }

    if (has_target)
    {
        sc2::Point3D p = target;
        p.z += 0.1f; // Raise the line off the ground a bit so it renders more clearly.
        debug->DebugSphereOut(target, 1.25f, sc2::Colors::Blue);
        debug->DebugTextOut(target_info, p1, sc2::Colors::Yellow);
    }
}

// passing in a unit type of 0 returns a count of all units
size_t UnitInfoManager::GetUnitTypeCount(const PlayerArrayIndex player, const sc2::UnitTypeID type, const bool include_incomplete_buildings) const
{
    size_t count = 0;

    for (auto & unit : GetUnits(player))
    {
        if ((!type || type == unit->unit_type) && (include_incomplete_buildings || unit->build_progress == 1.0f))
        {
            count++;
        }
    }

    return count;
}

void UnitInfoManager::DrawUnitInformation() const
{
    if (!bot_.Config().DrawEnemyUnitInfo)
    {
        return;
    }

    std::stringstream ss;

    // for each unit in the queue
    for (int t(0); t < 255; t++)
    {
        const int num_units =      unit_data_.at(PlayerArrayIndex::Self).GetNumUnits(t);
        const int num_dead_units =  unit_data_.at(PlayerArrayIndex::Enemy).GetNumDeadUnits(t);

        // if there exist units in the vector
        if (num_units > 0)
        {
            ss << num_units << "   " << num_dead_units << "   " << sc2::UnitTypeToName(t) << std::endl;
        }
    }
    
    for (auto & kv : GetUnitData(PlayerArrayIndex::Enemy).GetUnitInfoMap())
    {
        bot_.Debug()->DebugSphereOut(kv.second.lastPosition, 0.5f);
        bot_.Debug()->DebugTextOut(sc2::UnitTypeToName(kv.second.type), kv.second.lastPosition);
    }
}

int UnitInfoManager::GetNumAssignedWorkers(const sc2::Unit* depot)
{
    return unit_data_[PlayerArrayIndex::Self].GetNumAssignedWorkers(depot);
}

// mission_target is optional. Only required when a repair target is needed. 
void UnitInfoManager::SetJob(const sc2::Unit* unit, const UnitMission job, const sc2::Unit* mission_target)
{
    unit_data_[Util::GetPlayer(unit)].SetJob(unit, job, bot_, mission_target);
}

// This can only return your workers, not the enemy workers. 
std::set<const UnitInfo*> UnitInfoManager::GetWorkers()
{
    return unit_data_[PlayerArrayIndex::Self].GetWorkers();
}

// This can only return your scouts, not the enemy scouts. 
std::set<const UnitInfo*> UnitInfoManager::GetScouts()
{
    return unit_data_[PlayerArrayIndex::Self].GetScouts();
}

const UnitInfo* UnitInfoManager::GetUnitInfo(const sc2::Unit* unit)
{
    return &unit_data_[Util::GetPlayer(unit)].GetUnitInfoMap().at(unit->tag);
}

void UnitInfoManager::UpdateUnit(const sc2::Unit* unit)
{
    if (!(Util::GetPlayer(unit) == PlayerArrayIndex::Self || Util::GetPlayer(unit) == PlayerArrayIndex::Enemy))
    {
        return;
    }

    unit_data_[Util::GetPlayer(unit)].UpdateUnit(unit);
}

// is the unit valid?
bool UnitInfoManager::IsValidUnit(const sc2::Unit* unit)
{
    // we only care about our units and enemy units
    if (!(Util::GetPlayer(unit) == PlayerArrayIndex::Self || Util::GetPlayer(unit) == PlayerArrayIndex::Enemy))
    {
        return false;
    }

    // if it's a weird unit, don't bother
    if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_EGG || unit->unit_type == sc2::UNIT_TYPEID::ZERG_LARVA)
    {
        return false;
    }

    // if the position isn't valid throw it out
    if (!bot_.Map().IsOnMap(unit->pos))
    {
        return false;
    }
    
    // s'all good baby baby
    return true;
}

void UnitInfoManager::GetNearbyForce(std::vector<UnitInfo> & unit_info, sc2::Point2D p, const PlayerArrayIndex player, const float radius) const
{
    bool has_bunker = false;
    // for each unit we know about for that player
    for (const auto & kv : GetUnitData(player).GetUnitInfoMap())
    {
        const UnitInfo & ui(kv.second);

        // if it's a combat unit we care about
        // and it's finished! 
        if (Util::IsCombatUnitType(ui.type) && Util::Dist(ui.lastPosition,p) <= radius)
        {
            // add it to the vector
            unit_info.push_back(ui);
        }
    }
}

// Shorthand for the weird syntax required to get the unit data. This is available only inside this function. 
const UnitData & UnitInfoManager::GetUnitData(const PlayerArrayIndex player) const
{
    return unit_data_.find(player)->second;
}

// getCombatUnits only has any meaning for your own units. 
// unitData does not publicly expose this function to prevent accidental requsting of the set of enemy combat units. 
std::set<const UnitInfo*> UnitInfoManager::GetCombatUnits() const
{
    return GetUnitData(PlayerArrayIndex::Self).GetCombatUnits();
}

int UnitInfoManager::GetNumRepairWorkers(const sc2::Unit* unit) const
{
    return GetUnitData(PlayerArrayIndex::Self).GetNumRepairWorkers(unit);
}

// The game considers raised and lowered supply depots as different units. 
// This gets the total number you have, regardless if they are raised or lower. 
int UnitInfoManager::GetNumDepots(PlayerArrayIndex self) const
{
    return GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT)
        + GetUnitTypeCount(PlayerArrayIndex::Self, sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED);
}
