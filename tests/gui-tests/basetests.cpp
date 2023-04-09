#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/base/axis.h"
#include "../../src/base/canvas.h"
#include "../../src/base/graphitems.h"
#include "../../src/base/points.h"

using namespace Wisteria;
using namespace Wisteria::GraphItems;

TEST_CASE("Axis point equals", "[axis]")
    {
    CHECK(Axis::AxisPoint(11, L"11", false) == 11);
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) == 10));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) == 12));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) == 11.01));
    // with precision
    CHECK(Axis::AxisPoint(11.45, L"11", false) == 11.45);
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == 11.4));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == 10));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == 12));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == 11.01));

    // against other point
    CHECK(Axis::AxisPoint(11, L"11", false) == Axis::AxisPoint(11, L""));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) == Axis::AxisPoint(10, L"")));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) == Axis::AxisPoint(12, L"")));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) == Axis::AxisPoint(11.01, L"")));
    // with precision
    CHECK(Axis::AxisPoint(11.45, L"11", false) == Axis::AxisPoint(11.45, L""));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == Axis::AxisPoint(11.4, L"")));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == Axis::AxisPoint(10, L"")));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == Axis::AxisPoint(12, L"")));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) == Axis::AxisPoint(11.01, L"")));
    }

TEST_CASE("Axis point not equals", "[axis]")
    {
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) != 11));
    CHECK((Axis::AxisPoint(11, L"11", false) != 10));
    CHECK((Axis::AxisPoint(11, L"11", false) != 12));
    CHECK((Axis::AxisPoint(11, L"11", false) != 11.01));
    // with precision
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) != 11.45));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != 11.4));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != 10));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != 12));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != 11.01));

    // against other point
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) != Axis::AxisPoint(11, L"")));
    CHECK((Axis::AxisPoint(11, L"11", false) != Axis::AxisPoint(10, L"")));
    CHECK((Axis::AxisPoint(11, L"11", false) != Axis::AxisPoint(12, L"")));
    CHECK((Axis::AxisPoint(11, L"11", false) != Axis::AxisPoint(11.01, L"")));
    // with precision
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) != Axis::AxisPoint(11.45, L"")));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != Axis::AxisPoint(11.4, L"")));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != Axis::AxisPoint(10, L"")));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != Axis::AxisPoint(12, L"")));
    CHECK((Axis::AxisPoint(11.45, L"11", false) != Axis::AxisPoint(11.01, L"")));
    }

TEST_CASE("Axis point less than", "[axis]")
    {
    CHECK(Axis::AxisPoint(11, L"11", false) < 11.1);
    CHECK(Axis::AxisPoint(11, L"11", false) < 12);
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) < 11));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) < 10.99));
    // with precision
    CHECK(Axis::AxisPoint(11.45, L"11", false) < 11.5);
    CHECK(Axis::AxisPoint(11.45, L"11", false) < 12);
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) < 11.45));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) < 11));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) < 10.99));

    // against other point
    CHECK(Axis::AxisPoint(11, L"11", false) < Axis::AxisPoint(11.1, L""));
    CHECK(Axis::AxisPoint(11, L"11", false) < Axis::AxisPoint(12, L""));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) < Axis::AxisPoint(11, L"")));
    CHECK_FALSE((Axis::AxisPoint(11, L"11", false) < Axis::AxisPoint(10.99, L"")));
    // with precision
    CHECK(Axis::AxisPoint(11.45, L"11", false) < Axis::AxisPoint(11.5, L""));
    CHECK(Axis::AxisPoint(11.45, L"11", false) < Axis::AxisPoint(12, L""));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) < Axis::AxisPoint(11.45, L"")));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) < Axis::AxisPoint(11, L"")));
    CHECK_FALSE((Axis::AxisPoint(11.45, L"11", false) < Axis::AxisPoint(10.99, L"")));
    }

TEST_CASE("Set point", "[point]")
    {
    const wxPoint pts[5] =
        { wxPoint(98,48), wxPoint(102,48), wxPoint(102,52), wxPoint(98,52), wxPoint(0,0) };
    GraphItems::Polygon pi;
    pi.SetPoints(pts, 5);
    const std::vector<wxPoint> pts2 = pi.GetPoints();
    CHECK(pts2.size() == 5);
    CHECK(pts2[0] == wxPoint(98, 48));
    CHECK(pts2[1] == wxPoint(102, 48));
    CHECK(pts2[2] == wxPoint(102, 52));
    CHECK(pts2[3] == wxPoint(98, 52));
    CHECK(pts2[4] == wxPoint(0, 0));
    }

TEST_CASE("Build polygon", "[point]")
    {
    const wxPoint pts[5] = { wxPoint(98,48), wxPoint(102,48), wxPoint(102,52), wxPoint(98,52), wxPoint(0,0) };
    GraphItems::Polygon pi(GraphItemInfo(wxT("hello")), pts, 5);
    GraphItems::Polygon pi2 = pi;
    CHECK(pi2.GetText() == wxT("hello"));
    const std::vector<wxPoint> pts2 = pi2.GetPoints();
    CHECK(pts2.size() == 5);
    CHECK(pts2[0] == wxPoint(98,48));
    CHECK(pts2[1] == wxPoint(102,48));
    CHECK(pts2[2] == wxPoint(102,52));
    CHECK(pts2[3] == wxPoint(98,52));
    CHECK(pts2[4] == wxPoint(0,0));
    }

TEST_CASE("Points copy CTOR", "[point]")
    {
    const wxPoint pts[5] = { wxPoint(98,48), wxPoint(102,48), wxPoint(102,52), wxPoint(98,52), wxPoint(0,0) };
    GraphItems::Polygon pi(GraphItemInfo(wxT("hello")), pts, 5);
    GraphItems::Polygon pi2(pi);
    CHECK(pi2.GetText() == wxT("hello"));
    const std::vector<wxPoint> pts2 = pi2.GetPoints();
    CHECK(pts2.size() == 5);
    CHECK(pts2[0] == wxPoint(98,48));
    CHECK(pts2[1] == wxPoint(102,48));
    CHECK(pts2[2] == wxPoint(102,52));
    CHECK(pts2[3] == wxPoint(98,52));
    CHECK(pts2[4] == wxPoint(0,0));
    }

TEST_CASE("Point copy CTOR", "[point]")
    {
    GraphItems::Point2D pi(GraphItemInfo(wxT("hello")).AnchorPoint(wxPoint(100, 50)), 5);
    GraphItems::Point2D pi2(pi);
    CHECK(pi2.GetAnchorPoint() == wxPoint(100,50));
    CHECK(5 == pi2.GetRadius());
    CHECK(pi2.GetText() == L"hello");
    }

TEST_CASE("Point assignment", "[point]")
    {
    GraphItems::Point2D pi(GraphItemInfo(L"hello").AnchorPoint(wxPoint(100,50)), 5);
    GraphItems::Point2D pi2 = pi;
    CHECK(pi2.GetAnchorPoint() == wxPoint(100,50));
    CHECK(5 == pi2.GetRadius());
    CHECK(pi2.GetText() == L"hello");
    }