#include <fstream>
#include <sstream>

#include "ByunJRBot.h"
#include "common/BotAssert.h"
#include "information/ProxyTrainingData.h"

// The bot is not fully setup when the default constructor is called. Therefore, we need to have a seprate init function.
void ProxyTrainingData::InitAllValues(ByunJRBot & bot)
{
    bot_ = &bot;

    playable_max_ = bot.Observation()->GetGameInfo().playable_max;
    playable_min_ = bot.Observation()->GetGameInfo().playable_min;

    player_start_y_ = (int)bot.InformationManager().Bases().GetPlayerStartingBaseLocation(sc2::Unit::Alliance::Self)->GetPosition().y;
    // This won't work for four player maps.
    enemy_start_y_ = (int)bot.Observation()->GetGameInfo().enemy_start_locations[0].y;

    // init the result vector to have the correct number of elements. 
    // Done over a few lines to increase legibility.
    result_.resize(bot_->InformationManager().Map().PlayableMapHeight());
    for (auto &row : result_)
    {
        row.resize(bot_->InformationManager().Map().PlayableMapWidth());
    }

    LoadProxyTrainingData();

    SetupProxyLocation();
}
// WARNING: DOES NOT INCLUDE FILE EXTENSION
std::string ProxyTrainingData::GetTrainingDataFileName()
{
    return "data/ByunJR/" + bot_->Config().MapName + "TrainingData";
}

sc2::Point2DI ProxyTrainingData::FlipCoordinatesIfNecessary(const int x, const int y)
{
    sc2::Point2DI return_val;
    if (player_start_y_ < enemy_start_y_)
    {
        return_val.x = x;
        return_val.y = y;
    }
    else
    {
        return_val.x = (bot_->InformationManager().Map().PlayableMapWidth() - x);
        return_val.y = (bot_->InformationManager().Map().PlayableMapHeight() - y);
    }
    return return_val;
}

// Returns the proxy location in "True Map Space"
sc2::Point2DI ProxyTrainingData::GetProxyLocation()
{
    if(proxy_loc_.x == 0 || proxy_loc_.y == 0)
        std::cout << "Please setup the proxy location values before trying to retrieve them." << std::endl;
    const sc2::Point2DI proxy_location(proxy_loc_.x + playable_min_.x, proxy_loc_.y + playable_min_.y);

    if (bot_->Config().TrainingMode)
        return proxy_location;
    else
        return GetBestProxyLocation();
}

// Returns the best proxy location in "True Map Space"
sc2::Point2DI ProxyTrainingData::GetBestProxyLocation()
{
    BOT_ASSERT(best_proxy_loc_.x != 0 || best_proxy_loc_.y != 0, "Please setup the proxy location values before trying to retrieve them.");

    const sc2::Point2DI best_loc = FlipCoordinatesIfNecessary(best_proxy_loc_.x, best_proxy_loc_.y);

    // Convert coordinate systems.
    const sc2::Point2DI proxy_location(best_loc.x + playable_min_.x, best_loc.y + playable_min_.y );

    return proxy_location;
}

int ProxyTrainingData::GetReward()
{
    const sc2::Point2DI actual_proxy_loc = FlipCoordinatesIfNecessary(proxy_loc_.x, proxy_loc_.y);

    return result_[actual_proxy_loc.y][actual_proxy_loc.x];
}

sc2::Point2D ProxyTrainingData::GetNearestUntestedProxyLocation(const int x, const int y)
{
    sc2::Point2D closest_point;
    int dist = std::numeric_limits<int>::max();
    for (int i = 0; i < viable_locations_.size(); ++i)
    {
        const int delta_x = (viable_locations_[i].loc.x - x);
        const int delta_y = (viable_locations_[i].loc.y - y);
        const int new_dist = (delta_x * delta_x) + (delta_y * delta_y);
        if (new_dist < dist)
        {
            closest_point = sc2::Point2D(x, y);
            dist = new_dist;
        }
    }
    return closest_point;
}

// Is the proxy location ready to go? Has it been setup yet?
bool ProxyTrainingData::ProxyLocationReady() const
{
    // If we are training the bot with a genetic algorithm, the genetic algorithm may not have setup the proxy location.
    // If that is the case, the proxyLocation is not ready to be retrieved. 
    if (bot_->Config().TrainingMode && (proxy_loc_.x == 0 || proxy_loc_.y == 0) )
        return false;
    // If we are not using a genetic algorithm, the "best coordinates" are ready.
    else
        return true;
}

sc2::Point2DI ProxyTrainingData::GetRandomViableProxyLocation()
{
    return viable_locations_[rand() % static_cast<int>(viable_locations_.size())].loc;
}

// Load all the values from training data stored on the disk.
// If no training data is found, test all points on the map for buildable barracks locations and load that instead.
// Training data is stored in a standard .txt file. Spaces separate values, and new lines separate data entries.
// Format used on every line on the file:
// XCOORDINATE YCOORDINATE FITNESS
bool ProxyTrainingData::LoadProxyTrainingData()
{
    std::ifstream training_data;
    std::string line;

    training_data.open(GetTrainingDataFileName() + ".txt");

    // If we have an empty file, go ahead and test all the points on the map and use that instead of loading the file.
    if (training_data.peek() == std::ifstream::traits_type::eof())
    {
        TestAllPointsOnMap();
        // In order to save computation time, only half of the valid locations will be searched initially. 
        // After we feel like we have some good canidates for the "best" proxy location, 
        // we can look for nearby tile locations that were eliminated by this function.
        ReduceSearchSpace(2);
    }

    else if (training_data.is_open())
    {
        while (getline(training_data, line))
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
                result_[y][x] = val;
            }
        }
    }
    training_data.close();

    // No matter how we get our data, we ALWAYS will have to setup the list of suitable proxy locations.
    UpadateViableLocationsList();
    return 0;
}

// Iterate through the result (training data) data structure and update the viableLocations vector.
void ProxyTrainingData::UpadateViableLocationsList()
{
    int best_reward = std::numeric_limits<int>::max();
    for (int y = 0; y < result_.size(); ++y)
    {
        for (int x = 0; x < result_[y].size(); ++x)
        {
            if (result_[y][x] == MapDataValue::LocationWithoutResultValue)
            {
                const sc2::Point2DI point(x, y);
                ProxyLocation pl = { point, result_[y][x] };
                viable_locations_.push_back(pl);
            }
            else if (result_[y][x] > 0 && result_[y][x] < best_reward)
            {
                best_proxy_loc_.x = x;
                best_proxy_loc_.y = y;
                best_reward = result_[y][x];
            }
        }
    }

    if(!bot_->Config().TrainingMode)
    {
        bot_->Config().SetProxyLocation(best_proxy_loc_.x, best_proxy_loc_.y);
        proxy_loc_.x = best_proxy_loc_.x;
        proxy_loc_.y = best_proxy_loc_.y;
    }
}

void ProxyTrainingData::RecordResult(const int fitness)
{
    const sc2::Point2DI actual_proxy_loc = FlipCoordinatesIfNecessary(proxy_loc_.x, proxy_loc_.y);
    
    result_[actual_proxy_loc.y][actual_proxy_loc.x] = fitness;

    std::cout << fitness << "fitness" << std::endl;
    WriteAllTrainingData(GetTrainingDataFileName());
}

// If we can't build at the chosen location, update that information in our data structure.
// This function takes the parameters in "True Map Space"
bool ProxyTrainingData::IsProxyLocationValid(int x, int y) const
{
    if (bot_->InformationManager().Map().CanBuildTypeAtPosition(x, y, sc2::UNIT_TYPEID::TERRAN_BARRACKS))
        return true;
    return false;
}

// If we can't build at the chosen location, update that information in our data structure.
void ProxyTrainingData::TestAllPointsOnMap()
{
    for (int y = 0; y < bot_->InformationManager().Map().PlayableMapHeight(); ++y)
    {
        for (int x = 0; x < bot_->InformationManager().Map().PlayableMapWidth(); ++x)
        {
            if (!IsProxyLocationValid(x + static_cast<int>(playable_min_.x), y + static_cast<int>(playable_min_.y)))
            {
                result_[y][x] = MapDataValue::UnbuildableLocation;
            }
        }
    }
}

// reductionFactor means "for every {reduction_factor} items, keep only 1 of them."
// Example: If reduction_factor is 2, keep only half of the valid building locations.
// Example: If reduction_factor is 1, keep everything. 
void ProxyTrainingData::ReduceSearchSpace(int reduction_factor)
{
    BOT_ASSERT(reduction_factor > 0, "reductionFactor must be one or bigger");

    // If reductionFactor is one, nothing will change.
    // Save time by skipping the rest of the function.
    if (reduction_factor == 1) { return; }

    int valid_location_number = 0;
    for (int y = 0; y < bot_->InformationManager().Map().PlayableMapHeight(); ++y)
    {
        for (int x = 0; x < bot_->InformationManager().Map().PlayableMapWidth(); ++x)
        {
            // keep only 1/reductionfactor valid entries. 
            // We are only interested in the untested locations.
            if (valid_location_number % reduction_factor == 0 && result_[y][x] == MapDataValue::LocationWithoutResultValue)
            {
                result_[y][x] = MapDataValue::IgnoredLocationToSaveSearchSpace;
            }
            ++valid_location_number;
        }
    }
}

bool ProxyTrainingData::SetupProxyLocation()
{
    srand(static_cast<unsigned int>(time(NULL)));
    int index = rand() % viable_locations_.size();

    // There are two coordinate systems for storing the map locations.
    // "True Map Space" - Some maps are larger than the total play area.
    // "Training Space" or "Playable Space" - The play area only.
    // To convert from training space to true map space, add playable_min.
    // For the most part, "Training Space" does not exist outside of the ProxyTrainingData class.
    // proxy_loc_.x is stored in "Training Space."
    // proxy_loc_.x = (int) (viableLocations[index].m_loc.x);
    // proxy_loc_.y = (int) (viableLocations[index].m_loc.y);
    proxy_loc_.x = bot_->Config().ProxyLocationX;
    proxy_loc_.y = bot_->Config().ProxyLocationY;

    const sc2::Vector2D my_vec(static_cast<float>(proxy_loc_.x), static_cast<float>(proxy_loc_.y));
    std::cout << my_vec.x << "m_proxy_x " << my_vec.y << "m_proxy_y" << std::endl;
    return true;
}

void ProxyTrainingData::WriteAllTrainingData(const std::string filename)
{
    std::cout << "WritingAllTrainingDataToFile...";
    std::ofstream outfile;
    std::stringstream buffer;

    // Open the file in truncate mode to overwrite previous saved data.
    outfile.open(filename + ".txt", std::ios_base::trunc);
    for (int y = static_cast<int>(result_.size()) - 1; y >= 0; --y)
    {
        // If we want to "draw" the map, uncomment out the following line and another one a few lines below it. 
        //buffer << std::endl;
        for (int x = 0; x < result_[y].size(); ++x)
        {
            buffer << x << " " << y << " " << result_[y][x] << std::endl;
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
