#pragma once
#include <sc2api/sc2_api.h>

#include "information/BaseLocation.h"

class ByunJRBot;

class BaseLocationManager
{
    ByunJRBot & m_bot;

    std::vector<BaseLocation>                                   m_baseLocationData;
    std::vector<const BaseLocation*>                            m_baseLocationPtrs;
    std::vector<const BaseLocation*>                            m_startingBaseLocations;
    std::map<PlayerArrayIndex, const BaseLocation*>             m_playerStartingBaseLocations;
    std::map<PlayerArrayIndex, std::set<const BaseLocation*>>   m_occupiedBaseLocations;
    std::vector<std::vector<BaseLocation*>>                     m_tileBaseLocations;

	// If the enemy base is not yet scouted, the enemy base location will be set to the next unexplored enemy spawn location.
	bool														m_enemyBaseScouted;

    BaseLocation* getBaseLocation(const sc2::Point2D & pos) const;

public:

    BaseLocationManager(ByunJRBot & bot);
    
    void onStart();
    void onFrame();
    void drawBaseLocations();

    const std::vector<const BaseLocation*> & getBaseLocations() const;
    const std::vector<const BaseLocation*> & getStartingBaseLocations() const;
    const std::set<const BaseLocation*> & getOccupiedBaseLocations(PlayerArrayIndex player) const;
    const BaseLocation* getPlayerStartingBaseLocation(PlayerArrayIndex player) const;
    
    sc2::Point2D getNextExpansion(PlayerArrayIndex player) const;
};
