#include "ProxyManager.h"
#include "ByunJRBot.h"
#include "Util.h"
#include "Micro.h"
#include "sc2lib/sc2_lib.h"

// The bot is not fully setup when the default constructor is called. Therefore, we need to have a seprate init function.
void ProxyTrainingData::InitAllValues(ByunJRBot & bot)
{
    int arena_width = (int) (bot.Observation()->GetGameInfo().playable_max.x - bot.Observation()->GetGameInfo().playable_min.x);
    int arena_height = (int) (bot.Observation()->GetGameInfo().playable_max.y - bot.Observation()->GetGameInfo().playable_min.y);

    std::cout << "miny" << bot.Observation()->GetGameInfo().playable_min.y << "minx" << bot.Observation()->GetGameInfo().playable_min.x;
    // init the result vector to have the correct number of elements. 
    // Done over a few lines to increase legibility.

    m_result.resize(arena_height);
    for (auto &row : m_result)
    {
        row.resize(arena_width);
    }

    loadProxyTrainingData();

    writeAllTrainingData("MyTrainingData");
    setupProxyLocation();
}


// Load all the values from training data stored on the disk.
// If no training data is found, test all points on the map for buildable barracks locations and load that instead.
// Training data is stored in a standard .txt file. Spaces separate values, and new lines separate data entries.
// Format used on every line on the file:
// XCOORDINATE YCOORDINATE FITNESS
bool ProxyTrainingData::loadProxyTrainingData()
{
    std::ifstream trainingData;
    std::string line;

    trainingData.open("proxytraining.txt");

    // If we have an empty file, go ahead and test all the points on the map and use that instead of loading the file.
    if (trainingData.peek() == std::ifstream::traits_type::eof())
    {
        testAllPointsOnMap();
    }

    else if (trainingData.is_open())
    {
        while (getline(trainingData, line))
        {
            std::string temp = "";
            int x = -100;
            int y = -100;
            int val = -100;
            for (char& c : line)
            {
                if (!isspace(c))
                {
                    temp += c;
                }
                // if we hit a space, and x is still undefined, what we parsed is x.
                else if (x == -100)
                {
                    x = std::stoi(temp);
                    temp = "";
                }
                // if we hit a space, and y is still undefined, what we parsed is y.
                else if (y == -100)
                {
                    y = std::stoi(temp);
                    temp = "";
                }
                // Its not an x or a y, it must be val. 
                else
                {
                    // the x, y, and val variables will get reset next loop. No need to reset them here.
                }
            }
            // We reached the end of the line. Because there is no next character, we have to do the final check outside of the for loop.
            if (x != -100 && y != -100)
            {
                val = std::stoi(temp);
                m_result[y][x] = val;
            }
        }
    }
    trainingData.close();

    // No matter how we get our data, we ALWAYS will have to setup the list of suitable proxy locations.
    upadateViableLocationsList();
    return 0;
} 


// Iterate through the result data structure and update the ViableLocations vector.
void ProxyTrainingData::upadateViableLocationsList()
{
    for (int y = 0; y < m_result.size(); ++y)
    {
        for (int x = 0; x < m_result[y].size(); ++x)
        {
            if (m_result[y][x] != -1)
            {
                sc2::Point2D point((float) x, (float) y);
                ProxyLocation pl = { point, m_result[y][x] };
                ViableLocations.push_back(pl);
            }
        }
    }
}

void ProxyTrainingData::recordResult(int fitness)
{
    m_result[m_proxy_y][m_proxy_x] = fitness;
}

// If we can't build at the chosen location, update that information in our data structure.
bool ProxyTrainingData::isProxyLocationValid(int x, int y)
{
    if (m_map->canBuildTypeAtPosition(x, y, sc2::UNIT_TYPEID::TERRAN_BARRACKS))
        return true;
    return false;
}

// If we can't build at the chosen location, update that information in our data structure.
void ProxyTrainingData::testAllPointsOnMap()
{
    int play_width = (int) (m_playable_max.x - m_playable_min.x);
    int play_height = (int) (m_playable_max.y - m_playable_min.y);

    for (int y = 0; y < play_height; ++y)
    {
        for (int x = 0; x < play_width; ++x)
        {
            if (!isProxyLocationValid(x + (int) m_playable_min.x, y + (int) m_playable_min.y))
            {
                m_result[y][x] = -1;
            }
        }
    }
}

bool ProxyTrainingData::setupProxyLocation()
{
    std::ofstream outfile;


    srand( (unsigned int) time(NULL));
    int index = rand() % ViableLocations.size();

    sc2::Point2D play_min = m_playable_min;


    m_proxy_x = (int) (ViableLocations[index].m_loc.x + play_min.x);
    m_proxy_y = (int) (ViableLocations[index].m_loc.y + play_min.y);

    //for (const BaseLocation * startLocation : m_bot.Bases().getStartingBaseLocations())
    //{
    //    std::cout << startLocation->getPosition().x << "basexloc " << startLocation->getPosition().y << "baseyloc" << std::endl;
    //}
    sc2::Vector2D myVec( (float) m_proxy_x, (float) m_proxy_y);
    std::cout << myVec.x << "veclocx " << myVec.y << "veclocy" << std::endl;
    return true;
}

void ProxyTrainingData::writeAllTrainingData(std::string filename)
{

    std::cout << "WritingAllTrainingDataToFile...";
    std::ofstream outfile;
    std::stringstream buffer;

    // Open the file in truncate mode to overwrite previous saved data.
    outfile.open(filename + ".txt", std::ios_base::trunc);
    for (int y = (int) m_result.size() - 1; y >= 0; --y)
    {
        // If we want to "draw" the map, uncomment out the following line and another one a few lines below it. 
        //buffer << std::endl;
        for (int x = 0; x < m_result[y].size(); ++x)
        {
            buffer << x << " " << y << " " << m_result[y][x] << std::endl;
            // If we want to "draw" the map, uncomment out the following line and another one a few lines above it. 
            //buffer<< result[y][x];
        }
    }
    // Write the buffer to the output file.
    // A buffer is used to minimize read and write times. 
    // Without the buffer, writing can take over a minute.
    // With the buffer, writing only takes a few seconds. 
    outfile << buffer.str();

    // Save the changes to the file, and delete any file that was on record.
    outfile.close();
    std::cout << "DONE!" << std::endl;
}



#pragma region ProxyManagerMemberFunctions
ProxyManager::ProxyManager(ByunJRBot & bot)
    : m_bot(bot)
    , m_proxyUnitTag(0)
    , m_proxyUnderAttack(false)
{

}
// YOU MUST CALL setupProxyLocation() before this.
// TODO: Set this up as a constructor function.
bool ProxyManager::proxyBuildingAtChosenRandomLocation()
{
    std::ofstream outfile;

    //const sc2::Unit * ourScout = m_bot.GetUnit(m_proxyUnitTag);
    sc2::Vector2D myVec(m_ptd.getProxyLocation());

    //if (m_bot.GetUnit(m_proxyUnitTag)->pos.x > myVec.x - 1 && m_bot.GetUnit(m_proxyUnitTag)->pos.x < myVec.x + 1)
    //{
    //    m_bot.Workers().finishedWithWorker(m_proxyUnitTag);
    //}
    //else
    //{
    if (m_proxyUnitTag == 0)
    {
        Building b(sc2::UNIT_TYPEID::TERRAN_BARRACKS, myVec);
        m_proxyUnitTag = m_bot.Workers().getBuilder(b, false);
        Micro::SmartMove(m_proxyUnitTag, myVec, m_bot);
    }
    //}

    return true;
}

sc2::Point2D ProxyTrainingData::getProxyLocation()
{
    sc2::Point2D proxyLocation((float) m_proxy_x, (float) m_proxy_y);
    return proxyLocation;
}

void ProxyManager::onStart()
{
    m_ptd.InitAllValues(m_bot);
}

void ProxyManager::onFrame()
{
    proxyBuildingAtChosenRandomLocation();
}
 
void ProxyManager::OnUnitEnterVision(const sc2::Unit& unit)
{
    const sc2::Unit *UnitEnteredVision = m_bot.GetUnit(m_proxyUnitTag);
    if (!UnitEnteredVision) return;
    double dist(m_bot.Map().getGroundDistance(unit.pos, UnitEnteredVision->pos));

    if (dist < 8 && !m_firstReaperCreated)
    {
        m_bot.Resign();
        std::cout << "THERE IS NO POINT IN CONTINUING";
    }
}

void ProxyManager::onUnitCreated(const sc2::Unit& unit)
{
    if (unit.unit_type == sc2::UNIT_TYPEID::TERRAN_REAPER)
    {
        const BaseLocation * enemyBaseLocation = m_bot.Bases().getPlayerStartingBaseLocation(Players::Enemy);

        m_ptd.recordResult( (int)m_bot.Query()->PathingDistance(unit.tag, enemyBaseLocation->getPosition()));
        m_firstReaperCreated = true;
    }
}
//
//void ProxyManager::writeAllTrainingData()
//{
//    m_ptd.writeAllTrainingData("MyTrainingData");
//}

sc2::Point2D ProxyManager::getProxyLocation()
{
    return m_ptd.getProxyLocation();
}


#pragma endregion ProxyManagerMemberFunctions