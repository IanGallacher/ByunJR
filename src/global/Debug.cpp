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
    std::map<int, UnitInfo> ui = m_bot.UnitInfoManager().getUnitInfoMap(Players::Self);

    ss << "Workers: " << ui.size() << std::endl;

    int yspace = 0;

    for (auto const & workerTag : ui )
    {
        ss << workerTag.second.getJobCode() << " " << workerTag.first << "\n";
    }

    m_bot.Map().drawTextScreen(sc2::Point2D(0.75f, 0.2f), ss.str());
}
