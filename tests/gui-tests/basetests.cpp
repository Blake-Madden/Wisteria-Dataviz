#include "../../src/base/axis.h"
#include "../../src/base/canvas.h"
#include "../../src/base/graphitems.h"
#include "../../src/base/label.h"
#include "../../src/base/points.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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
    const wxPoint pts[5] = { wxPoint(98, 48), wxPoint(102, 48), wxPoint(102, 52), wxPoint(98, 52),
                             wxPoint(0, 0) };
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
    const std::array<wxPoint, 5> pts = { wxPoint(98, 48), wxPoint(102, 48), wxPoint(102, 52),
                                         wxPoint(98, 52), wxPoint(0, 0) };
    GraphItems::Polygon pi(GraphItemInfo(L"hello"), pts);
    GraphItems::Polygon pi2 = pi;
    CHECK(pi2.GetText() == L"hello");
    const std::vector<wxPoint> pts2 = pi2.GetPoints();
    CHECK(pts2.size() == 5);
    CHECK(pts2[0] == wxPoint(98, 48));
    CHECK(pts2[1] == wxPoint(102, 48));
    CHECK(pts2[2] == wxPoint(102, 52));
    CHECK(pts2[3] == wxPoint(98, 52));
    CHECK(pts2[4] == wxPoint(0, 0));
    }

TEST_CASE("Points copy CTOR", "[point]")
    {
    const std::array<wxPoint, 5> pts = { wxPoint(98, 48), wxPoint(102, 48), wxPoint(102, 52),
                                         wxPoint(98, 52), wxPoint(0, 0) };
    GraphItems::Polygon pi(GraphItemInfo(L"hello"), pts);
    GraphItems::Polygon pi2(pi);
    CHECK(pi2.GetText() == L"hello");
    const std::vector<wxPoint> pts2 = pi2.GetPoints();
    CHECK(pts2.size() == 5);
    CHECK(pts2[0] == wxPoint(98, 48));
    CHECK(pts2[1] == wxPoint(102, 48));
    CHECK(pts2[2] == wxPoint(102, 52));
    CHECK(pts2[3] == wxPoint(98, 52));
    CHECK(pts2[4] == wxPoint(0, 0));
    }

TEST_CASE("Point copy CTOR", "[point]")
    {
    GraphItems::Point2D pi(GraphItemInfo(L"hello").AnchorPoint(wxPoint(100, 50)), 5);
    GraphItems::Point2D pi2(pi);
    CHECK(pi2.GetAnchorPoint() == wxPoint(100, 50));
    CHECK(5 == pi2.GetRadius());
    CHECK(pi2.GetText() == L"hello");
    }

TEST_CASE("Point assignment", "[point]")
    {
    GraphItems::Point2D pi(GraphItemInfo(L"hello").AnchorPoint(wxPoint(100, 50)), 5);
    GraphItems::Point2D pi2 = pi;
    CHECK(pi2.GetAnchorPoint() == wxPoint(100, 50));
    CHECK(5 == pi2.GetRadius());
    CHECK(pi2.GetText() == L"hello");
    }

TEST_CASE("GraphItemBase Selection Toggle", "[graphitem]")
    {
    class MockItem : public GraphItemBase
        {
      public:
        using GraphItemBase::GraphItemBase;
        using GraphItemBase::SelectObjectAtPoint;

        void Offset([[maybe_unused]] int x, [[maybe_unused]] int y) override {}

        wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const override
            {
            return wxRect(0, 0, 100, 100);
            }

        void SetBoundingBox([[maybe_unused]] const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] double scaling) override
            {
            }

        bool HitTest(wxPoint pt, [[maybe_unused]] wxDC& dc) const override
            {
            return wxRect(0, 0, 100, 100).Contains(pt);
            }

        wxRect Draw([[maybe_unused]] wxDC& dc) const override { return wxRect{}; }
        };

    MockItem item(1.0, L"test");
    item.SetSelectable(true);

    wxMemoryDC dc;

    SECTION("Initial state") { CHECK_FALSE(item.IsSelected()); }

    SECTION("Toggle on")
        {
        item.SelectObjectAtPoint(wxPoint(50, 50), dc);
        CHECK(item.IsSelected());
        }

    SECTION("Toggle off")
        {
        item.SetSelected(true);
        item.SelectObjectAtPoint(wxPoint(50, 50), dc);
        CHECK_FALSE(item.IsSelected());
        }
    }

TEST_CASE("Canvas prepend empty row", "[canvas]")
    {
    auto* canvas = new Canvas{ wxTheApp->GetTopWindow() };
    const auto makeLabel = [canvas](const wxString& text)
    {
        return std::make_shared<Label>(
            GraphItemInfo{ text }.DPIScaling(canvas->GetDPIScaleFactor()));
    };
    const auto topLeft = makeLabel(L"top left");
    const auto topRight = makeLabel(L"top right");
    const auto bottomLeft = makeLabel(L"bottom left");
    canvas->SetFixedObjectsGridSize(2, 2);
    canvas->SetFixedObject(0, 0, topLeft);
    canvas->SetFixedObject(0, 1, topRight);
    canvas->SetFixedObject(1, 0, bottomLeft);

    canvas->PrependEmptyRow();

    SECTION("Grid grows by a row")
        {
        CHECK(canvas->GetFixedObjectsGridSize() == std::pair<size_t, size_t>{ 3, 2 });
        }

    SECTION("New row is at the top and empty")
        {
        CHECK(canvas->GetFixedObject(0, 0) == nullptr);
        CHECK(canvas->GetFixedObject(0, 1) == nullptr);
        }

    SECTION("Existing items move down")
        {
        CHECK(canvas->GetFixedObject(1, 0) == topLeft);
        CHECK(canvas->GetFixedObject(1, 1) == topRight);
        CHECK(canvas->GetFixedObject(2, 0) == bottomLeft);
        CHECK(canvas->GetFixedObject(2, 1) == nullptr);
        }

    SECTION("Row heights are uniform")
        {
        for (size_t row = 0; row < 3; ++row)
            {
            CHECK_THAT(canvas->GetRowInfo(row).GetHeightProportion(),
                       Catch::Matchers::WithinRel(1.0 / 3.0, 1e-6));
            }
        }

    canvas->Destroy();
    }

TEST_CASE("Canvas prepend empty column", "[canvas]")
    {
    auto* canvas = new Canvas{ wxTheApp->GetTopWindow() };
    const auto makeLabel = [canvas](const wxString& text)
    {
        return std::make_shared<Label>(
            GraphItemInfo{ text }.DPIScaling(canvas->GetDPIScaleFactor()));
    };
    const auto topLeft = makeLabel(L"top left");
    const auto topRight = makeLabel(L"top right");
    const auto bottomRight = makeLabel(L"bottom right");
    canvas->SetFixedObjectsGridSize(2, 2);
    canvas->SetFixedObject(0, 0, topLeft);
    canvas->SetFixedObject(0, 1, topRight);
    canvas->SetFixedObject(1, 1, bottomRight);

    canvas->PrependEmptyColumn();

    SECTION("Grid grows by a column")
        {
        CHECK(canvas->GetFixedObjectsGridSize() == std::pair<size_t, size_t>{ 2, 3 });
        }

    SECTION("New column is on the left and empty")
        {
        CHECK(canvas->GetFixedObject(0, 0) == nullptr);
        CHECK(canvas->GetFixedObject(1, 0) == nullptr);
        }

    SECTION("Existing items move right")
        {
        CHECK(canvas->GetFixedObject(0, 1) == topLeft);
        CHECK(canvas->GetFixedObject(0, 2) == topRight);
        CHECK(canvas->GetFixedObject(1, 1) == nullptr);
        CHECK(canvas->GetFixedObject(1, 2) == bottomRight);
        }

    canvas->Destroy();
    }

TEST_CASE("Canvas prepend to an empty grid", "[canvas]")
    {
    auto* canvas = new Canvas{ wxTheApp->GetTopWindow() };
    canvas->ClearFixedObjects();

    SECTION("Prepending a row")
        {
        canvas->PrependEmptyRow();
        CHECK(canvas->GetFixedObjectsGridSize() == std::pair<size_t, size_t>{ 1, 1 });
        CHECK(canvas->GetFixedObject(0, 0) == nullptr);
        }

    SECTION("Prepending a column")
        {
        canvas->PrependEmptyColumn();
        CHECK(canvas->GetFixedObjectsGridSize() == std::pair<size_t, size_t>{ 1, 1 });
        CHECK(canvas->GetFixedObject(0, 0) == nullptr);
        }

    canvas->Destroy();
    }
