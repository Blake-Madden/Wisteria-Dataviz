#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/safe_math.h"
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

TEST_CASE("Degree to radians", "[geometry]")
    {
    CHECK_THAT(1, WithinRel(geometry::radians_to_degrees(0.017453f), 1e-4));
    CHECK_THAT(7, WithinRel(geometry::radians_to_degrees(0.122173f), 1e-4));
    CHECK_THAT(-300, WithinRel(geometry::radians_to_degrees(-5.235987f), 1e-4));
    CHECK_THAT(0, WithinRel(geometry::radians_to_degrees(0), 1e-4));

    CHECK_THAT(0.017453f, WithinRel(geometry::degrees_to_radians(1), 1e-4));
    CHECK_THAT(0.122173f, WithinRel(geometry::degrees_to_radians(7), 1e-4));
    CHECK_THAT(-5.235987f, WithinRel(geometry::degrees_to_radians(-300), 1e-4));
    CHECK_THAT(0, WithinRel(geometry::degrees_to_radians(0), 1e-4));
    }

TEST_CASE("Right triangles", "[geometry]")
    {
    SECTION("OppositeSide")
        {
        CHECK_THAT(11.979, WithinRel(geometry::right_triangle_height_opposite_angle(15, 53), 1e-3));
        }

    SECTION("OppositeSide2")
        {
        CHECK_THAT(500, WithinRel(geometry::right_triangle_height_adjacent_angle(1000, 60), 1e-3));
        }

    SECTION("LineLength")
        {
        CHECK_THAT(7.8102, WithinRel(geometry::segment_length(std::make_pair(3,2),std::make_pair(9,7)), 1e-3));
        CHECK_THAT(7.8102, WithinRel(geometry::segment_length(std::make_pair(9,7),std::make_pair(3,2)), 1e-3));
        CHECK_THAT(11.66, WithinRel(geometry::segment_length(std::make_pair(-3,5),std::make_pair(7,-1)), 1e-2));
        }

    SECTION("LineAngle")
        {
        CHECK_THAT(0, WithinRel(geometry::segment_angle_degrees(std::make_pair(0,0),std::make_pair(100,0)), 1e-3));
        CHECK_THAT(90, WithinRel(geometry::segment_angle_degrees(std::make_pair(0,0),std::make_pair(0,100)), 1e-3));
        CHECK_THAT(180, WithinRel(geometry::segment_angle_degrees(std::make_pair(0,0),std::make_pair(-100,0)), 1e-3));
        CHECK_THAT(45, WithinRel(geometry::segment_angle_degrees(std::make_pair(0,0),std::make_pair(5,5)), 1e-3));
        CHECK_THAT(-45, WithinRel(geometry::segment_angle_degrees(std::make_pair(0,0),std::make_pair(5,-5)), 1e-3));
        }
    }

TEST_CASE("Rescale", "[geometry]")
    {
    SECTION("Width")
        {
        CHECK_THAT(50, WithinRel(geometry::rescaled_height(std::make_pair(200, 100), 100), 1e-4));
        CHECK_THAT(125, WithinRel(geometry::rescaled_height(std::make_pair(400, 250), 200), 1e-4));
        CHECK_THAT(300, WithinRel(geometry::rescaled_height(std::make_pair(200, 150), 400), 1e-4));
        // dumb new widths should return zero
        CHECK_THAT(0, WithinRel(geometry::rescaled_height(std::make_pair(400, 250), 0), 1e-4));
        CHECK_THAT(0, WithinRel(geometry::rescaled_height(std::make_pair(400, 250), -100), 1e-4));
        }
    SECTION("Height")
        {
        CHECK_THAT(100, WithinRel(geometry::rescaled_width(std::make_pair(200, 100), 50), 1e-4));
        CHECK_THAT(125, WithinRel(geometry::rescaled_width(std::make_pair(250, 400), 200), 1e-4));
        CHECK_THAT(500, WithinRel(geometry::rescaled_width(std::make_pair(250, 400), 800), 1e-4));
        // dumb new widths should return zero
        CHECK_THAT(0, WithinRel(geometry::rescaled_width(std::make_pair(400, 250), 0), 1e-4));
        CHECK_THAT(0, WithinRel(geometry::rescaled_width(std::make_pair(400, 250), -100), 1e-4));
        }
    SECTION("DownscaleNoScale")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(200, 100), std::make_pair(200, 100));
        CHECK_THAT(200, WithinRel(result.first, 1e-4));
        CHECK_THAT(100, WithinRel(result.second, 1e-4));

        result =
            geometry::downscaled_size(std::make_pair(200, 100), std::make_pair(300, 110));
        CHECK_THAT(200, WithinRel(result.first, 1e-4));
        CHECK_THAT(100, WithinRel(result.second, 1e-4));

        // dumb values
        result =
            geometry::downscaled_size(std::make_pair(200, 100), std::make_pair(300, -110));
        CHECK_THAT(0, WithinRel(result.first, 1e-4));
        CHECK_THAT(0, WithinRel(result.second, 1e-4));

        result =
            geometry::downscaled_size(std::make_pair(200, 100), std::make_pair(-300, 110));
        CHECK_THAT(0, WithinRel(result.first, 1e-4));
        CHECK_THAT(0, WithinRel(result.second, 1e-4));
        }
    SECTION("DownscaleByHeight")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(400, 100), std::make_pair(200, 100));
        CHECK_THAT(200, WithinRel(result.first, 1e-4));
        CHECK_THAT(50, WithinRel(result.second, 1e-4));
        }
    SECTION("DownscaleByWidth")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(200, 200), std::make_pair(200, 100));
        CHECK_THAT(100, WithinRel(result.first, 1e-4));
        CHECK_THAT(100, WithinRel(result.second, 1e-4));
        }
    SECTION("DownscaleByBothUseWidth")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(800, 200), std::make_pair(200, 100));
        CHECK_THAT(200, WithinRel(result.first, 1e-4));
        CHECK_THAT(50, WithinRel(result.second, 1e-4));
        }
    SECTION("DownscaleByBothUseHeight")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(400, 800), std::make_pair(200, 100));
        CHECK_THAT(50, WithinRel(result.first, 1e-4));
        CHECK_THAT(100, WithinRel(result.second, 1e-4));
        }
    }
