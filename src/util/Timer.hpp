#pragma once
#include <functional>
#include <thread>
#include <chrono>

#ifdef WIN32   // Windows system specific
    #include <windows.h>
#else          // Unix based system specific
    #include <sys/time.h>
#endif

class Timer
{
    double start_time_in_micro_sec_;                // starting time in micro-second
    double end_time_in_micro_sec_;                  // ending time in micro-second
    int    stopped_;                                // stop flag 
    #ifdef WIN32
        LARGE_INTEGER frequency_;                   // ticks per second
        LARGE_INTEGER start_count_;                 //
        LARGE_INTEGER end_count_;                   //
    #else
        timeval startCount;                         //
        timeval endCount;                           //
    #endif

public:

    Timer()
    {
        #ifdef WIN32
            QueryPerformanceFrequency(&frequency_);
            start_count_.QuadPart = 0;
            end_count_.QuadPart = 0;
        #else
            startCount.tv_sec = startCount.tv_usec = 0;
            endCount.tv_sec = endCount.tv_usec = 0;
        #endif

        stopped_ = 0;
        start_time_in_micro_sec_ = 0;
        end_time_in_micro_sec_ = 0;
        
        Start();
    }
    
    ~Timer() {}                                 // default destructor

    void Start()
    {
        stopped_ = 0; // reset stop flag
        
        #ifdef WIN32
            QueryPerformanceCounter(&start_count_);
        #else
            gettimeofday(&startCount, NULL);
        #endif
    }
    
    void Stop()
    {
        stopped_ = 1; // set timer stopped flag

        #ifdef WIN32
            QueryPerformanceCounter(&end_count_);
        #else
            gettimeofday(&endCount, NULL);
        #endif
    }
    
    double GetElapsedTimeInMicroSec()
    {
        #ifdef WIN32
            if(!stopped_)
                QueryPerformanceCounter(&end_count_);

            start_time_in_micro_sec_ = start_count_.QuadPart * (1000000.0 / frequency_.QuadPart);
            end_time_in_micro_sec_ = end_count_.QuadPart * (1000000.0 / frequency_.QuadPart);
        #else
            if(!stopped)
                gettimeofday(&endCount, NULL);

            startTimeInMicroSec = (startCount.tv_sec * 1000000.0) + startCount.tv_usec;
            endTimeInMicroSec = (endCount.tv_sec * 1000000.0) + endCount.tv_usec;
        #endif

        return end_time_in_micro_sec_ - start_time_in_micro_sec_;
    }
     
    double GetElapsedTimeInMilliSec()
    {
        return this->GetElapsedTimeInMicroSec() * 0.001;
    }


    double GetElapsedTimeInSec()
    {
        return this->GetElapsedTimeInMicroSec() * 0.000001;
    }

    double GetElapsedTime()
    {
        return this->GetElapsedTimeInSec();
    }

    template <class T>
    static void TimeFunction(std::function<T> function, ...)
    {
        Timer t;
        t.Start();
        va_list args;
        va_start(args, function);
        va_end(args);

        double ms = t.GetElapsedTimeInMilliSec();
        std::cout << "Function ran in " << ms << "ms\n";
    }
};

