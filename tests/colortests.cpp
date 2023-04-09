#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/base/colorbrewer.h"

using namespace Wisteria;
using namespace Wisteria::Colors;

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
