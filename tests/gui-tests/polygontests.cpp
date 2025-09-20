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
