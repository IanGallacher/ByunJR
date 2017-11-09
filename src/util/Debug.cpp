#include <sstream>
#include <vector>

#include "util/Debug.h"
#include "util/Util.h"


float max_z_ = 11;

DebugManager::DebugManager(sc2::Agent & bot, InformationManager & information_manager)
    : bot_(bot)
    , information_manager_(information_manager)
{
}

void DebugManager::DrawResourceDebugInfo() const
{
    const std::map<sc2::Tag, UnitInfo> ui = information_manager_.UnitInfo().GetUnitInfoMap(sc2::Unit::Alliance::Self);

    for (auto const & unit_info : ui)
    {
        if (Util::IsBuilding(unit_info.second.unit->unit_type)) continue;
        DrawText(unit_info.second.unit->pos, unit_info.second.GetJobCode());

        //auto depot = bot_.GetUnit(workerData.getWorkerDepot(workerTag));
        //if (depot)
        //{
        //    information_manager_.Map().drawLine(bot_.GetUnit(workerTag)->pos, depot->pos);
        //}
    }
}


void DebugManager::DrawEnemyDPSMap(std::vector<std::vector<int>> dps_map) const
{
    for(int y = 0; y < dps_map.size(); ++y)
    {
        for (int x = 0; x < dps_map[y].size(); ++x)
        {
            if(dps_map[y][x] != 1)
            DrawBox((float)x - 0.5, (float)y - 0.5, (float)x + 0.5, (float)y + 0.5);
        }
    }
}

void DebugManager::DrawMapSectors() const
{
    for (int y = 0; y < information_manager_.Map().TrueMapHeight(); ++y)
    {
        for (int x = 0; x < information_manager_.Map().TrueMapWidth(); ++x)
        {
            if (!information_manager_.Map().IsOnMap(x, y))
            {
                continue;
            }
            std::stringstream ss;
            ss << information_manager_.Map().GetSectorNumber(x, y);
            bot_.Debug()->DebugTextOut(ss.str(), sc2::Point3D(x + 0.5f, y + 0.5f, max_z_ + 0.1f), sc2::Colors::Yellow);
        }
    }
}

void DebugManager::DrawBaseLocations() const
{
    for (auto & base_location : information_manager_.Bases().GetBaseLocations())
    {
        DrawBaseLocation(*base_location);
    }

    // draw a purple sphere at the next expansion location
    const sc2::Point2D next_expansion_position = information_manager_.Bases().GetNextExpansion(sc2::Unit::Alliance::Self);

    DrawSphere(next_expansion_position, 1, sc2::Colors::Purple);
    DrawText(next_expansion_position, "Next Expansion Location", sc2::Colors::Purple);
}


void DebugManager::DrawBaseLocation(const BaseLocation & base_location) const 
{
    const sc2::Point2D base_pos = base_location.GetPosition();
    DrawSphere(base_pos, 1.0f, sc2::Colors::Yellow);

    std::stringstream ss;
    ss << "Start Loc:    " << (base_location.IsStartLocation() ? "true" : "false") << std::endl;
    ss << "Minerals:     " << base_location.GetMinerals().size() << std::endl;
    ss << "Geysers:      " << base_location.GetGeysers().size() << std::endl;
    ss << "Occupied By:  ";

    if (base_location.IsOccupiedByPlayer(sc2::Unit::Alliance::Self))
    {
        ss << "Self ";
    }

    if (base_location.IsOccupiedByPlayer(sc2::Unit::Alliance::Enemy))
    {
        ss << "Enemy ";
    }

    DrawText(base_pos, ss.str().c_str());

    DrawBoxAroundUnit(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER, base_pos);

    for (auto & mineral : base_location.GetMinerals())
    {
        DrawSphere(mineral->pos, 1.0f, sc2::Colors::Teal);
    }

    for (auto & geyser : base_location.GetGeysers())
    {
        DrawSphere(geyser->pos, 1.0f, sc2::Colors::Green);
    }

    if (base_location.IsStartLocation())
    {
        DrawSphere(base_location.GetPosition(), 1.0f, sc2::Colors::Red);
    }

    //m_distanceMap.draw(bot_);
}
//
//void DistanceMap::Draw(ByunJRBot & bot) const
//{
//    const int tiles_to_draw = 200;
//    for (size_t i(0); i < tiles_to_draw; ++i)
//    {
//        auto & tile = sorted_tile_positions_[i];
//        const int dist = GetDistance(tile.x, tile.y);
//
//        const sc2::Point2D text_pos(tile.x + 0.5f, tile.y + 0.5f);
//        std::stringstream ss;
//        ss << dist;
//
//        bot.DebugHelper().DrawText(text_pos, ss.str());
//    }
//}

void DebugManager::DrawMapWalkableTiles() const
{
    const sc2::Point2D camera = bot_.Observation()->GetCameraPos();
    for (int y = 0; y < information_manager_.Map().TrueMapHeight(); ++y)
    {
        for (int x = 0; x < information_manager_.Map().TrueMapWidth(); ++x)
        {
            if (!information_manager_.Map().IsOnMap(x, y))
            {
                continue;
            }
            sc2::Color color = information_manager_.Map().IsWalkable(x, y) ? sc2::Colors::Green : sc2::Colors::Red;
            if (information_manager_.Map().IsWalkable(x, y) && !information_manager_.Map().IsBuildable(x, y))
            {
                color = sc2::Colors::Yellow;
            }

            DrawSquareOnMap(x, y, x + 1, y + 1, color);
        }
    }
}

//void DebugManager::drawDepotDebugInfo()
//{
//    for (auto & baseTag : depots)
//    {
//        const auto base = bot_.GetUnit(baseTag);
//
//        if (!base) continue;
//        std::stringstream ss;
//        ss << "Workers: " << getNumAssignedWorkers(base);
//
//        information_manager_.Map().drawText(base->pos, ss.str());
//    }
//}

void DebugManager::DrawAllUnitInformation() const
{
    std::stringstream ss;
    const std::map<sc2::Tag, UnitInfo> ui = information_manager_.UnitInfo().GetUnitInfoMap(sc2::Unit::Alliance::Self);

    ss << "Workers: " << ui.size() << std::endl;

    int yspace = 0;

    for (auto const & unit_info : ui )
    {
        if (Util::IsBuilding(unit_info.second.unit->unit_type)) continue;
        ss << unit_info.second.GetJobCode() << " " << unit_info.first << std::endl;
    }

    DrawTextScreen(sc2::Point2D(0.75f, 0.2f), ss.str());
}

void DebugManager::DrawAllSelectedUnitsDebugInfo() const
{
    const sc2::Unit* unit = nullptr;
    for (const sc2::Unit* unit : bot_.Observation()->GetUnits())
    {
        if (unit->is_selected && unit->alliance == sc2::Unit::Self) {
            DrawUnitDebugInfo(unit);
        }
    }
}

std::string GetAbilityText(const sc2::AbilityID ability_id) {
    std::string str;
    str += sc2::AbilityTypeToName(ability_id);
    str += " (";
    str += std::to_string(uint32_t(ability_id));
    str += ")";
    return str;
}

void DebugManager::DrawUnitDebugInfo(const sc2::Unit* unit) const
{
    if (!unit) { return; }

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
    bot_.Debug()->DebugTextOut(debug_txt, unit->pos, sc2::Colors::Green);

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
        bot_.Debug()->DebugLineOut(p0, p1, sc2::Colors::Yellow);
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
        bot_.Debug()->DebugBoxOut(p_min, p_max, sc2::Colors::Blue);
    }

    // Sphere around the unit.
    {
        sc2::Point3D p = unit->pos;
        p.z += 0.1f; // Raise the line off the ground a bit so it renders more clearly.
        bot_.Debug()->DebugSphereOut(p, 1.25f, sc2::Colors::Purple);
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
        bot_.Debug()->DebugSphereOut(target, 1.25f, sc2::Colors::Blue);
        bot_.Debug()->DebugTextOut(target_info, p1, sc2::Colors::Yellow);
    }
}

void DebugManager::DrawLine(const float x1, const float y1, const float x2, const float y2, const sc2::Color & color) const
{
    bot_.Debug()->DebugLineOut(sc2::Point3D(x1, y1, max_z_ + 0.2f), sc2::Point3D(x2, y2, max_z_ + 0.2f), color);
}

void DebugManager::DrawLine(const sc2::Point2D & min, const sc2::Point2D max, const sc2::Color & color) const
{
    bot_.Debug()->DebugLineOut(sc2::Point3D(min.x, min.y, max_z_ + 0.2f), sc2::Point3D(max.x, max.y, max_z_ + 0.2f), color);
}

void DebugManager::DrawSquareOnMap(const float x1, const float y1, const float x2, const float y2, const sc2::Color & color) const
{
    // Add 0.5f to make sure the z coordinate does not intersect with the terrain.
    const float zcoordx1y1 = 12; // bot_.Observation()->TerrainHeight(sc2::Point2D(x1, y1)) + 0.5f;
    const float zcoordx1y2 = 12; //bot_.Observation()->TerrainHeight(sc2::Point2D(x1, y2)) + 0.5f;
    const float zcoordx2y1 = 12; //bot_.Observation()->TerrainHeight(sc2::Point2D(x2, y1)) + 0.5f;
    const float zcoordx2y2 = 12; //bot_.Observation()->TerrainHeight(sc2::Point2D(x2, y2)) + 0.5f;
    bot_.Debug()->DebugLineOut(sc2::Point3D(x1, y1, zcoordx1y1), sc2::Point3D(x2, y1, zcoordx2y1), color);
    bot_.Debug()->DebugLineOut(sc2::Point3D(x1, y1, zcoordx1y1), sc2::Point3D(x1, y2, zcoordx1y2), color);
    bot_.Debug()->DebugLineOut(sc2::Point3D(x2, y2, zcoordx2y2), sc2::Point3D(x2, y1, zcoordx2y1), color);
    bot_.Debug()->DebugLineOut(sc2::Point3D(x2, y2, zcoordx2y2), sc2::Point3D(x1, y2, zcoordx1y2), color);
}

void DebugManager::DrawBox(const float x1, const float y1, const float x2, const float y2, const sc2::Color & color) const
{
    bot_.Debug()->DebugBoxOut(sc2::Point3D(x1, y1, max_z_ + 2.0f), sc2::Point3D(x2, y2, max_z_ - 5.0f), color);
}

void DebugManager::DrawBox(const sc2::Point3D& min, const sc2::Point2D max, const sc2::Color & color) const
{
    bot_.Debug()->DebugBoxOut(sc2::Point3D(min.x, min.y, max_z_ + 5.0f), sc2::Point3D(max.x, max.y, max_z_ - 5.0f), color);
}

void DebugManager::DrawBox(const sc2::Point3D & min, const sc2::Point3D max, const sc2::Color & color) const
{
    bot_.Debug()->DebugBoxOut(sc2::Point3D(min.x, min.y, min.z), sc2::Point3D(max.x, max.y, max.z), color);
}

void DebugManager::DrawSphere(const sc2::Point2D& pos, const float radius, const sc2::Color & color) const
{
    bot_.Debug()->DebugSphereOut(sc2::Point3D(pos.x, pos.y, max_z_), radius, color);
}

void DebugManager::DrawSphere(const float x, const float y, const float radius, const sc2::Color & color) const
{
    bot_.Debug()->DebugSphereOut(sc2::Point3D(x, y, max_z_), radius, color);
}

void DebugManager::DrawText(const sc2::Point2D & pos, const std::string & str, const sc2::Color & color) const
{
    bot_.Debug()->DebugTextOut(str, sc2::Point3D(pos.x, pos.y, max_z_), color);
}

void DebugManager::DrawTextScreen(const sc2::Point2D& pos, const std::string & str, const sc2::Color & color) const
{
    bot_.Debug()->DebugTextOut(str, pos, color);
}

void DebugManager::DrawBoxAroundUnit(const sc2::UnitTypeID unit_type, const sc2::Point2D unit_pos, const sc2::Color color) const
{
    DrawBoxAroundUnit(unit_type, sc2::Point3D(unit_pos.x, unit_pos.y, information_manager_.Map().TerrainHeight(unit_pos.x, unit_pos.y)), color);
}

void DebugManager::DrawBoxAroundUnit(const sc2::UnitTypeID unit_type, const sc2::Point3D unit_pos, const sc2::Color color) const
{
    // In Starcraft 2, units are square. Get either the width or height, and divide by 2 to get radius. 
    const float radius = Util::GetUnitTypeHeight(unit_type, bot_)/2;

    sc2::Point3D p_min = unit_pos;
    p_min.x -= radius;
    p_min.y -= radius;
    //p_min.z -= radius;

    sc2::Point3D p_max = unit_pos;
    p_max.x += radius;
    p_max.y += radius;
    p_max.z += radius*2;

    DrawBox(p_min, p_max, color);
}

void DebugManager::DrawBoxAroundUnit(const sc2::Unit* unit, const sc2::Color color) const
{
    if (!unit) { std::cout << "Warning. No unit was given to drawBoxAroundUnit" << std::endl; return; }

    sc2::Point3D p_min = unit->pos;
    p_min.x -= unit->radius;
    p_min.y -= unit->radius;
    p_min.z -= unit->radius;

    sc2::Point3D p_max = unit->pos;
    p_max.x += unit->radius;
    p_max.y += unit->radius;
    p_max.z += unit->radius;

    DrawBox(p_min, p_max, color);
}

void DebugManager::DrawSphereAroundUnit(const sc2::Unit* unit, const sc2::Color color) const
{
    if (!unit) { return; }

    DrawSphere(unit->pos, 1, color);
}