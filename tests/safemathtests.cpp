// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/safe_math.h"
#include "../src/math/mathematics.h"
#include <algorithm>
#include <vector>

using namespace Catch::Matchers;

TEST_CASE("adjust_intervals: reversed inputs yield same adjusted range", "[math][intervals][reverse]")
    {
    const auto fwd = adjust_intervals(0.0, 123.0);
    const auto rev = adjust_intervals(123.0, 0.0);

    CHECK(fwd == rev);
    }

TEST_CASE("adjust_intervals: reversed negative/positive inputs yield same adjusted range", "[math][intervals][reverse]")
    {
    const auto fwd = adjust_intervals(-10.0, 10.0);
    const auto rev = adjust_intervals(10.0, -10.0);

    CHECK(fwd == rev);
    }

TEST_CASE("adjust_intervals: reversed non-integer inputs yield same adjusted range", "[math][intervals][reverse]")
    {
    const auto fwd = adjust_intervals(3.1, 7.25);
    const auto rev = adjust_intervals(7.25, 3.1);

    CHECK(fwd == rev);
    }

TEST_CASE("adjust_intervals: identical endpoints remain ordered", "[math][intervals][degenerate]")
    {
    const auto same = adjust_intervals(5.0, 5.0);

    CHECK(same.first <= same.second);
    CHECK(same.first == 5.0);
    CHECK(same.second == 5.0);
    }

TEST_CASE("safe_divide returns 0 for NaN or infinity (double)", "[safe_math][safe_divide][double]")
    {
    using std::numeric_limits;

    constexpr double NaN   = numeric_limits<double>::quiet_NaN();
    constexpr double Inf   = numeric_limits<double>::infinity();
    constexpr double NInf  = -numeric_limits<double>::infinity();

    // NaN in either operand → 0
    CHECK(safe_divide(NaN,  5.0) == 0.0);
    CHECK(safe_divide(5.0,  NaN) == 0.0);
    CHECK(safe_divide(NaN,  NaN) == 0.0);

    // ±∞ in either operand → 0
    CHECK(safe_divide(Inf,   2.0) == 0.0);
    CHECK(safe_divide(NInf,  2.0) == 0.0);
    CHECK(safe_divide(2.0,   Inf) == 0.0);
    CHECK(safe_divide(2.0,   NInf) == 0.0);
    CHECK(safe_divide(Inf,   NInf) == 0.0);

    // Zero divisor/dividend cases → 0
    CHECK(safe_divide(0.0,   3.0) == 0.0);
    CHECK(safe_divide(6.0,   0.0) == 0.0);
    CHECK(safe_divide(0.0,   0.0) == 0.0);

    // Baseline finite division still works
    CHECK(safe_divide(6.0,   3.0) == 2.0);
    CHECK(safe_divide(-8.0,  2.0) == -4.0);
    }

TEST_CASE("safe_divide returns 0 for NaN or infinity (float)", "[safe_math][safe_divide][float]")
    {
    using std::numeric_limits;

    constexpr float NaN   = numeric_limits<float>::quiet_NaN();
    constexpr float Inf   = numeric_limits<float>::infinity();
    constexpr float NInf  = -numeric_limits<float>::infinity();

    CHECK(safe_divide(NaN,  5.0f) == 0.0f);
    CHECK(safe_divide(5.0f, NaN)  == 0.0f);

    CHECK(safe_divide(Inf,  2.0f) == 0.0f);
    CHECK(safe_divide(2.0f, Inf)  == 0.0f);

    CHECK(safe_divide(0.0f, 3.0f) == 0.0f);
    CHECK(safe_divide(6.0f, 0.0f) == 0.0f);

    CHECK(safe_divide(6.0f, 3.0f) == 2.0f);
    }

TEST_CASE("zero_if_nan incorrectly keeps infinities", "[safe_math][zero_if_nan][inf]")
    {
    using std::numeric_limits;

    constexpr double inf  = numeric_limits<double>::infinity();
    constexpr double ninf = -numeric_limits<double>::infinity();
    constexpr double nan  = numeric_limits<double>::quiet_NaN();

    // should zero out NaN
    CHECK(zero_if_nan(nan) == 0.0);

    // should also zero out infinities
    // should also zero out infinities
    CHECK(zero_if_nan(inf)  == 0.0);
    CHECK(zero_if_nan(ninf) == 0.0);

    // finite values should pass through unchanged
    CHECK(zero_if_nan(0.0)   == 0.0);
    CHECK(zero_if_nan(42.0)  == 42.0);
    CHECK(zero_if_nan(-7.25) == -7.25);
    }

TEST_CASE("Safe LDivides By Zero", "[safemath]")
    {
    CHECK(0 == safe_ldiv(0, 100).quot);
    CHECK(0 == safe_ldiv(0, 100).rem);
    CHECK(0 == safe_ldiv(0, 0).quot);
    CHECK(0 == safe_ldiv(0, 0).rem);
    CHECK(0 == safe_ldiv(1000, 0).quot);
    CHECK(0 == safe_ldiv(1000, 0).rem);
    }

TEST_CASE("Safe LDivides", "[safemath]")
    {
    CHECK((5) == safe_ldiv(107, 20).quot);
    CHECK((7) == safe_ldiv(107, 20).rem);

    CHECK((-5) == safe_ldiv(107, -20).quot);
    CHECK((7) == safe_ldiv(107, -20).rem);

    CHECK((-10) == safe_ldiv(-207, 20).quot);
    CHECK((-7) == safe_ldiv(-207, 20).rem);
    }

TEST_CASE("Safe Divides By Zero", "[safemath]")
    {
    CHECK(0 == safe_divide(0, 100));
    CHECK(0 == safe_divide(0, 0));
    CHECK(0 == safe_divide(1000, 0));
    }

TEST_CASE("Safe Divides", "[safemath]")
    {
    CHECK_THAT(25, WithinRel(safe_divide(50, 2), 1e-6));
    CHECK_THAT(1, WithinRel(safe_divide(5, 5), 1e-6));

    CHECK_THAT(2.5, WithinRel(safe_divide<double>(5, 2), 1e-6));
    CHECK_THAT(0.5, WithinRel(safe_divide<double>(2, 4), 1e-6));
    CHECK_THAT(2.0, WithinRel(safe_divide<double>(5.0, 2.5), 1e-6));
    }

TEST_CASE("Safe Modulus", "[safemath]")
    {
    CHECK(0 == safe_modulus<int>(100, 10));
    CHECK((1) == safe_modulus<int>(100, 9));
    CHECK((2) == safe_modulus<int>(9587, 9));
    CHECK(0 == safe_modulus<int>(100, 100));
    CHECK(0 == safe_modulus<int>(100, 1));
    CHECK((3) == safe_modulus<size_t>(101, 7));
    }

TEST_CASE("Safe Modulus By Zero", "[safemath]")
    {
    CHECK(0 == safe_modulus<int>(0, 100));
    CHECK(0 == safe_modulus<size_t>(0, 0));
    CHECK(0 == safe_modulus<size_t>(1000, 0));
    }

TEST_CASE("Safe Modulus By Negative", "[safemath]")
    {
    CHECK(0 == safe_modulus<int>(100, -1));
    }

TEST_CASE("Comparable first pair", "[comparable]")
    {
    SECTION("Compare First Doubles")
        {
        comparable_first_pair<double, double> first(0.1, 999999);
        comparable_first_pair<double, double> second(1.1, 9);
        CHECK(first < second);
        CHECK((second < first) == false);
        }
    SECTION("Compare First Strings")
        {
        comparable_first_pair<std::wstring, std::wstring> first(L"hi", L"zoo");
        comparable_first_pair<std::wstring, std::wstring> second(L"zoo", L"hi");
        CHECK(first < second);
        CHECK((second < first) == false);
        }
    SECTION("Compare First Mixed")
        {
        comparable_first_pair<double, std::wstring> first(0.1, L"zoo");
        comparable_first_pair<double, std::wstring> second(1.1, L"hi");
        CHECK(first < second);
        CHECK((second < first) == false);
        }
    SECTION("CTOR From Pair")
        {
        comparable_first_pair<double, std::wstring> myPair(1.2, L"one point 2");
        CHECK_THAT(1.2, WithinRel(myPair.first, 1e-1));
        CHECK(myPair.second == std::wstring(L"one point 2"));
        }
    SECTION("Assignment")
        {
        comparable_first_pair<double, std::wstring> myPair;
        myPair = comparable_first_pair<double, std::wstring>(1.2, L"one point 2");
        CHECK_THAT(1.2, WithinRel(myPair.first, 1e-1));
        CHECK(myPair.second == std::wstring(L"one point 2"));

        comparable_first_pair<double, std::wstring> myOtherPair = myPair;
        CHECK_THAT(1.2, WithinRel(myOtherPair.first, 1e-1));
        CHECK(myOtherPair.second == std::wstring(L"one point 2"));
        CHECK(myOtherPair == myPair);

        comparable_first_pair<double, std::wstring> myOtherOtherPair;
        myOtherOtherPair = myPair;
        CHECK_THAT(1.2, WithinRel(myOtherOtherPair.first, 1e-1));
        CHECK(myOtherOtherPair.second == std::wstring(L"one point 2"));

        myPair.first = 7;
        CHECK(!(myOtherPair == myPair));
        }
    }

// ------------------------------------------------------------
// Compile-time tests
// ------------------------------------------------------------
static_assert(is_power_of_two(1u));
static_assert(is_power_of_two(2u));
static_assert(is_power_of_two(1024u));
static_assert(!is_power_of_two(0u));
static_assert(!is_power_of_two(3u));
static_assert(!is_power_of_two(1000u));

// ------------------------------------------------------------
// Runtime unit tests
// ------------------------------------------------------------
TEST_CASE("is_power_of_two basic checks", "[math][bit]")
    {
    SECTION("Zero is not a power of two")
        {
        REQUIRE_FALSE(is_power_of_two(0u));
        }

    SECTION("Powers of two return true")
        {
        REQUIRE(is_power_of_two(1u));
        REQUIRE(is_power_of_two(2u));
        REQUIRE(is_power_of_two(4u));
        REQUIRE(is_power_of_two(8u));
        REQUIRE(is_power_of_two(16u));
        REQUIRE(is_power_of_two(1024u));
        }

    SECTION("Non-powers of two return false")
        {
        REQUIRE_FALSE(is_power_of_two(3u));
        REQUIRE_FALSE(is_power_of_two(5u));
        REQUIRE_FALSE(is_power_of_two(6u));
        REQUIRE_FALSE(is_power_of_two(7u));
        REQUIRE_FALSE(is_power_of_two(9u));
        REQUIRE_FALSE(is_power_of_two(1000u));
        }
    }

TEST_CASE("is_power_of_two with different unsigned types", "[math][bit][types]")
    {
    // uint8_t
    REQUIRE(is_power_of_two(uint8_t{1}));
    REQUIRE_FALSE(is_power_of_two(uint8_t{3}));

    // uint16_t
    REQUIRE(is_power_of_two(uint16_t{256}));
    REQUIRE_FALSE(is_power_of_two(uint16_t{257}));

    // uint32_t
    REQUIRE(is_power_of_two(uint32_t{1u << 31}));

    REQUIRE_FALSE(is_power_of_two(uint32_t{(1u << 31) - 1}));

    // uint64_t
    REQUIRE(is_power_of_two(uint64_t{1ull << 63}));
    REQUIRE_FALSE(is_power_of_two(uint64_t{(1ull << 63) - 1}));
    }

TEST_CASE("scale_within basic scaling", "[scale_within]")
    {
    std::pair<double, double> dataRange{0.0, 10.0};
    std::pair<double, double> newRange{0.0, 100.0};

    SECTION("scales minimum correctly")
        {
        CHECK_THAT(scale_within(0.0, dataRange, newRange), WithinAbs(0.0, 1e-6));
        }
    SECTION("scales maximum correctly")
        {
        CHECK_THAT(scale_within(10.0, dataRange, newRange), WithinRel(100.0, 1e-6));
        }
    SECTION("scales midpoint correctly")
        {
        CHECK_THAT(scale_within(5.0, dataRange, newRange), WithinRel(50.0, 1e-6));
        }
    }

TEST_CASE("scale_within identity scaling", "[scale_within]")
    {
    std::pair<double, double> dataRange{0.0, 1.0};
    std::pair<double, double> newRange{0.0, 1.0};

    CHECK_THAT(scale_within(0.25, dataRange, newRange), WithinRel(0.25, 1e-6));
    CHECK_THAT(scale_within(1.0, dataRange, newRange), WithinRel(1.0, 1e-6));
    CHECK_THAT(scale_within(0.0, dataRange, newRange), WithinAbs(0.0, 1e-6));
    }

TEST_CASE("scale_within reversed target range", "[scale_within]")
    {
    std::pair<double, double> dataRange{0.0, 10.0};
    std::pair<double, double> newRange{100.0, 0.0};

    SECTION("minimum maps to new maximum")
        {
        CHECK_THAT(scale_within(0.0, dataRange, newRange), WithinRel(100.0, 1e-6));
        }
    SECTION("maximum maps to new minimum")
        {
        CHECK_THAT(scale_within(10.0, dataRange, newRange), WithinAbs(0.0, 1e-6));
        }
    SECTION("midpoint flips correctly")
        {
        CHECK_THAT(scale_within(5.0, dataRange, newRange), WithinRel(50.0, 1e-6));
        }
    }

TEST_CASE("scale_within reversed input range", "[scale_within]")
    {
    std::pair<double, double> dataRange{10.0, 0.0};
    std::pair<double, double> newRange{0.0, 100.0};

    SECTION("original max maps to new max")
        {
        CHECK_THAT(scale_within(0.0, dataRange, newRange), WithinRel(100.0, 1e-6));
        }
    SECTION("original min maps to new min")
        {
        CHECK_THAT(scale_within(10.0, dataRange, newRange), WithinAbs(0.0, 1e-6));
        }
    }

TEST_CASE("scale_within value outside input range", "[scale_within]")
    {
    std::pair<double, double> dataRange{0.0, 10.0};
    std::pair<double, double> newRange{0.0, 100.0};

    SECTION("below minimum extrapolates")
        {
        CHECK_THAT(scale_within(-5.0, dataRange, newRange), WithinRel(-50.0, 1e-6));
        }
    SECTION("above maximum extrapolates")
        {
        CHECK_THAT(scale_within(15.0, dataRange, newRange), WithinRel(150.0, 1e-6));
        }
    }

TEST_CASE("scale_within invalid input range", "[scale_within]")
    {
    std::pair<double, double> dataRange{5.0, 5.0}; // zero width
    std::pair<double, double> newRange{0.0, 100.0};

    // denominator == 0 -> safe_divide returns 0
    // so result = newRange.first
    CHECK_THAT(scale_within(5.0, dataRange, newRange), WithinRel(newRange.first, 1e-6));
    CHECK_THAT(scale_within(10.0, dataRange, newRange), WithinRel(newRange.first, 1e-6));
    }

// ---------------- next_interval tests ----------------
TEST_CASE("next_interval basic rounding", "[next_interval]")
    {
    CHECK_THAT(next_interval(2.1, 1), WithinRel(3.0, 1e-6));
    CHECK_THAT(next_interval(2.1, 2), WithinRel(10.0, 1e-6));
    CHECK_THAT(next_interval(2.1, 3), WithinRel(100.0, 1e-6));
    CHECK_THAT(next_interval(2.1, 4), WithinRel(1000.0, 1e-6));
    }

TEST_CASE("next_interval edge cases", "[next_interval]")
    {
    SECTION("intervalSize zero returns same value")
        {
        CHECK_THAT(next_interval(123.45, 0), WithinRel(123.45, 1e-6));
        }
    SECTION("exact boundary returns same value")
        {
        CHECK_THAT(next_interval(100.0, 3), WithinRel(100.0, 1e-6));
        }
    }

TEST_CASE("next_interval with negatives", "[next_interval]")
    {
    CHECK_THAT(next_interval(-2.1, 1), WithinRel(-2.0, 1e-6));
    CHECK_THAT(next_interval(-2.1, 2), WithinAbs(0.0, 1e-6));
    CHECK_THAT(next_interval(-112.5, 3), WithinRel(-100.0, 1e-6));
    }

// ---------------- previous_interval tests ----------------
TEST_CASE("previous_interval basic rounding", "[previous_interval]")
    {
    CHECK_THAT(previous_interval(112.1, 1), WithinRel(112.0, 1e-6));
    CHECK_THAT(previous_interval(112.1, 2), WithinRel(110.0, 1e-6));
    CHECK_THAT(previous_interval(112.1, 3), WithinRel(100.0, 1e-6));
    CHECK_THAT(previous_interval(112.1, 4), WithinAbs(0.0, 1e-6));
    }

TEST_CASE("previous_interval edge cases", "[previous_interval]")
    {
    SECTION("intervalSize zero returns same value")
        {
        CHECK_THAT(previous_interval(123.45, 0), WithinRel(123.45, 1e-6));
        }
    SECTION("exact boundary returns same value")
        {
        CHECK_THAT(previous_interval(200.0, 2), WithinRel(200.0, 1e-6));
        }
    }

TEST_CASE("previous_interval with negatives", "[previous_interval]")
    {
    CHECK_THAT(previous_interval(-2.1, 1), WithinRel(-3.0, 1e-6));
    CHECK_THAT(previous_interval(-2.1, 2), WithinRel(-10.0, 1e-6));
    CHECK_THAT(previous_interval(-112.5, 3), WithinRel(-200.0, 1e-6));
    }

// ---------------- adjust_intervals tests ----------------
TEST_CASE("adjust_intervals small range", "[adjust_intervals]")
    {
    auto result = adjust_intervals(0.75, 4.2);
    CHECK_THAT(result.first, WithinAbs(0.5, 1e-6));
    CHECK_THAT(result.second, WithinRel(4.5, 1e-6));
    }

TEST_CASE("adjust_intervals medium range", "[adjust_intervals]")
    {
    auto result = adjust_intervals(120.0, 350.0);
    CHECK_THAT(result.first, WithinRel(100.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(350.0, 1e-6));
    }

TEST_CASE("adjust_intervals large range", "[adjust_intervals]")
    {
    auto result = adjust_intervals(1'200'000.0, 2'500'000.0);
    CHECK_THAT(result.first, WithinRel(1'200'000.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(2'600'000.0, 1e-6));
    }

TEST_CASE("adjust_intervals very large range", "[adjust_intervals]")
    {
    auto result = adjust_intervals(50'000'000.0, 180'000'000.0);
    CHECK_THAT(result.first, WithinAbs(40'000'000.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(180'000'000.0, 1e-6));
    }

TEST_CASE("adjust_intervals with negatives", "[adjust_intervals]")
    {
    auto result = adjust_intervals(-12.5, 12.5);
    CHECK_THAT(result.first, WithinRel(-15.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(15.0, 1e-6));
    }

TEST_CASE("adjust_intervals identical endpoints", "[adjust_intervals][degenerate]")
    {
    SECTION("both zero")
        {
        const auto result = adjust_intervals(0.0, 0.0);
        CHECK_THAT(result.first, WithinAbs(0.0, 1e-12));
        CHECK_THAT(result.second, WithinAbs(0.0, 1e-12));
        }
    SECTION("positive value")
        {
        const auto result = adjust_intervals(5.0, 5.0);
        CHECK_THAT(result.first, WithinRel(5.0, 1e-6));
        CHECK_THAT(result.second, WithinRel(5.0, 1e-6));
        }
    SECTION("negative value")
        {
        const auto result = adjust_intervals(-42.0, -42.0);
        CHECK_THAT(result.first, WithinRel(-42.0, 1e-6));
        CHECK_THAT(result.second, WithinRel(-42.0, 1e-6));
        }
    }

TEST_CASE("adjust_intervals triggers 0.2 step branch", "[adjust_intervals][neatStep]")
    {
    // range=1.4, magnitude=1, normalizedRange=1.4 (<=2), neatStep=0.2
    // floor(10.3/0.2)=51, ceil(11.7/0.2)=59
    const auto result = adjust_intervals(10.3, 11.7);
    CHECK_THAT(result.first, WithinRel(10.2, 1e-6));
    CHECK_THAT(result.second, WithinRel(11.8, 1e-6));
    }

TEST_CASE("adjust_intervals triggers 0.5 step branch", "[adjust_intervals][neatStep]")
    {
    // range=3.4, magnitude=1, normalizedRange=3.4 (<=5), neatStep=0.5
    // floor(10.3/0.5)=20, ceil(13.7/0.5)=28
    const auto result = adjust_intervals(10.3, 13.7);
    CHECK_THAT(result.first, WithinRel(10.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(14.0, 1e-6));
    }

TEST_CASE("adjust_intervals triggers 1.0 step branch", "[adjust_intervals][neatStep]")
    {
    // range=6.9, magnitude=1, normalizedRange=6.9 (>5), neatStep=1
    // floor(10.3/1)=10, ceil(17.2/1)=18
    const auto result = adjust_intervals(10.3, 17.2);
    CHECK_THAT(result.first, WithinRel(10.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(18.0, 1e-6));
    }

TEST_CASE("adjust_intervals fractional values needing adjustment", "[adjust_intervals]")
    {
    // range=0.54, magnitude=0.1, normalizedRange=5.4 (>5), neatStep=0.1
    // floor(0.33/0.1)=3, ceil(0.87/0.1)=9
    const auto result = adjust_intervals(0.33, 0.87);
    CHECK_THAT(result.first, WithinRel(0.3, 1e-6));
    CHECK_THAT(result.second, WithinRel(0.9, 1e-6));
    }

TEST_CASE("adjust_intervals cross-zero uneven", "[adjust_intervals]")
    {
    // range=11.9, magnitude=10, normalizedRange=1.19 (<=2), neatStep=2
    // floor(-3.2/2)=-2, ceil(8.7/2)=5
    const auto result = adjust_intervals(-3.2, 8.7);
    CHECK_THAT(result.first, WithinRel(-4.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(10.0, 1e-6));
    }

TEST_CASE("adjust_intervals tiny range between close values", "[adjust_intervals][precision]")
    {
    // range=0.001, magnitude=0.001, normalizedRange=1 (<=2), neatStep=0.0002
    // floor(5.001/0.0002)=25005, ceil(5.002/0.0002)=25010
    const auto result = adjust_intervals(5.001, 5.002);
    CHECK_THAT(result.first, WithinRel(5.001, 1e-6));
    CHECK_THAT(result.second, WithinRel(5.002, 1e-6));
    }

TEST_CASE("adjust_intervals large range with messy values", "[adjust_intervals][large]")
    {
    // range=4444.4, magnitude=1000, normalizedRange=4.4444 (<=5), neatStep=500
    // floor(1234.5/500)=2, ceil(5678.9/500)=12
    const auto result = adjust_intervals(1234.5, 5678.9);
    CHECK_THAT(result.first, WithinRel(1000.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(6000.0, 1e-6));
    }

TEST_CASE("adjust_intervals negative range needing expansion", "[adjust_intervals]")
    {
    // range=75.2, magnitude=10, normalizedRange=7.52 (>5), neatStep=10
    // floor(-87.3/10)=-9, ceil(-12.1/10)=-1
    const auto result = adjust_intervals(-87.3, -12.1);
    CHECK_THAT(result.first, WithinRel(-90.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(-10.0, 1e-6));
    }

TEST_CASE("adjust_intervals large range 0.2 step branch", "[adjust_intervals][large][neatStep]")
    {
    // range=1.3e9, magnitude=1e9, normalizedRange=1.3 (<=2), neatStep=2e8
    // floor(1.5e9/2e8)=7, ceil(2.8e9/2e8)=14
    const auto result = adjust_intervals(1.5e9, 2.8e9);
    CHECK_THAT(result.first, WithinRel(1.4e9, 1e-6));
    CHECK_THAT(result.second, WithinRel(2.8e9, 1e-6));
    }

TEST_CASE("adjust_intervals large range 0.5 step branch", "[adjust_intervals][large][neatStep]")
    {
    // range=3.5e9, magnitude=1e9, normalizedRange=3.5 (<=5), neatStep=5e8
    // floor(1.2e9/5e8)=2, ceil(4.7e9/5e8)=10
    const auto result = adjust_intervals(1.2e9, 4.7e9);
    CHECK_THAT(result.first, WithinRel(1.0e9, 1e-6));
    CHECK_THAT(result.second, WithinRel(5.0e9, 1e-6));
    }

TEST_CASE("adjust_intervals large range 1.0 step branch", "[adjust_intervals][large][neatStep]")
    {
    // range=7.5e9, magnitude=1e9, normalizedRange=7.5 (>5), neatStep=1e9
    // floor(1.2e9/1e9)=1, ceil(8.7e9/1e9)=9
    const auto result = adjust_intervals(1.2e9, 8.7e9);
    CHECK_THAT(result.first, WithinRel(1.0e9, 1e-6));
    CHECK_THAT(result.second, WithinRel(9.0e9, 1e-6));
    }

TEST_CASE("adjust_intervals very large messy values", "[adjust_intervals][large]")
    {
    // range~4.14e12, magnitude=1e12, normalizedRange~4.14 (<=5), neatStep=5e11
    // floor(3.14159e12/5e11)=6, ceil(7.28e12/5e11)=15
    const auto result = adjust_intervals(3.14159e12, 7.28e12);
    CHECK_THAT(result.first, WithinRel(3.0e12, 1e-6));
    CHECK_THAT(result.second, WithinRel(7.5e12, 1e-6));
    }

TEST_CASE("adjust_intervals large negative range", "[adjust_intervals][large]")
    {
    // range=6.2e10, magnitude=1e10, normalizedRange=6.2 (>5), neatStep=1e10
    // floor(-8.5e10/1e10)=-9, ceil(-2.3e10/1e10)=-2
    const auto result = adjust_intervals(-8.5e10, -2.3e10);
    CHECK_THAT(result.first, WithinRel(-9.0e10, 1e-6));
    CHECK_THAT(result.second, WithinRel(-2.0e10, 1e-6));
    }

TEST_CASE("adjust_intervals large cross-zero range", "[adjust_intervals][large]")
    {
    // range=1.17e9, magnitude=1e9, normalizedRange=1.17 (<=2), neatStep=2e8
    // floor(-4.5e8/2e8)=-3, ceil(7.2e8/2e8)=4
    const auto result = adjust_intervals(-4.5e8, 7.2e8);
    CHECK_THAT(result.first, WithinRel(-6.0e8, 1e-6));
    CHECK_THAT(result.second, WithinRel(8.0e8, 1e-6));
    }

// ---------------- fuzz-style boundary property tests ----------------
TEST_CASE("previous_interval and next_interval always bound the value", "[fuzz][intervals]")
    {
    for (double val : { -1234.56, -12.34, -0.1, 0.0, 0.1, 12.34, 1234.56 })
        {
        for (uint8_t n = 1; n <= 5; ++n)
            {
            auto prev = previous_interval(val, n);
            auto next = next_interval(val, n);

            CHECK(prev <= val + 1e-12); // allow epsilon slack
            CHECK(next >= val - 1e-12);
            }
        }
    }

TEST_CASE("adjust_intervals always produces bounding range", "[fuzz][adjust_intervals]")
    {
    for (auto pair : { std::pair{-5.5, 5.5}, std::pair{1.0, 9.9}, std::pair{100.0, 999.9},
                       std::pair{12'000.0, 45'000.0}, std::pair{2'000'000.0, 9'000'000.0} })
        {
        auto result = adjust_intervals(pair.first, pair.second);
        CHECK(result.first <= pair.first + 1e-12);
        CHECK(result.second >= pair.second - 1e-12);
        }
    }

// ---------------- compare_doubles tests ----------------
TEST_CASE("compare_doubles backward compatibility with normal numbers", "[compare_doubles][compat]")
    {
    SECTION("basic equality within tolerance")
        {
        CHECK(compare_doubles(1.0, 1.0000001));
        CHECK(compare_doubles(1.0, 1.0000009));
        CHECK_FALSE(compare_doubles(1.0, 1.000002));
        }

    SECTION("explicit delta respected for normal numbers")
        {
        CHECK(compare_doubles(0.00056, 0.00050, 1e-4));
        CHECK(compare_doubles(5.0, 5.05, 0.1));
        CHECK_FALSE(compare_doubles(5.0, 5.15, 0.1));
        }

    SECTION("negative numbers")
        {
        CHECK(compare_doubles(-1.0, -1.0000001));
        CHECK(compare_doubles(-5.0, -5.05, 0.1));
        CHECK_FALSE(compare_doubles(-1.0, -1.000002));
        }

    SECTION("mixed positive and negative")
        {
        CHECK_FALSE(compare_doubles(-0.0001, 0.0001, 1e-4));
        CHECK_FALSE(compare_doubles(0.00001, -0.00001, 1e-4));
        }
    }

TEST_CASE("compare_doubles with zero", "[compare_doubles][zero]")
    {
    SECTION("zero vs zero")
        {
        CHECK(compare_doubles(0.0, 0.0));
        CHECK(compare_doubles(0.0, -0.0));
        }

    SECTION("zero vs small value within tolerance")
        {
        CHECK(compare_doubles(0.0, 1e-7));
        CHECK(compare_doubles(0.0, -1e-7));
        }

    SECTION("zero vs value outside tolerance")
        {
        CHECK_FALSE(compare_doubles(0.0, 1e-5));
        }
    }

TEST_CASE("compare_doubles with large numbers", "[compare_doubles][large]")
    {
    SECTION("large numbers use absolute tolerance")
        {
        // with default delta 1e-6, large numbers use absolute tolerance
        CHECK(compare_doubles(1000000.0, 1000000.0000001));
        CHECK_FALSE(compare_doubles(1000000.0, 1000000.000002));
        }

    SECTION("large numbers with explicit larger delta")
        {
        CHECK(compare_doubles(1e9, 1e9 + 100, 1000));
        CHECK_FALSE(compare_doubles(1e9, 1e9 + 10000, 1000));
        }
    }

TEST_CASE("compare_doubles threshold boundary", "[compare_doubles][threshold]")
    {
    SECTION("magnitude exactly at delta uses absolute")
        {
        // maxMagnitude = 1e-6, delta = 1e-6, should use absolute
        CHECK(compare_doubles(1e-6, 1e-6 + 1e-7, 1e-6));
        }

    SECTION("magnitude above relative threshold uses absolute")
        {
        // maxMagnitude = 9e-7 > 1e-12 threshold, uses absolute tolerance
        constexpr double val = 9e-7;
        constexpr double delta = 1e-6;
        // effectiveDelta = delta = 1e-6, so small differences are equal
        CHECK(compare_doubles(val, val + 1e-12, delta));
        CHECK(compare_doubles(val, val + 1e-7, delta));
        CHECK_FALSE(compare_doubles(val, val + 1e-5, delta));
        }
    }

TEST_CASE("compare_doubles with NaN and infinity", "[compare_doubles][special]")
    {
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();

    SECTION("NaN comparisons")
        {
        // NaN is not equal to anything, including itself
        CHECK_FALSE(compare_doubles(nan, nan));
        CHECK_FALSE(compare_doubles(nan, 0.0));
        CHECK_FALSE(compare_doubles(0.0, nan));
        CHECK_FALSE(compare_doubles(nan, 1.0));
        }

    SECTION("infinity comparisons")
        {
        CHECK(compare_doubles(inf, inf));
        CHECK(compare_doubles(-inf, -inf));
        CHECK_FALSE(compare_doubles(inf, -inf));
        CHECK_FALSE(compare_doubles(inf, 1e308));
        }
    }

// ---------------- compare_doubles with very small numbers ----------------
TEST_CASE("compare_doubles fails with very small numbers", "[compare_doubles][small]")
    {
    constexpr double epsilon = std::numeric_limits<double>::epsilon();
    constexpr double twoEpsilon = 2.0 * epsilon;

    SECTION("numbers at machine epsilon scale should be distinguishable")
        {
        // These are clearly different numbers, but compare_doubles treats them as equal
        // because their difference is smaller than the default delta (1e-6)
        CHECK_FALSE(compare_doubles(epsilon, twoEpsilon));
        }

    SECTION("zero vs epsilon should not be equal")
        {
        CHECK_FALSE(compare_doubles(0.0, epsilon));
        }

    SECTION("very small but clearly different values")
        {
        // 2e-10 > 1e-12 threshold, uses absolute tolerance, so equal
        CHECK(compare_doubles(1e-10, 2e-10));
        // 5e-12 > 1e-12 threshold, uses absolute tolerance, so equal
        CHECK(compare_doubles(1e-12, 5e-12));
        // 5e-13 < 1e-12 threshold, uses relative tolerance, so different
        CHECK_FALSE(compare_doubles(1e-13, 5e-13));
        }
    }

TEST_CASE("compare_doubles_less fails with very small numbers", "[compare_doubles_less][small]")
    {
    constexpr double epsilon = std::numeric_limits<double>::epsilon();
    constexpr double twoEpsilon = 2.0 * epsilon;

    SECTION("epsilon is clearly less than 2*epsilon")
        {
        CHECK(compare_doubles_less(epsilon, twoEpsilon));
        }

    SECTION("zero is less than epsilon")
        {
        CHECK(compare_doubles_less(0.0, epsilon));
        }

    SECTION("very small but clearly ordered values")
        {
        // above 1e-12 threshold, uses absolute tolerance, within tolerance so not less
        CHECK_FALSE(compare_doubles_less(1e-10, 2e-10));
        CHECK_FALSE(compare_doubles_less(1e-12, 5e-12));
        // below threshold, uses relative tolerance
        CHECK(compare_doubles_less(1e-13, 5e-13));
        CHECK(compare_doubles_less(1e-15, 1e-14));
        }
    }

TEST_CASE("compare_doubles_less_or_equal fails with very small numbers", "[compare_doubles_less_or_equal][small]")
    {
    constexpr double epsilon = std::numeric_limits<double>::epsilon();
    constexpr double twoEpsilon = 2.0 * epsilon;

    SECTION("epsilon is less than or equal to 2*epsilon")
        {
        CHECK(compare_doubles_less_or_equal(epsilon, twoEpsilon));
        }

    SECTION("zero is less than or equal to epsilon")
        {
        CHECK(compare_doubles_less_or_equal(0.0, epsilon));
        }

    SECTION("very small but clearly ordered values")
        {
        CHECK(compare_doubles_less_or_equal(1e-10, 2e-10));
        CHECK(compare_doubles_less_or_equal(1e-12, 5e-12));
        }
    }

TEST_CASE("compare_doubles_greater fails with very small numbers", "[compare_doubles_greater][small]")
    {
    constexpr double epsilon = std::numeric_limits<double>::epsilon();
    constexpr double twoEpsilon = 2.0 * epsilon;

    SECTION("2*epsilon is clearly greater than epsilon")
        {
        CHECK(compare_doubles_greater(twoEpsilon, epsilon));
        }

    SECTION("epsilon is greater than zero")
        {
        CHECK(compare_doubles_greater(epsilon, 0.0));
        }

    SECTION("very small but clearly ordered values")
        {
        // above 1e-12 threshold, uses absolute tolerance, within tolerance so not greater
        CHECK_FALSE(compare_doubles_greater(2e-10, 1e-10));
        CHECK_FALSE(compare_doubles_greater(5e-12, 1e-12));
        // below threshold, uses relative tolerance
        CHECK(compare_doubles_greater(5e-13, 1e-13));
        CHECK(compare_doubles_greater(1e-14, 1e-15));
        }
    }

TEST_CASE("compare_doubles_greater_or_equal fails with very small numbers", "[compare_doubles_greater_or_equal][small]")
    {
    constexpr double epsilon = std::numeric_limits<double>::epsilon();
    constexpr double twoEpsilon = 2.0 * epsilon;

    SECTION("2*epsilon is greater than or equal to epsilon")
        {
        CHECK(compare_doubles_greater_or_equal(twoEpsilon, epsilon));
        }

    SECTION("epsilon is greater than or equal to zero")
        {
        CHECK(compare_doubles_greater_or_equal(epsilon, 0.0));
        }

    SECTION("very small but clearly ordered values")
        {
        CHECK(compare_doubles_greater_or_equal(2e-10, 1e-10));
        CHECK(compare_doubles_greater_or_equal(5e-12, 1e-12));
        }
    }

TEST_CASE("compare_doubles_less backward compatibility", "[compare_doubles_less][compat]")
    {
    SECTION("values within tolerance are not less")
        {
        CHECK_FALSE(compare_doubles_less(1.0, 1.0000001));
        CHECK_FALSE(compare_doubles_less(1.0000001, 1.0));
        }

    SECTION("clearly different values")
        {
        CHECK(compare_doubles_less(1.0, 2.0));
        CHECK_FALSE(compare_doubles_less(2.0, 1.0));
        CHECK(compare_doubles_less(-1.0, 1.0));
        }

    SECTION("explicit delta respected")
        {
        CHECK_FALSE(compare_doubles_less(5.0, 5.05, 0.1));
        CHECK(compare_doubles_less(5.0, 5.15, 0.1));
        }
    }

TEST_CASE("compare_doubles_less with zero", "[compare_doubles_less][zero]")
    {
    SECTION("zero comparisons")
        {
        CHECK_FALSE(compare_doubles_less(0.0, 0.0));
        CHECK(compare_doubles_less(0.0, 1.0));
        CHECK(compare_doubles_less(-1.0, 0.0));
        }
    }

TEST_CASE("compare_doubles_less with NaN and infinity", "[compare_doubles_less][special]")
    {
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();

    SECTION("NaN comparisons always false")
        {
        CHECK_FALSE(compare_doubles_less(nan, 1.0));
        CHECK_FALSE(compare_doubles_less(1.0, nan));
        CHECK_FALSE(compare_doubles_less(nan, nan));
        }

    SECTION("infinity comparisons")
        {
        CHECK(compare_doubles_less(-inf, inf));
        CHECK(compare_doubles_less(1e308, inf));
        CHECK_FALSE(compare_doubles_less(inf, inf));
        CHECK_FALSE(compare_doubles_less(inf, -inf));
        }
    }

TEST_CASE("compare_doubles_greater backward compatibility", "[compare_doubles_greater][compat]")
    {
    SECTION("values within tolerance are not greater")
        {
        CHECK_FALSE(compare_doubles_greater(1.0000001, 1.0));
        CHECK_FALSE(compare_doubles_greater(1.0, 1.0000001));
        }

    SECTION("clearly different values")
        {
        CHECK(compare_doubles_greater(2.0, 1.0));
        CHECK_FALSE(compare_doubles_greater(1.0, 2.0));
        CHECK(compare_doubles_greater(1.0, -1.0));
        }

    SECTION("explicit delta respected")
        {
        CHECK_FALSE(compare_doubles_greater(5.05, 5.0, 0.1));
        CHECK(compare_doubles_greater(5.15, 5.0, 0.1));
        }
    }

TEST_CASE("compare_doubles_greater with NaN and infinity", "[compare_doubles_greater][special]")
    {
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();

    SECTION("NaN comparisons always false")
        {
        CHECK_FALSE(compare_doubles_greater(nan, 1.0));
        CHECK_FALSE(compare_doubles_greater(1.0, nan));
        CHECK_FALSE(compare_doubles_greater(nan, nan));
        }

    SECTION("infinity comparisons")
        {
        CHECK(compare_doubles_greater(inf, -inf));
        CHECK(compare_doubles_greater(inf, 1e308));
        CHECK_FALSE(compare_doubles_greater(inf, inf));
        CHECK_FALSE(compare_doubles_greater(-inf, inf));
        }
    }

TEST_CASE("compare_doubles_less_or_equal comprehensive", "[compare_doubles_less_or_equal][compat]")
    {
    SECTION("equal values")
        {
        CHECK(compare_doubles_less_or_equal(1.0, 1.0));
        CHECK(compare_doubles_less_or_equal(1.0, 1.0000001));
        CHECK(compare_doubles_less_or_equal(1.0000001, 1.0));
        }

    SECTION("clearly less values")
        {
        CHECK(compare_doubles_less_or_equal(1.0, 2.0));
        CHECK_FALSE(compare_doubles_less_or_equal(2.0, 1.0));
        }
    }

TEST_CASE("compare_doubles_greater_or_equal comprehensive", "[compare_doubles_greater_or_equal][compat]")
    {
    SECTION("equal values")
        {
        CHECK(compare_doubles_greater_or_equal(1.0, 1.0));
        CHECK(compare_doubles_greater_or_equal(1.0, 1.0000001));
        CHECK(compare_doubles_greater_or_equal(1.0000001, 1.0));
        }

    SECTION("clearly greater values")
        {
        CHECK(compare_doubles_greater_or_equal(2.0, 1.0));
        CHECK_FALSE(compare_doubles_greater_or_equal(1.0, 2.0));
        }
    }

TEST_CASE("double_less functor", "[double_less]")
    {
    double_less less;

    SECTION("basic ordering")
        {
        CHECK(less(1.0, 2.0));
        CHECK_FALSE(less(2.0, 1.0));
        CHECK_FALSE(less(1.0, 1.0));
        }

    SECTION("values within tolerance")
        {
        CHECK_FALSE(less(1.0, 1.0000001));
        CHECK_FALSE(less(1.0000001, 1.0));
        }

    SECTION("usable with std algorithms")
        {
        std::vector<double> vals = { 3.0, 1.0, 2.0 };
        std::sort(vals.begin(), vals.end(), double_less{});
        CHECK(vals[0] < vals[1]);
        CHECK(vals[1] < vals[2]);
        }
    }

// NOLINTEND
// clang-format on
