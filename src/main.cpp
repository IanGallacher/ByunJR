#include <iostream>
#include <string>
#include <sc2api/sc2_api.h>
#include <sc2utils/sc2_manage_process.h>

#include "ByunJRBot.h"
#include "ai/GeneticAlgorithm.h"
#include "rapidjson/document.h"
#include "util/JSONTools.h"
#include "util/Util.h"



#include "ai/pathfinding.h"
int main(int argc, char* argv[])
{
    //Pathfinding p;
    //p.TestDjikstra();
    //return 0;
    rapidjson::Document doc;
    std::string config = JSONTools::ReadFile("BotConfig.txt");
    if (config.length() == 0)
    {
        std::cerr << "Config file could not be found, and is required for starting the bot\n";
        std::cerr << "Please read the instructions and try again\n";
        exit(-1);
    }

    const bool parsing_failed = doc.Parse(config.c_str()).HasParseError();
    if (parsing_failed)
    {
        std::cerr << "Config file could not be parsed, and is required for starting the bot\n";
        std::cerr << "Please read the instructions and try again\n";
        exit(-1);
    }

    std::string bot_race_string;
    std::string enemy_race_string;
    std::string map_string;

    if (doc.HasMember("Game Info") && doc["Game Info"].IsObject())
    {
        const rapidjson::Value & info = doc["Game Info"];
        JSONTools::ReadString("BotRace", info, bot_race_string);
        JSONTools::ReadString("EnemyRace", info, enemy_race_string);
        JSONTools::ReadString("MapName", info, map_string);
        map_string += ".SC2Map"; // The MapName does not include the file extension.
    }
    else
    {
        std::cerr << "Config file has no 'Game Info' object, required for starting the bot\n";
        std::cerr << "Please read the instructions and try again\n";
        exit(-1);
    }
    std::cout << map_string << std::endl;


    std::cout << "GLHF" << std::endl;

    GeneticAlgorithm ga = GeneticAlgorithm();
    bool genetic_algorithm_setup = false;

    while (true) {
        // Test all 10 Candidates inside the population
        for (int i = 0; i < 10; i++)
        {
            sc2::Coordinator coordinator;
            if (!coordinator.LoadSettings(argc, argv))
            {
                std::cout << "Unable to find or parse settings." << std::endl;
                return 1;
            }

            coordinator.SetRealtime(false);

            //          Setting this = N means the bot's OnFrame gets called once every N frames
            //          The bot may crash or do unexpected things if its logic is not called every frame
            coordinator.SetStepSize(3);

            // Add the custom bot, it will control the players.
            ByunJRBot bot;

            coordinator.SetParticipants({
                CreateParticipant(Util::GetRaceFromString(bot_race_string), &bot),
                CreateComputer(Util::GetRaceFromString(enemy_race_string), sc2::Difficulty::VeryHard)
            });

            // Start the game.
            coordinator.LaunchStarcraft();
            coordinator.StartGame(map_string);
            bool already_init = false;
            while (coordinator.AllGamesEnded() != true && bot.IsWillingToFight())
            {
                coordinator.Update();

                if (genetic_algorithm_setup == false)
                {
                    for (int i = 0; i < 10; i++)
                    {
                        Population* pop = ga.GetPopulation();
                        // grab proxy training data once
                        const sc2::Point2DI point = bot.GetProxyManager().GetProxyTrainingData().GetRandomViableProxyLocation();
                        std::vector<int> genes = std::vector<int>();
                        genes.resize(2);
                        genes[0] = point.x;
                        genes[1] = point.y;
                        const Candidate can = Candidate(genes);
                        pop->SetCanidate(i, can);
                    }
                    genetic_algorithm_setup = true;
                }


                if (already_init == false)
                {
                    Candidate c = ga.GetPopulation()->GetCandidate(i);
                    bot.Config().SetProxyLocation(c.GetGene(0), c.GetGene(1));
                    bot.GetProxyManager().GetProxyTrainingData().SetupProxyLocation();
                    already_init = true;
                }
            }

            if (bot.Control()->SaveReplay("replay/asdf.Sc2Replay"))
            {
                std::cout << "REPLAYSUCESS" << "replay/asdf.Sc2Replay" << std::endl;
            }
            else
            {
                std::cout << "REPLAY FAIL" << "replay/asdf.Sc2Replay" << std::endl;
            }
            coordinator.LeaveGame();
            ga.SetReward(i, bot.GetProxyManager().GetProxyTrainingData().GetReward());
            if(i==9)
            {
                ga.EvolvePopulation(bot.GetProxyManager().GetProxyTrainingData());
                std::cout << "MUTATING" << std::endl;
            }
        }
    }

    return 0;
}
