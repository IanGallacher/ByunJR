#pragma once
#include <iomanip>

//#define BOT_BREAK __debugbreak();

#if true
    #define BOT_ASSERT(cond, msg, ...) \
        do \
        { \
            if (!(cond)) \
            { \
                Assert::ReportFailure(#cond, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
                /*BOT_BREAK*/ \
            } \
        } while(0)
#else
    #define BOT_ASSERT(cond, msg, ...) 
#endif

namespace Assert
{
    extern std::string last_error_message;

    const std::string CurrentDateTime();

    void ReportFailure(const char * condition, const char * file, int line, const char * msg, ...);
}
