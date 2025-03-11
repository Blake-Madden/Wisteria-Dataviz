/**@addtogroup Debugging
   @brief Macros used for performance analysis.
    Compile with the symbol `ENABLE_PROFILING` to enable.
   @details The macros included in this library can track the
    performance times for either entire functions or specific blocks of code.

    The benefits of this library over other profiling tools are:

    - Performance times are only collected on the sections of code that you specify. This is useful for when you are only interested
      in reviewing certain sections of code, rather than the entire codebase.
    - Along with collecting function times (PROFILE()), specific blocks of code can also be tracked (PROFILE_SECTION_START()).
    - When a section of code that is being profiled calls another profiled code block, then the first block will be paused. This will
      thus only show the time it took to execute the code in the initial block, excluding the time it took to call any subsequent blocks
      that are also being tracked. This is an important distinction from other profiling systems.

    Profiling information will be written to standard output and a specified file (SET_PROFILER_REPORT_PATH()).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the 3-Clause BSD License.

    SPDX-License-Identifier: BSD-3-Clause
   @par Citation
    This was inspired by the article:

    Hjelstrom, Greg, and Byon Garrabrant. "Real-Time Hierarchical Profiling." *Game Programming Gems 3*, Charles River Media, 2002, pp 146-152.
   @date 2005-2025
   @copyright Blake Madden
   @author Blake Madden
   @par Example
   @code
    int main()
        {
        SET_PROFILER_REPORT_PATH("c:\\temp\\profile.csv"); // optionally write to log file
        otherFunction();
        complexFunction();
        return 0; // profiler will now dump its results to the log file and standard out
        }

    void otherFunction()
        {
        PROFILE();
        ...code
        }

    void complexFunction()
        {
        some code...

        PROFILE_SECTION_START("complex function subsection");
        ...possible bottleneck code being profiled
        PROFILE_SECTION_END();

        more code...
        }
    @endcode
@{*/

#ifndef __DEBUG_PROFILE_H__
#define __DEBUG_PROFILE_H__

#include <set>
#include <algorithm>
#include <string>
#include <ctime>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <locale>
#include <cassert>

/**
@def PROFILE()
    @brief Profiles the current function and will write to the profile data when the function completes.
    @note This macro should be the first line of the function.

@def PROFILE_WITH_INFO(info)
    @brief Profiles the current function and will write to the profile data when the function completes.
    @param info Information specific to this function call (e.g., the argument values to this function).
     The information connected to the highest call time of this function will be shown in the results.
     An example is to convert the arguments to a profiled function into a string and pass that in here.
     When the results are dumped, the arguments associated with the highest call time to this function will be shown.
    @note This macro should be the first line of the function.

@def PROFILE_SECTION_START(section_name)
    @brief Profiles a section of code. A unique label describing the code section should be passed here.
    @details The profiling will stop when the code section goes out of scope. @sa PROFILE_SECTION_END
    @param section_name The user-defined name to associate with the code block.

@def PROFILE_SECTION_WITH_INFO_START(section_name, info)
    @brief Profiles a section of code. A unique label describing the code section should be passed here.
    @details The profiling will stop when the code section goes out of scope. @sa PROFILE_SECTION_END
    @param section_name The user-defined name to associate with the code block.
    @param info Information specific to this function call (e.g., the argument values to this function).
     The information connected to the highest call time of this function will be shown in the results.
     An example is to convert the arguments to a profiled function into a string and pass that in here.
     When the results are dumped, the arguments associated with the highest call time to this function will be shown.

@def PROFILE_SECTION_END()
    Ends a profiled section. @see PROFILE_SECTION_START
    @note If this is not called, then the current `PROFILE_SECTION_START()` block will implicitly stop at the
     end of the bracketed section that it is inside of.

@def SET_PROFILER_REPORT_PATH(path)
    @brief Sets the path to where the profile report will be written.
    @details This is a tab-delimited report containing the following columns:
     - Function name
     - Times calls
     - Total time (in milliseconds)
     - Total time (%)
     - Lowest call time
     - Highest call time
     - Average call time
     - Extra Info (connected to the call with the highest call time)
    @note Times are in milliseconds.

@def DUMP_PROFILER_REPORT()
    @brief Outputs all of the current profile information.
    @details This will automatically happen at the exit of the program, but can be explicitly called via this macro at any time.
*/
/** @} */

/* The standard __func__ macro doesn't include the name of the class for member functions,
   so it isn't as useful as it could be. Try to use more descriptive macros (if available) first.*/
#if defined(__VISUALC__)
    #define __DEBUG_FUNCTION_NAME__ __FUNCSIG__
#elif defined(__GNUG__)
    #define __DEBUG_FUNCTION_NAME__ __PRETTY_FUNCTION__
#else
    #define __DEBUG_FUNCTION_NAME__ __func__
#endif

#ifdef ENABLE_PROFILING
    #define PROFILE() \
        __debug::__profiler __debug_profiled_function__(__DEBUG_FUNCTION_NAME__)
    #define PROFILE_WITH_INFO(info) \
        __debug::__profiler __debug_profiled_function__info__(__DEBUG_FUNCTION_NAME__, (info))
    #define PROFILE_SECTION_START(section_name) \
        { __debug::__profiler __debug_profiled_section__(section_name)
    #define PROFILE_SECTION_WITH_INFO_START(section_name, info) \
        { __debug::__profiler __debug_profiled_function__info__(section_name, (info))
    #define PROFILE_SECTION_END() }
    #define SET_PROFILER_REPORT_PATH(path) \
                    __debug::__profile_reporter::set_output_path((path))
    #define DUMP_PROFILER_REPORT() \
                    __debug::__profile_reporter::dump_results()
#else
    #define PROFILE() ((void)0)
    #define PROFILE_WITH_INFO(info) ((void)0)
    #define PROFILE_SECTION_START(section_name) ((void)0)
    #define PROFILE_SECTION_WITH_INFO_START(section_name, info) ((void)0)
    #define PROFILE_SECTION_END() ((void)0)
    #define SET_PROFILER_REPORT_PATH(path) ((void)0)
    #define DUMP_PROFILER_REPORT() ((void)0)
#endif

#ifdef ENABLE_PROFILING
//-----------------------------------------------------------------------
// profiler definition
namespace __debug
    {
    //-------------------------------------
    class __profile_info
        {
    public:
        __profile_info(const char* name, const std::chrono::nanoseconds& duration_time) :
            m_name(name),
            m_called_count(1), m_lowest_duration_time(duration_time),
            m_highest_duration_time(duration_time),
            m_total_duration_time(duration_time), m_average_duration_time(duration_time)
            {}
        __profile_info(const char* name, const std::chrono::nanoseconds& duration_time,
                       const char* extra_info) :
            m_name(name), m_extra_info(extra_info),
            m_called_count(1), m_lowest_duration_time(duration_time),
            m_highest_duration_time(duration_time),
            m_total_duration_time(duration_time), m_average_duration_time(duration_time)
            {}
        [[nodiscard]]
        inline bool operator<(const __profile_info& that) const noexcept
            { return m_name < that.m_name; }
        [[nodiscard]]
        inline bool operator==(const __profile_info& that) const noexcept
            { return m_name == that.m_name; }
        void add_duration_time(const std::chrono::nanoseconds& duration_time, const char* extra_info);

        std::string m_name;
        std::string m_extra_info;
        size_t m_called_count{ 0 };
        std::chrono::nanoseconds m_lowest_duration_time;
        std::chrono::nanoseconds m_highest_duration_time;
        std::chrono::nanoseconds m_total_duration_time;
        std::chrono::nanoseconds m_average_duration_time;
        std::vector<std::chrono::nanoseconds> m_duration_times;
        };

    //-------------------------------------
    class __profiler
        {
    public:
        explicit __profiler(const char* name);
        __profiler(const char* name, const char* extra_info);
        __profiler(const __profiler& that) = delete;
        __profiler& operator=(const __profiler&) = delete;
        ~__profiler();

        inline static void push_profiler(__profiler* profiler);
        inline static void pop_profiler();

        inline void pause() noexcept
            { m_pause_starttime = std::chrono::high_resolution_clock::now(); }
        inline void unpause() noexcept
            {
            m_pause_endtime = std::chrono::high_resolution_clock::now();
            m_total_pause_duration += (m_pause_endtime - m_pause_starttime);
            }
    protected:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_starttime;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_endtime;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_pause_starttime;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_pause_endtime;
        std::chrono::nanoseconds m_total_pause_duration{ 0 };
        std::string m_block_name;
        std::string m_extra_info;
        };

    //-------------------------------------
    class __profile_reporter
        {
    public:
        __profile_reporter() noexcept {}
        __profile_reporter(const __profile_reporter& that) = delete;
        __profile_reporter& operator=(const __profile_reporter& that) = delete;
        ~__profile_reporter()
            { dump_results(); }
        static void set_output_path(const char* path)
            { m_outputPath = path; }
        static void dump_results();
        static std::string m_outputPath;
        static std::set<__profile_info> m_profiles;
        static std::vector<__profiler*> m_profilers;
        };
    }
#endif

#endif //__DEBUG_PROFILE_H__
