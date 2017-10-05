#include <sstream>
#include <vector>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "global/Debug.h"

DebugManager::DebugManager(ByunJRBot & bot)
    : m_bot(bot)
{
}

void DebugManager::drawAllUnitInformation() const
{
    std::stringstream ss;
    std::map<int, UnitInfo> ui = m_bot.UnitInfoManager().getUnitInfoMap(PlayerArrayIndex::Self);

    ss << "Workers: " << ui.size() << std::endl;

    int yspace = 0;

    for (auto const & workerTag : ui )
    {
        ss << workerTag.second.getJobCode() << " " << workerTag.first << std::endl;
    }

    m_bot.Map().drawTextScreen(sc2::Point2D(0.75f, 0.2f), ss.str());
}

void DebugManager::drawDebugInterface() const
{
    drawGameInformation();
}

void DebugManager::drawGameInformation() const
{
    std::stringstream ss;
    // ss << "Players: " << std::endl;
    ss << "Strategy: " << m_bot.Config().StrategyName << std::endl;
    ss << "Map Name: " << m_bot.Config().MapName << std::endl;
    // ss << "Time: " << std::endl;
    m_bot.Map().drawTextScreen(sc2::Point2D(0.75f, 0.1f), ss.str());
}
