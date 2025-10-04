#include "../../src/base/axis.h"
#include "../../src/data/jitter.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::GraphItems;

// Catch2 tests for Jitter (Whitesmiths indentation, CHECK-heavy, CalcSpread(freq))

// Helpers
static wxPoint P(int x, int y) { return wxPoint{ x, y }; }

static std::vector<wxPoint> make_same_slot_y(int base_x, int y, size_t n)
    {
    std::vector<wxPoint> v;
    v.reserve(n);
    for (size_t i = 0; i < n; ++i)
        {
        v.emplace_back(P(base_x, y));
        }
    return v;
    }

static std::vector<wxPoint> make_same_slot_x(int x, int base_y, size_t n)
    {
    std::vector<wxPoint> v;
    v.reserve(n);
    for (size_t i = 0; i < n; ++i)
        {
        v.emplace_back(P(x, base_y));
        }
    return v;
    }

static frequency_set<wxCoord> freq_from_y(const std::vector<wxPoint>& pts)
    {
    frequency_set<wxCoord> f;
    for (const auto& p : pts)
        {
        f.insert(p.y);
        }
    return f;
    }

static frequency_set<wxCoord> freq_from_x(const std::vector<wxPoint>& pts)
    {
    frequency_set<wxCoord> f;
    for (const auto& p : pts)
        {
        f.insert(p.x);
        }
    return f;
    }

TEST_CASE("Jitter: Y-dominant alternates sides and respects width")
    {
    Jitter jitter(AxisType::LeftYAxis);

    const double width = 24.0;
    jitter.SetJitterWidth(width);

    auto pts = make_same_slot_y(/*base_x*/ 100, /*y*/ 42, /*n*/ 9);

    auto freq = freq_from_y(pts);
    jitter.CalcSpread(freq);

    std::vector<wxPoint> out;
    out.reserve(pts.size());
    for (size_t i = 0; i < pts.size(); ++i)
        {
        auto p = pts[i];
        const bool moved = jitter.JitterPoint(p);
        out.push_back(p);

        if (i == 0)
            {
            CHECK_FALSE(moved);
            }
        else
            {
            CHECK(moved);
            }
        }

    for (const auto& p : out)
        {
        CHECK(p.y == 42);
        }

    const int x0 = 100;
    CHECK(out[0].x == x0);

    std::vector<int> dx;
    dx.reserve(out.size());
    for (auto& p : out)
        {
        dx.push_back(p.x - x0);
        }

    CHECK(dx[1] < 0);
    CHECK(dx[2] > 0);
    CHECK(dx[3] < 0);
    CHECK(dx[4] > 0);

    for (size_t i = 1; i < dx.size(); ++i)
        {
        CHECK(std::abs(dx[i]) <= static_cast<int>(std::ceil(width)));
        }
    }

TEST_CASE("Jitter: X-dominant jitters vertically and respects width")
    {
    Jitter jitter(AxisType::BottomXAxis);
    jitter.SetJitterWidth(18.0);

    auto pts = make_same_slot_x(/*x*/ 55, /*base_y*/ 200, /*n*/ 6);

    auto freq = freq_from_x(pts);
    jitter.CalcSpread(freq);

    std::vector<wxPoint> out;
    out.reserve(pts.size());
    for (size_t i = 0; i < pts.size(); ++i)
        {
        auto p = pts[i];
        const bool moved = jitter.JitterPoint(p);
        out.push_back(p);

        if (i == 0)
            {
            CHECK_FALSE(moved);
            }
        else
            {
            CHECK(moved);
            }
        }

    for (const auto& p : out)
        {
        CHECK(p.x == 55);
        }

    CHECK(out[1].y < 200);
    CHECK(out[2].y > 200);

    for (size_t i = 1; i < out.size(); ++i)
        {
        CHECK(std::abs(out[i].y - 200) <= static_cast<int>(std::ceil(18.0)));
        }
    }

TEST_CASE("Jitter: width <= 0 disables movement")
    {
    Jitter jitter(AxisType::LeftYAxis);
    jitter.SetJitterWidth(0.0); // quick width guard

    auto pts = make_same_slot_y(10, 5, 3);

    auto freq = freq_from_y(pts);
    jitter.CalcSpread(freq);

    for (auto& p : pts)
        {
        const wxPoint before = p;
        const bool moved = jitter.JitterPoint(p);
        CHECK_FALSE(moved);
        CHECK(p.x == before.x);
        CHECK(p.y == before.y);
        }
    }

TEST_CASE("Jitter: ResetJitterData starts a fresh batch (first stays centered again)")
    {
    Jitter jitter(AxisType::LeftYAxis);
    jitter.SetJitterWidth(12.0);

    // batch 1
    auto batch1 = make_same_slot_y(300, 777, 2);
    auto f1 = freq_from_y(batch1);
    jitter.CalcSpread(f1);

    CHECK_FALSE(jitter.JitterPoint(batch1[0]));
    CHECK(jitter.JitterPoint(batch1[1]));

    jitter.ResetJitterData();

    // batch 2 (fresh frequency context)
    auto batch2 = make_same_slot_y(300, 777, 2);
    auto f2 = freq_from_y(batch2);
    jitter.CalcSpread(f2);

    CHECK_FALSE(jitter.JitterPoint(batch2[0]));
    CHECK(jitter.JitterPoint(batch2[1]));
    }

TEST_CASE("Jitter: Deterministic for same input order")
    {
    Jitter a(AxisType::LeftYAxis);
    Jitter b(AxisType::LeftYAxis);

    const double width = 20.0;
    a.SetJitterWidth(width);
    b.SetJitterWidth(width);

    auto pts1 = std::vector<wxPoint>{ P(50, 10), P(50, 10), P(60, 11), P(60, 11), P(60, 11) };
    auto pts2 = pts1;

    auto f = freq_from_y(pts1); // same freq for both
    a.ResetJitterData();
    b.ResetJitterData();
    a.CalcSpread(f);
    b.CalcSpread(f);

    for (auto& p : pts1)
        {
        a.JitterPoint(p);
        }
    for (auto& p : pts2)
        {
        b.JitterPoint(p);
        }

    CHECK(pts1.size() == pts2.size());
    for (size_t i = 0; i < pts1.size(); ++i)
        {
        CHECK(pts1[i].x == pts2[i].x);
        CHECK(pts1[i].y == pts2[i].y);
        }
    }

TEST_CASE("Jitter: Mixed slots only jitter where collisions occur")
    {
    Jitter jitter(AxisType::LeftYAxis);
    jitter.SetJitterWidth(16.0);

    std::vector<wxPoint> pts = { P(200, 100), P(200, 100), P(200, 101) };

    auto freq = freq_from_y(pts);
    jitter.CalcSpread(freq);

    CHECK_FALSE(jitter.JitterPoint(pts[0])); // first at y=100
    CHECK(jitter.JitterPoint(pts[1]));       // second at y=100

    const wxPoint before = pts[2]; // lone y=101 shouldn't move
    const bool moved = jitter.JitterPoint(pts[2]);
    CHECK_FALSE(moved);
    CHECK(pts[2].x == before.x);
    CHECK(pts[2].y == before.y);
    }
