#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/base/polygon.h"

using namespace Wisteria::GraphItems;

TEST_CASE("GetPolygonArea basic polygons")
    {
    struct Point
        {
        double x;
        double y;
        };

    SECTION("Empty polygon has area 0")
        {
        std::vector<Point> polygon{};
        CHECK_THAT(Polygon::GetPolygonArea(polygon), Catch::Matchers::WithinAbs(0.0, 1e-6));
        }

    SECTION("Triangle with base 4 and height 3")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 4.0, 0.0 }, { 0.0, 3.0 } };
        // Area = 1/2 * base * height = 6
        CHECK_THAT(Polygon::GetPolygonArea(polygon), Catch::Matchers::WithinRel(6.0, 1e-6));
        }

    SECTION("Square with side length 2")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 2.0, 0.0 }, { 2.0, 2.0 }, { 0.0, 2.0 } };
        CHECK_THAT(Polygon::GetPolygonArea(polygon), Catch::Matchers::WithinRel(4.0, 1e-6));
        }

    SECTION("Rectangle 3x5")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 5.0, 0.0 }, { 5.0, 3.0 }, { 0.0, 3.0 } };
        CHECK_THAT(Polygon::GetPolygonArea(polygon), Catch::Matchers::WithinRel(15.0, 1e-6));
        }

    SECTION("Pentagon (convex, irregular)")
        {
        std::vector<Point> polygon{
            { 0.0, 0.0 }, { 2.0, 0.0 }, { 3.0, 1.5 }, { 1.0, 3.0 }, { -1.0, 1.5 }
        };
        CHECK_THAT(Polygon::GetPolygonArea(polygon), Catch::Matchers::WithinRel(7.5, 1e-6));
        }

    SECTION("Collinear points yield zero area")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 1.0, 1.0 }, { 2.0, 2.0 } };
        CHECK_THAT(Polygon::GetPolygonArea(polygon), Catch::Matchers::WithinAbs(0.0, 1e-6));
        }

    SECTION("Polygon with reversed point order yields same area")
        {
        std::vector<Point> polygonCW{ { 0.0, 0.0 }, { 4.0, 0.0 }, { 4.0, 3.0 }, { 0.0, 3.0 } };
        std::vector<Point> polygonCCW{ { 0.0, 0.0 }, { 0.0, 3.0 }, { 4.0, 3.0 }, { 4.0, 0.0 } };
        CHECK_THAT(Polygon::GetPolygonArea(polygonCW), Catch::Matchers::WithinRel(12.0, 1e-6));
        CHECK_THAT(Polygon::GetPolygonArea(polygonCCW), Catch::Matchers::WithinRel(12.0, 1e-6));
        }

    SECTION("Concave polygon (L-shape)")
        {
        std::vector<Point> polygon{ { 0.0, 0.0 }, { 4.0, 0.0 }, { 4.0, 3.0 },
                                    { 2.0, 3.0 }, { 2.0, 1.0 }, { 0.0, 1.0 } };
        // L-shape: area = area of 4x3 rectangle (12) - cutout 2x2 rectangle (4) = 8
        CHECK_THAT(Polygon::GetPolygonArea(polygon), Catch::Matchers::WithinRel(8.0, 1e-6));
        }
    }
