#include <sc2api/sc2_api.h>

#include "micro/SquadOrder.h"

SquadOrder::SquadOrder()
    : type_    (SquadOrderTypes::None)
    , radius_  (0)
{
}

SquadOrder::SquadOrder(const int type, const sc2::Point2D & position, const float radius, const std::string status)
    : type_    (type)
    , position_(position)
    , radius_  (radius)
    , status_  (status)
{
}

const std::string & SquadOrder::GetStatus() const
{
    return status_;
}

const sc2::Point2D & SquadOrder::GetPosition() const
{
    return position_;
}

const float & SquadOrder::GetRadius() const
{
    return radius_;
}

const size_t & SquadOrder::GetType() const
{
    return type_;
}