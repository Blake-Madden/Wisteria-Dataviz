/** @addtogroup Utilities
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_STATISTICS_H
#define WISTERIA_STATISTICS_H

#include "../debug/debug_assert.h"
#include "../util/frequencymap.h"
#include "mathematics.h"
#include "safe_math.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

/// @brief Namespace for statistics classes.
namespace statistics
    {
    /** @returns The valid (non-NaN) number of observations from the specified range.
        @param data The data to analyze.*/
    [[nodiscard]]
    inline size_t valid_n(const std::vector<double>& data) noexcept
        {
        return std::count_if(data.cbegin(), data.cend(),
                             [](double val) noexcept { return std::isfinite(val); });
        }

    /** @brief Calculates the mode(s) (most repeated value) from a specified range.
        @param data The data to analyze.
        @returns A set containing all modes from a specified range.\n
            In the case of a tie, multiple modes will be returned.
        @warning If analyzing floating-point data, NaN should be removed prior to calling this.*/
    template<typename T>
    [[nodiscard]]
    std::set<T> mode(const std::vector<T>& data)
        {
        std::set<T> modes;
        if (data.empty())
            {
            return modes;
            }
        frequency_set<T> groups;
        // this will sort the larger items to the front
        std::multimap<size_t, T, std::greater<size_t>> groupsByCount;
        // look at the full range of the data, not just non-NaN
        for (const auto& val : data)
            {
            groups.insert(val);
            }
        // flip the groupings so that the size of each group is the first element
        for (auto pos = groups.get_data().cbegin(); pos != groups.get_data().cend(); ++pos)
            {
            groupsByCount.emplace(pos->second, pos->first);
            }
        // start at the most frequent group and work forwards to get any other modes
        // that have the same count
        if (groupsByCount.size())
            {
            const size_t modeGroupSize = groupsByCount.cbegin()->first;
            for (auto groupsIter = groupsByCount.cbegin(); groupsIter != groupsByCount.cend();
                 ++groupsIter)
                {
                if (groupsIter->first == modeGroupSize)
                    {
                    modes.insert(groupsIter->second);
                    }
                else
                    {
                    break;
                    }
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
    template<typename T, typename predicateT>
    [[nodiscard]]
    std::set<T> mode(const std::vector<T>& data, predicateT transformValue)
        {
        std::set<T> modes;
        if (data.empty())
            {
            return modes;
            }
        frequency_set<T> groups;
        // this will sort the larger items to the front
        std::multimap<size_t, T, std::greater<size_t>> groupsByCount;
        // look at the full range of the data, not just non-NaN
        for (const auto& val : data)
            {
            groups.insert(transformValue(val));
            }
        // flip the groupings so that the size of each group is the first element
        for (auto pos = groups.get_data().cbegin(); pos != groups.get_data().cend(); ++pos)
            {
            groupsByCount.emplace(pos->second, pos->first);
            }
        // start at the most frequent group and work forwards to get any other modes
        // that have the same count
        if (groupsByCount.size())
            {
            const size_t modeGroupSize = groupsByCount.cbegin()->first;
            for (auto groupsIter = groupsByCount.cbegin(); groupsIter != groupsByCount.cend();
                 ++groupsIter)
                {
                if (groupsIter->first == modeGroupSize)
                    {
                    modes.insert(groupsIter->second);
                    }
                else
                    {
                    break;
                    }
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
        const double summation = std::accumulate(
            data.cbegin(), data.cend(), 0.0, [](const double initVal, const double val) noexcept
            { return initVal + (!std::isfinite(val) ? 0.0 : val); });
        if (N == 0)
            {
            throw std::invalid_argument("No observations in mean calculation.");
            }
        if (summation == 0.0)
            {
            return 0.0;
            }
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
            {
            return *begin;
            }
        if (sizeN == 0)
            {
            throw std::invalid_argument("No observations in median calculation.");
            }
        const size_t lowerMidPoint = (sizeN / 2) - 1; // subtract 1 because of 0-based indexing
        if (is_even(sizeN))
            {
            return (*(begin + lowerMidPoint) + *(begin + lowerMidPoint + 1)) /
                   static_cast<double>(2);
            }
        return *(begin + lowerMidPoint + 1);
        }

    /** @returns The median value from the specified range (assumes data is already sorted).
        @param data The data to analyze.
        @warning NaN values should be removed from the input prior to calling this.*/
    [[nodiscard]]
    inline double median_presorted(const std::vector<double>& data)
        {
        return median_presorted(data.cbegin(), data.cend());
        }

    /** @returns The median value from the specified range.
        @param data The data to analyze.*/
    [[nodiscard]]
    inline double median(const std::vector<double>& data)
        {
        std::vector<double> dest;
        dest.reserve(data.size());
        // don't copy NaN into buffer
        std::ranges::copy_if(data, std::back_inserter(dest),
                             [](const auto val) noexcept { return std::isfinite(val); });
        std::ranges::sort(dest);
        return median_presorted(dest);
        }

    /** @returns The sum of squares/cubes/etc. from the specified range.
        @param data The data to analyze.
        @param power The exponent value (e.g., 2 will give you the sum of squares).*/
    [[nodiscard]]
    inline double sum_of_powers(const std::vector<double>& data, const double power)
        {
        const double mean_val = mean(data);

        return std::accumulate(
            data.cbegin(), data.cend(), 0.0,
            [mean_val, power](const double lhs, const double rhs)
            {
                return lhs +
                       // ignore NaN
                       (!std::isfinite(rhs) ? 0.0 :
                                              std::pow(static_cast<double>(rhs - mean_val), power));
            });
        }

    /** @returns The variance from the specified range.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @throws std::invalid_argument If less than two observations are provided,
            throws an exception.*/
    [[nodiscard]]
    inline double variance(const std::vector<double>& data, const bool is_sample)
        {
        // sum of squares/N-1
        const double sos = sum_of_powers(data, 2);
        const size_t N = valid_n(data);
        if (N < 2)
            {
            throw std::invalid_argument("Not enough observations to calculate variance.");
            }
        if (sos == 0.0)
            {
            return 0.0;
            }
        return safe_divide<double>(sos, static_cast<double>(is_sample ? (N - 1) : N));
        }

    /** @returns The standard deviation from the specified range.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @throws std::invalid_argument If less than two observations are provided,
            throws an exception.*/
    [[nodiscard]]
    inline double standard_deviation(const std::vector<double>& data, const bool is_sample)
        {
        if (data.size() < 2)
            {
            throw std::invalid_argument("Not enough observations to calculate std. dev.");
            }
        // square root of variance
        return std::sqrt(variance(data, is_sample));
        }

    /** @returns A value, converted to a z-score.
        @param value The value to convert.
        @param mean The sample mean.
        @param stdDev The sample standard deviation.*/
    [[nodiscard]]
    inline double z_score(const double value, const double mean, const double stdDev) noexcept
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
        @throws std::invalid_argument If less than two observations are provided,
            throws an exception.*/
    [[nodiscard]]
    inline double standard_error_of_mean(const std::vector<double>& data, const bool is_sample)
        {
        const auto N = valid_n(data);
        if (N < 2)
            {
            throw std::invalid_argument("Not enough observations to calculate SEM.");
            }
        return safe_divide<double>(standard_deviation(data, is_sample),
                                   std::sqrt(static_cast<double>(N)));
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
        @throws std::invalid_argument If less than three observations are provided,
            throws an exception.*/
    [[nodiscard]]
    inline double skewness(const std::vector<double>& data, const bool is_sample)
        {
        const auto N = valid_n(data);
        if (N < 3)
            {
            throw std::invalid_argument("Not enough observations to calculate Skewness.");
            }

        return safe_divide<double>(N * sum_of_powers(data, 3),
                                   (N - 1) * (N - 2) *
                                       std::pow(standard_deviation(data, is_sample), 3));
        }

    /** @brief Gets the Kurtosis from the specified range.
        @details Kurtosis measures the peakedness of a distribution. Zero indicates a normal
            distribution, a positive value represents a sharp curve, and a negative value
            represents a flat distribution.
        @param data The data to analyze.
        @param is_sample Set to @c true to use sample variance (i.e., N-1).
        @returns The Kurtosis from the specified range.
        @throws std::invalid_argument If less than four observations are provided,
            throws an exception.*/
    [[nodiscard]]
    inline double kurtosis(const std::vector<double>& data, const bool is_sample)
        {
        const auto N = valid_n(data);
        if (N < 4)
            {
            throw std::invalid_argument("Not enough observations to calculate Kurtosis.");
            }

        return safe_divide<double>(
            N * (N + 1) * sum_of_powers(data, 4) -
                3 * sum_of_powers(data, 2) * sum_of_powers(data, 2) * (N - 1),
            (N - 1) * (N - 2) * (N - 3) * std::pow(standard_deviation(data, is_sample), 4));
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
    inline void quartiles_presorted(const std::vector<double>& data, double& lower_quartile_value,
                                    double& upper_quartile_value)
        {
        const size_t N = data.size();
        if (N == 0)
            {
            throw std::invalid_argument("No observations in quartiles calculation.");
            }

        const auto middlePosition =
            static_cast<size_t>(std::ceil(safe_divide<double>(static_cast<double>(N), 2.0)));
        // make sure we are splitting data into even halves
        assert(std::distance(data.cbegin(), data.cbegin() + middlePosition) ==
               std::distance(data.cbegin() + middlePosition - (is_even(N) ? 0 : 1), data.cend()));
        // lower half (will include the median point if N is odd)
        lower_quartile_value = median_presorted(data.cbegin(), data.cbegin() + middlePosition);
        // upper half (will step back to include median point if N is odd)
        upper_quartile_value =
            median_presorted(data.cbegin() + middlePosition - (is_even(N) ? 0 : 1), data.cend());
        }

    /** @brief Calculates the outlier and extreme ranges for a given range.
        @param LBV The lower boundary.
        @param UBV The upper boundary.
        @param[out] lower_outlier_boundary The lower outlier boundary.
        @param[out] upper_outlier_boundary The upper outlier boundary.
        @param[out] lower_extreme_boundary The lower extreme boundary.
        @param[out] upper_extreme_boundary The upper extreme boundary.*/
    template<typename T>
    inline void outlier_extreme_ranges(double LBV, double UBV, double& lower_outlier_boundary,
                                       double& upper_outlier_boundary,
                                       double& lower_extreme_boundary,
                                       double& upper_extreme_boundary) noexcept
        {
        constexpr double OUTLIER_COEFFICIENT = 1.5;
        lower_outlier_boundary = LBV - OUTLIER_COEFFICIENT * (UBV - LBV);
        upper_outlier_boundary = UBV + OUTLIER_COEFFICIENT * (UBV - LBV);
        lower_extreme_boundary = LBV - 2 * OUTLIER_COEFFICIENT * (UBV - LBV);
        upper_extreme_boundary = UBV + 2 * OUTLIER_COEFFICIENT * (UBV - LBV);
        }

    /** @brief Accepts a range of data and iteratively returns the outliers.
        @details You can get the outlier and extreme ranges from the data, as
         well as read the outlier values one-by-one.
        @code
         // analyze a data series and retrieve its outliers
         std::vector<double> values = { 5, 9, -3, 6, 7, 6, 6, 4, 3, 17 };
         // load the data
         statistics::find_outliers
            findOutlier(values);
         std::vector<int> theOutliers;
         // iterate through the outliers by calling operator().
         for (;;)
            {
            auto nextOutlier = findOutlier();
            if (nextOutlier == values.cend())
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
            {
            set_data(data);
            }

        /** @brief Sets the data and analyzes it.
            @param data The data to analyze.*/
        void set_data(const std::vector<double>& data)
            {
            // reset
            double lq{ std::numeric_limits<double>::quiet_NaN() },
                uq{ std::numeric_limits<double>::quiet_NaN() };
            lo = uo = le = ue = std::numeric_limits<double>::quiet_NaN();
            m_current_position = data.cbegin();
            m_end = data.cend();
            m_temp_buffer.clear();
            m_temp_buffer.reserve(data.size());
            // don't copy NaN into buffer
            std::ranges::copy_if(data, std::back_inserter(m_temp_buffer),
                                 [](const auto val) noexcept { return std::isfinite(val); });
            // if no valid data, then leave boundaries as NaN and bail
            if (m_temp_buffer.empty())
                {
                return;
                }
            std::ranges::sort(m_temp_buffer);
            // calculate the quartile ranges
            statistics::quartiles_presorted(m_temp_buffer, lq, uq);
            // calculate the outliers and extremes
            statistics::outlier_extreme_ranges<double>(lq, uq, lo, uo, le, ue);
            }

        /// @returns A pointer/iterator to the next outlier,
        ///     or end of the container if no more outliers.
        [[nodiscard]]
        std::vector<double>::const_iterator operator()() noexcept
            {
            m_current_position = std::find_if(
                m_current_position, m_end, [this](const auto& val) noexcept
                { return std::isfinite(val) && !is_within<double>(std::make_pair(lo, uo), val); });
            return (m_end == m_current_position) ? m_end : m_current_position++;
            }

        /// @returns The lower outlier boundary, or NaN if data are empty.
        [[nodiscard]]
        double get_lower_outlier_boundary() const noexcept
            {
            return lo;
            }

        /// @returns The upper outlier boundary, or NaN if data are empty.
        [[nodiscard]]
        double get_upper_outlier_boundary() const noexcept
            {
            return uo;
            }

        /// @returns The lower extreme boundary, or NaN if data are empty.
        [[nodiscard]]
        double get_lower_extreme_boundary() const noexcept
            {
            return le;
            }

        /// @returns The upper extreme boundary, or NaN if data are empty.
        [[nodiscard]]
        double get_upper_extreme_boundary() const noexcept
            {
            return ue;
            }

      private:
        std::vector<double>::const_iterator m_current_position;
        std::vector<double>::const_iterator m_end;
        std::vector<double> m_temp_buffer;
        double lo{ std::numeric_limits<double>::quiet_NaN() };
        double uo{ std::numeric_limits<double>::quiet_NaN() };
        double le{ std::numeric_limits<double>::quiet_NaN() };
        double ue{ std::numeric_limits<double>::quiet_NaN() };
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
    [[nodiscard]]
    inline double normalize(const T range_min, const T range_max, T value)
        {
        if (std::is_floating_point_v<T> &&
            (!std::isfinite(range_min) || !std::isfinite(range_max) || !std::isfinite(value)))
            {
            return value;
            }
        NON_UNIT_TEST_ASSERT(range_max >= range_min);
        NON_UNIT_TEST_ASSERT(is_within(value, range_min, range_max));
        if (range_max < range_min || !is_within(value, range_min, range_max))
            {
            throw std::invalid_argument("Invalid value or range used in call to normalize.");
            }
        const double range = range_max - range_min;
        return safe_divide<double>(value - range_min, range);
        }

    /** @param begin1 The start of the first range.
        @param end1 The end of the first range.
        @param begin2 The start of the second range.
        @param end2 The end of the second range.
        @returns The phi coefficient.*/
    template<typename T>
    [[nodiscard]]
    inline double phi_coefficient(const T begin1, const T end1, const T begin2, const T end2)
        {
        NON_UNIT_TEST_ASSERT((end1 - begin1) == (end2 - begin2) &&
                             "Arrays passed to phi_coefficient must be the same size!");
        if ((end1 - begin1) != (end2 - begin2))
            {
            throw std::range_error("Arrays passed to phi_coefficient must be the same size!");
            }
        long long n11(0), n10(0), n01(0), n00(0);
        for (ptrdiff_t i = 0; i < end1 - begin1; ++i)
            {
            (begin1[i] > 0 && begin2[i] > 0)   ? ++n11 :
            (begin1[i] > 0 && begin2[i] == 0)  ? ++n10 :
            (begin1[i] == 0 && begin2[i] > 0)  ? ++n01 :
            (begin1[i] == 0 && begin2[i] == 0) ? ++n00 :
                                                 0;
            }
        const long long n_dot_1 = n11 + n01;
        const long long n_dot_0 = n10 + n00;
        const long long n1_dot = n11 + n10;
        const long long n0_dot = n01 + n00;
        [[maybe_unused]]
        const long long n = n1_dot + n0_dot;
        const auto pc = safe_divide<double>((n11 * n00) - (n10 * n01),
                                            std::sqrt(n1_dot * n0_dot * n_dot_0 * n_dot_1));
        assert(is_within<double>(pc, -1, 1) &&
               "Error in phi coefficient calculation. Value should be -1 >= and <= 1.");
        return pc;
        }

    /** @brief Results from a simple linear regression analysis.
        @details Contains slope, intercept, R-squared, correlation coefficient,
            standard errors, t-statistic, and p-value for the slope.
        @par Citations:
            Kutner, Michael H., et al. *Applied Linear Statistical Models*. 5th ed.,
            McGraw-Hill/Irwin, 2004.*/
    struct linear_regression_results
        {
        /// @brief slope of the regression line (beta_1).
        double slope{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief y-intercept of the regression line (beta_0).
        double intercept{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief coefficient of determination (proportion of variance explained).
        double r_squared{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief Pearson correlation coefficient.
        double correlation{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief standard error of the estimate (residual standard error).
        double standard_error{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief standard error of the slope coefficient.
        double slope_standard_error{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief t-statistic for testing H0: slope = 0.
        double t_statistic{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief two-tailed p-value for the slope coefficient.
        double p_value{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief number of valid observation pairs used.
        size_t n{ 0 };
        /// @brief mean of x values (used for confidence interval calculations).
        double mean_x{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief sum of squared deviations of x (used for confidence interval calculations).
        double ss_xx{ std::numeric_limits<double>::quiet_NaN() };

        /// @returns @c true if the regression was successfully computed.
        [[nodiscard]]
        bool is_valid() const noexcept
            {
            return n >= 2 && std::isfinite(slope) && std::isfinite(intercept);
            }
        };

    /** @brief Computes the regularized incomplete beta function I_x(a, b).
        @param x The evaluation point (0 <= x <= 1).
        @param shape1 First shape parameter.
        @param shape2 Second shape parameter.
        @returns The regularized value (normalized to the range [0, 1]).\n
            If @c shape1 or @c b are negative, returns NaN.
        @note @c shape1 and @c b should be non-negative.
        @par Citations:
            Press, William H., et al. *Numerical Recipes: The Art of Scientific Computing*. 3rd ed.,
            Cambridge University Press, 2007, sec. 6.4.*/
    [[nodiscard]]
    inline double regularized_incomplete_beta(double x, double shape1, double shape2)
        {
        if (!std::isfinite(x) || !std::isfinite(shape1) || !std::isfinite(shape2) || shape1 < 0 ||
            shape2 < 0)
            {
            return std::numeric_limits<double>::quiet_NaN();
            }
        if (x <= 0.0)
            {
            return 0.0;
            }
        if (x >= 1.0)
            {
            return 1.0;
            }

        // use symmetry relation for numerical stability when x > (a+1)/(a+b+2)
        const bool symmetry = x > safe_divide<double>(shape1 + 1.0, shape1 + shape2 + 2.0);
        if (symmetry)
            {
            x = 1.0 - x;
            std::swap(shape1, shape2);
            }

        // compute log of the beta function coefficient
        // ln(x^a * (1-x)^b / B(a,b)) where B(a,b) = Gamma(a)*Gamma(b)/Gamma(a+b)
        const double logBetaCoeff = shape1 * std::log(x) + shape2 * std::log(1.0 - x) +
                                    std::lgamma(shape1 + shape2) - std::lgamma(shape1) -
                                    std::lgamma(shape2);
        const double betaCoeff = std::exp(logBetaCoeff);

        // Lentz's continued fraction algorithm
        // (Lentz, William J.
        //  "Generating Generalized Continued Fractions of Radicals by Newton's Iterative Method."
        //  Applied Optics, vol. 15, no. 3, 1976, pp. 668-71.)
        constexpr double tiny{ 1e-30 };
        constexpr double eps{ 1e-14 };
        constexpr size_t maxIter{ 200 };

        double fractionStepNumerator{ 1.0 };
        double fractionStepDenominator =
            1.0 - safe_divide<double>((shape1 + shape2) * x, shape1 + 1.0);
        if (std::abs(fractionStepDenominator) < tiny)
            {
            fractionStepDenominator = tiny;
            }
        fractionStepDenominator = safe_divide<double>(1.0, fractionStepDenominator);
        double result{ fractionStepDenominator };

        for (size_t m = 1; m <= maxIter; ++m)
            {
            // even step: d_{2m}
            const double mDouble = static_cast<double>(m);
            double numerator =
                safe_divide<double>(mDouble * (shape2 - mDouble) * x,
                                    (shape1 + 2.0 * mDouble - 1.0) * (shape1 + 2.0 * mDouble));

            fractionStepDenominator = 1.0 + numerator * fractionStepDenominator;
            if (std::abs(fractionStepDenominator) < tiny)
                {
                fractionStepDenominator = tiny;
                }
            fractionStepNumerator = 1.0 + safe_divide<double>(numerator, fractionStepNumerator);
            if (std::abs(fractionStepNumerator) < tiny)
                {
                fractionStepNumerator = tiny;
                }
            fractionStepDenominator = safe_divide<double>(1.0, fractionStepDenominator);
            result *= fractionStepDenominator * fractionStepNumerator;

            // odd step: d_{2m+1}
            numerator =
                safe_divide<double>(-(shape1 + mDouble) * (shape1 + shape2 + mDouble) * x,
                                    (shape1 + 2.0 * mDouble) * (shape1 + 2.0 * mDouble + 1.0));

            fractionStepDenominator = 1.0 + numerator * fractionStepDenominator;
            if (std::abs(fractionStepDenominator) < tiny)
                {
                fractionStepDenominator = tiny;
                }
            fractionStepNumerator = 1.0 + safe_divide<double>(numerator, fractionStepNumerator);
            if (std::abs(fractionStepNumerator) < tiny)
                {
                fractionStepNumerator = tiny;
                }
            fractionStepDenominator = safe_divide<double>(1.0, fractionStepDenominator);
            const double delta = fractionStepDenominator * fractionStepNumerator;
            result *= delta;

            if (std::abs(delta - 1.0) < eps)
                {
                break;
                }
            }

        const double value = safe_divide<double>(betaCoeff * result, shape1);
        return symmetry ? 1.0 - value : value;
        }

    /** @brief Calculates the two-tailed p-value from a t-distribution.
        @details Uses the relationship between the t-distribution CDF and the
            regularized incomplete beta function:
            P(T <= t) = 1 - 0.5 * I_{df/(df+t^2)}(df/2, 0.5) for t > 0.
        @param t The t-statistic.
        @param df The degrees of freedom (must be positive).
        @returns The two-tailed p-value, or NaN if df <= 0.
        @par Citations:
            Abramowitz, Milton, and Irene A. Stegun.
            *Handbook of Mathematical Functions with Formulas, Graphs, and Mathematical Tables*.
            9th printing, Dover Publications, 1965.*/
    [[nodiscard]]
    inline double t_distribution_p_value(double t, double df)
        {
        if (df <= 0.0 || !std::isfinite(t) || !std::isfinite(df))
            {
            return std::numeric_limits<double>::quiet_NaN();
            }

        // compute one-tailed probability using incomplete beta function
        const double x = safe_divide<double>(df, df + (t * t));
        const double oneTailed = 0.5 * regularized_incomplete_beta(x, df * 0.5, 0.5);

        // return two-tailed p-value
        return 2.0 * oneTailed;
        }

    /** @brief Calculates the quantile (inverse CDF) for a t-distribution.
        @details Uses Newton-Raphson iteration to find the t-value corresponding
            to a given probability. This is useful for computing critical values
            for confidence intervals.
        @param p The probability (0 < p < 1), typically 0.975 for a 95% two-tailed CI.
        @param df The degrees of freedom (must be positive).
        @returns The t-value such that P(T <= t) = p, or NaN if inputs are invalid.
        @par Citations:
            Abramowitz, Milton, and Irene A. Stegun.
            *Handbook of Mathematical Functions with Formulas, Graphs, and Mathematical Tables*.
            9th printing, Dover Publications, 1965.*/
    [[nodiscard]]
    inline double t_distribution_quantile(double p, double df)
        {
        if (df <= 0.0 || !std::isfinite(p) || !std::isfinite(df) || p <= 0.0 || p >= 1.0)
            {
            return std::numeric_limits<double>::quiet_NaN();
            }

        // initial estimate using normal approximation for large df
        // for smaller df, use a rough approximation
        double t = 0.0;
        if (df > 30)
            {
            // use normal approximation: Φ^(-1)(p) ≈ t for large df
            // Abramowitz & Stegun 26.2.23 rational approximation
            const double q = (p > 0.5) ? (1.0 - p) : p;
            const double w = std::sqrt(-2.0 * std::log(q));
            constexpr double c0 = 2.515517;
            constexpr double c1 = 0.802853;
            constexpr double c2 = 0.010328;
            constexpr double d1 = 1.432788;
            constexpr double d2 = 0.189269;
            constexpr double d3 = 0.001308;
            t = w - safe_divide<double>(c0 + c1 * w + c2 * w * w,
                                        1.0 + d1 * w + d2 * w * w + d3 * w * w * w);
            if (p < 0.5)
                {
                t = -t;
                }
            }
        else
            {
            // start with a reasonable guess based on the sign
            t = (p > 0.5) ? 1.0 : -1.0;
            }

        // Newton-Raphson iteration
        constexpr double eps{ 1e-10 };
        constexpr size_t maxIter{ 50 };

        for (size_t iter = 0; iter < maxIter; ++iter)
            {
            // current CDF value P(T <= t)
            // t_distribution_p_value returns two-tailed: 2 * P(T > |t|)
            // for t >= 0: P(T <= t) = 1 - P(T > t) = 1 - p_two_tailed/2
            // for t < 0:  P(T <= t) = P(T > |t|) = p_two_tailed/2
            const double pTwoTailed = t_distribution_p_value(t, df);
            const double currentP = (t >= 0.0) ? (1.0 - 0.5 * pTwoTailed) : (0.5 * pTwoTailed);

            const double error = currentP - p;
            if (std::abs(error) < eps)
                {
                break;
                }

            // PDF of t-distribution for the derivative
            // f(t) = Γ((df+1)/2) / (sqrt(df*π) * Γ(df/2)) * (1 + t²/df)^(-(df+1)/2)
            const double logPdf =
                std::lgamma((df + 1.0) * 0.5) - std::lgamma(df * 0.5) -
                0.5 * std::log(df * std::numbers::pi) -
                ((df + 1.0) * 0.5) * std::log(1.0 + safe_divide<double>(t * t, df));
            const double pdf = std::exp(logPdf);

            if (pdf < 1e-300)
                {
                // avoid division by very small numbers
                break;
                }

            // Newton step
            t -= safe_divide<double>(error, pdf);
            }

        return t;
        }

    /** @brief Performs simple linear regression (ordinary least squares).
        @details Fits the model `Y = beta_0 + beta_1 * X` using the least squares method.
        @param xData The independent variable values.
        @param yData The dependent variable values.
        @returns A linear_regression_results struct containing all computed statistics.\n
            Returns a default (invalid) result if fewer than 2 valid pairs exist.
        @note Uses pair-wise deletion when NaN values are encountered.
        @par Citations:
            Kutner, Michael H., et al. *Applied Linear Statistical Models*. 5th ed.,
            McGraw-Hill/Irwin, 2004.*/
    [[nodiscard]]
    inline linear_regression_results linear_regression(const std::vector<double>& xData,
                                                       const std::vector<double>& yData)
        {
        linear_regression_results results;

        if (xData.size() != yData.size() || xData.size() < 2)
            {
            return results;
            }

        double n{ 0.0 };
        double meanX{ 0.0 }, meanY{ 0.0 };
        double ssXX{ 0.0 }, ssYY{ 0.0 }, ssXY{ 0.0 };

        // filter NaN and apply Welford's algorithm
        for (size_t i = 0; i < xData.size(); ++i)
            {
            const double x = xData[i];
            const double y = yData[i];

            if (std::isfinite(x) && std::isfinite(y))
                {
                n += 1.0;
                const double dx = x - meanX;
                const double dy = y - meanY;

                // update means
                meanX += dx / n;
                meanY += dy / n;

                // update sums of squares and cross-products
                // using the updated mean for the second factor provides better stability
                ssXX += dx * (x - meanX);
                ssYY += dy * (y - meanY);
                ssXY += dx * (y - meanY);
                }
            }

        results.n = static_cast<size_t>(n);
        results.mean_x = meanX;
        results.ss_xx = ssXX;
        if (results.n < 2 || ssXX == 0.0)
            {
            return results;
            }

        // slope and intercept
        results.slope = safe_divide<double>(ssXY, ssXX);
        results.intercept = meanY - results.slope * meanX;

        // correlation and R-Squared
        if (ssYY > 0.0)
            {
            results.correlation = safe_divide<double>(ssXY, std::sqrt(ssXX * ssYY));
            results.r_squared = results.correlation * results.correlation;
            }
        else
            {
            // variance is zero, so R-squared is undefined
            results.correlation = std::numeric_limits<double>::quiet_NaN();
            results.r_squared = std::numeric_limits<double>::quiet_NaN();
            }

        // residuals and p-value
        const double dfError{ n - 2.0 };
        // for ssRes, summing squared residuals directly is safest,
        // but requires a second pass. Using the algebraic identity below:
        double ssRes = std::max(0.0, ssYY - results.slope * ssXY);

        if (dfError > 0.0)
            {
            // calculate the basic errors
            // (use standard division to allow tiny numbers)
            const double mse = safe_divide<double>(ssRes, dfError);
            results.standard_error = std::sqrt(mse);

            // always calculate slope_standard_error if ssXX is valid
            const double sqrt_ssXX = std::sqrt(ssXX);
            if (sqrt_ssXX > 0.0)
                {
                results.slope_standard_error =
                    safe_divide<double>(results.standard_error, sqrt_ssXX);

                // handle the t-statistic and p-value
                if (results.slope_standard_error > 0.0)
                    {
                    // tiny error results in a massive t-stat and tiny p-value
                    results.t_statistic =
                        safe_divide<double>(results.slope, results.slope_standard_error);
                    results.p_value = t_distribution_p_value(results.t_statistic, dfError);
                    }
                else
                    {
                    // This is the "perfect fit" way: standard error is exactly 0.0.
                    // If the slope is not zero, the t-stat is infinite and v-value is 0.
                    if (results.slope != 0.0)
                        {
                        results.t_statistic = std::numeric_limits<double>::infinity();
                        results.p_value = 0.0;
                        }
                    else
                        {
                        // flat horizontal line (slope 0, error 0)
                        results.t_statistic = 0.0;
                        results.p_value = 1.0;
                        }
                    }
                }
            }

        return results;
        }
    } // namespace statistics

/** @}*/

#endif // WISTERIA_STATISTICS_H
