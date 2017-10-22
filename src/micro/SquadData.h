#pragma once
#include "micro/Squad.h"

class ByunJRBot;

class SquadData
{
    ByunJRBot & bot_;
    std::map<std::string, Squad> squads_;

    void    UpdateAllSquads();
    void    VerifySquadUniqueMembership();

public:

    SquadData(ByunJRBot & bot);

    void            OnFrame();
    void            ClearSquadData();

    bool            CanAssignUnitToSquad(const sc2::Unit* unit, const Squad & squad) const;
    void            AssignUnitToSquad(const sc2::Unit* unit, Squad & squad);
    void            AddSquad(const std::string & squad_name, const Squad & squad);
    void            RemoveSquad(const std::string & squad_name);
    void            DrawSquadInformation();


    bool            SquadExists(const std::string & squad_name);
    bool            UnitIsInSquad(const sc2::Unit* unit) const;
    const Squad*    GetUnitSquad(const sc2::Unit* unit) const;
    Squad*          GetUnitSquad(const sc2::Unit* unit);

    Squad &         GetSquad(const std::string & squad_name);
    const std::map<std::string, Squad> & GetSquads() const;
};
