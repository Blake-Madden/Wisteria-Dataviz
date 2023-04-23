/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_JITTER_H__
#define __WISTERIA_JITTER_H__

#include <algorithm>
#include "../math/safe_math.h"
#include "../math/mathematics.h"
#include "../util/frequencymap.h"
#include "../base/graphitems.h"
#include "../base/axis.h"

namespace Wisteria::Data
    {
    /** @brief Shifts (single-series) points around an axis using the bee-swarm method.
        @details When multiple data lie at the same point, jittering them slightly along the
            non-dominant axis makes it easier to see both points. This class keeps track of points
            like this and jitters them across both sides of the axis inside of a provided area.

            This applies to plots where the position of a point only relates to one axis.
            As an example, a vertical box plot, where the Y axis shows a datum's value,
            and shifting the datum along the X axis (but still inside the box) will not affect
            the meaning of its value.

            Likewise, jittering does not make sense for plots where both the X and Y axis values
            for a datum are meaningful (e.g., a scatterplots).
        @note This is a low-level class that is usually handled by plots.
            Client code should not need to use this.
        @par Example
        @code
        Jitter jtr{ AxisType::LeftYAxis };

        // CalcSpread() should be called once when an axis's data is set
        frequency_set<double> jitterPoints;
        // assuming "ptCollection" is a pointer to a Dataset.
        for (const auto& datum : ptCollection->GetYColumn().GetValues())
            { jitterPoints.insert(datum); }
        jtr.CalcSpread(jitterPoints);

        ...

        // During the RecalcSizes() or Draw() phase of a plot, adjust/reset the
        // jittering effect before jittering each point.

        // "boxRect" would be the box around a vertical box plot.
        // (If the axis was horizontal, the GetHeight() should be used.)
        jitter.SetJitterWidth(boxRect.GetWidth());

        // before calling JitterPoint() in a loop, call either ResetJitterData()
        // or SetJitterWidth() to flush any prior calculations of where
        // points are being jittered to.
        jitter.ResetJitterData();

        // as you iterate through the data and determine their physical positions,
        // jitter them to avoid points obscuring other points.
        if (GetPhysicalCoordinates(box.GetXAxisPosition(),
                                   box.GetData()->GetYColumn().GetValue(i), pt))
            {
            jitter.JitterPoint(pt);
            // draw the point, or add it to a Points2D collection to manage
            }
        @endcode

        @date 2020
       */
    class Jitter
        {
    public:
        /** @brief Constructor.
            @param jitterWidth The entire width (in pixels) around both sides of the axis where
                you can jitter points out to.
            @param dominantAxis The axis that holds the important value;
                the opposite axis will be jittered.\n
                For example, LeftYAxis will cause jittering along the x axis.*/
        Jitter(const size_t jitterWidth, const AxisType dominantAxis) :
            m_jitterSideWidth(safe_divide<size_t>(jitterWidth, 2)),
            m_dominantAxis(dominantAxis)
            {}
        /** @brief Constructor.
            @param dominantAxis The axis that holds the important value;
                the opposite axis will be jittered.\n
                For example, LeftYAxis will cause jittering along the x axis.*/
        explicit Jitter(const AxisType dominantAxis) : m_dominantAxis(dominantAxis)
            {}
        /// @private
        Jitter() = delete;

        /** @brief Determines how many points should be spread across each side of the axis,
                based on the point with the highest frequency.
            @param points The data points to analyze.
            @note This only needs to be called once after a plot's data changes,
                does not need to be called after calls to SetJitterWidth() or ResetJitterData().*/
        template<typename T>
        void CalcSpread(const frequency_set<T>& points) noexcept
            {
            if (points.get_data().size() == 0)
                {
                m_numberOfPointsOnEachSide = 0;
                return;
                }
            m_numberOfPointsOnEachSide =
                static_cast<size_t>(round_to_integer(
                safe_divide<double>(
                    std::max_element(points.get_data().cbegin(), points.get_data().cend(),
                        [](const auto& first, const auto& second)
                            { return first.second < second.second; })->second,
                    2)));
            }

        /** @brief Sets the width of how far the points can be jittered around the axis.
            @param jitterWidth The entire width (in pixels) around both sides of the axis
                where you can jitter points out to.
            @note Will call ResetJitterData() for you.*/
        void SetJitterWidth(const size_t jitterWidth) noexcept
            {
            m_jitterSideWidth = safe_divide<size_t>(jitterWidth, 2);
            ResetJitterData();
            }

        /** @brief Call this before a series of calls to JitterPoint().
            @details Will not affect the number of points on each side of the axis,
                just clears data from previous jittering calls.
            @note Calling SetJitterWidth() will call this for you.*/
        void ResetJitterData() noexcept
            { m_plottedPoints.clear(); }

        /** @brief Jitters a point.
            @param[in,out] pt The point to be jittered (along the non-dominant axis).
            @returns Whether the point was jittered.
            @note This will accumulate the points passed into it to keep track
                of the offset of where points should be jittered to.
                When finished with jittering points, call ResetJitterData()
                (or SetJitterWidth()) before performing another series of calls to this function.*/
        bool JitterPoint(wxPoint& pt)
            {
            if (m_dominantAxis == AxisType::LeftYAxis ||
                m_dominantAxis == AxisType::RightYAxis)
                {
                const auto plottedPointInfo = m_plottedPoints.insert(pt.y);
                // only jitter if there is already another point on the axis line
                if (plottedPointInfo->second > 1)
                    {
                    // if an even number of points, jitter to the left
                    pt.x += is_even(plottedPointInfo->second) ?
                        -static_cast<wxCoord>(safe_divide(m_jitterSideWidth, m_numberOfPointsOnEachSide) *
                                std::clamp<size_t>(safe_divide<size_t>(plottedPointInfo->second,2),
                                                   1, m_numberOfPointsOnEachSide)) :
                        // or to the right if odd
                        static_cast<wxCoord>(safe_divide(m_jitterSideWidth, m_numberOfPointsOnEachSide) *
                            std::clamp<size_t>(safe_divide<size_t>(plottedPointInfo->second,2),
                                                1, m_numberOfPointsOnEachSide));
                    return true;
                    }
                return false;
                }
            else
                {
                const auto plottedPointInfo = m_plottedPoints.insert(pt.x);
                if (plottedPointInfo->second > 1)
                    {
                    pt.y += is_even(plottedPointInfo->second) ?
                        -static_cast<wxCoord>(safe_divide(m_jitterSideWidth, m_numberOfPointsOnEachSide) *
                                std::clamp<size_t>(safe_divide<size_t>(plottedPointInfo->second,2),
                                                   1, m_numberOfPointsOnEachSide)) :
                         static_cast<wxCoord>(safe_divide(m_jitterSideWidth, m_numberOfPointsOnEachSide) *
                                std::clamp<size_t>(safe_divide<size_t>(plottedPointInfo->second,2),
                                                   1, m_numberOfPointsOnEachSide));
                    return true;
                    }
                return false;
                }
            }
    private:
        frequency_set<wxCoord> m_plottedPoints;
        size_t m_jitterSideWidth{ 0 };
        size_t m_numberOfPointsOnEachSide{ 50 };
        AxisType m_dominantAxis{ AxisType::LeftYAxis };
        };
    }

/** @}*/

#endif //__WISTERIA_JITTER_H__
