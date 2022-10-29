/** @addtogroup Utilities
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __SAFE_MATH_H__
#define __SAFE_MATH_H__

#include <cmath>
#include <cassert>

/// @brief Returns the given value, unless it is NaN. In that case, returns zero.
/// @param val The value to review.
/// @returns Value, or zero if it is NaN.
[[nodiscard]] constexpr double zero_if_nan(const double val) noexcept
    { return std::isnan(val) ? 0 : val; }

// DIVISION OPERATIONS
//-------------------

/// @brief Modulus operation that checks for modulus by zero or into zero
///     (returns zero for those situations).
/// @param dividend The dividend (i.e., the value being divided).
/// @param divisor The divisor (i.e., the value dividing by).
/// @returns The remainder of the modulus operation, or zero if one of the values was invalid.
template<typename T>
[[nodiscard]] inline constexpr T safe_modulus(const T dividend, const T divisor) noexcept
    {
    if (dividend == 0 || divisor == 0)
        { return 0; }
    return dividend%divisor;
    }

/// @brief Division operation that checks for division by zero or into zero
///     (returns zero for those situations).
/// @param dividend The dividend (i.e., the value being divided).
/// @param divisor The divisor (i.e., the value dividing by).
/// @returns The quotient of the division operation, or zero if one of the values was invalid.
/// @note If the template type has floating point precision, then the result will retain its precision.
template<typename T>
[[nodiscard]] inline constexpr T safe_divide(const T dividend, const T divisor) noexcept
    {
    if (dividend == 0 || divisor == 0)
        { return 0; }
    return dividend/static_cast<T>(divisor);
    }

/// @brief Division (with remainder) operation that checks for division by zero or into zero
///     (returns zero for those situations).Essentially, this is a safe wrapper around ldiv().
/// @param dividend The dividend (i.e., the value being divided).
/// @param divisor The divisor (i.e., the value dividing by).
/// @returns The quotient of the division operation and its remainder (as longs),
///     or zeros if the input was invalid.
template<typename T>
[[nodiscard]] inline constexpr ldiv_t safe_ldiv(const T dividend, const T divisor) noexcept
    {
    ldiv_t result{ 0, 0 }; result.quot = 0; result.rem = 0;
    if (dividend == 0 || divisor == 0)
        { return result; }
    return ldiv(static_cast<long>(dividend), static_cast<long>(divisor));
    }

// FLOATING-POINT OPERATIONS
//-----------------

/** @brief Compares two double values (given the specified precision).
    @param actual The value being reviewed.
    @param expected The expected value to compare against.
    @param delta The tolerance of how different the values can be. The larger the delta, the
        higher precision used in the comparison.
    @returns @c true if the value matches the expected value.*/
[[nodiscard]] inline bool compare_doubles(const double actual, const double expected,
                                          const double delta = 1e-6) noexcept
    {
    assert(delta >= 0 && "delta value should be positive when comparing doubles");
    return (std::fabs(actual-expected) <= std::fabs(delta));
    }

/** @brief Compares two double values for less than (given the specified precision).
    @param left The value being reviewed.
    @param right The other value to compare against.
    @param delta The tolerance of how different the values can be. The larger the delta, the
     higher precision used in the comparison.
    @returns @c true if the value is less than the other value.*/
[[nodiscard]] inline bool compare_doubles_less(const double left, const double right,
                                               const double delta = 1e-6) noexcept
    {
    assert(delta >= 0 && "delta value should be positive when comparing doubles");
    return std::fabs(left-right) > std::fabs(delta) && (left < right);
    }

/** @brief Compares two double values for less than or equal to (given the specified precision).
    @param left The value being reviewed.
    @param right The other value to compare against.
    @param delta The tolerance of how different the values can be. The larger the delta, the
     higher precision used in the comparison.
    @returns @c true if the value is less than or equal to the other value.*/
[[nodiscard]] inline bool compare_doubles_less_or_equal(const double left, const double right,
                                                        const double delta = 1e-6) noexcept
    {
    assert(delta >= 0 && "delta value should be positive when comparing doubles");
    return compare_doubles_less(left,right,delta) || compare_doubles(left,right,delta);
    }

/** @brief Compares two double values for greater than (given the specified precision).
    @param left The value being reviewed.
    @param right The other value to compare against.
    @param delta The tolerance of how different the values can be. The larger the delta, the
     higher precision used in the comparison.
    @returns @c true if the value is greater than the other value.*/
[[nodiscard]] inline bool compare_doubles_greater(const double left, const double right,
                                                  const double delta = 1e-6) noexcept
    {
    assert(delta >= 0 && "delta value should be positive when comparing doubles");
    return std::fabs(left-right) > std::fabs(delta) && (left > right);
    }

/** @brief Compares two double values for greater than or equal to (given the specified precision).
    @param left The value being reviewed.
    @param right The other value to compare against.
    @param delta The tolerance of how different the values can be. The larger the delta, the
        higher precision used in the comparison.
    @returns @c true if the value is greater than or equal to the other value.*/
[[nodiscard]] inline bool compare_doubles_greater_or_equal(const double left, const double right,
                                                           const double delta = 1e-6) noexcept
    {
    assert(delta >= 0 && "delta value should be positive when comparing doubles");
    return compare_doubles_greater(left,right,delta) || compare_doubles(left,right,delta);
    }

/// @brief "less" interface for double values.
class double_less
    {
public:
    /** @brief Compares two doubles.
        @param left The left value.
        @param right The right value.
        @returns @c true if @c left is less than @c right.*/
    [[nodiscard]] inline bool operator()(const double& left, const double& right) const noexcept
        { return compare_doubles_less(left, right); }
    };

// INTEGER OPERATIONS
//------------------

/// @brief Converts an integral type to a boolean. Compilers complain about directly assigning
///     an int to a bool (casting doesn't help either), so this works around that.
/// @param intVal The integer value to convert to a boolean.
/// @returns The boolean equivalent of the integer.
template<typename T>
[[nodiscard]] constexpr bool int_to_bool(const T intVal) noexcept
    { return (intVal != 0); }

/// @brief Converts a boolean to integer (e.g., @c true = 1, @c false = 0).
/// @param boolVal The boolean value to review.
/// @returns @c 1 if @c boolVal is @c true, 0 if @c false.
[[nodiscard]] constexpr int bool_to_int(const bool boolVal) noexcept
    { return (boolVal ? 1 : 0); }

/** @}*/

#endif //__SAFE_MATH_H__
