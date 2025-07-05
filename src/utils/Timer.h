#ifndef TIMER_H
#define TIMER_H

#include <iostream>
#include <chrono>
#include <string>


namespace img_deinterlace
{
    class Timer {
    public:
        Timer(const std::string &tag) : m_Tag(tag), m_Stopped(false) {
            m_StartTime = std::chrono::high_resolution_clock::now();
        }

        ~Timer() {
            Stop();
        }

        void Stop() {
            if (m_Stopped) return; // Prevent double stop

            auto endTime = std::chrono::high_resolution_clock::now();

            auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTime).time_since_epoch().count();
            auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTime).time_since_epoch().count();

            auto duration = end - start; 
            double ms = duration * 0.001; 

            std::cout << "[Timer] " << m_Tag << " took " << ms << " ms\n";

            m_Stopped = true;
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
        std::string m_Tag;
        bool m_Stopped;
    };
}


#endif //!TIMER_H