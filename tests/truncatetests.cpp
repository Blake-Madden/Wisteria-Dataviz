// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

TEST_CASE("Truncate decimals", "[truncate]")
    {
    SECTION("Integer")
        {
        CHECK_THAT(1.0, WithinRel(truncate_decimal_place(1.59, 0), 1e-6));
        CHECK_THAT(0.0, WithinRel(truncate_decimal_place(0, 10), 1e-6));
        CHECK_THAT(0.0, WithinRel(truncate_decimal_place(0, 0), 1e-6));
        CHECK_THAT(-15.0, WithinRel(truncate_decimal_place(-15.659, 0), 1e-6));
        }
    SECTION("PositiveDecimal")
        {
        CHECK_THAT(1.5, WithinRel(truncate_decimal_place(1.59, 10), 1e-6));
        CHECK_THAT(1.5, WithinRel(truncate_decimal_place(1.51, 10), 1e-6));
        CHECK_THAT(1.5, WithinRel(truncate_decimal_place(1.55, 10), 1e-6));
        CHECK_THAT(1.5, WithinRel(truncate_decimal_place(1.559, 10), 1e-6));
        CHECK_THAT(1.5, WithinRel(truncate_decimal_place(1.551, 10), 1e-6));
        CHECK_THAT(1.55, WithinRel(truncate_decimal_place(1.559, 100), 10e-2));
        CHECK_THAT(1.551, WithinRel(truncate_decimal_place(1.5517, 1000), 10e-3));
        }
    SECTION("NegativeDecimal")
        {
        CHECK_THAT(truncate_decimal_place(-1.59, 10), WithinRel(-1.5, 10e-2));
        CHECK_THAT(truncate_decimal_place(-5.67, 10), WithinRel(-5.6, 10e-2));
        CHECK_THAT(truncate_decimal_place(-15.659, 100), WithinRel(-15.65, 10e-2));
        }
    SECTION("Low Numbers")
        {
        CHECK_THAT(0.0, WithinRel(truncate_decimal_place(0.09, 10), 10e-2));
        CHECK_THAT(0.09, WithinRel(truncate_decimal_place(0.09, 100), 10e-2));
        }
    SECTION("HugeNumbers")
        {
        CHECK_THAT(9895452451.4, WithinRel(truncate_decimal_place(9895452451.44971854, 10), 10e-2));
        CHECK_THAT(9895452451.44, WithinRel(truncate_decimal_place(9895452451.44971854, 100), 10e-2));
        CHECK_THAT(9895452451.449, WithinRel(truncate_decimal_place(9895452451.44971854, 1000), 10e-3));
        CHECK_THAT(9895452451.4497, WithinRel(truncate_decimal_place(9895452451.44971854, 10000), 10e-4));
        CHECK_THAT(9895452451.44971, WithinRel(truncate_decimal_place(9895452451.44971854, 100000), 10e-5));
        CHECK_THAT(9895452451.449718, WithinRel(truncate_decimal_place(9895452451.44971854, 1000000), 10e-6));
        CHECK_THAT(9895452451.4497185, WithinRel(truncate_decimal_place(9895452451.44971854, 10000000),  10e-7));

        CHECK_THAT(-9895452451.4, WithinRel(truncate_decimal_place(-9895452451.44971854, 10), 10e-2));
        CHECK_THAT(-9895452451.44, WithinRel(truncate_decimal_place(-9895452451.44971854, 100), 10e-2));
        CHECK_THAT(-9895452451.449, WithinRel(truncate_decimal_place(-9895452451.44971854, 1000), 10e-3));
        CHECK_THAT(-9895452451.4497, WithinRel(truncate_decimal_place(-9895452451.44971854, 10000), 10e-4));
        CHECK_THAT(-9895452451.44971, WithinRel(truncate_decimal_place(-9895452451.44971854, 100000), 10e-5));
        CHECK_THAT(-9895452451.449718, WithinRel(truncate_decimal_place(-9895452451.44971854, 1000000), 10e-6));
        CHECK_THAT(-9895452451.4497185, WithinRel(truncate_decimal_place(-9895452451.44971854, 10000000), 10e-7));
        }
    }

// NOLINTEND
// clang-format on
