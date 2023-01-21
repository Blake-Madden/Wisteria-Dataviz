#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/base/axis.h"
#include "../src/base/canvas.h"
#include "../src/base/colorbrewer.h"
#include "../src/base/graphitems.h"
#include "../src/base/points.h"

using namespace Wisteria;
using namespace Wisteria::Colors;
using namespace Wisteria::GraphItems;

TEST_CASE("Data range", "[colorbrewer]")
    {
    std::vector<wxColour> colorSpectrum;
    colorSpectrum.push_back(wxColour(0, 0, 255)); // red
    // even more colors can be entered here
    colorSpectrum.push_back(wxColour(255, 0, 0)); // blue

    std::vector<double> colNums;
    colNums.push_back(50); colNums.push_back(1); colNums.push_back(25.5);
    ColorBrewer cb;
    cb.SetColorScale(colorSpectrum.begin(), colorSpectrum.end());
    auto res = cb.BrewColors(colNums.begin(), colNums.end());

    CHECK(3 == res.size());

    CHECK(255 == res[0].Red());
    CHECK(0 == res[0].Green());
    CHECK(0 == res[0].Blue());

    CHECK(0 == res[1].Red());
    CHECK(0 == res[1].Green());
    CHECK(255 == res[1].Blue());

    CHECK(127 == res[2].Red());
    CHECK(0 == res[2].Green());
    CHECK(127 == res[2].Blue());
    }

TEST_CASE("2 colors", "[colorbrewer]")
    {
    std::vector<wxColour> colorSpectrum;
    colorSpectrum.push_back(wxColour(0, 0, 255)); // red
    colorSpectrum.push_back(wxColour(255, 0, 0)); // blue

    std::vector<double> colNums = { 50, 25.5, 1 };

    ColorBrewer cb;
    cb.SetColorScale(colorSpectrum.begin(), colorSpectrum.end());
    // just for initializing the range
    [[maybe_unused]] auto resu = cb.BrewColors(colNums.begin(), colNums.end());
    auto res = cb.BrewColor(50); // will yield red
    CHECK(255 == res.Red());
    CHECK(0 == res.Green());
    CHECK(0 == res.Blue());

    res = cb.BrewColor(1); // will yield blue
    CHECK(0 == res.Red());
    CHECK(0 == res.Green());
    CHECK(255 == res.Blue());

    res = cb.BrewColor(25.5); // will yield something in between (green)
    CHECK(127 == res.Red());
    CHECK(0 == res.Green());
    }

TEST_CASE("3 colors", "[colorbrewer]")
    {
    std::vector<wxColour> colorSpectrum;
    colorSpectrum.push_back(wxColour(0, 0, 255)); // red
    colorSpectrum.push_back(wxColour(0, 200, 240)); // greenish-blue
    colorSpectrum.push_back(wxColour(255, 0, 0)); // blue

    std::vector<double> colNums;
    colNums.push_back(50); colNums.push_back(25.5); colNums.push_back(1);

    ColorBrewer cb;
    cb.SetColorScale(colorSpectrum.begin(), colorSpectrum.end());
    // just for initializing the range
    [[maybe_unused]] auto resu = cb.BrewColors(colNums.begin(), colNums.end());
    auto res = cb.BrewColor(50); // will yield red
    CHECK(255 == res.Red());
    CHECK(0 == res.Green());
    CHECK(0 == res.Blue());

    res = cb.BrewColor(1); // will yield blue
    CHECK(0 == res.Red());
    CHECK(0 == res.Green());
    CHECK(255 == res.Blue());

    res = cb.BrewColor(25.5); // will yield something in between (green)
    CHECK(0 == res.Red());
    CHECK(200 == res.Green());
    CHECK(240 == res.Blue());
    }

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

TEST_CASE("Split text to fit length", "[label]")
    {
    Label lbl;
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School\nSophomore");

    lbl.SetText(L"High School: Sophomore");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School:\nSophomore");
    // trim padding
    lbl.SetText(L"High School Sophomore ");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School\nSophomore");
    // no delimiters
    lbl.SetText(L"HighSchoolSophomore");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"HighSchoolSophomore");
    // delimiter at end only
    lbl.SetText(L"HighSchoolSophomore ");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"HighSchoolSophomore");
    // string not long enough
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(30);
    CHECK(lbl.GetText() == L"High School Sophomore");
    // dumb suggested length
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(0);
    CHECK(lbl.GetText() == L"High\nSchool\nSophomore");
    // lot of delimiting
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(4);
    CHECK(lbl.GetText() == L"High\nSchool\nSophomore");
    }

TEST_CASE("Split text to fit length with new lines", "[label]")
    {
    Label lbl;
    lbl.SetText(L"High School-Junior\nhigh");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School-\nJunior high");
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