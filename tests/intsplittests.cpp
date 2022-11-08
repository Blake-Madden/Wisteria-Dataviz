#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

TEST_CASE("Split integers", "[intsplit]")
    {
    SECTION("Join Split Zero")
        {
        uint32_t lo, hi;
        constexpr uint64_t res = join_int32s(0, 0);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(0) == lo);
        CHECK(static_cast<uint32_t>(0) == hi);
        }
    SECTION("Join Split Normal Values")
        {
        uint32_t lo, hi;
        constexpr uint64_t res = join_int32s(16, 19);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(16) == lo);
        CHECK(static_cast<uint32_t>(19) == hi);
        }
    SECTION("Join Split Normal Values2")
        {
        uint32_t lo, hi;
        constexpr uint64_t res = join_int32s(2, 5);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(2) == lo);
        CHECK(static_cast<uint32_t>(5) == hi);
        }
    SECTION("Join Split Normal Values3")
        {
        uint32_t lo, hi;
        constexpr uint64_t res = join_int32s(97, 52);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(97) == lo);
        CHECK(static_cast<uint32_t>(52) == hi);
        }
    SECTION("Join Split Normal Values4")
        {
        uint32_t lo, hi;
        uint64_t res = join_int32s(360, 756);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(360) == lo);
        CHECK(static_cast<uint32_t>(756) == hi);
        }
    SECTION("Join Split Normal Values5")
        {
        uint32_t lo, hi;
        uint64_t res = join_int32s(7, 9);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(7) == lo);
        CHECK(static_cast<uint32_t>(9) == hi);
        }
    SECTION("Join Split Same Values")
        {
        uint32_t lo, hi;
        constexpr uint64_t res = join_int32s(10, 10);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(10) == lo);
        CHECK(static_cast<uint32_t>(10) == hi);
        }
    SECTION("Join Split Large Values")
        {
        uint32_t lo, hi;
        constexpr uint64_t res = join_int32s(4581, 9842);
        split_int64(res, lo, hi);
        CHECK(static_cast<uint32_t>(4581) == lo);
        CHECK(static_cast<uint32_t>(9842) == hi);
        }
    SECTION("Join Split Double")
        {
        uint32_t lo, hi;
        constexpr double res = static_cast<double>(join_int32s(4581, 9842));
        split_int64(static_cast<uint64_t>(res), lo, hi);
        CHECK(static_cast<uint32_t>(4581) == lo);
        CHECK(static_cast<uint32_t>(9842) == hi);
        }
    }

TEST_CASE("Integer to bool", "[inttobool]")
    {
    CHECK(int_to_bool(1));
    CHECK(int_to_bool(-1));
    CHECK(int_to_bool(9));
    CHECK(int_to_bool(-9));
    CHECK_FALSE(int_to_bool(0));
    CHECK_FALSE(int_to_bool(false));
    CHECK(int_to_bool(true));
    }