/** @addtogroup Utilities
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_MATHEMATICS_H
#define WISTERIA_MATHEMATICS_H

#include "../debug/debug_assert.h"
#include "safe_math.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>

/// @brief Math constants.
namespace math_constants
    {
    /// @brief The golden ratio.
    constexpr double golden_ratio = 1.618;

    /// @brief Empty (i.e., 0%).
    constexpr double empty = 0.0;

    /// @brief Twentieth (i.e., 5%).
    constexpr double twentieth = 0.05;

    /// @brief Tenth (i.e., 10%).
    constexpr double tenth = 0.1;

    /// @brief Fifth (i.e., 20%).
    constexpr double fifth = 0.2;

    /// @brief Quarter (i.e., 25%).
    constexpr double quarter = 0.25;

    /// @brief Fourth (i.e., 25%).
    constexpr double fourth = quarter;

    /// @brief Eighth (i.e., 12.5%).
    constexpr double eighth = quarter / static_cast<double>(2);

    /// @brief Half (i.e., 50%).
    constexpr double half = 0.5;

    /// @brief Three quarters (i.e., 75%).
    constexpr double three_quarters = 0.75;

    /// @brief Three fourths (i.e., 75%).
    constexpr double three_fourths = three_quarters;

    /// @brief Third (i.e., 33% or 1/3).
    constexpr double third = 1 / static_cast<double>(3);

    /// @brief Two thirds (i.e., 66% or 2/3).
    constexpr double two_thirds = third * 2;

    /// @brief Full (i.e., 100%).
    constexpr double full = 1.0;

    /// @brief Synonym for full (i.e., 100%).
    constexpr double whole = full;
    } // namespace math_constants

/// @returns @c true if a value is within a given range.
/// @param value The value to review.
/// @param first The start of the comparison range.
/// @param second The end of the comparison range.
template<typename T>
[[nodiscard]]
constexpr bool is_within(const T value, const T first, const T second) noexcept
    {
    assert(first <= second);
    return (value >= first && value <= second);
    }

/// @returns @c true if @c value is within a given the @c range.
/// @param range The comparison range.
/// @param value The value to review.
template<typename T>
[[nodiscard]]
constexpr bool is_within(const std::pair<T, T> range, const T value) noexcept
    {
    assert(range.first <= range.second);
    return (value >= range.first && value <= range.second);
    }

/// @brief Determines if a value is within a given range.
template<typename T>
class within
    {
  public:
    /** @brief Constructor.
        @param rangeBegin The beginning of the valid range.
        @param rangeEnd The end of the valid range.*/
    within(T rangeBegin, T rangeEnd) noexcept : m_range_begin(rangeBegin), m_range_end(rangeEnd) {}

    within() = delete;

    /** @returns @c true if @c value is within the valid range of values.
        @param value The value to review.*/
    [[nodiscard]]
    inline bool operator()(T value) const noexcept
        {
        return is_within(std::make_pair(m_range_begin, m_range_end), value);
        }

  private:
    T m_range_begin;
    T m_range_end;
    };

/// @brief Pair that compares on the first item.
template<typename T1, typename T2>
class comparable_first_pair
    {
  public:
    /// @brief Default constructor.
    comparable_first_pair() = default;

    /** @brief Constructor that takes to separate values.
        @param t1 The first of the pair.
        @param t2 The second of the pair.*/
    comparable_first_pair(const T1& t1, const T2& t2) noexcept : first(t1), second(t2) {}

    /** @brief Constructor that assigns a standard pair to this one.
        @param that The standard pair to assign from.*/
    comparable_first_pair(const comparable_first_pair<T1, T2>& that)
        : first(that.first), second(that.second)
        {
        }

    /** @brief Assigns a pair to this one.
        @param that The standard pair to assign from.*/
    void operator=(const comparable_first_pair<T1, T2>& that)
        {
        first = that.first;
        second = that.second;
        }

    /** @returns @c true if this is less than another pair.
        @param that The other pair to compare against.
        @note The first items of the pairs are what are compared.*/
    [[nodiscard]]
    bool operator<(const comparable_first_pair<T1, T2>& that) const noexcept
        {
        return first < that.first;
        }

    /** @returns @c true if this is equal to another pair.
        @param that The other pair to compare against.
        @note The first items of the pairs are what are compared. The second items can be different.
            This is a key difference from `std::pair`.*/
    [[nodiscard]]
    bool operator==(const comparable_first_pair<T1, T2>& that) const noexcept
        {
        return first == that.first;
        }

    /// @brief The first item in the pair.
    T1 first;
    /// @brief The second item in the pair.
    T2 second;
    };

/// @returns The first string with a value from a list of strings.
/// @param list The list of strings.
template<typename T>
[[nodiscard]]
constexpr std::basic_string<T> coalesce(std::initializer_list<std::basic_string_view<T>> list)
    {
    for (const auto& item : list)
        {
        if (item.length())
            {
            return std::basic_string<T>(item);
            }
        }
    return std::basic_string<T>{};
    }

/// @returns The first item with a size (length) from a list.
/// @param list The list of items.
template<typename T>
[[nodiscard]]
constexpr T coalesce(std::initializer_list<T> list)
    {
    for (const auto& item : list)
        {
        if (item.length())
            {
            return item;
            }
        }
    return T{};
    }

/** @brief Rescales a value from one range into another range.
    @details The following formula is used:

     f(min) = a
     f(max) = b

            (b-a)(x - min)
     f(x) = --------------  + a
               max - min

     https://stackoverflow.com/questions/5294955/how-to-scale-down-a-range-of-numbers-with-a-known-min-and-max-value
    @param unscaledValue The original value to rescale.
    @param dataRange The min and max of the data that the value came from.
    @param newDataRange The min and max of the range that the value is being rescaled to.
    @returns The value, rescaled to the new data range.
    @todo needs unit tests.*/
[[nodiscard]]
inline constexpr double scale_within(double unscaledValue,
                                     const std::pair<double, double>& dataRange,
                                     const std::pair<double, double>& newDataRange)
    {
    return safe_divide<double>(
               ((newDataRange.second - newDataRange.first) * (unscaledValue - dataRange.first)),
               (dataRange.second - dataRange.first)) +
           newDataRange.first;
    }

/** @returns The next base-10 interval from the given @c value using a specified number of digits.
    @param value The value to step from.
    @param intervalSize The size of the interval to step to.\n
     For example, with a value of 2.1, the following steps would yield these results:
     - 1 -> 3
     - 2 -> 10
     - 3 -> 100
     - 4 -> 1,000
    @todo needs a unit test.*/
[[nodiscard]]
inline constexpr double next_interval(const double value, const uint8_t intervalSize) noexcept
    {
    return (intervalSize == 0) ?
               value :
               (std::ceil(value / std::pow(10, intervalSize - 1)) * std::pow(10, intervalSize - 1));
    }

/** @returns The previous base-10 interval from the given @c value using
        a specified number of digits.
    @param value The value to step from.
    @param intervalSize The size of the interval to step to.\n
     For example, with a value of 112.1, the following steps would yield these results:
     - 1 -> 112
     - 2 -> 110
     - 3 -> 100
     - 4 -> 0
    @todo needs a unit test.*/
[[nodiscard]]
inline constexpr double previous_interval(const double value, const uint8_t intervalSize) noexcept
    {
    return (intervalSize == 0) ? value :
                                 (std::floor(value / std::pow(10, intervalSize - 1)) *
                                  std::pow(10, intervalSize - 1));
    }

/** @returns Intelligent intervals for @c start and @c end to fall within.
    @param start The start of the interval.
    @param end The end of the interval.\n
     For example:\n
     - 0.75 & 4.2 -> 0-5
    @todo needs a unit test.*/
[[nodiscard]]
inline constexpr std::pair<double, double> adjust_intervals(const double start,
                                                            const double end) noexcept
    {
    const auto rangeSize = (end - start);
    uint8_t intervalSize{};
    if (rangeSize > 100'000'000)
        {
        intervalSize = 9;
        }
    else if (rangeSize > 10'000'000)
        {
        intervalSize = 8;
        }
    else if (rangeSize > 1'000'000)
        {
        intervalSize = 7;
        }
    else if (rangeSize > 100'000)
        {
        intervalSize = 6;
        }
    else if (rangeSize > 10'000)
        {
        intervalSize = 5;
        }
    else if (rangeSize > 1'000)
        {
        intervalSize = 4;
        }
    else if (rangeSize > 100)
        {
        intervalSize = 3;
        }
    else if (rangeSize > 10)
        {
        intervalSize = 2;
        }
    else
        {
        intervalSize = 1;
        }

    return std::make_pair(previous_interval(start, intervalSize), next_interval(end, intervalSize));
    }

/** @brief Combines two 32-bit integers into one 64-bit integer.
    @returns A 64-bit integer containing lowHalf in its lower half, and highHalf in the higher half.
    @param lowHalf The 32-bit integer to go into the lower half of the returned 64-bit integer.
    @param highHalf The 32-bit integer to go into the higher half of the returned 64-bit integer.*/
[[nodiscard]]
inline constexpr uint64_t join_int32s(const uint32_t lowHalf, const uint32_t highHalf) noexcept
    {
    uint64_t sum = highHalf;
    sum = sum << 32;
    sum = sum + lowHalf;
    return sum;
    }

/** @brief Splits a 64-bit integer into two 32-bit integers.
    @param value The 64-bit integer to split.
    @param[out] lowHalf The (32-bit) lower-half of the 64-bit integer.
    @param[out] highHalf The (32-bit) upper-half of the 64-bit integer.*/
inline void split_int64(const uint64_t value, uint32_t& lowHalf, uint32_t& highHalf) noexcept
    {
    highHalf = static_cast<uint32_t>(value >> 32);
    lowHalf = static_cast<uint32_t>(value);
    }

/// @returns The mantissa (floating-point value beyond the decimal) of a double value.
/// @param value The value to review.
[[nodiscard]]
inline double get_mantissa(const double value) noexcept
    {
    double ifpart = 0;
    return std::modf(value, &ifpart);
    }

/// @returns @c -1 for negative infinity, @c +1 for positive infinity, and @c 0 if finite.
/// @param value The value to review.
template<typename T>
[[nodiscard]]
inline int is_infinity(T value) noexcept
    {
    if (!std::numeric_limits<T>::has_infinity)
        {
        return 0;
        }
    const T inf = std::numeric_limits<T>::infinity();
    if (value == +inf)
        {
        return +1;
        }
    if (value == -inf)
        {
        return -1;
        }
    return 0;
    }

/// @returns Whether a double value has any floating-point data (up to 1e-6).
/// @param value The value to review.
[[nodiscard]]
inline bool has_fractional_part(const double value) noexcept
    {
    double ifpart = 0, fpart = 0;
    fpart = std::modf(value, &ifpart);
    return !compare_doubles(fpart, 0.000000, 1e-6);
    }

/** @brief Rounds a (floating-point) number. Anything less than .5 is rounded down, anything equal
        to or greater than .5 is rounded up.
    @param x The number to round.
    @returns The rounded number.*/
template<typename T>
[[nodiscard]]
inline double round_to_integer(const T x) noexcept
    {
    double ipart = 0;
    const double fpart = modf(x, &ipart);
    // workaround double precision problem where .5 assigned to a value gets
    // treated like .4999. Here we do a high precision comparison of the mantissa
    // so that if .5 was assigned then it gets seen as such, and .4999 assigned
    // gets seen as such too.
    if (compare_doubles(std::fabs(fpart), 0.500f, 1e-3))
        {
        return (x < 0) ? ipart - 1 : ipart + 1;
        }
    return (x < 0) ? (std::ceil(x - 0.5f)) : (std::floor(x + 0.5f));
    }

/** @brief Rounds a double value to a specified precision (e.g., 5.16 -> 5.2).
    @param x Value to round.
    @param decimalPlace The decimal place to round down to. 10 is the
        tenths place, 100 the hundredths place, etc.
    @returns The rounded number.*/
[[nodiscard]]
inline double round_decimal_place(const double x, const size_t decimalPlace) noexcept
    {
    if (decimalPlace == 0)
        {
        return round_to_integer(x);
        }
    double ipart = 0;
    const double fpart = round_to_integer(modf(x, &ipart) * decimalPlace);
    return safe_divide<double>(fpart, static_cast<double>(decimalPlace)) + ipart;
    }

/** @brief Truncates a double value down to a specified precision (e.g., 5.16 -> 5.1).
    @param x Value to truncated.
    @param decimalPlace The decimal place to truncate down to. 10 is the
        tenths place, 100 the hundredths place, etc.
    @returns The truncated value.
    @note Rounding is not used in this function, the number is simply
        "chopped" down to the specified precision.*/
[[nodiscard]]
inline double truncate_decimal_place(const double x, const size_t decimalPlace) noexcept
    {
    double ipart{ 0 };
    const double fpart =
        (x < 0) ? ceil(modf(x, &ipart) * decimalPlace) : floor(modf(x, &ipart) * decimalPlace);
    if (decimalPlace == 0)
        {
        return ipart;
        }
    return safe_divide<double>(fpart, static_cast<double>(decimalPlace)) + ipart;
    }

/// @brief Floors a number.
template<typename T>
class floor_value
    {
  public:
    /// @returns The floored value of @c value.
    /// @param value The value to floor.
    [[nodiscard]]
    inline T operator()(const T& value) const noexcept
        {
        return std::floor(value);
        }
    };

/// @returns Whether a number is even.
/// @param value The number to review.
template<typename T>
[[nodiscard]]
inline constexpr bool is_even(const T value) noexcept
    {
    return (value % 2) == 0;
    }

/// @brief Specialized version of is_even() for (double) floating point value
///     types that need to be "floored" first.
/// @param value The number to review.
/// @returns Whether @c value is even.
[[nodiscard]]
inline bool is_even(const double value) noexcept
    {
    return (static_cast<long>(std::floor(std::abs(value))) % 2) == 0;
    }

/// @brief Specialized version of is_even() for floating point value types
///     that need to be "floored" first.
/// @param value The number to review.
/// @returns Whether @c value is even.
[[nodiscard]]
inline bool is_even(const float value) noexcept
    {
    return (static_cast<long>(std::floor(std::abs(value))) % 2) == 0;
    }

/// @brief Determines if a number is even.
template<typename T>
class even
    {
  public:
    /// @returns Whether @c val is even.
    /// @param val The value to analyze.
    [[nodiscard]]
    inline bool operator()(const T& val) const noexcept
        {
        return is_even(val);
        }
    };

/// @brief Geometric functions.
namespace geometry
    {
    /** @brief Gets the distance between points.
        @param pt1 The first point.
        @param pt2 The second point.
        @returns The distance between the points.*/
    [[nodiscard]]
    inline double distance_between_points(const std::pair<double, double>& pt1,
                                          const std::pair<double, double>& pt2) noexcept
        {
        const double xDiff = pt1.first - pt2.first;
        const double yDiff = pt1.second - pt2.second;
        return std::sqrt((xDiff * xDiff) + (yDiff * yDiff));
        }

    /** @brief Determines if a point is inside a circle.
        @param ctr The center point of the circle.
        @param radius of the circle.
        @param pt The point to review.
        @returns @c true if the point is inside the circle.*/
    [[nodiscard]]
    inline bool is_point_inside_circle(const std::pair<double, double>& ctr, const double radius,
                                       const std::pair<double, double>& pt) noexcept
        {
        return (distance_between_points(ctr, pt) <= radius);
        }

    /** @brief Calculates the width of a rectangle that can fit inside a circle.
        @param radius The radius of the circle.
        @returns The width of the largest rectangle that can fit inside the circle.*/
    [[nodiscard]]
    inline double radius_to_inner_rect_width(const double radius) noexcept
        {
        return radius * std::sqrt(2);
        }

    /** @brief Converts circumference to radius.
        @param circumference The circumference of the circle.
        @returns The radius of the circle.*/
    [[nodiscard]]
    inline constexpr double circumference_to_radius(const double circumference) noexcept
        {
        return safe_divide<double>(safe_divide<double>(circumference, std::numbers::pi), 2);
        }

    /** @brief Converts degrees (i.e., an angle) to radians.
        @param degrees The degrees to convert.
        @returns The degrees (angle), converted to radians.*/
    [[nodiscard]]
    inline constexpr double degrees_to_radians(const double degrees) noexcept
        {
        return degrees * std::numbers::pi / static_cast<double>(180);
        }

    /** @brief Converts radians to degrees (i.e., an angle).
        @param radians The radians to convert.
        @returns The radians, converted to degrees.*/
    [[nodiscard]]
    inline constexpr double radians_to_degrees(const double radians) noexcept
        {
        return radians * 180 / static_cast<double>(std::numbers::pi);
        }

    /** @brief Given a square area with an arc drawn from its center point counter-clockwise
            (from the 3 o'clock position), calculates where the end point of the arc would be at.
        @param areaSize The size of the area with the arc drawn within it.
        @param degrees The angle (in degrees) of the arc,
            drawn from 3 o'clock going counterclockwise.
        @returns The end point of the arc.*/
    [[nodiscard]]
    inline constexpr std::pair<double, double> arc_vertex(const std::pair<double, double>& areaSize,
                                                          const double degrees) noexcept
        {
        return std::make_pair(
            (areaSize.first * safe_divide<double>(std::cos(degrees_to_radians(degrees)), 2) +
             safe_divide<double>(areaSize.first, 2)),
            (-areaSize.second * safe_divide<double>(std::sin(degrees_to_radians(degrees)), 2) +
             safe_divide<double>(areaSize.second, 2)));
        }

    /** @returns The height of a right triangle, using the angle between the slope and the base
       side.
        @param hypotenuse The hypotenuse of the triangle (the slope of the triangle).
        @param angleInDegrees The angle between the hypotenuse and bottom side
            (this angle is opposite to the height side).

               |*
             h |  *
             e |    *
             i |      * hypotenuse
             g |        *
             h |          *
             t |   opposite *
               |      angle   *
               |________________*
                     width        */
    [[nodiscard]]
    inline double right_triangle_height_opposite_angle(const double hypotenuse,
                                                       const double angleInDegrees) noexcept
        {
        return (std::sin(degrees_to_radians(angleInDegrees)) * hypotenuse);
        }

    /** @returns The height of a right triangle,
            using the angle between the slope and the height side.
        @param hypotenuse The hypotenuse of the triangle (the slope of the triangle).
        @param angleInDegrees The angle between the hypotenuse and upward side.

               |*
             h |  *
             e |    *
             i | angle* hypotenuse
             g |        *
             h |          *
             t |            *
               |              *
               |________________*
                     width        */
    [[nodiscard]]
    inline double right_triangle_height_adjacent_angle(const double hypotenuse,
                                                       const double angleInDegrees) noexcept
        {
        return (std::cos(degrees_to_radians(angleInDegrees)) * hypotenuse);
        }

    /** @returns The line segment length between two points.
        @param pt1 The first point.
        @param pt2 The second point.
        @note The ordering of points doesn't matter, the length will be the same either way.*/
    [[nodiscard]]
    inline double segment_length(const std::pair<double, double>& pt1,
                                 const std::pair<double, double>& pt2) noexcept
        {
        return std::sqrt(std::pow(pt1.first - pt2.first, 2) + std::pow(pt1.second - pt2.second, 2));
        }

    /** @brief Finds a point along a line, passed on a percent of the line length.
        @returns The point along the line at the provided percentage of the line's length.
            The returned pair represents the x and y values of the middle point.
        @param pt1 The first point of the line.
        @param pt2 The second point of the line.
        @param segmentRatio The percentage of the line's length to the point from.
            For example, `0.5` will return the middle point of the line.\n
            This value should be between `0` and `1.0`.
        @note Adapted from https://stackoverflow.com/questions/1934210/finding-a-point-on-a-line*/
    [[nodiscard]]
    inline std::pair<double, double> point_along_line(const std::pair<double, double>& pt1,
                                                      const std::pair<double, double>& pt2,
                                                      double segmentRatio) noexcept
        {
        assert(segmentRatio >= 0 && segmentRatio <= 1.0 && "segmentRatio must be between 0 and 1!");
        segmentRatio = std::clamp(segmentRatio, 0.0, 1.0);

        // find point that divides the segment
        const auto newX = (segmentRatio * pt2.first) + ((1 - segmentRatio) * pt1.first);
        // into the ratio(1 - r) : r
        const auto newY = (segmentRatio * pt2.second) + ((1 - segmentRatio) * pt1.second);
        return std::make_pair(newX, newY);
        }

    /** @brief Takes the corners of a (possibly irregular) rectangle and returns those
            corners, deflating the rectangle by the provided percentage.
        @param[in,out] pt1 Corner 1 of the rectangle.
        @param[in,out] pt2 Corner 2 of the rectangle.
        @param[in,out] pt3 Corner 3 of the rectangle.
        @param[in,out] pt4 Corner 4 of the rectangle.
        @param deflatePercentage How much to deflate the rectangle.
            This is a percentage between `0` and `1.0`.*/
    inline void deflate_rect(std::pair<double, double>& pt1, std::pair<double, double>& pt2,
                             std::pair<double, double>& pt3, std::pair<double, double>& pt4,
                             double deflatePercentage)
        {
        assert(deflatePercentage >= 0 && deflatePercentage <= 1.0 &&
               "deflatePercentage must be between 0 and 1!");
        deflatePercentage = std::clamp(deflatePercentage, 0.0, 1.0);

        const auto getMidPoint = [deflatePercentage](const auto p1, const auto p2, const auto p3)
        {
            // If one of the lines is longer, then we need to adjust where the
            // point along its edge is to truly get an evenly deflated rectangle.
            const auto line1Length{ segment_length(p1, p2) };
            const auto line2Length{ segment_length(p2, p3) };
            const auto diffPercent = safe_divide(std::abs(line2Length - line1Length),
                                                 std::max(line1Length, line2Length));
            const auto deflateAdjustmentForLongerLine =
                deflatePercentage + ((1.0 - deflatePercentage) * diffPercent);

            const auto linePt1 = point_along_line(
                p1, p2,
                (line1Length > line2Length) ? deflateAdjustmentForLongerLine : deflatePercentage);
            const auto linePt2 = point_along_line(p2, p3,
                                                  1.0 - ((line2Length > line1Length) ?
                                                             deflateAdjustmentForLongerLine :
                                                             deflatePercentage));

            return point_along_line(linePt1, linePt2, math_constants::half);
        };

        auto lineMidPt1 = getMidPoint(pt1, pt2, pt3);
        auto lineMidPt2 = getMidPoint(pt2, pt3, pt4);
        auto lineMidPt3 = getMidPoint(pt3, pt4, pt1);
        auto lineMidPt4 = getMidPoint(pt4, pt1, pt2);

        pt1 = lineMidPt4;
        pt2 = lineMidPt1;
        pt3 = lineMidPt2;
        pt4 = lineMidPt3;
        }

    /** @brief Gets the middle point between two points, where this point would
            create a spline between the two points.
        @details This assumes that the two points represent a line going
            left-to-right, and the middle point will be adjusted up or down
            to create a "ribbon" like spline.
        @returns The middle point between two points and a boolean indicating
            that the spline is going upwards. (@c false means that the splice
            is going downwards.)\n
            The returned pair represents the x and y values of the middle point.
        @param pt1 The first point.
        @param pt2 The second point.
        @todo needs unit tested.*/
    [[nodiscard]]
    inline std::tuple<double, double, bool>
    middle_point_horizontal_spline(const std::pair<double, double>& pt1,
                                   const std::pair<double, double>& pt2) noexcept
        {
        // see which point is which in our left-to-right flow
        const auto rightPt{ (pt1.first > pt2.first) ? pt1 : pt2 };

        const auto [x, y] = point_along_line(pt1, pt2, math_constants::half);
        const auto distanceBetweenMidYAndRightY{ rightPt.second - y };
        return std::make_tuple(x, y + safe_divide(distanceBetweenMidYAndRightY, 2.0),
                               (y >= rightPt.second));
        }

    /** @brief Gets the middle point between two points, where this point would
            create a spline between the two points.
        @details This assumes that the two points represent a line going
            left-to-right, and the middle point will be adjusted up
            to create a "ribbon" like spline.
        @returns The middle point between two points.\n
            The returned pair represents the x and y values of the middle point.
        @param pt1 The first point.
        @param pt2 The second point.
        @todo needs unit tested.*/
    [[nodiscard]]
    inline std::pair<double, double>
    middle_point_horizontal_upward_spline(const std::pair<double, double>& pt1,
                                          const std::pair<double, double>& pt2) noexcept
        {
        // see which point is which in our left-to-right flow
        const auto rightPt{ (pt1.first > pt2.first) ? pt1 : pt2 };

        const auto [x, y] = point_along_line(pt1, pt2, math_constants::half);
        const auto distanceBetweenMidYAndRightY{ rightPt.second - y };
        return std::make_pair(x, y - std::abs(safe_divide(distanceBetweenMidYAndRightY, 2.0)));
        }

    /** @brief Gets the middle point between two points, where this point would
            create a spline between the two points.
        @details This assumes that the two points represent a line going
            left-to-right, and the middle point will be adjusted down
            to create a "ribbon" like spline.
        @returns The middle point between two points.\n
            The returned pair represents the x and y values of the middle point.
        @param pt1 The first point.
        @param pt2 The second point.
        @todo needs unit tested.*/
    [[nodiscard]]
    inline std::pair<double, double>
    middle_point_horizontal_downward_spline(const std::pair<double, double>& pt1,
                                            const std::pair<double, double>& pt2) noexcept
        {
        // see which point is which in our left-to-right flow
        const auto rightPt{ (pt1.first > pt2.first) ? pt1 : pt2 };

        const auto [x, y] = point_along_line(pt1, pt2, math_constants::half);
        const auto distanceBetweenMidYAndRightY{ rightPt.second - y };
        return std::make_pair(x, y + std::abs(safe_divide(distanceBetweenMidYAndRightY, 2.0)));
        }

    /** @returns The angle (in degrees) of a line segment.
        @param pt1 The first point of the line segment.
        @param pt2 The second point of the line segment.
        @note The angle is going from @c pt1 to @c pt2.*/
    [[nodiscard]]
    inline double segment_angle_degrees(const std::pair<double, double>& pt1,
                                        const std::pair<double, double>& pt2) noexcept
        {
        return radians_to_degrees(std::atan2(pt2.second - pt1.second, pt2.first - pt1.first));
        }

    /** @brief Given an angle and line length, finds end point of the line.
        @param angleInDegrees The angle of the line (in degrees).
        @param length The length of the line.
        @param origin The x,y origin of the line.
        @returns The x,y end of the line.
        @todo needs unit tested.*/
    [[nodiscard]]
    inline std::pair<double, double> find_point(const double angleInDegrees, const double length,
                                                const std::pair<double, double>& origin) noexcept
        {
        return std::make_pair(origin.first + length * std::cos(degrees_to_radians(angleInDegrees)),
                              origin.second +
                                  length * std::sin(degrees_to_radians(angleInDegrees)));
        }

    /** @brief Given a starting size, calculate the new height if the size is rescaled
            to the given width, maintaining the aspect ratio.
        @returns The rescaled height.
        @param size The starting size (width x height).
        @param newWidth The width to scale the image to.
        @returns The new height.
        @note A negative new width will yield a zero height (a negative width is nonsensical,
            but at least try to return something sensical).*/
    [[nodiscard]]
    inline double rescaled_height(const std::pair<double, double>& size,
                                  const double newWidth) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 && newWidth >= 0) &&
                             "size value cannot be negative");
        if (size.first < 0 || size.second < 0 || newWidth < 0)
            {
            return 0;
            }
        return (newWidth <= 0) ? 0 : (size.second * safe_divide<double>(newWidth, size.first));
        }

    /** @brief Given a starting size, calculate the new width if the size is rescaled
            to the given height, maintaining the aspect ratio.
        @returns The rescaled width.
        @param size The starting size (width x height).
        @param newHeight The height to scale the image to.
        @returns The new width.
        @note A negative new height will yield a zero width (a negative height is nonsensical,
            but at least try to return something sensical).*/
    [[nodiscard]]
    inline double rescaled_width(const std::pair<double, double>& size,
                                 const double newHeight) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 && newHeight >= 0) &&
                             "size value cannot be negative");
        if (size.first < 0 || size.second < 0 || newHeight < 0)
            {
            return 0;
            }
        return (newHeight <= 0) ? 0 : (size.first * safe_divide<double>(newHeight, size.second));
        }

    /** @brief Takes a size (width x height) and fits it into a smaller bounding box.
        @returns The rescaled size (with aspect ratio maintained).
        @param size The initial size to be downscaled.
        @param boundingSize The bounding box to fit the size into.
        @note The assumption here is that @c size is either wider or taller (or both) than
            @c boundingSize and that it is being scaled down to fit inside @c boundingSize.
            If it is already small enough to fit in @c boundingSize,
                then the original size is returned.*/
    [[nodiscard]]
    inline std::pair<double, double>
    downscaled_size(const std::pair<double, double>& size,
                    const std::pair<double, double>& boundingSize) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 && boundingSize.first >= 0 &&
                              boundingSize.second >= 0) &&
                             "size value cannot be negative");
        // passing in negative trash, only thing to really do is return 0
        if (size.first < 0 || size.second < 0 || boundingSize.first < 0 || boundingSize.second < 0)
            {
            return std::make_pair(0, 0);
            }
        // if size fits inside new size, then no need to downscale
        if (size.first <= boundingSize.first && size.second <= boundingSize.second)
            {
            return size;
            }
        // original height is larger, so scale down by height
        else if (size.first <= boundingSize.first && size.second > boundingSize.second)
            {
            return std::make_pair(rescaled_width(size, boundingSize.second), boundingSize.second);
            }
        // original width is larger, so scale down by width
        else if (size.first > boundingSize.first && size.second <= boundingSize.second)
            {
            return std::make_pair(boundingSize.first, rescaled_height(size, boundingSize.first));
            }
        // original width and height are both larger,
        // but width is more proportionally larger, so scale down by that
        else if (size.first > boundingSize.first && size.second > boundingSize.second &&
                 (size.first - boundingSize.first) > (size.second - boundingSize.second))
            {
            // shrink the width to the bounding box and scale down
            //  the height maintaining the aspect ratio
            const auto adjustedSize =
                std::make_pair(boundingSize.first, rescaled_height(size, boundingSize.first));
            // the scale it down to the bounding box
            return downscaled_size(adjustedSize, boundingSize);
            }
        // otherwise, original width and height are both larger,
        // but height is more proportionally larger, so scale down by that
        else
            {
            const auto adjustedSize =
                std::make_pair(rescaled_width(size, boundingSize.second), boundingSize.second);
            return downscaled_size(adjustedSize, boundingSize);
            }
        }

    /** @brief Takes a size (width x height) and fits it into a larger bounding box.
        @returns The rescaled size (with aspect ratio maintained).
        @param size The initial size to be upscaled.
        @param boundingSize The bounding box to fit the size into.
        @note The assumption here is that @c size is either narrower or shorter (or both) than
            @c boundingSize and that it is being upscaled to fit inside @c boundingSize.
            If it is already larger enough to consume one of the dimensions of @c boundingSize,
            then the original size is returned.*/
    [[nodiscard]]
    inline std::pair<double, double>
    upscaled_size(const std::pair<double, double>& size,
                  const std::pair<double, double>& boundingSize) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 && boundingSize.first >= 0 &&
                              boundingSize.second >= 0) &&
                             "size value cannot be negative");
        // passing in negative trash, only thing to really do is return 0
        if (size.first < 0 || size.second < 0 || boundingSize.first < 0 || boundingSize.second < 0)
            {
            return std::make_pair(0, 0);
            }
        // if size fits outside new size, then no need to upscale
        if (size.first >= boundingSize.first && size.second >= boundingSize.second)
            {
            return size;
            }
        // original height is smaller, so scale up by height
        else if (size.first >= boundingSize.first && size.second < boundingSize.second)
            {
            return std::make_pair(rescaled_width(size, boundingSize.second), boundingSize.second);
            }
        // original width is smaller, so scale up by width
        else if (size.first < boundingSize.first && size.second >= boundingSize.second)
            {
            return std::make_pair(boundingSize.first, rescaled_height(size, boundingSize.first));
            }
        // original width and height are both smaller,
        // but width is more proportionally smaller, so scale up by that
        else if (size.first < boundingSize.first && size.second < boundingSize.second &&
                 (size.first - boundingSize.first) < (size.second - boundingSize.second))
            {
            // grow the width to the bounding box and scale up the height maintaining the aspect
            // ratio
            const auto adjustedSize =
                std::make_pair(boundingSize.first, rescaled_height(size, boundingSize.first));
            // the scale it up to the bounding box
            return downscaled_size(adjustedSize, boundingSize);
            }
        // otherwise, original width and height are both smaller,
        // but height is more proportionally smaller, so scale up by that
        else
            {
            const auto adjustedSize =
                std::make_pair(rescaled_width(size, boundingSize.second), boundingSize.second);
            return downscaled_size(adjustedSize, boundingSize);
            }
        }
    } // namespace geometry

/** @}*/

#endif // WISTERIA_MATHEMATICS_H
