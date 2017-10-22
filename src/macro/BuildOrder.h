#pragma once

class BuildOrder
{
    sc2::Race race_;
    std::vector<sc2::UnitTypeID> build_order_;

public:

    BuildOrder();
    BuildOrder(const sc2::Race & race);
    BuildOrder(const sc2::Race & race, const std::vector<sc2::UnitTypeID> & meta_vector);

    void Add(const sc2::UnitTypeID & type);
    size_t Size() const;
    const sc2::Race & GetRace() const;
    const sc2::UnitTypeID & operator [] (const size_t & index) const;
    sc2::UnitTypeID & operator [] (const size_t & index);
};

