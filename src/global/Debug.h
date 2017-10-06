#pragma once

class ByunJRBot;

class DebugManager
{
    ByunJRBot &                 m_bot;
public:
    DebugManager(ByunJRBot & bot);
    void drawResourceDebugInfo();
    void drawAllUnitInformation() const;
    void drawDebugInterface() const;
    void drawGameInformation() const;
};