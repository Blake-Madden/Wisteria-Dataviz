/** @addtogroup Utilities
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PARALLEL_H
#define WISTERIA_PARALLEL_H

#include "../math/mathematics.h"
#include <algorithm>
#include <thread>
#include <vector>

/// @brief Parallel-processing utilities.
namespace parallel
    {
    /** @brief Runs @p indexFunc for each index in [firstIndex, endIndex], splitting the
            range across worker threads when there's enough work to justify it.
        @param firstIndex The first index (inclusive).
        @param endIndex The last index (exclusive).
        @param indexFunc Function taking an index (`int`) to invoke for each item in the range.*/
    template<typename IndexFuncT>
    void for_each_index_parallel(const int firstIndex, const int endIndex, IndexFuncT indexFunc)
        {
        const int indexCount{ endIndex - firstIndex };
        if (indexCount <= 0)
            {
            return;
            }

        const unsigned int threadCount{ std::clamp<unsigned int>(
            std::thread::hardware_concurrency(), 1, static_cast<unsigned int>(indexCount)) };

        if (threadCount <= 1)
            {
            for (int index = firstIndex; index < endIndex; ++index)
                {
                indexFunc(index);
                }
            return;
            }

        std::vector<std::jthread> workers;
        workers.reserve(threadCount);
        const int indicesPerThread{ safe_divide<int>(indexCount, static_cast<int>(threadCount)) };
        int start{ firstIndex };
        for (unsigned int i = 0; i < threadCount; ++i)
            {
            const int end{ (i + 1 == threadCount) ?
                               // last thread handles the rest of the indices
                               endIndex :
                               // all threads up to the last one get an even slice of the
                               // indices they should process
                               (start + indicesPerThread) };
            workers.emplace_back(
                [start, end, indexFunc]()
                {
                    for (int index = start; index < end; ++index)
                        {
                        indexFunc(index);
                        }
                });
            start = end;
            }
        }
    } // namespace parallel

#endif // WISTERIA_PARALLEL_H
