#include <sstream>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "micro/SquadData.h"
#include "util/Util.h"

SquadData::SquadData(ByunJRBot& bot)
    : bot_(bot)
{

}

void SquadData::OnFrame()
{
    UpdateAllSquads();
    VerifySquadUniqueMembership();
    DrawSquadInformation();
}

void SquadData::ClearSquadData()
{
    // give back workers who were in squads
    for (auto& kv : squads_)
    {
        Squad& squad = kv.second;
        for (auto& unit_tag : squad.GetUnits())
        {
            auto unit = bot_.GetUnit(unit_tag);
            BOT_ASSERT(unit, "null unit");

            if (Util::IsWorker(unit))
            {
                bot_.InformationManager().UnitInfo().SetJob(bot_.GetUnit(unit_tag), UnitMission::Idle);
            }
        }
    }

    squads_.clear();
}

void SquadData::RemoveSquad(const std::string& squad_name)
{
    const auto& squad_ptr = squads_.find(squad_name);

    BOT_ASSERT(squad_ptr != squads_.end(), "Trying to clear a squad that didn't exist: %s", squad_name.c_str());
    if (squad_ptr == squads_.end())
    {
        return;
    }

    for (auto unit_tag : squad_ptr->second.GetUnits())
    {
        auto unit = bot_.GetUnit(unit_tag);
        BOT_ASSERT(unit, "null unit");

        if (Util::IsWorker(unit))
        {
            bot_.InformationManager().UnitInfo().SetJob(bot_.GetUnit(unit_tag), UnitMission::Idle);
        }
    }

    squads_.erase(squad_name);
}

const std::map<std::string, Squad>& SquadData::GetSquads() const
{
    return squads_;
}

bool SquadData::SquadExists(const std::string& squad_name)
{
    return squads_.find(squad_name) != squads_.end();
}

void SquadData::AddSquad(const std::string& squad_name, const Squad& squad)
{
    squads_.insert(std::pair<std::string, Squad>(squad_name, squad));
}

void SquadData::UpdateAllSquads()
{
    for (auto& kv : squads_)
    {
        kv.second.OnFrame();
    }
}

void SquadData::DrawSquadInformation()
{
    if (!bot_.Config().DrawSquadInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Squad Data\n\n";

    for (auto& kv : squads_)
    {
        const Squad& squad = kv.second;

        auto& units = squad.GetUnits();
        const SquadOrder& order = squad.GetSquadOrder();

        ss << squad.GetName() << " " << units.size() << " (";
        ss << static_cast<int>(order.GetPosition().x) << ", " << static_cast<int>(order.GetPosition().y) << ")\n";

        bot_.DebugHelper().DrawSphere(order.GetPosition(), 5, sc2::Colors::Red);
        bot_.DebugHelper().DrawText(order.GetPosition(), squad.GetName(), sc2::Colors::Red);

        for (auto unit_tag : units)
        {
            auto unit = bot_.GetUnit(unit_tag);
            BOT_ASSERT(unit, "null unit");

            bot_.DebugHelper().DrawText(unit->pos, squad.GetName(), sc2::Colors::Green);
        }
    }

    bot_.DebugHelper().DrawTextScreen(sc2::Point2D(0.5f, 0.2f), ss.str(), sc2::Colors::Red);
}

void SquadData::VerifySquadUniqueMembership()
{
    std::vector<sc2::Tag> assigned;

    for (const auto& kv : squads_)
    {
        for (auto& unit_tag : kv.second.GetUnits())
        {
            if (std::find(assigned.begin(), assigned.end(), unit_tag) != assigned.end())
            {
                std::cout << "Warning: Unit is in at least two squads: " << unit_tag << std::endl;
            }

            assigned.push_back(unit_tag);
        }
    }
}

bool SquadData::UnitIsInSquad(const sc2::Tag& unit) const
{
    return GetUnitSquad(unit) != nullptr;
}

const Squad* SquadData::GetUnitSquad(const sc2::Tag& unit) const
{
    for (const auto& kv : squads_)
    {
        if (kv.second.ContainsUnit(unit))
        {
            return &kv.second;
        }
    }

    return nullptr;
}

Squad * SquadData::GetUnitSquad(const sc2::Tag & unit)
{
    for (auto& kv : squads_)
    {
        if (kv.second.ContainsUnit(unit))
        {
            return &kv.second;
        }
    }

    return nullptr;
}

void SquadData::AssignUnitToSquad(const sc2::Tag& unit, Squad & squad)
{
    BOT_ASSERT(CanAssignUnitToSquad(unit, squad), "We shouldn't be re-assigning this unit!");

    Squad* previous_squad = GetUnitSquad(unit);

    if (previous_squad)
    {
        previous_squad->RemoveUnit(unit);
    }

    squad.AddUnit(unit);
}

bool SquadData::CanAssignUnitToSquad(const sc2::Tag & unit, const Squad & squad) const
{
    const Squad * unit_squad = GetUnitSquad(unit);

    // make sure strictly less than so we don't reassign to the same squad etc
    return !unit_squad || (unit_squad->GetPriority() < squad.GetPriority());
}

Squad & SquadData::GetSquad(const std::string & squad_name)
{
    BOT_ASSERT(SquadExists(squad_name), "Trying to access squad that doesn't exist: %s", squad_name);
    if (!SquadExists(squad_name))
    {
        int a = 10;
    }

    return squads_.at(squad_name);
}