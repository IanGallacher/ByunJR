#include <sstream>
#include <vector>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "global/Debug.h"
#include "util/Util.h"

DebugManager::DebugManager(ByunJRBot & bot)
    : m_bot(bot)
{
}

void DebugManager::drawResourceDebugInfo()
{
    const std::map<sc2::Tag, UnitInfo> ui = m_bot.InformationManager().UnitInfo().getUnitInfoMap(PlayerArrayIndex::Self);

    for (auto const & unitInfo : ui)
    {
        if (Util::IsBuilding(unitInfo.second.unit->unit_type)) continue;
        m_bot.Map().drawText(unitInfo.second.unit->pos, unitInfo.second.getJobCode());

        //auto depot = m_bot.GetUnit(m_workerData.getWorkerDepot(workerTag));
        //if (depot)
        //{
        //    m_bot.Map().drawLine(m_bot.GetUnit(workerTag)->pos, depot->pos);
        //}
    }
}

//void DebugManager::drawDepotDebugInfo()
//{
//	for (auto & baseTag : m_depots)
//	{
//		const auto base = m_bot.GetUnit(baseTag);
//
//		if (!base) continue;
//		std::stringstream ss;
//		ss << "Workers: " << getNumAssignedWorkers(base);
//
//		m_bot.Map().drawText(base->pos, ss.str());
//	}
//}

void DebugManager::drawAllUnitInformation() const
{
    std::stringstream ss;
    const std::map<sc2::Tag, UnitInfo> ui = m_bot.InformationManager().UnitInfo().getUnitInfoMap(PlayerArrayIndex::Self);

    ss << "Workers: " << ui.size() << std::endl;

    int yspace = 0;

    for (auto const & unitInfo : ui )
    {
        if (Util::IsBuilding(unitInfo.second.unit->unit_type)) continue;
        ss << unitInfo.second.getJobCode() << " " << unitInfo.first << std::endl;
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
