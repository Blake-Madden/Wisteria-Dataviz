// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/mathematics.h"
#include <limits>

using namespace Catch::Matchers;

TEST_CASE("Even", "[even]")
    {
    SECTION("Integer")
        {
        CHECK(is_even(2) == true);
        CHECK(is_even(27514578) == true);

        even<int> evenFunctor;
        CHECK(evenFunctor(2) == true);
        CHECK(evenFunctor(27514578) == true);
        }
    SECTION("Negative integer")
        {
        CHECK(is_even(-2) == true);
        CHECK(is_even(-27514578) == true);

        even<int> evenFunctor;
        CHECK(evenFunctor(-2) == true);
        CHECK(evenFunctor(-27514578) == true);
        }
    SECTION("Float")
        {
        CHECK(is_even(static_cast<float>(2.021787)) == true);
        CHECK(is_even(static_cast<float>(27514578.20248)) == true);

        even<float> evenFunctor;
        CHECK(evenFunctor(static_cast<float>(2.021787)) == true);
        CHECK(evenFunctor(static_cast<float>(27514578.20248)) == true);
        }
    SECTION("Negative float")
        {
        CHECK(is_even(static_cast<float>(-2.021787)) == true);
        CHECK(is_even(static_cast<float>(-27514578.20248)) == true);

        even<float> evenFunctor;
        CHECK(evenFunctor(static_cast<float>(-2.021787)) == true);
        CHECK(evenFunctor(static_cast<float>(-27514578.20248)) == true);
        }
    SECTION("Double")
        {
        CHECK(is_even(static_cast<double>(2.021787)) == true);
        CHECK(is_even(static_cast<double>(27514578.20248)) == true);

        even<double> evenFunctor;
        CHECK(evenFunctor(static_cast<double>(2.021787)) == true);
        CHECK(evenFunctor(static_cast<double>(27514578.20248)) == true);
        }
    SECTION("Negative double")
        {
        CHECK(is_even(static_cast<double>(-2.021787)) == true);
        CHECK(is_even(static_cast<double>(-27514578.20248)) == true);

        even<double> evenFunctor;
        CHECK(evenFunctor(static_cast<double>(-2.021787)) == true);
        CHECK(evenFunctor(static_cast<double>(-27514578.20248)) == true);
        }
    SECTION("Odd")
        {
        CHECK(is_even(3) == false);
        CHECK(is_even(27514573) == false);
        }
    SECTION("Negative odd")
        {
        CHECK(is_even(-3) == false);
        CHECK(is_even(-27514573) == false);
        }
    SECTION("Float")
        {
        CHECK(is_even(3.021787) == false);
        CHECK(is_even(27514573.20248) == false);
        }
    SECTION("Negative float")
        {
        CHECK(is_even(-3.021787) == false);
        CHECK(is_even(-27514573.20248) == false);
        }
    SECTION("NaN and infinity")
        {
        constexpr double nan = std::numeric_limits<double>::quiet_NaN();
        constexpr double inf = std::numeric_limits<double>::infinity();

        CHECK(is_even(nan) == false);
        CHECK(is_even(inf) == false);
        CHECK(is_even(-inf) == false);
        }
    SECTION("Large magnitude doubles")
        {
        // values far beyond the range of std::int64_t; the old implementation
        // cast through int64_t and would overflow (UB) for values like these
        CHECK(is_even(1e20) == true);
        CHECK(is_even(std::numeric_limits<double>::max()) == true);
        }
    SECTION("Exact integer-valued doubles")
        {
        CHECK(is_even(4.0) == true);
        CHECK(is_even(3.0) == false);
        CHECK(is_even(-4.0) == true);
        CHECK(is_even(-3.0) == false);
        }
    SECTION("Negative zero")
        {
        CHECK(is_even(-0.0) == true);
        }
    }

// NOLINTEND
// clang-format on
