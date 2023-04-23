/** @addtogroup Utilities
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>
#include <limits>
#include <cmath>
#include <vector>
#include <map>
#include <execution>
#include "mathematics.h"
#include "safe_math.h"
#include "../util/frequencymap.h"
#include "../debug/debug_assert.h"

/// @brief Namespace for statistics classes.
namespace statistics
    {
    /** @returns The valid (non-NaN) number of observations from the specified range.
        @param data The data to analyze.*/
    [[nodiscard]]
    inline size_t valid_n(const std::vector<double>& data) noexcept
        {
        return static_cast<size_t>(
            std::accumulate(data.cbegin(), data.cend(), 0.0,
            [](const auto initVal, const auto val) noexcept
                { return initVal + (std::isnan(val) ? 0 : 1); }));
        }

    /** @brief Calculates the mode(s) (most repeated value) from a specified range.
        @param data The data to analyze.
        @returns A set containing all modes from a specified range.\n
            In the case of a tie, multiple modes will be returned.
        @warning If analyzing floating-point data, NaN should be removed prior to calling this.*/
    template <typename T>
    [[nodiscard]]
    std::set<T> mode(const std::vector<T>& data)
        {
        std::set<T> modes;
        if (data.size() == 0)
            { return modes; }
        frequency_set<T> groups;
        // this will sort the larger items to the front
        std::multimap<size_t, T, std::greater<size_t>> groupsByCount;
        // look at the full range of the data, not just non-NaN
        for (const auto& val : data)
            { groups.insert(val); }
        // flip the groupings so that the size of each group is the first element
        for (auto pos = groups.get_data().cbegin();
            pos != groups.get_data().cend();
            ++pos)
            { groupsByCount.emplace(pos->second, pos->first); }
        // start at the most frequent group and work forwards to get any other modes
        // that have the same count
        if (groupsByCount.size())
            {
            const size_t modeGroupSize = groupsByCount.cbegin()->first;
            for (auto groupsIter = groupsByCount.cbegin(); groupsIter != groupsByCount.cend(); ++groupsIter)
                {
                if (groupsIter->first == modeGroupSize)
                    { modes.insert(groupsIter->second); }
                else
                    { break; }
                }
            }

        return modes;
        }

    /** @brief Calculates the mode(s) (most repeated value) from a specified range.
        @param data The data to analyze.
        @param transformValue Function to transform values when grouping them.\n
            For example, you can pass in a functor to round double values into integers.
        @returns A set containing all modes from a specified range.\n
            In the case of a tie, multiple modes will be returned.
        @warning If analyzing floating-point data, NaN should be removed prior to calling this.*/
    template <typename T, typename predicateT>
    [[nodiscard]]
    std::set<T> mode(const std::vector<T>& data, predicateT transformValue)
        {
        std::set<T> modes;
        if (data.size() == 0)
            { return modes; }
        frequency_set<T> groups;
        // this will sort the larger items to the front
        std::multimap<size_t, T, std::greater<size_t>> groupsByCount;
        // look at the full range of the data, not just non-NaN
        for (const auto& val : data)
            { groups.insert(transformValue(val)); }
        // flip the groupings so that the size of each group is the first element
        for (auto pos = groups.get_data().cbegin();
            pos != groups.get_data().cend();
            ++pos)
            { groupsByCount.emplace(pos->second, pos->first); }
        // start at the most frequent group and work forwards to get any other modes that have the same count
        if (groupsByCount.size())
            {
            const size_t modeGroupSize = groupsByCount.cbegin()->first;
            for (auto groupsIter = groupsByCount.cbegin();
                groupsIter != groupsByCount.cend();
                ++groupsIter)
                {
                if (groupsIter->first == modeGroupSize)
                    { modes.insert(groupsIter->second); }
                else
                    { break; }
                }
            }

        return modes;
        }

    /** @returns The means (average) value from the specified range.
        @param data The data to analyze.
        @throws std::invalid_argument If no observations are provided, throws an exception.*/
    [[nodiscard]]
    inline double mean(const std::vector<double>& data)
        {
        const auto N = valid_n(data);
        const double summation = std::accumulate(data.cbegin(), data.cend(), 0.0,
            [](const double initVal, const double val) noexcept
            { return initVal + (std::isnan(val) ? 0.0 : val); });
        if (N == 0)
            { throw std::invalid_argument("No observations in mean calculation."); }
        if (summation == 0.0f)
            { return 0.0f; }
        return safe_divide<double>(summation, static_cast<double>(N));
        }

    /** @returns The median value from the specified range (assumes data is already sorted).
        @param begin The start of the data range.
        @param end The end of the data range.
        @warning NaN values should be removed from the input prior to calling this.
        @throws std::invalid_argument If no observations are provided, throws an exception.*/
    [[nodiscard]]
    inline double median_presorted(const std::vector<double>::const_iterator& begin,
                                                 const std::vector<double>::const_iterator& end)
        {
        // since we are looking at specific positions in the data,
        // we have to look at the whole range of the data, not just
        // the non-NaN values
        const size_t sizeN = std::distance(begin, end);
        if (sizeN == 1)
            { return *begin; }
        if (sizeN == 0)
            { throw std::invalid_argument("No observations in median calculation."); }
        const size_t lowerMidPoint = (sizeN/2)-1; // subtract 1 because of 0-based indexing
        if (is_even(sizeN))
            {
            return (*(begin + lowerMidPoint) + *(begin + lowerMidPoint + 1))
                / static_cast<double>(2);
            }
        else
            { return *(begin + lowerMidPoint + 1); }
        }

    /** @returns The median value from the specified range (assumes data is already sorted).
        @param data The data to analyze.
        @warning NaN values should be removed from the input prior to calling this.*/
    [[nodiscard]]
    inline double median_presorted(const std::vector<double>& data)
        { return median_presorted(data.cbegin(), data.cend()); }

    /** @returns The median value from the specified range.
        @param data The data to analyze.*/
    [[nodiscard]]
    inline double median(const std::vector<double>& data)
        {
        std::vector<double> dest;
        dest.reserve(data.size());
        // don't copy NaN into buffer
        std::copy_if(data.cbegin(), data.cend(),
            std::back_inserter(dest),
            [](const auto val) noexcept
              { return !std::isnan(val); });
        std::sort(std::execution::par, dest.begin(), dest.end());
        return median_presorted(dest);
        }

    /** @returns The sum of squares/cubes/etc. from the specified range.
        @param data The data to analyze.
        @param power The exponent value (e.g., 2 will give you the sum of squares).*/
    [[nodiscard]]
    inline double sum_of_powers(const std::vector<double>& data, const double power)
        {
        const double mean_val = mean(data);

        return std::accumulate(data.cbegin(), data.cend(), 0.0,
            [mean_val, power](const double lhs, const double rhs)
                {
                return lhs +
                    // ignore NaN
                    (std::isnan(rhs) ? 0.0 :
                     std::pow(static_cast<double>(rhs - mean_val), power));
                }
            );
        }

    /** @returns The variance from the specified range.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @throws std::invalid_argument If less than two observations are provided, throws an exception.*/
    [[nodiscard]]
    inline double variance(const std::vector<double>& data, const bool is_sample)
        {
        // sum of squares/N-1
        const double sos = sum_of_powers(data, 2);
        const size_t N = valid_n(data);
        if (N < 2)
            { throw std::invalid_argument("Not enough observations to calculate variance."); }
        if (sos == 0.0f)
            { return 0.0f; }
        return safe_divide<double>(sos, static_cast<double>(is_sample ? (N-1) : N));
        }

    /** @returns The standard deviation from the specified range.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @throws std::invalid_argument If less than two observations are provided, throws an exception.*/
    [[nodiscard]]
    inline double standard_deviation(const std::vector<double>& data, const bool is_sample)
        {
        if (data.size() < 2)
            { throw std::invalid_argument("Not enough observations to calculate std. dev."); }
        // square root of variance
        return std::sqrt(variance(data, is_sample) );
        }

    /** @returns A value, converted to a z-score.
        @param value The value to convert.
        @param mean The sample mean.
        @param stdDev The sample standard deviation.
        @todo Need unit tests.*/
    [[nodiscard]] inline double z_score(const double value, const double mean, const double stdDev) noexcept
        {
        return safe_divide<double>(value - mean, stdDev);
        }

    /** @returns The standard error of the mean from the specified range.
            The standard deviation of all sample mean estimates of a population mean.
            For example, if multiple samples of size N are taken from a population,
            the means will more than likely vary between samplings.
            The standard error will measure the standard deviation of these sample means.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @throws std::invalid_argument If less than two observations are provided, throws an exception.*/
    [[nodiscard]]
    inline double standard_error_of_mean(const std::vector<double>& data,
                                                       const bool is_sample)
        {
        const auto N = valid_n(data);
        if (N < 2)
            { throw std::invalid_argument("Not enough observations to calculate SEM."); }
        return safe_divide<double>(standard_deviation(data, is_sample), std::sqrt(static_cast<double>(N)));
        }

    /** @brief Gets the skewness from the specified range.
        @details Skewness measures the asymmetry of the probability distribution.
            A zero skew indicates a symmetrical balance in the distribution.
            A negative skew indicates that the left side of the distribution is longer and most
            of the values are concentrated on the right.
            A positive skew indicates that the right side of the distribution is longer and most
            of the values are concentrated on the left.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @returns The skewness from the specified range.
        @throws std::invalid_argument If less than three observations are provided, throws an exception.*/
    [[nodiscard]]
    inline double skewness(const std::vector<double>& data, const bool is_sample)
        {
        const auto N = valid_n(data);
        if (N < 3)
            { throw std::invalid_argument("Not enough observations to calculate Skewness."); }

        return safe_divide<double>(N*sum_of_powers(data, 3),
                                    (N-1)*(N-2)*std::pow(standard_deviation(data, is_sample),3));
        }

    /** @brief Gets the Kurtosis from the specified range.
        @details Kurtosis measures the peakedness of a distribution. Zero indicates a normal distribution,
         a positive value represents a sharp curve, and a negative value represents a flat distribution.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @returns The Kurtosis from the specified range.
        @throws std::invalid_argument If less than four observations are provided, throws an exception.*/
    [[nodiscard]]
    inline double kurtosis(const std::vector<double>& data, const bool is_sample)
        {
        const auto N = valid_n(data);
        if (N < 4)
            { throw std::invalid_argument("Not enough observations to calculate Kurtosis."); }

        return safe_divide<double>(N*(N+1) * sum_of_powers(data, 4) - 3*sum_of_powers(data, 2) * sum_of_powers(data, 2) * (N-1),
                                   (N-1)*(N-2)*(N-3)*std::pow(standard_deviation(data, is_sample),4));
        }

    /** @brief Calculates the 25th and 75th percentiles from the specified range using the
            Tukey hinges method. Median is taken from lower and upper halves if N is even.
            If N is odd, then overall median is included in both the lower and upper half and
            median is taken from those halves. This is that method that R appears to do.
        @param data The data to analyze.
        @param[out] lower_quartile_value The calculated lower quartile.
        @param[out] upper_quartile_value The calculated upper quartile.
        @note Data must be sorted beforehand.
        @throws std::invalid_argument If no observations are provided, throws an exception.*/
    inline void quartiles_presorted(const std::vector<double>& data,
                                    double& lower_quartile_value,
                                    double& upper_quartile_value)
        {
        const size_t N = data.size();
        if (N == 0)
            { throw std::invalid_argument("No observations in quartiles calculation."); }

        const auto middlePosition = static_cast<size_t>(std::ceil(
            safe_divide<double>(static_cast<double>(N), 2.0)));
        // make sure we are splitting data into even halves
        assert(std::distance(data.cbegin(), data.cbegin()+middlePosition) ==
               std::distance(data.cbegin()+middlePosition-(is_even(N) ? 0 : 1), data.cend()));
        // lower half (will include the median point if N is odd)
        lower_quartile_value = median_presorted(data.cbegin(), data.cbegin()+middlePosition);
        // upper half (will step back to include median point if N is odd)
        upper_quartile_value = median_presorted(data.cbegin()+middlePosition-(is_even(N) ? 0 : 1), data.cend());
        }

    /** @brief Calculates the outlier and extreme ranges for a given range.
        @param LBV The lower boundary.
        @param UBV The upper boundary.
        @param[out] lower_outlier_boundary The lower outlier boundary.
        @param[out] upper_outlier_boundary The upper outlier boundary.
        @param[out] lower_extreme_boundary The lower extreme boundary.
        @param[out] upper_extreme_boundary The upper extreme boundary.*/
    template<typename T>
    inline void outlier_extreme_ranges(
        double LBV, double UBV,
        double& lower_outlier_boundary,
        double& upper_outlier_boundary,
        double& lower_extreme_boundary,
        double& upper_extreme_boundary) noexcept
        {
        constexpr double OUTLIER_COEFFICIENT = 1.5;
        lower_outlier_boundary = LBV - OUTLIER_COEFFICIENT*(UBV - LBV);
        upper_outlier_boundary = UBV + OUTLIER_COEFFICIENT*(UBV - LBV);
        lower_extreme_boundary = LBV - 2*OUTLIER_COEFFICIENT*(UBV - LBV);
        upper_extreme_boundary = UBV + 2*OUTLIER_COEFFICIENT*(UBV - LBV);
        }

    /** @brief Accepts a range of data and iteratively returns the outliers.
        @details You can get the outlier and extreme ranges from the data, as
         well as read the outlier values one-by-one.
        @code
         // analyze a data series and retrieve its outliers
         std::vector<double> values = { 5, 9, -3, 6, 7, 6, 6, 4, 3, 17 }
         // load the data
         statistics::find_outliers
            findOutlier(values));
         std::vector<int> theOutliers;
         // iterate through the outliers by calling operator().
         for (;;)
            {
            auto nextOutlier = findOutlier();
            if (nextOutlier == values.end())
                { break; }
            theOutliers.push_back(*nextOutlier);
            }
         // theOutliers will now be filled with -3 and 17
        @endcode
       */
    class find_outliers
        {
    public:
        /** @brief Constructor that accepts data and analyzes it.
            @param data The data to analyze.*/
        explicit find_outliers(const std::vector<double>& data)
            : m_current_position(data.cbegin()), m_end(data.cend())
            { set_data(data); }
        /** @brief Sets the data and analyzes it.
            @param data The data to analyze.*/
        void set_data(const std::vector<double>& data)
            {
            double lq(0), uq(0);
            m_current_position = data.cbegin();
            m_end = data.cend();
            m_temp_buffer.clear();
            m_temp_buffer.reserve(data.size());
            // don't copy NaN into buffer
            std::copy_if(data.cbegin(), data.cend(),
                std::back_inserter(m_temp_buffer),
                [](const auto val) noexcept
                  { return !std::isnan(val); });
            std::sort(std::execution::par, m_temp_buffer.begin(), m_temp_buffer.end() );
            // calculate the quartile ranges
            statistics::quartiles_presorted(
                m_temp_buffer,
                lq, uq);
            // calculate the outliers and extremes
            statistics::outlier_extreme_ranges<double>(
                lq, uq,
                lo, uo, le, ue);
            }
        /// @returns A pointer/iterator to the next outlier,
        ///     or end of the container if no more outliers.
        [[nodiscard]] std::vector<double>::const_iterator operator()() noexcept
            {
            m_current_position = std::find_if(
                std::execution::par,
                m_current_position, m_end,
                [this](const auto& val) noexcept
                    { return !is_within<double>(std::make_pair(lo, uo), val); }
                );
            return (m_end == m_current_position) ? m_end : m_current_position++;
            }
        /// @returns The lower outlier boundary.
        [[nodiscard]] double get_lower_outlier_boundary() const noexcept
            { return lo; }
        /// @returns The upper outlier boundary.
        [[nodiscard]] double get_upper_outlier_boundary() const noexcept
            { return uo; }
        /// @returns The lower extreme boundary.
        [[nodiscard]] double get_lower_extreme_boundary() const noexcept
            { return le; }
        /// @returns The upper extreme boundary.
        [[nodiscard]] double get_upper_extreme_boundary() const noexcept
            { return ue; }
    private:
        std::vector<double>::const_iterator m_current_position;
        std::vector<double>::const_iterator m_end;
        std::vector<double> m_temp_buffer;
        double lo{ 0 };
        double uo{ 0 };
        double le{ 0 };
        double ue{ 0 };
        };

    /** @returns The normalized (i.e., within the 0-1 range) value for a number compared
            to the specified range.
        @param range_min The start of the range to normalize the value to.
        @param range_max The end of the range to normalize the value to.
        @param value The value to normalize.
        @note If the provided range is zero and the value is at that same value, then
            zero will be returned (the high and low are the same here, so zero is used).
        @throws std::invalid_argument If max is higher than min, throws an exception.*/
    template<typename T>
    [[nodiscard]] inline double normalize(const T range_min, const T range_max, T value)
        {
        if (std::is_floating_point_v<T> &&
            (std::isnan(range_min) || std::isnan(range_max) || std::isnan(value)))
            { return value; }
        NON_UNIT_TEST_ASSERT(range_max >= range_min);
        NON_UNIT_TEST_ASSERT(is_within(value,range_min,range_max));
        if (range_max < range_min || !is_within(value,range_min,range_max))
            { throw std::invalid_argument("Invalid value or range used in call to normalize."); }
        const double range = range_max-range_min;
        return safe_divide<double>(value-range_min, range);
        }

    /** @param begin1 The start of the first range.
        @param end1 The end of the first range.
        @param begin2 The start of the second range.
        @param end2 The end of the second range.
        @returns The phi coefficient.
        @todo needs to be validated and unit tested*/
    template <typename T>
    [[nodiscard]] inline double phi_coefficient(const T begin1, const T end1,
                                  const T begin2, const T end2)
        {
        NON_UNIT_TEST_ASSERT((end1-begin1) == (end2-begin2) && "Arrays passed to phi_coefficient must be the same size!");
        if ((end1-begin1) != (end2-begin2))
            { throw std::range_error("Arrays passed to phi_coefficient must be the same size!"); }
        long long n11(0), n10(0), n01(0), n00(0);
        for (ptrdiff_t i = 0; i < end1-begin1; ++i)
            {
            (begin1[i] > 0 && begin2[i] > 0) ? ++n11 :
                (begin1[i] > 0 && begin2[i] == 0) ? ++n10 :
                (begin1[i] == 0 && begin2[i] > 0) ? ++n01 :
                (begin1[i] == 0 && begin2[i] == 0) ? ++n00 : 0;
            }
        const long long n_dot_1 = n11+n01;
        const long long n_dot_0 = n10+n00;
        const long long n1_dot = n11+n10;
        const long long n0_dot = n01+n00;
        [[maybe_unused]] const long long n = n1_dot+n0_dot;
        const double pc = safe_divide<double>((n11*n00)-(n10*n01), std::sqrt(n1_dot*n0_dot*n_dot_0*n_dot_1) );
        assert(is_within<double>(pc,-1,1) && "Error in phi coefficient calculation. Value should be -1 >= and <= 1.");
        return pc;
        }
    }

/** @}*/

#endif //__STATISTICS_H__
