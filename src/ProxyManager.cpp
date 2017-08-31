#include "ProxyManager.h"
#include "CCBot.h"
#include "Util.h"
#include "Micro.h"

// globals
int proxy_x;
int proxy_y;

ProxyManager::ProxyManager(CCBot & bot)
    : m_bot(bot)
    , m_proxyUnitTag(0)
    , m_proxyUnderAttack(false)
{
}

void ProxyManager::onStart()
{
    setupProxyLocation();
}

void ProxyManager::onFrame()
{
    proxyBuildingAtRandomLocation();
//    drawScoutInformation();
}

bool ProxyManager::loadProxyTrainingData()
{
    std::ofstream outfile;
    outfile.open("proxytraining.txt", std::ios_base::app);
    outfile << std::endl << "Data";
    return 0;
}

bool ProxyManager::writeProxyTrainingData(int x, int y, int fitness)
{
    std::ofstream outfile;

    outfile.open("proxytraining.txt", std::ios_base::app);

    // append the training results to the end of the text file.
    outfile << std::endl << x << " " << y << " " << fitness;
    return 0;
}

// If we can't build at the chosen location, update that information in our data structure.
bool ProxyManager::isProxyLocationValid(int x, int y)
{
    if (m_bot.Map().canBuildTypeAtPosition(x, y, sc2::UNIT_TYPEID::TERRAN_BARRACKS))
        return true;
    return false;
}

bool ProxyManager::setupProxyLocation()
{
    std::ofstream outfile;
    srand(time(NULL));
    proxy_x = rand() % m_bot.Map().width();
    proxy_y = rand() % m_bot.Map().height();
    std::cout << proxy_x << "xloc " << proxy_y << "yloc" << std::endl;

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
bool ProxyManager::proxyBuildingAtRandomLocation()
{
    std::ofstream outfile;

    // Invalid training data is stored as -1.
    if (isProxyLocationValid(proxy_x, proxy_y))
    {
        std::cout << "invalid proxy location, recording to diary ;)";
        writeProxyTrainingData(proxy_x, proxy_y, -1);
        return true;
    }

    const sc2::Unit * ourScout = m_bot.GetUnit(m_proxyUnitTag);
    sc2::Vector2D myVec(proxy_x, proxy_y);

    //if (m_bot.GetUnit(m_proxyUnitTag)->pos.x > myVec.x - 1 && m_bot.GetUnit(m_proxyUnitTag)->pos.x < myVec.x + 1)
    //{
    //    m_bot.Workers().finishedWithWorker(m_proxyUnitTag);
    //}
    //else
    //{
    //    Micro::SmartMove(m_proxyUnitTag, myVec, m_bot);
    //}

    return true;
}

sc2::Point2D ProxyManager::getProxyLocation()
{
    sc2::Point2D myPoint(proxy_x, proxy_y);
    return myPoint;
}