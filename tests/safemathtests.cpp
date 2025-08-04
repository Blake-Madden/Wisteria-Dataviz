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

// NOLINTEND
// clang-format on
