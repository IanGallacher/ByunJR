#include <sstream>

#include "ByunJRBot.h"
#include "common/Common.h"
#include "information/BaseLocation.h"
#include "util/Util.h"

const int NearBaseLocationTileDistance = 20;

BaseLocation::BaseLocation(ByunJRBot & bot, int baseID, const std::vector<const sc2::Unit*> & resources)
    : m_bot(bot)
    , m_baseID               (baseID)
    , m_isStartLocation      (false)
    , m_left                 (std::numeric_limits<float>::max())
    , m_right                (std::numeric_limits<float>::lowest())
    , m_top                  (std::numeric_limits<float>::lowest())
    , m_bottom               (std::numeric_limits<float>::max())
{
    m_isPlayerStartLocation[PlayerArrayIndex::Self] = false;
	// Enemy start locations are simply POTENTIAL spawn locations.
    m_isPlayerStartLocation[PlayerArrayIndex::Enemy] = false;
    m_isPlayerOccupying[PlayerArrayIndex::Self] = false;
    m_isPlayerOccupying[PlayerArrayIndex::Enemy] = false;

    float resourceCenterX = 0;
    float resourceCenterY = 0;

    // add each of the resources to its corresponding container
    for (auto & resource : resources)
    {
        if (Util::IsMineral(resource))
        {
            m_minerals.push_back(resource);
            m_mineralPositions.push_back(resource->pos);

            // add the position of the minerals to the center
            resourceCenterX += resource->pos.x;
            resourceCenterY += resource->pos.y;
        }
        else
        {
            m_geysers.push_back(resource);
            m_geyserPositions.push_back(resource->pos);

            // pull the resource center toward the geyser if it exists
            resourceCenterX += resource->pos.x;
            resourceCenterY += resource->pos.y;
        }

        // set the limits of the base location bounding box
        const float resWidth = 1;
        const float resHeight = 0.5;

        m_left   = std::min(m_left,   resource->pos.x - resWidth);
        m_right  = std::max(m_right,  resource->pos.x + resWidth);
        m_top    = std::max(m_top,    resource->pos.y + resHeight);
        m_bottom = std::min(m_bottom, resource->pos.y - resHeight);
    }

    // calculate the center of the resources
    size_t numResources = m_minerals.size() + m_geysers.size();

    m_centerOfResources = sc2::Point2D(m_left + (m_right-m_left)/2.0f, m_top + (m_bottom-m_top)/2.0f);

    // compute this BaseLocation's DistanceMap, which will compute the ground distance
    // from the center of its recourses to every other tile on the map
    m_distanceMap = m_bot.Map().getDistanceMap(m_centerOfResources);

    // check to see if this is a start location for the map
    for (auto & pos : m_bot.Observation()->GetGameInfo().enemy_start_locations)
    {
        if (containsPosition(pos))
        {
            m_isStartLocation = true;
			m_isPlayerStartLocation[PlayerArrayIndex::Enemy] = true;
            m_depotPosition = pos;
        }
    }
    
    // if this base location position is near our own resource depot, it's our start location
    for (auto & unit : m_bot.Observation()->GetUnits())
    {
        if (Util::GetPlayer(unit) == PlayerArrayIndex::Self && Util::IsTownHall(unit) && containsPosition(unit->pos))
        {
            m_isPlayerStartLocation[PlayerArrayIndex::Self] = true;
            m_isStartLocation = true;
            m_isPlayerOccupying[PlayerArrayIndex::Self] = true;
            break;
        }
    }
    
    // if it's not a start location, we need to calculate the depot position
    if (!isStartLocation())
    {
        // the position of the depot will be the closest spot we can build one from the resource center
        for (auto & tile : getClosestTiles())
        {
            // TODO: m_depotPosition = depot position for this base location
        }
    }
}

// TODO: calculate the actual depot position
const sc2::Point2D & BaseLocation::getDepotPosition() const
{
    return getPosition();
}

void BaseLocation::setPlayerOccupying(PlayerArrayIndex player, bool occupying)
{
    m_isPlayerOccupying[player] = occupying;

    // if this base is a start location that's occupied by the enemy, it's that enemy's start location
    if (occupying && player == PlayerArrayIndex::Enemy && isStartLocation() && m_isPlayerStartLocation[player] == false)
    {
        m_isPlayerStartLocation[player] = true;
    }
}

bool BaseLocation::isInResourceBox(int x, int y) const
{
    return x >= m_left && x < m_right && y < m_top && y >= m_bottom;
}

bool BaseLocation::isOccupiedByPlayer(PlayerArrayIndex player) const
{
    return m_isPlayerOccupying.at(player);
}

bool BaseLocation::isExplored() const
{
    return false;
}
// isPlayerStartLocation returns if you spawned at the given location. 
// m_isPlayerStartLocation.at(PlayerArrayIndex::Enemy) only gives POTENTIAL enemy spawn locations. 
// For clarity, there is a seprate function for that. 
bool BaseLocation::isPlayerStartLocation() const
{
    return m_isPlayerStartLocation.at(PlayerArrayIndex::Self);
}
bool BaseLocation::isPotentialEnemyStartLocation() const
{
	return m_isPlayerStartLocation.at(PlayerArrayIndex::Enemy);
}

bool BaseLocation::isMineralOnly() const
{
	return getGeysers().empty();
}

bool BaseLocation::containsPosition(const sc2::Point2D & pos) const
{
    if (!m_bot.Map().isOnMap(pos) || (pos.x == 0 && pos.y == 0))
    {
        return false;
    }

    return getGroundDistance(pos) < NearBaseLocationTileDistance;
}

const std::vector<const sc2::Unit*>& BaseLocation::getGeysers() const
{
    return m_geysers;
}

const std::vector<const sc2::Unit*>& BaseLocation::getMinerals() const
{
    return m_minerals;
}

const sc2::Point2D & BaseLocation::getPosition() const
{
    //return m_depotPosition;
    return m_centerOfResources;
}

int BaseLocation::getGroundDistance(const sc2::Point2D & pos) const
{
    return Util::Dist(pos, m_centerOfResources);
    //return m_distanceMap.getDistance(pos);
}

bool BaseLocation::isStartLocation() const
{
    return m_isStartLocation;
}

const std::vector<sc2::Point2D> & BaseLocation::getClosestTiles() const
{
    return m_distanceMap.getSortedTiles();
}

void BaseLocation::draw()
{
    m_bot.Map().drawSphere(m_centerOfResources, 1.0f, sc2::Colors::Yellow);

    std::stringstream ss;
    ss << "BaseLocation: " << m_baseID << std::endl;
    ss << "Start Loc:    " << (isStartLocation() ? "true" : "false") << std::endl;
    ss << "Minerals:     " << m_mineralPositions.size() << std::endl;
    ss << "Geysers:      " << m_geyserPositions.size() << std::endl;
    ss << "Occupied By:  ";

    if (isOccupiedByPlayer(PlayerArrayIndex::Self))
    {
        ss << "Self ";
    }

    if (isOccupiedByPlayer(PlayerArrayIndex::Enemy))
    {
        ss << "Enemy ";
    }

    m_bot.Map().drawText(sc2::Point2D(m_left, m_top+3), ss.str().c_str());
    m_bot.Map().drawText(sc2::Point2D(m_left, m_bottom), ss.str().c_str());

    // draw the base bounding box
    m_bot.Map().drawBox(m_left, m_top, m_right, m_bottom);

    for (float x=m_left; x < m_right; ++x)
    {
        m_bot.Map().drawLine(x, m_top, x, m_bottom, sc2::Color(160, 160, 160));
    }

    for (float y=m_bottom; y<m_top; ++y)
    {
        m_bot.Map().drawLine(m_left, y, m_right, y, sc2::Color(160, 160, 160));
    }

    for (auto & mineralPos : m_mineralPositions)
    {
        m_bot.Map().drawSphere(mineralPos, 1.0f, sc2::Colors::Teal);
    }

    for (auto & geyserPos : m_geyserPositions)
    {
        m_bot.Map().drawSphere(geyserPos, 1.0f, sc2::Colors::Green);
    }

    if (m_isStartLocation)
    {
        m_bot.Map().drawSphere(m_depotPosition, 1.0f, sc2::Colors::Red);
    }
    
    const float ccWidth = 5;
    const float ccHeight = 4;
    const sc2::Point2D boxtl = m_depotPosition - sc2::Point2D(ccWidth/2, -ccHeight/2);
    const sc2::Point2D boxbr = m_depotPosition + sc2::Point2D(ccWidth/2, -ccHeight/2);

    m_bot.Map().drawBox(boxtl.x, boxtl.y, boxbr.x, boxbr.y, sc2::Colors::Red);

    //m_distanceMap.draw(m_bot);
}