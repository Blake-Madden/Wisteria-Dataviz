// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/safe_math.h"
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

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
    
    CHECK((183) == safe_ldiv(2759.5, 15.0).quot);
    CHECK((14) == safe_ldiv(2759.5, 15.0).rem);

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

TEST_CASE("Infinity", "[infinity]")
    {
    CHECK_FALSE(is_infinity(0.017453));
    CHECK(is_infinity(log((float)0)));
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
    CHECK_THAT(result.first, WithinAbs(0.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(5.0, 1e-6));
    }

TEST_CASE("adjust_intervals medium range", "[adjust_intervals]")
    {
    auto result = adjust_intervals(120.0, 350.0);
    CHECK_THAT(result.first, WithinRel(100.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(400.0, 1e-6));
    }

TEST_CASE("adjust_intervals large range", "[adjust_intervals]")
    {
    auto result = adjust_intervals(1'200'000.0, 2'500'000.0);
    CHECK_THAT(result.first, WithinRel(1'000'000.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(3'000'000.0, 1e-6));
    }

TEST_CASE("adjust_intervals very large range", "[adjust_intervals]")
    {
    auto result = adjust_intervals(50'000'000.0, 180'000'000.0);
    CHECK_THAT(result.first, WithinAbs(0.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(200'000'000.0, 1e-6));
    }

TEST_CASE("adjust_intervals with negatives", "[adjust_intervals]")
    {
    auto result = adjust_intervals(-12.5, 12.5);
    CHECK_THAT(result.first, WithinRel(-20.0, 1e-6));
    CHECK_THAT(result.second, WithinRel(20.0, 1e-6));
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

// NOLINTEND
// clang-format on
