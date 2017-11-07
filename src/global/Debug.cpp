#include <sstream>
#include <vector>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "global/Debug.h"
#include "util/Util.h"


float max_z_ = 11;

DebugManager::DebugManager(ByunJRBot & bot)
    : bot_(bot)
{
}

void DebugManager::DrawResourceDebugInfo() const
{
    const std::map<sc2::Tag, UnitInfo> ui = bot_.InformationManager().UnitInfo().GetUnitInfoMap(sc2::Unit::Alliance::Self);

    for (auto const & unit_info : ui)
    {
        if (Util::IsBuilding(unit_info.second.unit->unit_type)) continue;
        bot_.DebugHelper().DrawText(unit_info.second.unit->pos, unit_info.second.GetJobCode());

        //auto depot = bot_.GetUnit(workerData.getWorkerDepot(workerTag));
        //if (depot)
        //{
        //    bot_.InformationManager().Map().drawLine(bot_.GetUnit(workerTag)->pos, depot->pos);
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
    for (int y = 0; y < bot_.InformationManager().Map().TrueMapHeight(); ++y)
    {
        for (int x = 0; x < bot_.InformationManager().Map().TrueMapWidth(); ++x)
        {
            if (!bot_.InformationManager().Map().IsOnMap(x, y))
            {
                continue;
            }
            std::stringstream ss;
            ss << bot_.InformationManager().Map().GetSectorNumber(x, y);
            bot_.Debug()->DebugTextOut(ss.str(), sc2::Point3D(x + 0.5f, y + 0.5f, max_z_ + 0.1f), sc2::Colors::Yellow);
        }
    }
}

void DebugManager::DrawBaseLocations() const
{

    for (auto & base_location : bot_.InformationManager().Bases().GetBaseLocations())
    {
        //base_location->Draw();
    }

    // draw a purple sphere at the next expansion location
    const sc2::Point2D next_expansion_position = bot_.InformationManager().Bases().GetNextExpansion(sc2::Unit::Alliance::Self);

    bot_.DebugHelper().DrawSphere(next_expansion_position, 1, sc2::Colors::Purple);
    bot_.DebugHelper().DrawText(next_expansion_position, "Next Expansion Location", sc2::Colors::Purple);
}
//
//
//void BaseLocation::Draw()
//{
//    bot_.DebugHelper().DrawSphere(center_of_resources_, 1.0f, sc2::Colors::Yellow);
//
//    std::stringstream ss;
//    ss << "BaseLocation: " << baseID << std::endl;
//    ss << "Start Loc:    " << (IsStartLocation() ? "true" : "false") << std::endl;
//    ss << "Minerals:     " << mineral_positions_.size() << std::endl;
//    ss << "Geysers:      " << geyser_positions_.size() << std::endl;
//    ss << "Occupied By:  ";
//
//    if (IsOccupiedByPlayer(sc2::Unit::Alliance::Self))
//    {
//        ss << "Self ";
//    }
//
//    if (IsOccupiedByPlayer(sc2::Unit::Alliance::Enemy))
//    {
//        ss << "Enemy ";
//    }
//
//    bot_.DebugHelper().DrawText(sc2::Point2D(left_, top_ + 3), ss.str().c_str());
//    bot_.DebugHelper().DrawText(sc2::Point2D(left_, bottom_), ss.str().c_str());
//
//    // draw the base bounding box
//    bot_.DebugHelper().DrawBox(left_, top_, right_, bottom_);
//
//    for (float x = left_; x < right_; ++x)
//    {
//        bot_.DebugHelper().DrawLine(x, top_, x, bottom_, sc2::Color(160, 160, 160));
//    }
//
//    for (float y = bottom_; y<top_; ++y)
//    {
//        bot_.DebugHelper().DrawLine(left_, y, right_, y, sc2::Color(160, 160, 160));
//    }
//
//    for (auto & mineral_pos : mineral_positions_)
//    {
//        bot_.DebugHelper().DrawSphere(mineral_pos, 1.0f, sc2::Colors::Teal);
//    }
//
//    for (auto & geyser_pos : geyser_positions_)
//    {
//        bot_.DebugHelper().DrawSphere(geyser_pos, 1.0f, sc2::Colors::Green);
//    }
//
//    if (is_start_location_)
//    {
//        bot_.DebugHelper().DrawSphere(depot_position_, 1.0f, sc2::Colors::Red);
//    }
//
//    const float cc_width = 5;
//    const float cc_height = 4;
//    const sc2::Point2D boxtl = depot_position_ - sc2::Point2D(cc_width / 2, -cc_height / 2);
//    const sc2::Point2D boxbr = depot_position_ + sc2::Point2D(cc_width / 2, -cc_height / 2);
//
//    bot_.DebugHelper().DrawBox(boxtl.x, boxtl.y, boxbr.x, boxbr.y, sc2::Colors::Red);
//
//    //m_distanceMap.draw(bot_);
//}
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
    for (int y = 0; y < bot_.InformationManager().Map().TrueMapHeight(); ++y)
    {
        for (int x = 0; x < bot_.InformationManager().Map().TrueMapWidth(); ++x)
        {
            if (!bot_.InformationManager().Map().IsOnMap(x, y))
            {
                continue;
            }
            sc2::Color color = bot_.InformationManager().Map().IsWalkable(x, y) ? sc2::Colors::Green : sc2::Colors::Red;
            if (bot_.InformationManager().Map().IsWalkable(x, y) && !bot_.InformationManager().Map().IsBuildable(x, y))
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
//        bot_.InformationManager().Map().drawText(base->pos, ss.str());
//    }
//}

void DebugManager::DrawAllUnitInformation() const
{
    std::stringstream ss;
    const std::map<sc2::Tag, UnitInfo> ui = bot_.InformationManager().UnitInfo().GetUnitInfoMap(sc2::Unit::Alliance::Self);

    ss << "Workers: " << ui.size() << std::endl;

    int yspace = 0;

    for (auto const & unit_info : ui )
    {
        if (Util::IsBuilding(unit_info.second.unit->unit_type)) continue;
        ss << unit_info.second.GetJobCode() << " " << unit_info.first << std::endl;
    }

    bot_.DebugHelper().DrawTextScreen(sc2::Point2D(0.75f, 0.2f), ss.str());
}

void DebugManager::DrawDebugInterface() const
{
    DrawGameInformation();
}

void DebugManager::DrawGameInformation() const
{
    std::stringstream ss;
    // ss << "Players: " << std::endl;
    ss << "Strategy: " << bot_.Config().StrategyName << std::endl;
    ss << "Map Name: " << bot_.Config().MapName << std::endl;
    // ss << "Time: " << std::endl;
    bot_.DebugHelper().DrawTextScreen(sc2::Point2D(0.75f, 0.1f), ss.str());
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
    DrawBoxAroundUnit(unit_type, sc2::Point3D(unit_pos.x, unit_pos.y, bot_.InformationManager().Map().TerrainHeight(unit_pos.x, unit_pos.y)), color);
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