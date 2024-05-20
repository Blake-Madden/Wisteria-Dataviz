// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <map>
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

TEST_CASE("Compare doubles", "[comparedoubles]")
    {
    SECTION("Compare doubles")
        {
        CHECK(compare_doubles(7, 7));
        CHECK_FALSE(compare_doubles(7, 8));
        CHECK(compare_doubles(0.0000, 0.0000));
        CHECK(compare_doubles(0.0005, 0.0005));
        CHECK(compare_doubles(0.00056, 0.00050, 1e-4));
        CHECK_FALSE(compare_doubles(0.00056, 0.00050, 1e-5));
        CHECK(compare_doubles(125.3568, 125.3568));
        CHECK(compare_doubles(125.3567, 125.3568, 1e-3));
        CHECK_FALSE(compare_doubles(125.3567, 125.3568, 1e-4));
        }
    SECTION("Close values")
        {
        CHECK(compare_doubles(0.5, 0.5));
        CHECK(compare_doubles(0.500, 0.500, 1e-3));
        CHECK_FALSE(compare_doubles(0.500, 0.499, 1e-3));
        }
    SECTION("Extreme values")
        {
        CHECK(compare_doubles(std::numeric_limits<double>::max(), std::numeric_limits<double>::max()));
        CHECK(compare_doubles(std::numeric_limits<double>::min(), std::numeric_limits<double>::min()));
        CHECK_FALSE(compare_doubles(std::numeric_limits<double>::min(), std::numeric_limits<double>::min() + .01));
        }
    SECTION("High precisions")
        {
        CHECK(compare_doubles(-597785.54785554, -597785.54780554, 1e-4));
        CHECK(compare_doubles(-597785.54785554, -597785.54785554, 1e-6));
        CHECK(compare_doubles(-597785.54785554, -597785.54785554, 1e-8));
        CHECK_FALSE(compare_doubles(-597785.54785554, -597785.54785449, 1e-6));
        CHECK_FALSE(compare_doubles(-597785.54785554, -597785.54780554, 1e-5));
        CHECK_FALSE(compare_doubles(-597785.54785554, -597786.54780554, 1e-4));
        CHECK_FALSE(compare_doubles(-597785.54785554, -597785.54769554, 1e-4));
        CHECK(compare_doubles(-597785.54785554, -597785.54779554, 1e-4));
        CHECK(compare_doubles(-597785.54784554, -597785.54780554, 1e-4));
        }
    SECTION("Less or equal")
        {
        CHECK(compare_doubles_less_or_equal(2.4, 2.4));//are equal
        CHECK(compare_doubles_less_or_equal(2.4230, 2.4230));//are equal
        CHECK(compare_doubles_less_or_equal(2.42302, 2.42301, 1e-4));//seen as equal at given precision
        CHECK_FALSE(compare_doubles_less_or_equal(2.42302, 2.42301, 1e-5));//seen as greater than
        CHECK(compare_doubles_less_or_equal(2.42301, 2.42302, 1e-4));//seen as equal at given precision
        CHECK(compare_doubles_less_or_equal(2.42301, 2.42302, 1e-5));//seen as less than
        }
    SECTION("Greater")
        {
        CHECK_FALSE(compare_doubles_greater(2.4, 2.4));//are equal
        CHECK_FALSE(compare_doubles_greater(2.4230, 2.4230, 1e-4));//are equal
        CHECK_FALSE(compare_doubles_greater(2.42302, 2.42301, 1e-4));//seen as equal at given precision
        CHECK(compare_doubles_greater(2.423012, 2.423011));//seen as greater than at default precision
        CHECK(compare_doubles_greater(2.42302, 2.42301, 1e-5));//seen as greater than
        CHECK_FALSE(compare_doubles_greater(2.42301, 2.42302, 1e-4));//seen as equal at given precision
        CHECK_FALSE(compare_doubles_greater(2.42301, 2.42302, 1e-5));//seen as less than
        }
    SECTION("Less")
        {
        CHECK_FALSE(compare_doubles_less(2.4, 2.4));//are equal
        CHECK_FALSE(compare_doubles_less(2.4230, 2.4230));//are equal
        CHECK_FALSE(compare_doubles_less(2.42302, 2.42301, 1e-4));//seen as equal at given precision
        CHECK_FALSE(compare_doubles_less(2.42302, 2.42301, 1e-5));//seen as greater than
        CHECK_FALSE(compare_doubles_less(2.42301, 2.42302, 1e-4));//seen as equal at given precision
        CHECK(compare_doubles_less(2.42301, 2.42302));//seen as less than at default precision
        CHECK(compare_doubles_less(2.42301, 2.42302, 1e-5));//seen as less than
        }
    SECTION("Less functor")
        {
        std::map<double, int, double_less> dmap;
        dmap.insert(std::pair<double, int>(2.4, 1));
        dmap.insert(std::pair<double, int>(2.45, 2));
        dmap.insert(std::pair<double, int>(3.85, 3));
        dmap.insert(std::pair<double, int>(4.25, 4));
        dmap.insert(std::pair<double, int>(4.3078, 5));
        CHECK((dmap.find(2.4) != dmap.end() && (dmap.find(2.4)->second == 1)));
        CHECK((dmap.find(2.45) != dmap.end() && (dmap.find(2.45)->second == 2)));
        CHECK((dmap.find(3.85) != dmap.end() && (dmap.find(3.85)->second == 3)));
        CHECK((dmap.find(4.25) != dmap.end() && (dmap.find(4.25)->second == 4)));
        CHECK((dmap.find(4.3078) != dmap.end() && (dmap.find(4.3078)->second == 5)));
        }
    SECTION("Fractional")
        {
        CHECK(has_fractional_part(1.05));
        CHECK(has_fractional_part(1.000005));
        CHECK_FALSE(has_fractional_part(1.00));
        CHECK_FALSE(has_fractional_part(7));
        CHECK(has_fractional_part(1000.05));
        CHECK(has_fractional_part(-5.1));
        }
    }

// NOLINTEND
// clang-format on
