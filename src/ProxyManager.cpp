#include "ProxyManager.h"
#include "CCBot.h"
#include "Util.h"
#include "Micro.h"
#include "sc2lib/sc2_lib.h"


class ProxyLocation {
public :
    sc2::Point2D m_loc;
    int m_fitness;

    ProxyLocation(sc2::Point2D loc, int fitness)
    {
        m_loc = loc;
        m_fitness = fitness;
    }
};


#pragma region GlobalVariables

int proxy_x;
int proxy_y;
// There is a subtle difference between result and ViableLocations.
// Result is a vector of vectors that represent ALL points on the map. 
// ViableLocations is an UNINDEXED list that does not include places that get scouted easily or are impossible to build on.
// When picking a random proxy location, ViableLocations is used to make sure that the location we pick is always viable. 
std::vector<std::vector<int>> result;  // stored in the format result[y][x]
std::vector<ProxyLocation> ViableLocations;

#pragma endregion GlobalVariables

#pragma region ProxyManagerMemberFunctions
ProxyManager::ProxyManager(CCBot & bot)
    : m_bot(bot)
    , m_proxyUnitTag(0)
    , m_proxyUnderAttack(false)
{

}

void ProxyManager::onStart()
{
    int arena_width = m_bot.Observation()->GetGameInfo().playable_max.x - m_bot.Observation()->GetGameInfo().playable_min.x;
    int arena_height = m_bot.Observation()->GetGameInfo().playable_max.y - m_bot.Observation()->GetGameInfo().playable_min.y;

    std::cout << "miny" << m_bot.Observation()->GetGameInfo().playable_min.y << "minx" << m_bot.Observation()->GetGameInfo().playable_min.x;
    // init the result vector to have the correct number of elements. 
    // Done over a few lines to increase legibility.
    result.resize(arena_height);
    for (auto &row : result)
    {
        row.resize(arena_width);
    }

    loadProxyTrainingData();

    writeAllTrainingData();
    setupProxyLocation();
}

void ProxyManager::onFrame()
{
    proxyBuildingAtChosenRandomLocation();
}

// Load all the values from training data stored on the disk.
// If no training data is found, test all points on the map and load that instead.
// Training data is stored in a standard .txt file. Spaces separate values, and new lines separate data entries.
// Format used on every line on the file:
// XCOORDINATE YCOORDINATE FITNESS
bool ProxyManager::loadProxyTrainingData()
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
                if ( !isspace(c) )
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
                result[y][x] = val;
            }
        }
    }
    trainingData.close();

    // No matter how we get our data, we ALWAYS will have to setup the list of suitable proxy locations.
    upadateViableLocationsList();
    return 0;
}

// Iterate through the result data structure and update the ViableLocations vector.
void ProxyManager::upadateViableLocationsList()
{
    for (int y = 0; y < result.size(); ++y)
    {
        for (int x = 0; x < result[y].size(); ++x)
        {
            if (result[y][x] != -1)
            {
                sc2::Point2D point(x, y);
                ProxyLocation pl = { point, result[y][x] };
                ViableLocations.push_back(pl);
            }
        }
    }
}

void ProxyManager::recordResult(int x, int y, int fitness)
{
    result[y][x] = fitness;
}

bool ProxyManager::writeAllTrainingData()
{
    std::cout << "WritingAllTrainingDataToFile...";
    std::ofstream outfile;
    std::stringstream buffer;

    // Open the file in truncate mode to overwrite previous saved data.
    outfile.open("proxytraining.txt", std::ios_base::trunc);
    for (int y = result.size() - 1; y >= 0; --y)
    {
        // If we want to "draw" the map, uncomment out the following line and another one a few lines below it. 
        //buffer << std::endl;
        for (int x = 0; x < result[y].size(); ++x)
        { 
            buffer << x << " " << y << " " << result[y][x] << std::endl;
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
    return 0;
}

// If we can't build at the chosen location, update that information in our data structure.
bool ProxyManager::isProxyLocationValid(int x, int y)
{
    if (m_bot.Map().canBuildTypeAtPosition(x, y, sc2::UNIT_TYPEID::TERRAN_BARRACKS))
        return true;
    return false;
}

// If we can't build at the chosen location, update that information in our data structure.
void ProxyManager::testAllPointsOnMap()
{
    sc2::Point2D play_min = m_bot.Observation()->GetGameInfo().playable_min;
    sc2::Point2D play_max = m_bot.Observation()->GetGameInfo().playable_max;
    int play_width = play_max.x - play_min.x;
    int play_height = play_max.y - play_min.y;

    for (int y = 0; y < play_height; ++y)
    {
        for (int x = 0; x < play_width; ++x)
        {
            if(!isProxyLocationValid(x + play_min.x, y + play_min.y))
            {
                result[y][x] = -1;
            }
        }
    }
}

bool ProxyManager::setupProxyLocation()
{
    std::ofstream outfile;


    srand(time(NULL));
    int index = rand() % ViableLocations.size();

    sc2::Point2D play_min = m_bot.Observation()->GetGameInfo().playable_min;


    proxy_x = ViableLocations[index].m_loc.x + play_min.x;
    proxy_y = ViableLocations[index].m_loc.y + play_min.y;

    for (const BaseLocation * startLocation : m_bot.Bases().getStartingBaseLocations())
    {
        std::cout << startLocation->getPosition().x << "basexloc " << startLocation->getPosition().y << "baseyloc" << std::endl;
    }
    sc2::Vector2D myVec(proxy_x, proxy_y);
    std::cout << myVec.x << "veclocx " << myVec.y << "veclocy" << std::endl;
    return true;
}

// YOU MUST CALL setupProxyLocation() before this.
// TODO: Set this up as a constructor function.
bool ProxyManager::proxyBuildingAtChosenRandomLocation()
{
    std::ofstream outfile;

    //const sc2::Unit * ourScout = m_bot.GetUnit(m_proxyUnitTag);
    sc2::Vector2D myVec(proxy_x, proxy_y);

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

sc2::Point2D ProxyManager::getProxyLocation()
{
    sc2::Point2D myPoint(proxy_x, proxy_y);
    return myPoint;
}
#pragma endregion ProxyManagerMemberFunctions