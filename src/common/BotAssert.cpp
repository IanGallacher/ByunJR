#include <iostream>
#include <sstream>

#include "common/BotAssert.h"

namespace Assert
{
    std::string last_error_message;

    const std::string CurrentDateTime() 
    {
//        time_t     now = time(0);
//        struct tm  tstruct;
//        char       buf[80];
//        //tstruct = *localtime(&now);
//        localtime_s(&tstruct, &now);
//        strftime(buf, sizeof(buf), "%Y-%m-%d_%X", &tstruct);
//
//        for (size_t i=0; i<80; ++i)
//        {
//            if (buf[i] == ':') buf[i] = '-';
//        }
//
//        return buf;
        return "0";
    }

    void ReportFailure(const char * condition, const char * file, int line, const char * msg, ...)
    {
        char message_buffer[1024] = "";
        if (msg != nullptr)
        {
            //va_list args;
            //va_start(args, msg);
            //vsprintf(messageBuffer, msg, args);
            //vsnprintf_s(messageBuffer, 1024, msg, args);
            //va_end(args);
            std::cout << msg << std::endl;
        }

        std::stringstream ss;
        ss                                              << std::endl;
        ss << "!Assert:   " << condition                << std::endl;
        ss << "File:      " << file                     << std::endl;
        ss << "Message:   " << message_buffer            << std::endl;
        ss << "Line:      " << line                     << std::endl;
        ss << "Time:      " << CurrentDateTime()        << std::endl;
        
        last_error_message = message_buffer;

        std::cerr << ss.str();
    }
}

