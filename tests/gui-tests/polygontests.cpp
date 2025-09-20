#include "../../src/base/polygon.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Wisteria::GraphItems;

struct MyPoint
    {
    int x;
    int y;
    [[nodiscard]]
    bool operator==(const MyPoint that) const noexcept { return x == that.x && y == that.y; }
    };

TEST_CASE("GetPolygonArea basic polygons", "[polygon]")
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

TEST_CASE("Polygon::GetPolygonWidth", "[polygon]")
    {
    SECTION("Polygon::GetPolygonWidth - deterministic, integer polygons")
        {
        // Simple rectangle 4x3
        std::vector<wxPoint> rectangle{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 0, 3 } };
        CHECK(Polygon::GetPolygonWidth(rectangle) ==
              5); // x goes from 0 to 4, plus one extra from loop

        // Square 3x3
        std::vector<wxPoint> square{ { 1, 1 }, { 3, 1 }, { 3, 3 }, { 1, 3 } };
        CHECK(Polygon::GetPolygonWidth(square) == 3); // x from 1 to 3 → width = 3

        // L-shape polygon (concave)
        std::vector<wxPoint> lshape{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 2, 3 }, { 2, 1 }, { 0, 1 } };
        // Width is determined by the farthest x that remains inside polygon
        // For this L-shape, x from 0 to 4 → width = 5
        CHECK(Polygon::GetPolygonWidth(lshape) == 5);

        // Diamond with notch (concave rotated polygon)
        std::vector<wxPoint> diamond{ { 0, 3 },  { 2, 1 },   { 1, 0 },  { 2, -1 },
                                      { 0, -3 }, { -2, -1 }, { -1, 0 }, { -2, 1 } };
        // Width is from -2 to 2 → 5
        CHECK(Polygon::GetPolygonWidth(diamond) == 5);

        // Single point polygon
        std::vector<wxPoint> pointPolygon{ { 0, 0 } };
        CHECK(Polygon::GetPolygonWidth(pointPolygon) == 1);

        // Line polygon (horizontal)
        std::vector<wxPoint> horizontalLine{ { 0, 0 }, { 3, 0 } };
        CHECK(Polygon::GetPolygonWidth(horizontalLine) == 4);
        }
    }

TEST_CASE("Polygon::IsRectInsidePolygon", "[polygon]")
    {
    // ----------------------------
    // Convex rectangle polygon
    // ----------------------------
    std::vector<wxPoint> rectPolygon{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 0, 3 } };

    SECTION("Convex polygon - fully inside / on edges / fully outside")
        {
        wxRect rect1{ 1, 1, 2, 1 }; // fully inside
        CHECK(Polygon::IsRectInsidePolygon(rect1, rectPolygon));

        wxRect rect2{ 0, 0, 4, 3 }; // coincides with edges
        CHECK(Polygon::IsRectInsidePolygon(rect2, rectPolygon));

        wxRect rect3{ -1, 0, 2, 2 }; // partially outside
        CHECK_FALSE(Polygon::IsRectInsidePolygon(rect3, rectPolygon));

        wxRect rect4{ 5, 2, 1, 1 }; // completely outside
        CHECK_FALSE(Polygon::IsRectInsidePolygon(rect4, rectPolygon));
        }

    // ----------------------------
    // Concave L-shape polygon
    // ----------------------------
    std::vector<wxPoint> lshape{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 2, 3 }, { 2, 1 }, { 0, 1 } };

    SECTION("Concave L-shape polygon - deterministic")
        {
        wxRect rect5{ 1, 0, 1, 1 }; // inside bottom rectangle
        CHECK(Polygon::IsRectInsidePolygon(rect5, lshape));

        wxRect rect6{ 3, 2, 1, 1 }; // inside top-right rectangle
        CHECK(Polygon::IsRectInsidePolygon(rect6, lshape));

        wxRect rect7{ 0, 0, 2, 3 }; // partially in concave notch → outside
        CHECK_FALSE(Polygon::IsRectInsidePolygon(rect7, lshape));

        wxRect rect8{ 5, 2, 1, 1 }; // fully outside
        CHECK_FALSE(Polygon::IsRectInsidePolygon(rect8, lshape));
        }

    // ----------------------------
    // Concave rotated diamond polygon
    // ----------------------------
    std::vector<wxPoint> diamond{ { 0, 3 },  { 2, 1 },   { 1, 0 },  { 2, -1 },
                                  { 0, -3 }, { -2, -1 }, { -1, 0 }, { -2, 1 } };

    SECTION("Concave rotated diamond polygon - deterministic")
        {
        wxRect rect9{ 0, 0, 1, 1 }; // fully inside main body
        CHECK(Polygon::IsRectInsidePolygon(rect9, diamond));

        wxRect rect10{ 1, 0, 1, 1 }; // inside convex region
        CHECK(Polygon::IsRectInsidePolygon(rect10, diamond));

        wxRect rect11{ 0, -1, 1, 1 }; // fully inside bottom-left convex region
        CHECK(Polygon::IsRectInsidePolygon(rect11, diamond));

        wxRect rect12{ 3, 3, 1, 1 }; // fully outside
        CHECK_FALSE(Polygon::IsRectInsidePolygon(rect12, diamond));
        }
    }

TEST_CASE("Polygon::GetPolygonBoundingBox", "[polygon]")
    {
    // ----------------------------
    // Convex rectangle polygon
    // ----------------------------
    std::vector<wxPoint> rectPolygon{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 0, 3 } };

    SECTION("Convex rectangle")
        {
        const auto bbox = Polygon::GetPolygonBoundingBox(rectPolygon);
        CHECK(bbox.GetTopLeft() == wxPoint{ 0, 0 });
        CHECK(bbox.GetBottomRight() == wxPoint{ 4, 3 });
        }

    // ----------------------------
    // Concave L-shape polygon
    // ----------------------------
    std::vector<wxPoint> lshape{ { 0, 0 }, { 4, 0 }, { 4, 3 }, { 2, 3 }, { 2, 1 }, { 0, 1 } };

    SECTION("Concave L-shape")
        {
        const auto bbox = Polygon::GetPolygonBoundingBox(lshape);
        CHECK(bbox.GetTopLeft() == wxPoint{ 0, 0 });
        CHECK(bbox.GetBottomRight() == wxPoint{ 4, 3 });
        }

    // ----------------------------
    // Concave rotated diamond polygon
    // ----------------------------
    std::vector<wxPoint> diamond{ { 0, 3 },  { 2, 1 },   { 1, 0 },  { 2, -1 },
                                  { 0, -3 }, { -2, -1 }, { -1, 0 }, { -2, 1 } };

    SECTION("Concave rotated diamond")
        {
        const auto bbox = Polygon::GetPolygonBoundingBox(diamond);
        CHECK(bbox.GetTopLeft() == wxPoint{ -2, -3 });
        CHECK(bbox.GetBottomRight() == wxPoint{ 2, 3 });
        }

    // ----------------------------
    // Rotated rectangle polygon
    // ----------------------------
    std::vector<wxPoint> rotatedRect{ { 1, 1 }, { 4, 0 }, { 5, 3 }, { 2, 4 } };

    SECTION("Rotated rectangle")
        {
        const auto bbox = Polygon::GetPolygonBoundingBox(rotatedRect);
        CHECK(bbox.GetTopLeft() == wxPoint{ 1, 0 });
        CHECK(bbox.GetBottomRight() == wxPoint{ 5, 4 });
        }
    }

TEST_CASE("Polygon::IsRectInsideRect", "[polygon]")
    {
    // Outer rectangle
    wxRect outer{ 0, 0, 4, 3 }; // x=0..4, y=0..3

    SECTION("Inner rectangle fully inside outer rectangle")
        {
        wxRect inner{ 1, 1, 2, 1 }; // x=1..3, y=1..2
        CHECK(Polygon::IsRectInsideRect(inner, outer));
        }

    SECTION("Inner rectangle identical to outer rectangle")
        {
        wxRect innerSame{ 0, 0, 4, 3 }; // same corners
        CHECK(Polygon::IsRectInsideRect(innerSame, outer));

        wxRect innerShifted{ 0, 0, 3, 4 }; // corners do not match exactly
        CHECK_FALSE(Polygon::IsRectInsideRect(innerShifted, outer));
        }

    SECTION("Inner rectangle partially outside outer rectangle")
        {
        wxRect innerPartial{ 3, 2, 3, 2 }; // x=3..6, y=2..4 → partially outside
        CHECK_FALSE(Polygon::IsRectInsideRect(innerPartial, outer));
        }

    SECTION("Inner rectangle completely outside outer rectangle")
        {
        wxRect innerOutside{ 5, 5, 1, 1 };
        CHECK_FALSE(Polygon::IsRectInsideRect(innerOutside, outer));
        }

    SECTION("Inner rectangle larger than outer rectangle")
        {
        wxRect innerLarger{ -1, -1, 6, 5 };
        CHECK_FALSE(Polygon::IsRectInsideRect(innerLarger, outer));
        }
    }

TEST_CASE("Polygon::GetPercentInsideRect", "[polygon]")
    {
    wxRect outer{ 0, 0, 4, 3 }; // x=0..3, y=0..2 inclusive

    SECTION("Inner rectangle fully inside outer rectangle")
        {
        wxRect inner{ 1, 1, 2, 1 }; // fully inside
        const auto [wPct, hPct] = Polygon::GetPercentInsideRect(inner, outer);
        CHECK_THAT(wPct, Catch::Matchers::WithinAbs(1.0, 1e-6));
        CHECK_THAT(hPct, Catch::Matchers::WithinAbs(1.0, 1e-6));
        }

    SECTION("Inner rectangle identical to outer rectangle")
        {
        wxRect inner{ 0, 0, 4, 3 };
        const auto [wPct, hPct] = Polygon::GetPercentInsideRect(inner, outer);
        CHECK_THAT(wPct, Catch::Matchers::WithinAbs(1.0, 1e-6));
        CHECK_THAT(hPct, Catch::Matchers::WithinAbs(1.0, 1e-6));
        }

    SECTION("Inner rectangle partially outside on right and bottom")
        {
        wxRect inner{ 2, 1, 3, 2 }; // x=2..4, y=1..2
        const auto [wPct, hPct] = Polygon::GetPercentInsideRect(inner, outer);
        CHECK_THAT(wPct, Catch::Matchers::WithinAbs(2.0 / 3.0, 1e-6));
        CHECK_THAT(hPct, Catch::Matchers::WithinAbs(1.0, 1e-6));
        }

    SECTION("Inner rectangle partially outside on left and top")
        {
        wxRect inner{ -1, -1, 3, 3 }; // x=-1..1, y=-1..1
        const auto [wPct, hPct] = Polygon::GetPercentInsideRect(inner, outer);
        CHECK_THAT(wPct, Catch::Matchers::WithinAbs(2.0 / 3.0, 1e-6));
        CHECK_THAT(hPct, Catch::Matchers::WithinAbs(2.0 / 3.0, 1e-6));
        }

    SECTION("Inner rectangle fully outside to the right and below")
        {
        wxRect inner{ 5, 5, 2, 2 };
        const auto [wPct, hPct] = Polygon::GetPercentInsideRect(inner, outer);
        CHECK_THAT(wPct, Catch::Matchers::WithinAbs(0.0, 1e-6));
        CHECK_THAT(hPct, Catch::Matchers::WithinAbs(0.0, 1e-6));
        }

    SECTION("Inner rectangle larger than outer rectangle")
        {
        wxRect inner{ -1, -1, 6, 5 }; // x=-1..4, y=-1..3
        const auto [wPct, hPct] = Polygon::GetPercentInsideRect(inner, outer);
        CHECK_THAT(wPct, Catch::Matchers::WithinAbs(4.0 / 6.0, 1e-6));
        CHECK_THAT(hPct, Catch::Matchers::WithinAbs(3.0 / 5.0, 1e-6));
        }
    }
