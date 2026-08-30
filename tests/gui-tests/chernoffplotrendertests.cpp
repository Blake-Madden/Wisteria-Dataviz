///////////////////////////////////////////////////////////////////////////////
// Name:        chernoffplotrendertests.cpp
// Purpose:     Characterization tests for ChernoffFacesPlot layout output
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests lock in the observable layout output of ChernoffFacesPlot: the
// number of render objects produced. ChernoffFacesPlot derives from Graph2D and
// draws one cartoon face per observation in a grid, with facial features mapped
// to continuous columns; it exposes no data axes, so the fingerprint is just the
// render object count. The assertions are invariance based; a separate
// exact-value guard compares against a recorded baseline.

#include <catch2/catch_test_macros.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../src/graphs/chernoffplot.h"
#include "graphrenderharness.h"

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace wisteria_render_tests;

namespace
    {
    struct LayoutFingerprint
        {
        size_t m_objectCount{ 0 };

        [[nodiscard]]
        bool operator==(const LayoutFingerprint& that) const
            {
            return m_objectCount == that.m_objectCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "objects=" << m_objectCount;
            return text.str();
            }
        };

    struct ChartSpec
        {
        size_t m_observationCount{ 9 };
        size_t m_featureCount{ 5 };
        bool m_showLabels{ true };
        Gender m_gender{ Gender::Female };
        };

    [[nodiscard]]
    std::shared_ptr<ChernoffFacesPlot> BuildChart(Canvas* canvas, const ChartSpec& spec)
        {
        auto dataset = std::make_shared<Dataset>();
        std::vector<wxString> featureNames;
        for (size_t feat = 0; feat < spec.m_featureCount; ++feat)
            {
            const wxString name{ wxString::Format(L"var%d", static_cast<int>(feat)) };
            featureNames.push_back(name);
            dataset->AddContinuousColumn(name);
            }
        for (size_t obs = 0; obs < spec.m_observationCount; ++obs)
            {
            std::vector<double> values;
            for (size_t feat = 0; feat < spec.m_featureCount; ++feat)
                {
                values.push_back(static_cast<double>((obs * 7 + feat * 13) % 100));
                }
            dataset->AddRow(RowInfo()
                                .Continuous(values)
                                .Id(wxString::Format(L"face%d", static_cast<int>(obs))));
            }

        auto chart = std::make_shared<ChernoffFacesPlot>(canvas);
        chart->SetGender(spec.m_gender);
        chart->ShowLabels(spec.m_showLabels);
        chart->SetData(
            dataset, featureNames[0],
            spec.m_featureCount > 1 ? std::optional<wxString>{ featureNames[1] } : std::nullopt,
            spec.m_featureCount > 2 ? std::optional<wxString>{ featureNames[2] } : std::nullopt,
            spec.m_featureCount > 3 ? std::optional<wxString>{ featureNames[3] } : std::nullopt,
            spec.m_featureCount > 4 ? std::optional<wxString>{ featureNames[4] } : std::nullopt);
        return chart;
        }

    [[nodiscard]]
    LayoutFingerprint LayOutAndCapture(Canvas* canvas,
                                      const std::shared_ptr<ChernoffFacesPlot>& chart)
        {
        LayOutOffscreen(canvas, chart);

        LayoutFingerprint print;
        print.m_objectCount = chart->GetObjectCount();
        return print;
        }

    [[nodiscard]]
    std::vector<std::pair<std::string, ChartSpec>> AllSpecs()
        {
        std::vector<std::pair<std::string, ChartSpec>> specs;
        specs.push_back({ "five-vars", ChartSpec{} });
        specs.push_back({ "one-var", ChartSpec{ .m_featureCount = 1 } });
        specs.push_back({ "male", ChartSpec{ .m_gender = Gender::Male } });
        specs.push_back({ "no-labels", ChartSpec{ .m_showLabels = false } });
        specs.push_back({ "single-face", ChartSpec{ .m_observationCount = 1 } });
        specs.push_back({ "many-faces", ChartSpec{ .m_observationCount = 36 } });
        return specs;
        }

    // Exact layout output recorded from the [.dump] test, keyed by spec name.
    // Update only for a deliberate, reviewed change to ChernoffFacesPlot layout.
    [[nodiscard]]
    LayoutFingerprint ExpectedFingerprint(const std::string& specName)
        {
        // fields: object count
        if (specName == "five-vars" || specName == "one-var" || specName == "male")
            {
            return LayoutFingerprint{ 22 };
            }
        if (specName == "no-labels")
            {
            return LayoutFingerprint{ 13 };
            }
        if (specName == "single-face")
            {
            return LayoutFingerprint{ 6 };
            }
        if (specName == "many-faces")
            {
            return LayoutFingerprint{ 76 };
            }
        FAIL("no recorded baseline for spec '" << specName << "'");
        return LayoutFingerprint{};
        }
    } // namespace

TEST_CASE("ChernoffFacesPlot layout is deterministic and idempotent", "[chernoffplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvasA = MakeCanvas();
            auto chartA = BuildChart(canvasA, spec);
            const auto firstPass = LayOutAndCapture(canvasA, chartA);
            const auto secondPass = LayOutAndCapture(canvasA, chartA);

            auto* canvasB = MakeCanvas();
            auto chartB = BuildChart(canvasB, spec);
            const auto freshPass = LayOutAndCapture(canvasB, chartB);

            INFO("fingerprint: " << firstPass.ToString());
            CHECK(secondPass == firstPass);
            CHECK(freshPass == firstPass);
            }
        }
    }

TEST_CASE("ChernoffFacesPlot layout matches the recorded baseline", "[chernoffplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto actual = LayOutAndCapture(canvas, chart);
            const auto expected = ExpectedFingerprint(name);

            INFO("expected: " << expected.ToString());
            INFO("actual:   " << actual.ToString());
            CHECK(actual == expected);
            }
        }
    }

TEST_CASE("ChernoffFacesPlot emits render objects for every feature recipe", "[chernoffplot][render]")
    {
    for (const auto& [name, spec] : AllSpecs())
        {
        SECTION(name)
            {
            auto* canvas = MakeCanvas();
            auto chart = BuildChart(canvas, spec);
            const auto print = LayOutAndCapture(canvas, chart);

            INFO("fingerprint: " << print.ToString());
            CHECK(print.m_objectCount > 0);
            }
        }
    }

// Best-effort human-readable dump of every fingerprint, written next to the test
// executable.
TEST_CASE("ChernoffFacesPlot characterization dump", "[chernoffplot][render][.dump]")
    {
    const wxString outPath{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() +
                            L"/chernoffplot_characterization.txt" };
    wxFileOutputStream fileStream{ outPath };
    if (!fileStream.IsOk())
        {
        WARN("could not open " << outPath.ToStdString());
        return;
        }
    wxTextOutputStream textStream{ fileStream };

    for (const auto& [name, spec] : AllSpecs())
        {
        auto* canvas = MakeCanvas();
        auto chart = BuildChart(canvas, spec);
        const auto print = LayOutAndCapture(canvas, chart);
        textStream << wxString::Format(L"%s\t%s\n", wxString::FromUTF8(name),
                                      wxString::FromUTF8(print.ToString()));
        }
    SUCCEED("wrote " << outPath.ToStdString());
    }
