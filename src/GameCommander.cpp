#include <sstream>

#include "ByunJRBot.h"
#include "GameCommander.h"
#include "common/Common.h"
#include "common/BotAssert.h"
#include "util/Util.h"

GameCommander::GameCommander(ByunJRBot & bot)
    : m_bot                 (bot)
    , m_initialScoutSet     (false)
{

}

