#include "ByunJRBot.h"
#include "common/Common.h"
#include "information/BaseLocationManager.h"
#include "util/Util.h"
 
BaseLocationManager::BaseLocationManager(ByunJRBot & bot)
    : m_bot(bot)
{
    
}

void BaseLocationManager::onStart()
{
    m_tileBaseLocations = std::vector<std::vector<BaseLocation*>>(m_bot.Map().width(), std::vector<BaseLocation*>(m_bot.Map().height(), nullptr));
    m_playerStartingBaseLocations[PlayerArrayIndex::Self]  = nullptr;
    m_playerStartingBaseLocations[PlayerArrayIndex::Enemy] = nullptr; 

	// construct the sets of occupied base locations
	m_occupiedBaseLocations[PlayerArrayIndex::Self] = std::set<const BaseLocation*>();
	m_occupiedBaseLocations[PlayerArrayIndex::Enemy] = std::set<const BaseLocation*>();
	m_enemyBaseScouted = false;
    
    // a BaseLocation will be anything where there are minerals to mine
    // so we will first look over all minerals and cluster them based on some distance
    const int clusterDistance = 15;
    
    // Stores each cluster of resources based on some ground distance.
    // These will be used to identify base locations.
    std::vector<std::vector<const sc2::Unit*>> resourceClusters;

    // For every mineral field and gas geyser out there, add it to a resource cluster.
    for (auto resource : m_bot.Observation()->GetUnits(sc2::Unit::Alliance::Neutral))
    {
        // skip minerals that don't have more than 100 starting minerals
        // these are probably stupid map-blocking minerals to confuse us.

        // Skip any unit that is not a gas geyser or mineral field.
        if (!Util::IsMineral(resource) && !Util::IsGeyser(resource)) continue;

        bool foundCluster = false;
        for (auto & cluster : resourceClusters)
        {
            const float dist = Util::Dist(resource->pos, Util::CalcCenter(cluster));
            
            // quick initial air distance check to eliminate most resources
            if (dist < clusterDistance)
            {
                // now do a more expensive ground distance check
                const float groundDist = dist; //m_bot.Map().getGroundDistance(mineral.pos, Util::CalcCenter(cluster));
                if (groundDist >= 0 && groundDist < clusterDistance)
                {
                    cluster.push_back(resource);
                    foundCluster = true;
                    break;
                }
            }
        }

        if (!foundCluster)
        {
            resourceClusters.push_back(std::vector<const sc2::Unit*>());
            resourceClusters.back().push_back(resource);
        }
    }

    // add the base locations if there are more than 4 resouces in the cluster
    int baseID = 0;
    for (auto & cluster : resourceClusters)
    {
        if (cluster.size() > 4)
        {
            m_baseLocationData.push_back(BaseLocation(m_bot, baseID++, cluster));
        }
    }

    // construct the vectors of base location pointers, this is safe since they will never change
    for (auto & baseLocation : m_baseLocationData)
    {
        m_baseLocationPtrs.push_back(&baseLocation);
        // if it's a start location, add it to the start locations
        if (baseLocation.isStartLocation())
        {
            m_startingBaseLocations.push_back(&baseLocation);
        }

        // if it's our starting location, set the pointer
        if (baseLocation.isPlayerStartLocation())
        {
            m_playerStartingBaseLocations[PlayerArrayIndex::Self] = &baseLocation;
        }

		// If there is only one enemy spawn location, we know where the enemy is. 
		if (m_bot.Observation()->GetGameInfo().enemy_start_locations.size() == 1 
	     && baseLocation.isPotentialEnemyStartLocation())
        {
			// Make sure that there really only is one enemy base. 
			assert(m_enemyBaseScouted == false);
			m_playerStartingBaseLocations[PlayerArrayIndex::Enemy] = &baseLocation;
			m_enemyBaseScouted = true;
        }
    }

    // construct the map of tile positions to base locations
    for (float x=0; x < m_bot.Map().width(); ++x)
    {
        for (int y=0; y < m_bot.Map().height(); ++y)
        {
            for (auto & baseLocation : m_baseLocationData)
            {
                sc2::Point2D pos(x + 0.5f, y + 0.5f);

                if (baseLocation.containsPosition(pos))
                {
                    m_tileBaseLocations[(int)x][(int)y] = &baseLocation;
                    
                    break;
                }
            }
        }
    }
}

void BaseLocationManager::onFrame()
{   
    drawBaseLocations();

    // reset the player occupation information for each location
    for (auto & baseLocation : m_baseLocationData)
    {
        baseLocation.setPlayerOccupying(PlayerArrayIndex::Self, false);
        baseLocation.setPlayerOccupying(PlayerArrayIndex::Enemy, false);
    }

    // for each unit on the map, update which base location it may be occupying
    for (auto & unit : m_bot.Observation()->GetUnits(sc2::Unit::Alliance::Ally))
    {
        // we only care about buildings on the ground
        if (!Util::IsBuilding(unit->unit_type) || unit->is_flying)
        {
            continue;
        }

        BaseLocation* baseLocation = getBaseLocation(unit->pos);

        if (baseLocation != nullptr)
        {
            baseLocation->setPlayerOccupying(Util::GetPlayer(unit), true);
        }
    }

    // update enemy base occupations
    for (const auto & kv : m_bot.InformationManager().UnitInfo().getUnitInfoMap(PlayerArrayIndex::Enemy))
    {
        const UnitInfo & ui = kv.second;

        if (!Util::IsBuilding(ui.type))
        {
            continue;
        }

        BaseLocation* baseLocation = getBaseLocation(ui.lastPosition);

        if (baseLocation != nullptr)
        {
            baseLocation->setPlayerOccupying(PlayerArrayIndex::Enemy, true);
        }
    }

	// If we have not yet scouted the enemy base, try to figure out where they started. 
	// This can happen one of two ways. 
	if (!m_enemyBaseScouted)
	{
		// 1. we've seen the enemy base directly, so the baselocation will know the enemy location.
		if (m_playerStartingBaseLocations[PlayerArrayIndex::Enemy] == nullptr)
		{
			for (auto & baseLocation : m_baseLocationData)
			{
				if (baseLocation.isPlayerStartLocation())
				{
					 m_playerStartingBaseLocations[PlayerArrayIndex::Enemy] = &baseLocation;
					 m_enemyBaseScouted = true;
				}
			}
		}
    
		// 2. we've explored every other start location and haven't seen the enemy yet
		if (m_playerStartingBaseLocations[PlayerArrayIndex::Enemy] == nullptr)
		{
			const int numStartLocations = (int)getStartingBaseLocations().size();
			int numExploredLocations = 0;
			BaseLocation* unexplored = nullptr;

			for (auto & baseLocation : m_baseLocationData)
			{
				if (!baseLocation.isStartLocation())
				{
					continue;
				}

				if (baseLocation.isExplored())
				{
					numExploredLocations++;
				}
				else
				{
					unexplored = &baseLocation;
				}
			}

			// if we have explored all but one location, then the unexplored one is the enemy start location
			if (numExploredLocations == numStartLocations - 1 && unexplored != nullptr)
			{
				m_playerStartingBaseLocations[PlayerArrayIndex::Enemy] = unexplored;
				unexplored->setPlayerOccupying(PlayerArrayIndex::Enemy, true);
				m_enemyBaseScouted = true;
			}
		}
	}

    // update the occupied base locations for each player
    m_occupiedBaseLocations[PlayerArrayIndex::Self] = std::set<const BaseLocation*>();
    m_occupiedBaseLocations[PlayerArrayIndex::Enemy] = std::set<const BaseLocation*>();
    for (auto & baseLocation : m_baseLocationData)
    {
        if (baseLocation.isOccupiedByPlayer(PlayerArrayIndex::Self))
        {
            m_occupiedBaseLocations[PlayerArrayIndex::Self].insert(&baseLocation);
        }

        if (baseLocation.isOccupiedByPlayer(PlayerArrayIndex::Enemy))
        {
            m_occupiedBaseLocations[PlayerArrayIndex::Enemy].insert(&baseLocation);
        }
    }

    // draw the debug information for each base location
    
}

BaseLocation* BaseLocationManager::getBaseLocation(const sc2::Point2D & pos) const
{
	if (!m_bot.Map().isOnMap(pos)) { std::cout << "Warning: requeste base location not on map" << std::endl; return nullptr; }

    return m_tileBaseLocations[(int)pos.x][(int)pos.y];
}

void BaseLocationManager::drawBaseLocations()
{
    if (!m_bot.Config().DrawBaseLocationInfo)
    {
        return;
    }

    for (auto & baseLocation : m_baseLocationData)
    {
        baseLocation.draw();
    }

    // draw a purple sphere at the next expansion location
    const sc2::Point2D nextExpansionPosition = getNextExpansion(PlayerArrayIndex::Self);

    m_bot.Map().drawSphere(nextExpansionPosition, 1, sc2::Colors::Purple);
    m_bot.Map().drawText(nextExpansionPosition, "Next Expansion Location", sc2::Colors::Purple);
}

const std::vector<const BaseLocation*> & BaseLocationManager::getBaseLocations() const
{
    return m_baseLocationPtrs;
}

const std::vector<const BaseLocation*> & BaseLocationManager::getStartingBaseLocations() const
{
    return m_startingBaseLocations;
}

const BaseLocation* BaseLocationManager::getPlayerStartingBaseLocation(const PlayerArrayIndex player) const
{
    return m_playerStartingBaseLocations.at(player);
}

const std::set<const BaseLocation*> & BaseLocationManager::getOccupiedBaseLocations(const PlayerArrayIndex player) const
{
    return m_occupiedBaseLocations.at(player);
}


sc2::Point2D BaseLocationManager::getNextExpansion(const PlayerArrayIndex player) const
{
    const BaseLocation* homeBase = getPlayerStartingBaseLocation(player);
    const BaseLocation* closestBase = nullptr;
    int minDistance = std::numeric_limits<int>::max();

    sc2::Point2D homeTile = homeBase->getPosition();
    
    for (auto & base : getBaseLocations())
    {
        // skip mineral only and starting locations (TODO: fix this)
        if (base->isMineralOnly() || base->isStartLocation())
        {
            continue;
        }

        // get the tile position of the base
        const auto tile = base->getDepotPosition();
        
        const bool buildingInTheWay = false; // TODO: check if there are any units on the tile

        if (buildingInTheWay)
        {
            continue;
        }

        // the base's distance from our main nexus
        const int distanceFromHome = homeBase->getGroundDistance(tile);

        // if it is not connected, continue
        if (distanceFromHome < 0)
        {
            continue;
        }

        if (!closestBase || distanceFromHome < minDistance)
        {
            closestBase = base;
            minDistance = distanceFromHome;
        }
    }

    return closestBase ? closestBase->getDepotPosition() : sc2::Point2D(0.0f, 0.0f);
}
