// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

TEST_CASE("Round", "[round]")
	{
    SECTION("Round")
        {
        CHECK_THAT(2, WithinRel(round_to_integer(1.5), 1e-6));
        CHECK_THAT(2, WithinRel(round_to_integer(1.6), 1e-6));
        CHECK_THAT(1, WithinRel(round_to_integer(1.4), 1e-6));
        CHECK_THAT(0, WithinRel(round_to_integer(0.0), 1e-6));
        CHECK_THAT(-12, WithinRel(round_to_integer(-11.6), 1e-6));
        CHECK_THAT(-12, WithinRel(round_to_integer(-11.5), 1e-6));
        CHECK_THAT(-11, WithinRel(round_to_integer(-11.4), 1e-6));
        CHECK_THAT(12, WithinRel(round_to_integer(11.6), 1e-6));
        CHECK_THAT(12, WithinRel(round_to_integer(11.5), 1e-6));
        CHECK_THAT(11, WithinRel(round_to_integer(11.4), 1e-6));
        }
    SECTION("Fringe values")
        {
        CHECK_THAT(5.3, WithinRel(round_decimal_place(5.3499, 10), 1e-6));
        CHECK_THAT(5.4, WithinRel(round_decimal_place(5.3500, 10), 1e-6));
        CHECK_THAT(-5.3, WithinRel(round_decimal_place(-5.3499, 10), 1e-6));
        CHECK_THAT(-5.4, WithinRel(round_decimal_place(-5.3500, 10), 1e-6));
        }
    SECTION("Decimals")
        {
        CHECK_THAT(2.0, WithinRel(round_decimal_place(1.541254, 0), 1e-6));
        CHECK_THAT(1.5, WithinRel(round_decimal_place(1.541254, 10), 1e-6));
        CHECK_THAT(1.6, WithinRel(round_decimal_place(1.551254, 10), 1e-6));
        CHECK_THAT(1.6, WithinRel(round_decimal_place(1.5931254, 10), 1e-6));
        CHECK_THAT(1.6, WithinRel(round_decimal_place(1.5931254, 10), 1e-6));
        CHECK_THAT(0, WithinRel(round_decimal_place(0, 10), 1e-6));
        CHECK_THAT(0, WithinRel(round_decimal_place(0, 0), 1e-6));
        CHECK_THAT(1.6, WithinRel(round_decimal_place(1.5931254, 10), 10e-2));
        CHECK_THAT(1.59, WithinRel(round_decimal_place(1.5931254, 100), 10e-2));
        CHECK_THAT(1.60, WithinRel(round_decimal_place(1.5951254, 100), 10e-2));
        CHECK_THAT(1.596, WithinRel(round_decimal_place(1.59561254, 1000), 10e-3));
        CHECK_THAT(1.5956, WithinRel(round_decimal_place(1.59561254, 10000), 10e-4));
        CHECK_THAT(1.59561, WithinRel(round_decimal_place(1.59561454, 100000), 10e-5));
        CHECK_THAT(1.595615, WithinRel(round_decimal_place(1.59561454, 1000000), 10e-6));

        CHECK_THAT(-2.0, WithinRel(round_decimal_place(-1.541254, 0), 1e-6));
        CHECK_THAT(-1.5, WithinRel(round_decimal_place(-1.541254, 10), 1e-6));
        CHECK_THAT(-1.6, WithinRel(round_decimal_place(-1.551254, 10), 1e-6));
        CHECK_THAT(-1.6, WithinRel(round_decimal_place(-1.5931254, 10), 1e-6));
        CHECK_THAT(-1.6, WithinRel(round_decimal_place(-1.5931254, 10), 1e-6));
        CHECK_THAT(0, WithinRel(round_decimal_place(0, 10), 1e-6));
        CHECK_THAT(-1.6, WithinRel(round_decimal_place(-1.5931254, 10), 10e-2));
        CHECK_THAT(-1.59, WithinRel(round_decimal_place(-1.5931254, 100), 10e-2));
        CHECK_THAT(-1.60, WithinRel(round_decimal_place(-1.5951254, 100), 10e-2));
        CHECK_THAT(-1.596, WithinRel(round_decimal_place(-1.59561254, 1000), 10e-3));
        CHECK_THAT(-1.5956, WithinRel(round_decimal_place(-1.59561254, 10000), 10e-4));
        CHECK_THAT(-1.59561, WithinRel(round_decimal_place(-1.59561454, 100000), 10e-5));
        CHECK_THAT(-1.595615, WithinRel(round_decimal_place(-1.59561454, 1000000), 10e-6));
        }
    SECTION("Decimal huge numbers")
        {
        CHECK_THAT(9895452451.4, WithinRel(round_decimal_place(9895452451.44971854, 10), 10e-2));
        CHECK_THAT(9895452451.45, WithinRel(round_decimal_place(9895452451.44971854, 100), 10e-2));
        CHECK_THAT(9895452451.450, WithinRel(round_decimal_place(9895452451.44971854, 1000), 10e-3));
        CHECK_THAT(9895452451.4497, WithinRel(round_decimal_place(9895452451.44971854, 10000), 10e-4));
        CHECK_THAT(9895452451.44972, WithinRel(round_decimal_place(9895452451.44971854, 100000), 10e-5));
        CHECK_THAT(9895452451.449719, WithinRel(round_decimal_place(9895452451.44971854, 1000000), 10e-6));
        CHECK_THAT(9895452451.4497185, WithinRel(round_decimal_place(9895452451.44971854, 10000000), 10e-7));

        CHECK_THAT(-9895452451.4, WithinRel(round_decimal_place(-9895452451.44971854, 10), 10e-2));
        CHECK_THAT(-9895452451.45, WithinRel(round_decimal_place(-9895452451.44971854, 100), 10e-2));
        CHECK_THAT(-9895452451.450, WithinRel(round_decimal_place(-9895452451.44971854, 1000), 10e-3));
        CHECK_THAT(-9895452451.4497, WithinRel(round_decimal_place(-9895452451.44971854, 10000), 10e-4));
        CHECK_THAT(-9895452451.44972, WithinRel(round_decimal_place(-9895452451.44971854, 100000), 10e-5));
        CHECK_THAT(-9895452451.449719, WithinRel(round_decimal_place(-9895452451.44971854, 1000000), 10e-6));
        CHECK_THAT(-9895452451.4497185, WithinRel(round_decimal_place(-9895452451.44971854, 10000000), 10e-7));
        }
	}

// NOLINTEND
// clang-format on
