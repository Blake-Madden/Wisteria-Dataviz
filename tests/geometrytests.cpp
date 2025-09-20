// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/safe_math.h"
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;
using namespace geometry;

struct MyPoint
    {
    int x;
    int y;
    [[nodiscard]]
    bool operator==(const MyPoint that) const noexcept { return x == that.x && y == that.y; }
    };

TEST_CASE("geometry::get_polygon_area basic polygons", "[polygon]")
    {
    struct Point
        {
        double x;
        double y;
        };

    SECTION("Empty polygon has area 0")
        {
        std::vector<Point> polygon{};
        CHECK_THAT(geometry::get_polygon_area(polygon), Catch::Matchers::WithinAbs(0.0, 1e-6));
        }

    SECTION("Triangle with base 4 and height 3")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 4.0, 0.0 }, { 0.0, 3.0 } };
        // Area = 1/2 * base * height = 6
        CHECK_THAT(geometry::get_polygon_area(polygon), Catch::Matchers::WithinRel(6.0, 1e-6));
        }

    SECTION("Square with side length 2")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 2.0, 0.0 }, { 2.0, 2.0 }, { 0.0, 2.0 } };
        CHECK_THAT(geometry::get_polygon_area(polygon), Catch::Matchers::WithinRel(4.0, 1e-6));
        }

    SECTION("Rectangle 3x5")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 5.0, 0.0 }, { 5.0, 3.0 }, { 0.0, 3.0 } };
        CHECK_THAT(geometry::get_polygon_area(polygon), Catch::Matchers::WithinRel(15.0, 1e-6));
        }

    SECTION("Pentagon (convex, irregular)")
        {
        std::vector<Point> polygon{
            { 0.0, 0.0 }, { 2.0, 0.0 }, { 3.0, 1.5 }, { 1.0, 3.0 }, { -1.0, 1.5 }
        };
        CHECK_THAT(geometry::get_polygon_area(polygon), Catch::Matchers::WithinRel(7.5, 1e-6));
        }

    SECTION("Collinear points yield zero area")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 1.0, 1.0 }, { 2.0, 2.0 } };
        CHECK_THAT(geometry::get_polygon_area(polygon), Catch::Matchers::WithinAbs(0.0, 1e-6));
        }

    SECTION("Polygon with reversed point order yields same area")
        {
        std::vector<Point> polygonCW{ { 0.0, 0.0 }, { 4.0, 0.0 }, { 4.0, 3.0 }, { 0.0, 3.0 } };
        std::vector<Point> polygonCCW{ { 0.0, 0.0 }, { 0.0, 3.0 }, { 4.0, 3.0 }, { 4.0, 0.0 } };
        CHECK_THAT(geometry::get_polygon_area(polygonCW), Catch::Matchers::WithinRel(12.0, 1e-6));
        CHECK_THAT(geometry::get_polygon_area(polygonCCW), Catch::Matchers::WithinRel(12.0, 1e-6));
        }

    SECTION("Concave polygon (L-shape)")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 4.0, 0.0 }, { 4.0, 3.0 },
                                    { 2.0, 3.0 }, { 2.0, 1.0 }, { 0.0, 1.0 } };
        // L-shape: area = area of 4x3 rectangle (12) - cutout 2x2 rectangle (4) = 8
        CHECK_THAT(geometry::get_polygon_area(polygon), Catch::Matchers::WithinRel(8.0, 1e-6));
        }
    }

TEST_CASE("geometry::is_inside_polygon basic and concave polygons", "[polygon]")
    {
    SECTION("Empty polygon returns false")
        {
        std::vector<MyPoint> polygon{};
        MyPoint pt{ 0, 0 };
        CHECK_FALSE(geometry::is_inside_polygon(pt, polygon));
        }

    SECTION("Single point polygon")
        {
        std::vector<MyPoint> polygon{ { 1, 1 } };
        CHECK(geometry::is_inside_polygon(MyPoint{ 1, 1 }, polygon)); // point on vertex
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 0, 0 }, polygon));
        }

    SECTION("Triangle polygon")
        {
        std::vector<MyPoint> polygon{ { 0, 0 }, { 5, 0 }, { 0, 3 } };

        // Inside
        CHECK(geometry::is_inside_polygon(MyPoint{ 1, 1 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 1 }, polygon));

        // Outside
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 5, 3 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ -1, -1 }, polygon));

        // On vertex
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 0 }, polygon));

        // On edge
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 1, 0 }, polygon));
        }

    SECTION("Rectangle polygon")
        {
        std::vector<MyPoint> polygon{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 0, 3 } };

        // Inside
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 1 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 3, 2 }, polygon));

        // Outside
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 5, 1 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ -1, 2 }, polygon));

        // On vertex
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 4, 3 }, polygon));

        // On edge
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 4, 1 }, polygon));
        }

    SECTION("Concave L-shape polygon (integer points, deterministic)")
        {
        std::vector<MyPoint> polygon{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 2, 3 }, { 2, 1 }, { 0, 1 } };

        // Safe inside points (bottom rectangle)
        CHECK(geometry::is_inside_polygon(MyPoint{ 1, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 3, 0 }, polygon));

        // Safe outside points
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 5, 1 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 0, 4 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ -1, 0 }, polygon));

        // Vertices (considered inside)
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 4, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 3 }, polygon));

        // Edges (considered inside)
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 3, 0 }, polygon));
        }

    SECTION("Rotated square (diamond shape)")
        {
        std::vector<MyPoint> polygon{ { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 } };

        // Inside
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 1, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, -1 }, polygon));

        // Outside
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 3, 0 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 0, 3 }, polygon));

        // On vertex
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 2 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, 0 }, polygon));

        // On edge
        CHECK(geometry::is_inside_polygon(MyPoint{ 1, 1 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ -1, -1 }, polygon));
        }

    SECTION("Concave rotated polygon (diamond with notch, integer points, deterministic)")
        {
        std::vector<MyPoint> polygon{ { 0, 3 },  { 2, 1 },   { 1, 0 },  { 2, -1 },
                                      { 0, -3 }, { -2, -1 }, { -1, 0 }, { -2, 1 } };

        // Safe inside points (main polygon body)
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 0 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 1 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ -1, 1 }, polygon));
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, -1 }, polygon));

        // Points on edges or vertices (considered inside)
        CHECK(geometry::is_inside_polygon(MyPoint{ 1, 0 }, polygon));   // edge
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 3 }, polygon));   // vertex
        CHECK(geometry::is_inside_polygon(MyPoint{ 2, -1 }, polygon));  // vertex
        CHECK(geometry::is_inside_polygon(MyPoint{ 0, 2 }, polygon));   // edge
        CHECK(geometry::is_inside_polygon(MyPoint{ -1, -1 }, polygon)); // edge

        // Outside points (must be false)
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 3, 0 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 0, 4 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ -3, 0 }, polygon));
        CHECK_FALSE(geometry::is_inside_polygon(MyPoint{ 0, -4 }, polygon));
        }
    }

TEST_CASE("geometry::get_polygon_width", "[polygon]")
    {
    SECTION("geometry::get_polygon_width - deterministic, integer polygons")
        {
        // Simple rectangle 4x3
        std::vector<MyPoint> rectangle{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 0, 3 } };
        CHECK(geometry::get_polygon_width(rectangle) ==
              5); // x goes from 0 to 4, plus one extra from loop

        // Square 3x3
        std::vector<MyPoint> square{ { 1, 1 }, { 3, 1 }, { 3, 3 }, { 1, 3 } };
        CHECK(geometry::get_polygon_width(square) == 3); // x from 1 to 3 → width = 3

        // L-shape polygon (concave)
        std::vector<MyPoint> lshape{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 2, 3 }, { 2, 1 }, { 0, 1 } };
        // Width is determined by the farthest x that remains inside polygon
        // For this L-shape, x from 0 to 4 → width = 5
        CHECK(geometry::get_polygon_width(lshape) == 5);

        // Diamond with notch (concave rotated polygon)
        std::vector<MyPoint> diamond{ { 0, 3 },  { 2, 1 },   { 1, 0 },  { 2, -1 },
                                      { 0, -3 }, { -2, -1 }, { -1, 0 }, { -2, 1 } };
        // Width is from -2 to 2 → 5
        CHECK(geometry::get_polygon_width(diamond) == 5);

        // Single point polygon
        std::vector<MyPoint> pointPolygon{ { 0, 0 } };
        CHECK(geometry::get_polygon_width(pointPolygon) == 1);

        // Line polygon (horizontal)
        std::vector<MyPoint> horizontalLine{ { 0, 0 }, { 3, 0 } };
        CHECK(geometry::get_polygon_width(horizontalLine) == 4);
        }
    }

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
