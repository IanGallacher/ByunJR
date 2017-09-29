#include "ProxyManager.h"
#include "ByunJRBot.h"
#include "Util.h"
#include "Micro.h"
#include "sc2lib/sc2_lib.h"

// The bot is not fully setup when the default constructor is called. Therefore, we need to have a seprate init function.
void ProxyTrainingData::InitAllValues(ByunJRBot & bot)
{
    m_playable_max = bot.Observation()->GetGameInfo().playable_max;
    m_playable_min = bot.Observation()->GetGameInfo().playable_min;

    m_arena_width = (int) (bot.Observation()->GetGameInfo().playable_max.x - bot.Observation()->GetGameInfo().playable_min.x);
    m_arena_height = (int) (bot.Observation()->GetGameInfo().playable_max.y - bot.Observation()->GetGameInfo().playable_min.y);

    m_bot = &bot;

    m_playerStart_y = (int) bot.Bases().getPlayerStartingBaseLocation(Players::Self)->getPosition().y;
    // This won't work for four player maps.
    m_enemyStart_y = (int) bot.Observation()->GetGameInfo().enemy_start_locations[0].y;


    std::cout << "miny" << bot.Observation()->GetGameInfo().playable_min.y << "minx" << bot.Observation()->GetGameInfo().playable_min.x;
    
    // init the result vector to have the correct number of elements. 
    // Done over a few lines to increase legibility.
    m_result.resize(m_arena_height);
    for (auto &row : m_result)
    {
        row.resize(m_arena_width);
    }

    loadProxyTrainingData();

    writeAllTrainingData(getTrainingDataFileName());
    setupProxyLocation();
}

std::string ProxyTrainingData::getTrainingDataFileName()
{
    return m_bot->Config().MapName + "TrainingData";
}

// Returns the proxy location in "True Map Space"
sc2::Point2D ProxyTrainingData::getProxyLocation()
{
    BOT_ASSERT(m_proxy_x != 0 || m_proxy_y != 0, "Please setup the proxy location values before trying to retrieve them.");
    sc2::Point2D proxyLocation((float) m_proxy_x + m_playable_min.x, (float) m_proxy_y + m_playable_min.y);
    return proxyLocation;
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

    trainingData.open(getTrainingDataFileName());

    // If we have an empty file, go ahead and test all the points on the map and use that instead of loading the file.
    if (trainingData.peek() == std::ifstream::traits_type::eof())
    {
        testAllPointsOnMap();
        // In order to save computation time, only half of the valid locations will be searched initially. 
        // After we feel like we have some good canidates for the "best" proxy location, 
        // we can look for nearby tile locations that were eliminated by this function.
        reduceSearchSpace(2);
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

// Iterate through the result (training data) data structure and update the ViableLocations vector.
void ProxyTrainingData::upadateViableLocationsList()
{
    for (int y = 0; y < m_result.size(); ++y)
    {
        for (int x = 0; x < m_result[y].size(); ++x)
        {
            if (m_result[y][x] == MapDataValue::LocationWithoutResultValue)
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
    if(m_playerStart_y < m_enemyStart_y)
       m_result[m_proxy_y][m_proxy_x] = fitness;
    else
        m_result[m_arena_height - m_proxy_y][m_arena_width - m_proxy_x] = fitness;

    std::cout << fitness << "fitness" << std::endl;
    writeAllTrainingData(getTrainingDataFileName());
}

// If we can't build at the chosen location, update that information in our data structure.
// This function takes the parameters in "True Map Space"
bool ProxyTrainingData::isProxyLocationValid(int x, int y)
{
    if (m_bot->Map().canBuildTypeAtPosition(x, y, sc2::UNIT_TYPEID::TERRAN_BARRACKS))
        return true;
    return false;
}

// If we can't build at the chosen location, update that information in our data structure.
void ProxyTrainingData::testAllPointsOnMap()
{
    BOT_ASSERT(m_arena_height != 0, "Play area height is zero!");
    BOT_ASSERT(m_arena_width != 0, "Play area height is zero!");

    for (int y = 0; y < m_arena_height; ++y)
    {
        for (int x = 0; x < m_arena_width; ++x)
        {
            if (!isProxyLocationValid(x + (int) m_playable_min.x, y + (int) m_playable_min.y))
            {
                m_result[y][x] = MapDataValue::UnbuildableLocation;
            }
        }
    }
}

// reductionFactor means "for every {reductionFactor} items, keep only 1 of them."
// Example: If reductionFactor is 2, keep only half of the valid building locations.
// Example: If reductionFactor is 1, keep everything. 
void ProxyTrainingData::reduceSearchSpace(int reductionFactor)
{
    BOT_ASSERT(m_arena_height != 0, "Play area height is zero!");
    BOT_ASSERT(m_arena_width != 0, "Play area height is zero!");
    BOT_ASSERT(reductionFactor > 0, "reductionFactor must be one or bigger");

    // If reductionFactor is one, nothing will change.
    // Save time by skipping the rest of the function.
    if (reductionFactor == 1) { return; }

    int validLocationNumber = 0;
    for (int y = 0; y < m_arena_height; ++y)
    {
        for (int x = 0; x < m_arena_width; ++x)
        {
            if (validLocationNumber % 2 == 0)
            {
                m_result[y][x] = MapDataValue::IgnoredLocationToSaveSearchSpace;
            }
            ++validLocationNumber;
        }
    }
}

bool ProxyTrainingData::setupProxyLocation()
{
    srand( (unsigned int) time(NULL));
    int index = rand() % ViableLocations.size();

    // There are two coordinate systems for storing the proxy location.
    // "True Map Space" - Some maps are larger than the total play area.
    // "Training Space" - The play area only.
    // To convert from training space to true map space, add m_playable_min.
    // For the most part, "Training Space" does not exist outside of the ProxyTrainingData class.
    // m_proxy_x is stored in "Training Space."
    m_proxy_x = (int) (ViableLocations[index].m_loc.x);
    m_proxy_y = (int) (ViableLocations[index].m_loc.y);

    sc2::Vector2D myVec( (float) m_proxy_x, (float) m_proxy_y);
    std::cout << myVec.x << "m_proxy_x " << myVec.y << "m_proxy_y" << std::endl;
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

void ProxyManager::onStart()
{
    m_firstReaperCreated = false;
    m_ptd.InitAllValues(m_bot);
}

void ProxyManager::onFrame()
{
    proxyBuildingAtChosenRandomLocation();
}

void ProxyManager::onUnitCreated(const sc2::Unit& unit)
{
    if (unit.unit_type == sc2::UNIT_TYPEID::TERRAN_REAPER && !m_firstReaperCreated)
    {
        const BaseLocation * enemyBaseLocation = m_bot.Bases().getPlayerStartingBaseLocation(Players::Enemy);

        m_bot.Resign();
        m_ptd.recordResult((int)m_bot.Query()->PathingDistance(&unit, enemyBaseLocation->getPosition()));
        m_firstReaperCreated = true;
    }
}

void ProxyManager::onUnitEnterVision(const sc2::Unit& unit)
{
    const sc2::Unit *proxySCV = m_bot.GetUnit(m_proxyUnitTag);
    if (!proxySCV) return;
    double dist( sqrt((unit.pos.x-proxySCV->pos.x)*(unit.pos.x-proxySCV->pos.x)+(unit.pos.y-proxySCV->pos.y)*(unit.pos.y-proxySCV->pos.y)));

    if (dist < 8 && !m_firstReaperCreated)
    {
        m_bot.Resign();
        m_ptd.recordResult(-9);
        std::cout << "THERE IS NO POINT IN CONTINUING";
    }
}

// YOU MUST CALL m_ptd.InitAllValues() before this.
bool ProxyManager::proxyBuildingAtChosenRandomLocation()
{
    std::ofstream outfile;

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
        m_bot.Workers().setProxyWorker(m_proxyUnitTag);
        Micro::SmartMove(m_proxyUnitTag, myVec, m_bot);
    }
    //}

    return true;
}

sc2::Point2D ProxyManager::getProxyLocation()
{
    return m_ptd.getProxyLocation();
}


#pragma endregion ProxyManagerMemberFunctions