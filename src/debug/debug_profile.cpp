///////////////////////////////////////////////////////////////////////////////
// Name:        debug_profile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "debug_profile.h"

#ifdef ENABLE_PROFILING

namespace __debug
    {
    std::string __profile_reporter::m_outputPath = "profile.csv";
    std::set<__profile_info> __profile_reporter::m_profiles;
    std::vector<__profiler*> __profile_reporter::m_profilers;
    static __profile_reporter __profile_reporter__;

    void __profile_info::add_duration_time(const std::chrono::nanoseconds& duration_time,
                                           const char* extra_info)
        {
        ++m_called_count;
        m_total_duration_time += duration_time;
        m_average_duration_time = m_total_duration_time / m_called_count;
        m_lowest_duration_time = (duration_time < m_lowest_duration_time) ?
            duration_time : m_lowest_duration_time;
        if (duration_time > m_highest_duration_time)
            {
            m_highest_duration_time = duration_time;
            if (extra_info)
                { m_extra_info.assign(extra_info); }
            }
        m_duration_times.emplace_back(duration_time);
        }

    __profiler::__profiler(const char* name) :
        m_starttime(std::chrono::high_resolution_clock::now()),
        m_block_name(name)
        { push_profiler(this); }

    __profiler::__profiler(const char* name, const char* extra_info) :
        m_starttime(std::chrono::high_resolution_clock::now()),
        m_block_name(name), m_extra_info(extra_info)
        { push_profiler(this); }

    __profiler::~__profiler()
        {
        m_endtime = std::chrono::high_resolution_clock::now();
        const auto totalTime = (m_endtime-m_starttime) - m_total_pause_duration;

        const auto [iterator, inserted] = __profile_reporter__.m_profiles.emplace(
            m_block_name.c_str(), totalTime, m_extra_info.c_str() );
        // if it was already in the table, then add the current duration time to it
        if (!inserted)
            {
            auto node = __profile_reporter__.m_profiles.extract(iterator);
            node.value().add_duration_time(totalTime, m_extra_info.c_str());
            __profile_reporter__.m_profiles.insert(std::move(node));
            }
        pop_profiler();
        }

    void __profiler::push_profiler(__profiler* profiler)
        {
        if (__profile_reporter__.m_profilers.size() )
            {
            (*(__profile_reporter__.m_profilers.back())).pause();
            }
        __profile_reporter__.m_profilers.push_back(profiler);
        }

    void __profiler::pop_profiler()
        {
        __profile_reporter__.m_profilers.pop_back();
        if (__profile_reporter__.m_profilers.size() )
            {
            (*(__profile_reporter__.m_profilers.back())).unpause();
            }
        }

    void __profile_reporter::dump_results()
        {
        std::fstream output;
        if (m_outputPath.length())
            { output.open(m_outputPath.c_str(), std::ios::out|std::ios::trunc); }

        if (m_profiles.size())
            {
            std::chrono::nanoseconds totalTime{0};
            for (const auto& pos : m_profiles)
                { totalTime += pos.m_total_duration_time; }
            // write a header
            std::string header = "Name\tTimes calls\tTotal time (in milliseconds)\t"
                                 "Total time (%)\tLowest call time\tHighest call time\t"
                                 "Average call time\tExtra Info (from highest call time)\n";
            if (output.is_open())
                { output << header.c_str(); }
            std::wcout << header.c_str();
            // write out the profiled blocked
            std::stringstream stream;
            stream.imbue(std::locale{""}); // show thousands separator for milliseconds
            for (auto pos : m_profiles)
                {
                stream.clear();
                stream.str(std::string());
                // simplify template info in function names
                const auto templateStart = pos.m_name.find_first_of('<');
                const auto templateEnd = pos.m_name.rfind(">::");
                if (templateStart != std::string::npos && templateEnd != std::string::npos)
                    { pos.m_name.replace(templateStart+1, (templateEnd+2)-templateStart, "...>::"); }
                stream << pos.m_name << "\t" <<
                        pos.m_called_count << "\t" <<
                        std::chrono::duration_cast<std::chrono::milliseconds>(pos.m_total_duration_time).count() << "\t" <<
                        std::floor(
                            (static_cast<long double>(pos.m_total_duration_time.count())/
                             static_cast<long double>(totalTime.count()))
                            *100) << "%\t" <<
                        std::chrono::duration_cast<std::chrono::milliseconds>(pos.m_lowest_duration_time).count() << "\t" <<
                        std::chrono::duration_cast<std::chrono::milliseconds>(pos.m_highest_duration_time).count() << "\t" <<
                        std::chrono::duration_cast<std::chrono::milliseconds>(pos.m_average_duration_time).count() << "\t" <<
                        pos.m_extra_info << "\n";
                if (output.is_open())
                    { output << stream.str(); }
                // dump it to standard output too
                std::cout << stream.str();
                }
            }
        else
            {
            // just empty output file if no profile data to dump
            // (in case there was junk in it from a previous run)
            if (output.is_open())
                { output.clear(); }
            }
        }
    }

#endif
