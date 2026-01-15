///////////////////////////////////////////////////////////////////////////////
// Name:        debug_profile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "debug_profile.h"
#include <fstream>
#include <iostream>

#ifdef ENABLE_PROFILING

namespace debug_profile
    {
    std::filesystem::path debug_profile_reporter::m_outputPath = "profile.csv";
    std::set<debug_profile_info> debug_profile_reporter::m_profiles;
    std::vector<debug_profiler*> debug_profile_reporter::m_profilers;

    void debug_profile_info::add_duration_time(const std::chrono::nanoseconds& duration_time,
                                               const char* extra_info)
        {
        ++m_called_count;
        m_total_duration_time += duration_time;
        m_average_duration_time = m_total_duration_time / m_called_count;
        m_lowest_duration_time =
            (duration_time < m_lowest_duration_time) ? duration_time : m_lowest_duration_time;
        if (duration_time > m_highest_duration_time)
            {
            m_highest_duration_time = duration_time;
            if (extra_info != nullptr)
                {
                m_extra_info.assign(extra_info);
                }
            }
        m_duration_times.push_back(duration_time);
        }

    debug_profiler::debug_profiler(const char* name)
        : m_starttime(std::chrono::high_resolution_clock::now()), m_block_name(name)
        {
        push_profiler(this);
        }

    debug_profiler::debug_profiler(const char* name, const char* extra_info)
        : m_starttime(std::chrono::high_resolution_clock::now()), m_block_name(name),
          m_extra_info(extra_info)
        {
        push_profiler(this);
        }

    debug_profiler::~debug_profiler()
        {
        m_endtime = std::chrono::high_resolution_clock::now();
        const auto totalTime = (m_endtime - m_starttime) - m_total_pause_duration;

        const auto [iterator, inserted] = debug_profile::debug_profile_reporter::m_profiles.emplace(
            m_block_name.c_str(), totalTime, m_extra_info.c_str());
        // if it was already in the table, then add the current duration time to it
        if (!inserted)
            {
            auto node = debug_profile::debug_profile_reporter::m_profiles.extract(iterator);
            node.value().add_duration_time(totalTime, m_extra_info.c_str());
            debug_profile::debug_profile_reporter::m_profiles.insert(std::move(node));
            }
        pop_profiler();
        }

    void debug_profiler::push_profiler(debug_profiler* profiler)
        {
        if (!debug_profile::debug_profile_reporter::m_profilers.empty())
            {
            ((debug_profile::debug_profile_reporter::m_profilers.back()))->pause();
            }
        debug_profile::debug_profile_reporter::m_profilers.push_back(profiler);
        }

    void debug_profiler::pop_profiler()
        {
        debug_profile::debug_profile_reporter::m_profilers.pop_back();
        if (!debug_profile::debug_profile_reporter::m_profilers.empty())
            {
            ((debug_profile::debug_profile_reporter::m_profilers.back()))->unpause();
            }
        }

    void debug_profile_reporter::dump_results()
        {
        std::ofstream output;
        if (!m_outputPath.empty())
            {
            output.open(m_outputPath, std::ios::out | std::ios::trunc | std::ios::binary);
            }

        if (!m_profiles.empty())
            {
            std::chrono::nanoseconds totalTime{ 0 };
            for (const auto& pos : m_profiles)
                {
                totalTime += pos.m_total_duration_time;
                }
            // write a header
            const std::string header = "Name\tTimes calls\tTotal time (in milliseconds)\t"
                                       "Total time (%)\tLowest call time\tHighest call time\t"
                                       "Average call time\tExtra Info (from highest call time)\n";
            if (output.is_open())
                {
                output << header.c_str();
                }
            std::wcout << header.c_str();
            // write out the profiled blocked
            std::stringstream stream;
            stream.imbue(std::locale{ "" }); // show thousands separator for milliseconds
            // OK to make copies here so that we can string decorators from the name
            for (auto pos : m_profiles)
                {
                stream.clear();
                stream.str(std::string());
                // simplify template info in function names
                const auto templateStart = pos.m_name.find_first_of('<');
                const auto templateEnd = pos.m_name.rfind(">::");
                if (templateStart != std::string::npos && templateEnd != std::string::npos)
                    {
                    pos.m_name.replace(templateStart + 1, (templateEnd + 2) - templateStart,
                                       "...>::");
                    }
                stream << pos.m_name << "\t" << pos.m_called_count << "\t"
                       << std::chrono::duration_cast<std::chrono::milliseconds>(
                              pos.m_total_duration_time)
                              .count()
                       << "\t"
                       << std::floor((static_cast<long double>(pos.m_total_duration_time.count()) /
                                      static_cast<long double>(totalTime.count())) *
                                     100)
                       << "%\t"
                       << std::chrono::duration_cast<std::chrono::milliseconds>(
                              pos.m_lowest_duration_time)
                              .count()
                       << "\t"
                       << std::chrono::duration_cast<std::chrono::milliseconds>(
                              pos.m_highest_duration_time)
                              .count()
                       << "\t"
                       << std::chrono::duration_cast<std::chrono::milliseconds>(
                              pos.m_average_duration_time)
                              .count()
                       << "\t" << pos.m_extra_info << "\n";
                if (output.is_open())
                    {
                    output << stream.str();
                    }
                // dump it to standard output too
                std::cout << stream.str();
                }
            }
        else
            {
            // just empty output file if no profile data to dump
            // (in case there was junk in it from a previous run)
            if (output.is_open())
                {
                output.clear();
                }
            }
        }
    } // namespace debug_profile

#endif
