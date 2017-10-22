#pragma once

namespace SquadOrderTypes
{
    enum { None, Idle, Attack, Defend, Regroup, Drop, SquadOrderTypes };
}

class SquadOrder
{
    size_t          type_;
    float           radius_;
    sc2::Point2D    position_;
    std::string     status_;

public:

    SquadOrder();
    SquadOrder(int type, const sc2::Point2D & position, float radius, std::string status = "Default");

    const std::string & GetStatus() const;
    const sc2::Point2D & GetPosition() const;
    const float & GetRadius() const;
    const size_t & GetType() const;
};