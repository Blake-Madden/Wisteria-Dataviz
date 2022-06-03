/** @addtogroup Utilities
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __MATHEMATICS_H__
#define __MATHEMATICS_H__

#include <cmath>
#include <limits>
#include <cassert>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <initializer_list>
#include <string_view>
#include "safe_math.h"
#include "../debug/debug_assert.h"

/// @todo replace this when upgrading to C++20.
#ifndef M_PI
    #define M_PI 3.1415926535897932384626433832795
#endif

/// @brief Golden ratio
constexpr double golden_ratio = 1.618;

/// @returns `true` if a value is within a given range.
/// @param value The value to review.
/// @param first The start of the comparison range.
/// @param second The end of the comparison range.
template<typename T>
[[nodiscard]] constexpr bool is_within(const T value, const T first, const T second) noexcept
    {
    assert(first <= second);
    return (value >= first && value <= second);
    }

/// @returns `true` if @c value is within a given the @c range.
/// @param range The comparison range.
/// @param value The value to review.
template<typename T>
[[nodiscard]] constexpr bool is_within(const std::pair<T,T> range, const T value) noexcept
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
    within(T rangeBegin, T rangeEnd) noexcept
        : m_range_begin(rangeBegin), m_range_end(rangeEnd)
        {}
    within() = delete;
    /** @returns `true` if @c value is within the valid range of values.
        @param value The value to review.*/
    [[nodiscard]] inline bool operator()(T value) const noexcept
        { return is_within(std::make_pair(m_range_begin, m_range_end), value); }
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
    comparable_first_pair(const T1& t1, const T2& t2) noexcept : first(t1), second(t2)
        {}
    /** @brief Constructor that assigns a standard pair to this one.
        @param that The standard pair to assign from.*/
    comparable_first_pair(const comparable_first_pair<T1, T2>& that) : first(that.first), second(that.second)
        {}
    /** @brief Assigns a pair to this one.
        @param that The standard pair to assign from.*/
    void operator=(const comparable_first_pair<T1, T2>& that)
        {
        first = that.first;
        second = that.second;
        }
    /** @returns `true` if this is less than another pair.
        @param that The other pair to compare against.
        @note The first items of the pairs are what are compared.*/
    [[nodiscard]] bool operator<(const comparable_first_pair<T1,T2>& that) const
        { return first < that.first; }
    /** @returns `true` if this is equal to another pair.
        @param that The other pair to compare against.
        @note The first items of the pairs are what are compared. The second items can be different.
         This is a key difference from `std::pair`.*/
    [[nodiscard]] bool operator==(const comparable_first_pair<T1,T2>& that) const
        { return first == that.first; }
    /// @brief The first item in the pair.
    T1 first;
    /// @brief The second item in the pair.
    T2 second;
    };

/// @returns The first string with a value from a list of strings.
/// @param list The list of strings.
template<typename T>
[[nodiscard]] constexpr std::basic_string<T> coalesce(std::initializer_list<std::basic_string_view<T>> list)
    {
    for (const auto item : list)
        {
        if (item.length())
            { return std::basic_string<T>(item); }
        }
    return std::basic_string<T>();
    }

/// @returns The first item with a size (length) from a list.
/// @param list The list of items.
template<typename T>
[[nodiscard]] constexpr T coalesce(std::initializer_list<T> list)
    {
    for (const auto item : list)
        {
        if (item.length())
            { return item; }
        }
    return T();
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
[[nodiscard]] constexpr inline double scale_within(double unscaledValue,
                                                   std::pair<double, double> dataRange,
                                                   std::pair<double, double> newDataRange)
    {
    return safe_divide<double>(((newDataRange.second - newDataRange.first) * (unscaledValue - dataRange.first)),
                                (dataRange.second - dataRange.first)) + newDataRange.first;
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
[[nodiscard]] constexpr inline double next_interval(const double value, const uint8_t intervalSize) noexcept
    {
    return (intervalSize == 0) ? value :
        (std::ceil(value/std::pow(10, intervalSize-1))*std::pow(10, intervalSize-1));
    }

/** @returns The previous base-10 interval from the given @c value using a specified number of digits.
    @param value The value to step from.
    @param intervalSize The size of the interval to step to.\n
     For example, with a value of 112.1, the following steps would yield these results:
     - 1 -> 112
     - 2 -> 110
     - 3 -> 100
     - 4 -> 0
    @todo needs a unit test.*/
[[nodiscard]] constexpr inline double previous_interval(const double value, const uint8_t intervalSize) noexcept
    {
    return (intervalSize == 0) ? value :
        (std::floor(value/std::pow(10, intervalSize-1))*std::pow(10, intervalSize-1));
    }

/** @returns Intelligent intervals for @c start and @c end to fall within.
    @param start The start of the interval.
    @param end The end of the interval.
     For example:
     - 0.75 & 4.2 -> 0-5
    @todo needs a unit test.*/
[[nodiscard]] constexpr inline std::pair<double, double> adjust_intervals(const double start, const double end) noexcept
    {
    const auto rangeSize = (end-start);
    uint8_t intervalSize{ 1 };
    if (rangeSize > 100'000'000)
        { intervalSize = 9; }
    else if (rangeSize > 10'000'000)
        { intervalSize = 8; }
    else if (rangeSize > 1'000'000)
        { intervalSize = 7; }
    else if (rangeSize > 100'000)
        { intervalSize = 6; }
    else if (rangeSize > 10'000)
        { intervalSize = 5; }
    else if (rangeSize > 1'000)
        { intervalSize = 4; }
    else if (rangeSize > 100)
        { intervalSize = 3; }
    else if (rangeSize > 10)
        { intervalSize = 2; }
    else
        { intervalSize = 1; }

    return std::make_pair(previous_interval(start, intervalSize), next_interval(end, intervalSize));
    }

/** @brief Combines two 32-bit integers into one 64-bit integer.
    @returns A 64-bit integer containing lowHalf in its lower half, and highHalf in the higher half.
    @param lowHalf The 32-bit integer to go into the lower half of the returned 64-bit integer.
    @param highHalf The 32-bit integer to go into the higher half of the returned 64-bit integer.*/
[[nodiscard]] constexpr inline uint64_t join_int32s(const uint32_t lowHalf, const uint32_t highHalf) noexcept
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
[[nodiscard]] inline double get_mantissa(const double value) noexcept
    {
    double ifpart = 0;
    return std::modf(value, &ifpart);
    }

/// @returns -1 for negative infinity, +1 for positive infinity, and 0 if finite.
/// @param value The value to review.
template <typename T>
[[nodiscard]] inline int is_infinity(T value) noexcept
    {
    if (!std::numeric_limits<T>::has_infinity)
        return 0;
    const T inf = std::numeric_limits<T>::infinity();
    if (value == +inf)
        return +1;
    if (value == -inf)
        return -1;
    return 0;
    }

/// @returns Whether a double value has any floating-point data (up to 1e-6).
/// @param value The value to review.
[[nodiscard]] inline bool has_fractional_part(const double value) noexcept
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
[[nodiscard]] inline double round_to_integer(const T x) noexcept
    {
    double ipart = 0;
    const double fpart = modf(x, &ipart);
    // workaround double precision problem where .5 assigned to a value gets
    // treated like .4999. Here we do a high precision comparison of the mantissa
    // so that if .5 was assigned then it gets seen as such, and .4999 assigned
    // gets seen as such too.
    if (compare_doubles(std::fabs(fpart),0.500f,1e-3))
        {
        return (x < 0) ?
            ipart-1 : ipart+1;
        }
    return (x < 0) ?
        (std::ceil(x-0.5f)) :
        (std::floor(x+0.5f));
    }

/** @brief Rounds a double value to a specified precision (e.g., 5.16 -> 5.2).
    @param x Value to round.
    @param decimalPlace The decimal place to round down to. 10 is the
     tenths place, 100 the hundredths place, etc.
    @returns The rounded number.*/
[[nodiscard]] inline double round_decimal_place(const double x, const size_t decimalPlace) noexcept
    {
    if (decimalPlace == 0)
        { return round_to_integer(x); }
    double ipart = 0;
    const double fpart = round_to_integer(modf(x, &ipart)*decimalPlace);
    return safe_divide<double>(fpart, decimalPlace)+ipart;
    }

/** @brief Truncates a double value down to a specified precision (e.g., 5.16 -> 5.1).
    @param x Value to truncated.
    @param decimalPlace The decimal place to truncate down to. 10 is the
     tenths place, 100 the hundredths place, etc.
    @returns The truncated value.
    @note Rounding is not used in this function, the number is simply "chopped" down to the specified precision.*/
[[nodiscard]] inline double truncate_decimal_place(const double x, const size_t decimalPlace) noexcept
    {
    double ipart{ 0 };
    const double fpart = (x < 0) ? ceil(modf(x, &ipart)*decimalPlace) : floor(modf(x, &ipart)*decimalPlace);
    if (decimalPlace == 0)
        { return ipart; }
    return safe_divide<double>(fpart, decimalPlace)+ipart;
    }

/// @brief Floors a number.
template<typename T>
class floor_value
    {
public:
    /// @returns The floored value of @c value.
    /// @param value The value to floor.
    [[nodiscard]] inline T operator()(const T& value) const noexcept
        { return std::floor(value); }
    };

/// @returns Whether a number is even.
/// @param value The number to review.
template<typename T>
[[nodiscard]] inline bool is_even(const T value) noexcept
    { return (value%2) == 0; }
/// @brief Specialized version of is_even() for (double) floating point value
///  types that need to be "floored" first.
/// @param value The number to review.
/// @returns Whether @c value is even.
[[nodiscard]] inline bool is_even(const double value) noexcept
    { return (static_cast<long>(std::floor(std::abs(value)))%2) == 0; }
/// @brief Specialized version of is_even() for floating point value types
///  that need to be "floored" first.
/// @param value The number to review.
/// @returns Whether @c value is even.
[[nodiscard]] inline bool is_even(const float value) noexcept
    { return (static_cast<long>(std::floor(std::abs(value)))%2) == 0; }

/// @brief Determines if a number is even.
template <typename T>
class even
    {
public:
    /// @returns Whether @c val is even.
    /// @param val The value to analyze.
    [[nodiscard]] inline bool operator()(const T& val) const noexcept
        { return is_even(val); }
    };

/// @brief Geometric functions.
namespace geometry
    {
    /** @brief Gets the distance between points.
        @param pt1 The first point.
        @param pt2 The second point.
        @returns The distance between the points.
        @todo Add unit test.*/
    inline double distance_between_points(const std::pair<double, double> pt1,
                                          const std::pair<double, double> pt2) noexcept
        {
        const double xDiff = pt1.first - pt2.first;
        const double yDiff = pt1.second - pt2.second;
        return std::sqrt((xDiff * xDiff) + (yDiff * yDiff));
        }
    /** @brief Determines if a point is inside of a circle.
        @param ctr The center point of the circle.
        @param radius of the circle.
        @param pt The point to review.
        @todo Add a unit test.*/
    inline bool is_point_inside_circle(const std::pair<double, double> ctr,
                                       const double radius,
                                       const std::pair<double, double> pt) noexcept
        { return (radius >= distance_between_points(ctr, pt)); }
    /** @brief Calculates the width of a rectangle that can fit inside of a circle.
        @param radius The radius of the circle.
        @returns The width of the largest rectangle that can fit inside of the circle.*/
    [[nodiscard]] inline double radius_to_inner_rect_width(const double radius) noexcept
        { return radius * std::sqrt(2); }
    /** @brief Converts circumference to radius.
        @param circumference The circumference of the circle.
        @returns The radius of the circle.*/
    [[nodiscard]] constexpr inline double circumference_to_radius(const double circumference) noexcept
        { return safe_divide<double>(safe_divide<double>(circumference, M_PI), 2); }

    /** @brief Converts degrees (i.e., an angle) to radians.
        @param degrees The degrees to convert.
        @returns The degrees (angle), converted to radians.*/
    [[nodiscard]] constexpr inline double degrees_to_radians(const double degrees) noexcept
        { return degrees * M_PI/static_cast<double>(180); }

    /** @brief Converts radians to degrees (i.e., an angle).
        @param radians The radians to convert.
        @returns The radians, converted to degrees.*/
    [[nodiscard]] constexpr inline double radians_to_degrees(const double radians) noexcept
        { return radians * 180/static_cast<double>(M_PI); }

    /** @brief Given a square area with an arc drawn from its center point counter-clockwise
         (from the 3 o'clock position), calculates where the end point of the arc would be at.
        @param areaSize The size of the area with the arc drawn within it.
        @param degrees The angle (in degrees) of the arc, drawn from 3 o'clock going counter clockwise.
        @returns The end point of the arc.*/
    [[nodiscard]] constexpr inline std::pair<double, double> calc_arc_vertex(
                                                            std::pair<double, double> areaSize,
                                                            const double degrees) noexcept
        {
        return std::make_pair(
            (areaSize.first * safe_divide<double>(std::cos(degrees_to_radians(degrees)), 2) +
                safe_divide<double>(areaSize.first, 2)),
            (-areaSize.second * safe_divide<double>(std::sin(degrees_to_radians(degrees)), 2) +
                safe_divide<double>(areaSize.second, 2)) );
        }

    /** @returns The height of a right triangle, using the angle between the slope and the base side.
        @param hypotenuse The hypotenuse of the triangle (the slope of the triangle).
        @param angleInDegrees The angle between the hypotenuse and bottom side
         (this angle is opposite from the height side).

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
    [[nodiscard]] inline double calc_right_triangle_height_opposite_angle(const double hypotenuse,
                                                                          const double angleInDegrees) noexcept
        { return (std::sin(degrees_to_radians(angleInDegrees)) * hypotenuse); }

    /** @returns The height of a right triangle, using the angle between the slope and the height side.
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
    [[nodiscard]] inline double calc_right_triangle_height_adjacent_angle(const double hypotenuse,
                                                                          const double angleInDegrees) noexcept
        { return (std::cos(degrees_to_radians(angleInDegrees)) * hypotenuse); }

    /** @returns The line segment length between two points.
        @param pt1 The first point.
        @param pt2 The second point.
        @note The ordering of points doesn't matter, the length will be the same either way.*/
    [[nodiscard]] inline double calc_segment_length(const std::pair<double,double> pt1,
                                                    const std::pair<double,double> pt2) noexcept
        { return std::sqrt(std::pow(pt1.first-pt2.first, 2) + std::pow(pt1.second-pt2.second, 2)); }

    /** @returns The angle (in degrees) of a line segment.
        @param pt1 The first point of the line segment.
        @param pt2 The second point of the line segment.
        @note The angle is going from @c pt1 to @c pt2.*/
    [[nodiscard]] inline double calc_segment_angle_degrees(const std::pair<double,double> pt1,
                                                           const std::pair<double,double> pt2) noexcept
        { return radians_to_degrees(std::atan2(pt2.second-pt1.second, pt2.first-pt1.first)); }

    /** @brief Given an angle and line length, finds end point of the line.
        @param angleInDegrees The angle of the line (in degrees).
        @param length The length of the line.
        @param origin The x,y origin of the line.
        @returns The x,y end of the line.
        @todo needs unit tested.*/
    [[nodiscard]] inline std::pair<double,double> find_point(const double angleInDegrees,
                                               const double length,
                                               const std::pair<double,double> origin) noexcept
        {
        return std::make_pair(origin.first + length*std::cos(degrees_to_radians(angleInDegrees)),
                              origin.second + length*std::sin(degrees_to_radians(angleInDegrees)) );
        }

    /** @brief Given a starting size, calculate the new height if the size is rescaled
         to the given width, maintaining the aspect ratio.
        @returns The rescaled height.
        @param size The starting size (width x height).
        @param newWidth The width to scale the image to.
        @returns The new height.
        @note A negative new width will yield a zero height (a negative width is nonsensical,
         but at least try to return something sensical).*/
    [[nodiscard]] inline double calculate_rescale_height(const std::pair<double,double> size,
                                                         const double newWidth) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 && newWidth >= 0) &&
            "size value cannot be negative");
        if (size.first < 0 || size.second < 0 || newWidth < 0)
            { return 0; }
        return (newWidth<=0) ? 0 : (size.second*safe_divide<double>(newWidth,size.first));
        }

    /** @brief Given a starting size, calculate the new width if the size is rescaled
         to the given height, maintaining the aspect ratio.
        @returns The rescaled width.
        @param size The starting size (width x height).
        @param newHeight The height to scale the image to.
        @returns The new width.
        @note A negative new height will yield a zero width (a negative height is nonsensical,
         but at least try to return something sensical).*/
    [[nodiscard]] inline double calculate_rescale_width(const std::pair<double,double> size,
                                                        const double newHeight) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 && newHeight >= 0) &&
            "size value cannot be negative");
        if (size.first < 0 || size.second < 0 || newHeight < 0)
            { return 0; }
        return (newHeight<=0) ? 0: (size.first*safe_divide<double>(newHeight,size.second));
        }

    /** @brief Takes a size (width x height) and fits it into a smaller bounding box.
        @returns The rescaled size (with aspect ratio maintained).
        @param size The initial size to be downscaled.
        @param boundingSize The bounding box to fit the size into.
        @note The assumption here is that @c size is either wider or taller (or both) than
         @c boundingSize and that it is being scaled down to fit inside of @c boundingSize.
         If it is already small enough to fit in @c boundingSize, then the original size is returned.*/
    [[nodiscard]] inline std::pair<double,double> calculate_downscaled_size(
                                                const std::pair<double,double> size,
                                                const std::pair<double,double> boundingSize) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 &&
                              boundingSize.first >= 0 && boundingSize.second >= 0) &&
                             "size value cannot be negative");
        // passing in negative trash, only thing to really do is return 0
        if (size.first < 0 || size.second < 0 || boundingSize.first < 0 || boundingSize.second < 0)
            { return std::make_pair(0, 0); }
        // if size fits inside of new size, then no need to downscale
        if (size.first <= boundingSize.first && size.second <= boundingSize.second)
            { return size; }
        // original height is larger, so scale down by height
        else if (size.first <= boundingSize.first && size.second > boundingSize.second)
            { return std::make_pair(calculate_rescale_width(size, boundingSize.second), boundingSize.second); }
        // original width is larger, so scale down by width
        else if (size.first > boundingSize.first && size.second <= boundingSize.second)
            { return std::make_pair(boundingSize.first, calculate_rescale_height(size, boundingSize.first)); }
        // original width and height are both larger,
        // but width is more proportionally larger, so scale down by that
        else if (size.first > boundingSize.first && size.second > boundingSize.second &&
            (size.first-boundingSize.first) > (size.second-boundingSize.second))
            {
            // shrink the width to the bounding box and scale down the height maintaining the aspect ratio
            const auto adjustedSize = std::make_pair(boundingSize.first,
                                                     calculate_rescale_height(size, boundingSize.first));
            // the scale it down to the bounding box
            return calculate_downscaled_size(adjustedSize, boundingSize);
            }
        // otherwise, original width and height are both larger,
        // but height is more proportionally larger, so scale down by that
        else
            {
            const auto adjustedSize = std::make_pair(calculate_rescale_width(size, boundingSize.second),
                                                     boundingSize.second);
            return calculate_downscaled_size(adjustedSize, boundingSize);
            }
        }

    /** @brief Takes a size (width x height) and fits it into a larger bounding box.
        @returns The rescaled size (with aspect ratio maintained).
        @param size The initial size to be upscaled.
        @param boundingSize The bounding box to fit the size into.
        @note The assumption here is that @c size is either narrower or shorter (or both) than
         @c boundingSize and that it is being upscaled down to fit inside of @c boundingSize.
         If it is already larger enough to consume on of the dimensions of @c boundingSize,
         then the original size is returned.*/
    [[nodiscard]] inline std::pair<double,double> calculate_upscaled_size(
                                    const std::pair<double,double> size,
                                    const std::pair<double,double> boundingSize) noexcept
        {
        NON_UNIT_TEST_ASSERT((size.first >= 0 && size.second >= 0 &&
                              boundingSize.first >= 0 && boundingSize.second >= 0) &&
                             "size value cannot be negative");
        // passing in negative trash, only thing to really do is return 0
        if (size.first < 0 || size.second < 0 || boundingSize.first < 0 || boundingSize.second < 0)
            { return std::make_pair(0, 0); }
        // if size fits outside of new size, then no need to upscale
        if (size.first >= boundingSize.first && size.second >= boundingSize.second)
            { return size; }
        // original height is smaller, so scale up by height
        else if (size.first >= boundingSize.first && size.second < boundingSize.second)
            { return std::make_pair(calculate_rescale_width(size, boundingSize.second), boundingSize.second); }
        // original width is smaller, so scale up by width
        else if (size.first < boundingSize.first && size.second >= boundingSize.second)
            { return std::make_pair(boundingSize.first, calculate_rescale_height(size, boundingSize.first)); }
        // original width and height are both smaller,
        // but width is more proportionally smaller, so scale up by that
        else if (size.first < boundingSize.first && size.second < boundingSize.second &&
            (size.first-boundingSize.first) < (size.second-boundingSize.second))
            {
            // grow the width to the bounding box and scale up the height maintaining the aspect ratio
            const auto adjustedSize = std::make_pair(boundingSize.first,
                                                     calculate_rescale_height(size, boundingSize.first));
            // the scale it up to the bounding box
            return calculate_downscaled_size(adjustedSize, boundingSize);
            }
        // otherwise, original width and height are both smaller,
        // but height is more proportionally smaller, so scale up by that
        else
            {
            const auto adjustedSize = std::make_pair(calculate_rescale_width(size, boundingSize.second),
                                                     boundingSize.second);
            return calculate_downscaled_size(adjustedSize, boundingSize);
            }
        }
    };

/** @}*/

#endif //__MATHEMATICS_H__
