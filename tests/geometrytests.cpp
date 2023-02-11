#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/safe_math.h"
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

TEST_CASE("Point inside", "[geometry]")
    {
    SECTION("Points distance")
        {
        CHECK_THAT(38.89, WithinRel(geometry::distance_between_points(std::make_pair(52, 30), std::make_pair(79, 2)), 1e-2));
        CHECK_THAT(357.68, WithinRel(geometry::distance_between_points(std::make_pair(101, 56), std::make_pair(3, 400)), 1e-2));
        CHECK_THAT(0, WithinRel(geometry::distance_between_points(std::make_pair(25, 25), std::make_pair(25, 25)), 1e-2));
        CHECK_THAT(10, WithinRel(geometry::distance_between_points(std::make_pair(25, 25), std::make_pair(35, 25)), 1e-2));
        CHECK_THAT(10, WithinRel(geometry::distance_between_points(std::make_pair(25, 25), std::make_pair(25, 15)), 1e-2));
        CHECK_THAT(9.43, WithinRel(geometry::distance_between_points(std::make_pair(25, 25), std::make_pair(30, 17)), 1e-2));
        CHECK_THAT(10.29, WithinRel(geometry::distance_between_points(std::make_pair(25, 25), std::make_pair(30, 34)), 1e-2));
        }
    SECTION("Point inside circle")
        {
        CHECK(geometry::is_point_inside_circle(std::make_pair(25,25), 10, std::make_pair(25, 25)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25,25), 10, std::make_pair(15, 25)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25,25), 10, std::make_pair(35, 25)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25,25), 10, std::make_pair(25, 15)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25,25), 10, std::make_pair(25, 35)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(20, 20)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(30, 20)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(20, 30)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(30, 33)));
        CHECK(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(30, 17)));

        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(30, 34)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(30, 15)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(14, 25)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(36, 25)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(25, 14)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(25, 36)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(0, 0)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(50, 50)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(-25, 25)));
        CHECK_FALSE(geometry::is_point_inside_circle(std::make_pair(25, 25), 10, std::make_pair(25, -25)));
        }
    }

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
    SECTION("Opposite side")
        {
        CHECK_THAT(11.979, WithinRel(geometry::right_triangle_height_opposite_angle(15, 53), 1e-3));
        }

    SECTION("Opposite side 2")
        {
        CHECK_THAT(500, WithinRel(geometry::right_triangle_height_adjacent_angle(1000, 60), 1e-3));
        }

    SECTION("Line length")
        {
        CHECK_THAT(7.8102, WithinRel(geometry::segment_length(std::make_pair(3,2),std::make_pair(9,7)), 1e-3));
        CHECK_THAT(7.8102, WithinRel(geometry::segment_length(std::make_pair(9,7),std::make_pair(3,2)), 1e-3));
        CHECK_THAT(11.66, WithinRel(geometry::segment_length(std::make_pair(-3,5),std::make_pair(7,-1)), 1e-2));
        }

    SECTION("Line angle")
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
    SECTION("Downscale no scale")
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
    SECTION("Downscale by height")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(400, 100), std::make_pair(200, 100));
        CHECK_THAT(200, WithinRel(result.first, 1e-4));
        CHECK_THAT(50, WithinRel(result.second, 1e-4));
        }
    SECTION("Downscale by width")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(200, 200), std::make_pair(200, 100));
        CHECK_THAT(100, WithinRel(result.first, 1e-4));
        CHECK_THAT(100, WithinRel(result.second, 1e-4));
        }
    SECTION("Downscale by both use width")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(800, 200), std::make_pair(200, 100));
        CHECK_THAT(200, WithinRel(result.first, 1e-4));
        CHECK_THAT(50, WithinRel(result.second, 1e-4));
        }
    SECTION("Downscale by both use height")
        {
        std::pair<double, double> result =
            geometry::downscaled_size(std::make_pair(400, 800), std::make_pair(200, 100));
        CHECK_THAT(50, WithinRel(result.first, 1e-4));
        CHECK_THAT(100, WithinRel(result.second, 1e-4));
        }
    }
