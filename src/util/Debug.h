#pragma once
#include "InformationManager.h"

class DebugManager
{
    // Debug functions require mutable agents.
    sc2::Agent & bot_;
    InformationManager & information_manager_;
public:
    DebugManager(sc2::Agent & bot, InformationManager & information_manager);
    void DrawResourceDebugInfo() const;
    void DrawEnemyDPSMap(std::vector<std::vector<int>> dps_map) const;
    void DrawMapSectors() const;
    void DrawBaseLocations() const;
    void DrawBaseLocation(const BaseLocation & base_location) const;
    void DrawMapWalkableTiles() const;
    void DrawAllUnitInformation() const;

    void DrawAllSelectedUnitsDebugInfo() const;
    void DrawUnitDebugInfo(const sc2::Unit * unit) const;


    // Debug Helper functions
    void DrawLine(float x1, float y1, float x2, float y2, const sc2::Color& color = sc2::Colors::White) const;
    void DrawLine(const sc2::Point2D& min, const sc2::Point2D max, const sc2::Color& color = sc2::Colors::White) const;
    void DrawSquareOnMap(float x1, float y1, float x2, float y2, const sc2::Color& color = sc2::Colors::White) const;
    void DrawBox(float x1, float y1, float x2, float y2, const sc2::Color& color = sc2::Colors::White) const;
    void DrawBox(const sc2::Point3D& min, const sc2::Point2D max, const sc2::Color& color = sc2::Colors::White) const;
    void DrawBox(const sc2::Point3D& min, const sc2::Point3D max, const sc2::Color& color) const;
    void DrawSphere(float x1, float x2, float radius, const sc2::Color& color = sc2::Colors::White) const;
    void DrawSphere(const sc2::Point2D& pos, float radius, const sc2::Color& color = sc2::Colors::White) const;
    void DrawText(const sc2::Point2D& pos, const std::string& str, const sc2::Color& color = sc2::Colors::White) const;
    void DrawTextScreen(const sc2::Point2D& pos, const std::string& str, const sc2::Color& color = sc2::Colors::White) const;
    void DrawBoxAroundUnit(const sc2::UnitTypeID unit_type, const sc2::Point2D unit_pos, const sc2::Color color = sc2::Colors::White) const;
    void DrawBoxAroundUnit(const sc2::UnitTypeID unit_type, const sc2::Point3D unit_pos, const sc2::Color color = sc2::Colors::White) const;
    void DrawBoxAroundUnit(const sc2::Unit* unit, const sc2::Color color) const;
    void DrawSphereAroundUnit(const sc2::Unit* uinit, sc2::Color color) const;
};