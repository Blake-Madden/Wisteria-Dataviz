// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/safe_math.h"
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;
using namespace geometry;

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

// ---------------- find_point tests ----------------

TEST_CASE("find_point cardinal directions", "[find_point]")
    {
    auto origin = std::make_pair(0.0, 0.0);

    auto east = find_point(0.0, 10.0, origin);
    CHECK_THAT(east.first, WithinRel(10.0, 1e-6));
    CHECK_THAT(east.second, WithinAbs(0.0, 1e-6));

    auto north = find_point(90.0, 5.0, origin);
    CHECK_THAT(north.first, WithinAbs(0.0, 1e-6));
    CHECK_THAT(north.second, WithinRel(5.0, 1e-6));

    auto west = find_point(180.0, 3.0, origin);
    CHECK_THAT(west.first, WithinRel(-3.0, 1e-6));
    CHECK_THAT(west.second, WithinAbs(0.0, 1e-6));

    auto south = find_point(270.0, 7.0, origin);
    CHECK_THAT(south.first, WithinAbs(0.0, 1e-6));
    CHECK_THAT(south.second, WithinRel(-7.0, 1e-6));
    }

TEST_CASE("find_point with non-origin", "[find_point]")
    {
    auto origin = std::make_pair(2.0, 3.0);

    auto northeast = find_point(45.0, std::sqrt(2.0), origin);
    CHECK_THAT(northeast.first, WithinRel(3.0, 1e-6));
    CHECK_THAT(northeast.second, WithinRel(4.0, 1e-6));
    }

// ---------------- middle_point_horizontal_spline tests ----------------

TEST_CASE("middle_point_horizontal_spline upwards", "[middle_point_horizontal_spline]")
    {
    auto pt1 = std::make_pair(0.0, 0.0);
    auto pt2 = std::make_pair(10.0, 10.0);

    auto [mx, my, up] = middle_point_horizontal_spline(pt1, pt2);

    CHECK_THAT(mx, WithinRel(5.0, 1e-6));
    CHECK(my > 5.0); // raised above midpoint
    CHECK(up == false); // spline goes upwards means flag = false
    }

TEST_CASE("middle_point_horizontal_spline downwards", "[middle_point_horizontal_spline]")
    {
    auto pt1 = std::make_pair(0.0, 10.0);
    auto pt2 = std::make_pair(10.0, 0.0);

    auto [mx, my, up] = middle_point_horizontal_spline(pt1, pt2);

    CHECK_THAT(mx, WithinRel(5.0, 1e-6));
    CHECK(my < 5.0); // lowered below midpoint
    CHECK(up == true); // spline goes downwards means flag = true
    }

// ---------------- middle_point_horizontal_upward_spline tests ----------------

TEST_CASE("middle_point_horizontal_upward_spline", "[middle_point_horizontal_upward_spline]")
    {
    auto pt1 = std::make_pair(0.0, 0.0);
    auto pt2 = std::make_pair(10.0, 10.0);

    auto [mx, my] = middle_point_horizontal_upward_spline(pt1, pt2);

    CHECK_THAT(mx, WithinRel(5.0, 1e-6));
    CHECK(my < 5.0); // shifted upward (negative y adjustment)
    }

// ---------------- middle_point_horizontal_downward_spline tests ----------------

TEST_CASE("middle_point_horizontal_downward_spline", "[middle_point_horizontal_downward_spline]")
    {
    auto pt1 = std::make_pair(0.0, 0.0);
    auto pt2 = std::make_pair(10.0, 10.0);

    auto [mx, my] = middle_point_horizontal_downward_spline(pt1, pt2);

    CHECK_THAT(mx, WithinRel(5.0, 1e-6));
    CHECK(my > 5.0); // shifted downward (positive y adjustment)
    }

TEST_CASE("find_point with negative angles", "[find_point]")
    {
    auto origin = std::make_pair(0.0, 0.0);

    auto neg90 = find_point(-90.0, 5.0, origin);
    CHECK_THAT(neg90.first, WithinAbs(0.0, 1e-6));
    CHECK_THAT(neg90.second, WithinRel(-5.0, 1e-6));

    auto neg45 = find_point(-45.0, std::sqrt(2.0), origin);
    CHECK_THAT(neg45.first, WithinRel(1.0, 1e-6));
    CHECK_THAT(neg45.second, WithinRel(-1.0, 1e-6));
    }

TEST_CASE("find_point with negative lengths", "[find_point]")
    {
    auto origin = std::make_pair(0.0, 0.0);

    // length -5 at 0° should point left instead of right
    auto westViaNegLength = find_point(0.0, -5.0, origin);
    CHECK_THAT(westViaNegLength.first, WithinRel(-5.0, 1e-6));
    CHECK_THAT(westViaNegLength.second, WithinAbs(0.0, 1e-6));

    // length -5 at 90° should point downward instead of upward
    auto southViaNegLength = find_point(90.0, -5.0, origin);
    CHECK_THAT(southViaNegLength.first, WithinAbs(0.0, 1e-6));
    CHECK_THAT(southViaNegLength.second, WithinRel(-5.0, 1e-6));
    }

TEST_CASE("find_point with wraparound angles", "[find_point]")
    {
    auto origin = std::make_pair(0.0, 0.0);

    // 450° is equivalent to 90°
    auto wrap450 = find_point(450.0, 5.0, origin);
    auto norm90 = find_point(90.0, 5.0, origin);
    CHECK_THAT(wrap450.first, WithinAbs(norm90.first, 1e-6));
    CHECK_THAT(wrap450.second, WithinRel(norm90.second, 1e-6));

    // -270° is also equivalent to 90°
    auto wrapNeg270 = find_point(-270.0, 5.0, origin);
    CHECK_THAT(wrapNeg270.first, WithinAbs(norm90.first, 1e-6));
    CHECK_THAT(wrapNeg270.second, WithinRel(norm90.second, 1e-6));
    }

TEST_CASE("middle_point_horizontal_spline symmetry and wrap cases", "[middle_point_horizontal_spline]")
    {
    // Symmetry: swapping points should not affect the spline midpoint
    auto pt1 = std::make_pair(0.0, 0.0);
    auto pt2 = std::make_pair(10.0, 10.0);

    auto [x1, y1, up1] = middle_point_horizontal_spline(pt1, pt2);
    auto [x2, y2, up2] = middle_point_horizontal_spline(pt2, pt1);

    CHECK_THAT(x1, WithinRel(x2, 1e-6));
    CHECK_THAT(y1, WithinRel(y2, 1e-6));
    CHECK(up1 == up2);

    // Horizontal line: midpoint should be exactly halfway and spline is "flat"
    auto flatPt1 = std::make_pair(0.0, 5.0);
    auto flatPt2 = std::make_pair(10.0, 5.0);

    auto [flatX, flatY, flatUp] = middle_point_horizontal_spline(flatPt1, flatPt2);
    CHECK_THAT(flatX, WithinRel(5.0, 1e-6));
    CHECK_THAT(flatY, WithinRel(5.0, 1e-6));
    CHECK(flatUp == true); // by definition, equal heights -> spline goes "up"
    }

TEST_CASE("middle_point_horizontal_upward_spline symmetry", "[middle_point_horizontal_upward_spline]")
    {
    auto pt1 = std::make_pair(0.0, 0.0);
    auto pt2 = std::make_pair(10.0, 10.0);

    auto [x1, y1] = middle_point_horizontal_upward_spline(pt1, pt2);
    auto [x2, y2] = middle_point_horizontal_upward_spline(pt2, pt1);

    CHECK_THAT(x1, WithinRel(x2, 1e-6));
    CHECK_THAT(y1, WithinRel(y2, 1e-6));

    // Flat case
    auto flatPt1 = std::make_pair(0.0, 5.0);
    auto flatPt2 = std::make_pair(10.0, 5.0);

    auto [flatX, flatY] = middle_point_horizontal_upward_spline(flatPt1, flatPt2);
    CHECK_THAT(flatX, WithinRel(5.0, 1e-6));
    CHECK_THAT(flatY, WithinRel(5.0, 1e-6));
    }

TEST_CASE("middle_point_horizontal_downward_spline symmetry", "[middle_point_horizontal_downward_spline]")
    {
    auto pt1 = std::make_pair(0.0, 0.0);
    auto pt2 = std::make_pair(10.0, 10.0);

    auto [x1, y1] = middle_point_horizontal_downward_spline(pt1, pt2);
    auto [x2, y2] = middle_point_horizontal_downward_spline(pt2, pt1);

    CHECK_THAT(x1, WithinRel(x2, 1e-6));
    CHECK_THAT(y1, WithinRel(y2, 1e-6));

    // Flat case
    auto flatPt1 = std::make_pair(0.0, 5.0);
    auto flatPt2 = std::make_pair(10.0, 5.0);

    auto [flatX, flatY] = middle_point_horizontal_downward_spline(flatPt1, flatPt2);
    CHECK_THAT(flatX, WithinRel(5.0, 1e-6));
    CHECK_THAT(flatY, WithinRel(5.0, 1e-6));
    }

// NOLINTEND
// clang-format on
