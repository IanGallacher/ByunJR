#include <sc2api/sc2_api.h>

#include "macro/BuildOrder.h"

BuildOrder::BuildOrder()
    : race_(sc2::Race::Terran)
{

}

BuildOrder::BuildOrder(const sc2::Race & race)
    : race_(race)
{

}

BuildOrder::BuildOrder(const sc2::Race & race, const std::vector<sc2::UnitTypeID> & vec)
    : race_(race)
    , build_order_(vec)
{

}

void BuildOrder::Add(const sc2::UnitTypeID & type)
{
    build_order_.push_back(type);
}

const sc2::Race & BuildOrder::GetRace() const
{
    return race_;
}

size_t BuildOrder::Size() const
{
    return build_order_.size();
}

const sc2::UnitTypeID & BuildOrder::operator [] (const size_t & index) const
{
    return build_order_[index];
}

sc2::UnitTypeID & BuildOrder::operator [] (const size_t & index)
{
    return build_order_[index];
}