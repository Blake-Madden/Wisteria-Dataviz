///////////////////////////////////////////////////////////////////////////////
// Name:        ishikawadiagramrendertests.cpp
// Purpose:     Layout tests for IshikawaDiagram
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// These tests check the number of render objects that IshikawaDiagram produces
// for a given set of branches, causes, and sub-causes, and that bad column
// names are reported.

#include "../../src/graphs/ishikawadiagram.h"
#include "graphrenderharness.h"
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <memory>
#include <vector>

using namespace Wisteria;
using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace wisteria_render_tests;

namespace
    {
    // the four (hidden) axes, the tail fin, spine, the fish head (with its eye and mouth)
    // and the effect label, then per branch: a rib and (optionally) a label,
    // per cause or sub-cause: a twig and a label, and per cause: a dot
    constexpr size_t FIXED_OBJECTS{ 8 };

    [[nodiscard]]
    std::vector<GroupIdType> BuildCodes(const std::vector<wxString>& values,
                                        ColumnWithStringTable::StringTableType& table)
        {
        std::map<wxString, GroupIdType> codes;
        std::vector<GroupIdType> result;
        for (const auto& value : values)
            {
            if (codes.find(value) == codes.cend())
                {
                const auto nextCode{ static_cast<GroupIdType>(codes.size()) };
                codes.insert({ value, nextCode });
                table.insert({ nextCode, value });
                }
            result.push_back(codes.at(value));
            }
        return result;
        }

    [[nodiscard]]
    std::shared_ptr<Dataset> BuildDataset()
        {
        const std::vector<std::vector<wxString>> columns{
            { L"Training", L"Fatigue", wxString{} }, // Man
            { L"Speed", wxString{}, wxString{} },    // Machine
            { wxString{}, wxString{}, wxString{} },  // Material (empty)
            { L"Basic", L"Advanced", wxString{} }    // Training sub-causes
        };
        const std::vector<wxString> names{ L"Man", L"Machine", L"Material", L"TrainingDetails" };

        std::vector<std::vector<GroupIdType>> codes;
        auto dataset = std::make_shared<Dataset>();
        for (size_t col = 0; col < columns.size(); ++col)
            {
            ColumnWithStringTable::StringTableType table;
            codes.push_back(BuildCodes(columns[col], table));
            dataset->AddCategoricalColumn(names[col], table);
            }
        for (size_t row = 0; row < 3; ++row)
            {
            std::vector<GroupIdType> rowCodes;
            for (const auto& colCodes : codes)
                {
                rowCodes.push_back(colCodes[row]);
                }
            dataset->AddRow(RowInfo().Categoricals(rowCodes).Id(
                wxString::Format(L"obs%d", static_cast<int>(row))));
            }
        return dataset;
        }

    [[nodiscard]]
    size_t ObjectCount(const std::vector<IshikawaDiagram::Branch>& branches, const bool showEmpty,
                       const bool showLabels)
        {
        auto* canvas = MakeCanvas();
        auto chart = std::make_shared<IshikawaDiagram>(canvas);
        chart->ShowEmptyBranches(showEmpty);
        chart->ShowBranchLabels(showLabels);
        chart->SetData(BuildDataset(), L"Surface flaws", branches);
        LayOutOffscreen(canvas, chart);
        return chart->GetObjectCount();
        }
    } // namespace

TEST_CASE("IshikawaDiagram draws a twig and label per cause", "[ishikawadiagram][render]")
    {
    // man: 2 causes, machine: 1 cause; each branch has a rib and a label
    const size_t count{ ObjectCount(
        { { L"Personnel", L"Man", {} }, { L"Machines", L"Machine", {} } }, true, true) };
    CHECK(count == FIXED_OBJECTS + (2 * 2) + (3 * 3));
    }

TEST_CASE("IshikawaDiagram draws sub-causes", "[ishikawadiagram][render]")
    {
    const size_t withSubs{ ObjectCount(
        { { L"Personnel", L"Man", { { L"Training", L"TrainingDetails" } } } }, true, true) };
    const size_t withoutSubs{ ObjectCount({ { L"Personnel", L"Man", {} } }, true, true) };
    // 2 sub-causes, each a twig and a label, plus the connector joining them to the cause
    CHECK(withSubs == withoutSubs + (2 * 2) + 1);
    }

TEST_CASE("IshikawaDiagram can hide empty branches and branch labels", "[ishikawadiagram][render]")
    {
    const std::vector<IshikawaDiagram::Branch> branches{ { L"Personnel", L"Man", {} },
                                                         { L"Materials", L"Material", {} } };
    const size_t all{ ObjectCount(branches, true, true) };
    const size_t nonEmpty{ ObjectCount(branches, false, true) };
    const size_t noLabels{ ObjectCount(branches, true, false) };
    CHECK(all == nonEmpty + 2);
    CHECK(noLabels == all - 2);
    }

TEST_CASE("IshikawaDiagram treats a branch without a column as empty", "[ishikawadiagram][render]")
    {
    const std::vector<IshikawaDiagram::Branch> branches{ { L"Personnel", L"Man", {} },
                                                         { L"Spare", wxString{}, {} } };
    const size_t all{ ObjectCount(branches, true, true) };
    const size_t nonEmpty{ ObjectCount(branches, false, true) };
    CHECK(all == nonEmpty + 2);
    }

TEST_CASE("IshikawaDiagram parses and formats typed causes", "[ishikawadiagram]")
    {
    const auto parsed = IshikawaDiagram::ParseConstants(L"Speed \"Operator training\"  Age");
    REQUIRE(parsed.size() == 3);
    CHECK(parsed[0] == L"Speed");
    CHECK(parsed[1] == L"Operator training");
    CHECK(parsed[2] == L"Age");
    CHECK(IshikawaDiagram::FormatConstants(parsed) == L"Speed \"Operator training\" Age");
    CHECK(IshikawaDiagram::ParseConstants(wxString{}).empty());
    }

TEST_CASE("IshikawaDiagram default branch names", "[ishikawadiagram]")
    {
    CHECK(IshikawaDiagram::GetDefaultBranchLabel(0) == _(L"People"));
    CHECK(IshikawaDiagram::IsDefaultBranchName(0, _(L"People")));
    CHECK(IshikawaDiagram::IsDefaultBranchName(0, L"man"));
    CHECK_FALSE(IshikawaDiagram::IsDefaultBranchName(1, L"Man"));
    CHECK(IshikawaDiagram::IsDefaultBranchName(1, L"MACHINE"));
    CHECK_FALSE(
        IshikawaDiagram::IsDefaultBranchName(IshikawaDiagram::DEFAULT_BRANCH_COUNT, wxString{}));
    }

TEST_CASE("IshikawaDiagram typed causes round-trip", "[ishikawadiagram]")
    {
    const std::vector<wxString> causes{ L"Speed",           L"Operator training",
                                        L"6\" bit",         L"\"",
                                        L"Tab\tseparated",  L" padded ",
                                        L"say \"hi\" twice" };
    const auto parsed = IshikawaDiagram::ParseConstants(IshikawaDiagram::FormatConstants(causes));
    REQUIRE(parsed.size() == causes.size());
    for (size_t i = 0; i < causes.size(); ++i)
        {
        CHECK(parsed[i] == causes[i]);
        }
    }

TEST_CASE("IshikawaDiagram draws typed causes and sub-causes", "[ishikawadiagram][render]")
    {
    // 2 typed causes with 1 typed sub-cause: 3 twigs (each a line and a label),
    // a dot for each of the 2 causes, plus a connector for the sub-cause
    IshikawaDiagram::Branch branch{ L"Personnel", wxString{}, {}, { L"Speed", L"Age" } };
    branch.m_subCauses.push_back({ L"Speed", wxString{}, { L"Gear" } });
    const size_t count{ ObjectCount({ branch }, true, true) };
    CHECK(count == FIXED_OBJECTS + 2 + (3 * 2) + 2 + 1);
    }

TEST_CASE("IshikawaDiagram is unchanged when SetData fails", "[ishikawadiagram][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<IshikawaDiagram>(canvas);
    chart->SetData(BuildDataset(), L"First", { { L"Personnel", L"Man", {} } });
    REQUIRE(chart->GetBranches().size() == 1);

    CHECK_THROWS_AS(chart->SetData(BuildDataset(), L"Second",
                                   { { L"Personnel", L"Man", {} }, { L"Bad", L"Nope", {} } }),
                    std::runtime_error);
    CHECK(chart->GetEffect() == L"First");
    REQUIRE(chart->GetBranches().size() == 1);
    CHECK(chart->GetBranches()[0].m_column == L"Man");
    }

TEST_CASE("IshikawaDiagram reports missing columns", "[ishikawadiagram][render]")
    {
    auto* canvas = MakeCanvas();
    auto chart = std::make_shared<IshikawaDiagram>(canvas);
    CHECK_THROWS_AS(chart->SetData(BuildDataset(), L"Effect", { { L"Bad", L"Nope", {} } }),
                    std::runtime_error);
    CHECK_THROWS_AS(chart->SetData(BuildDataset(), L"Effect",
                                   { { L"Man", L"Man", { { L"Training", L"Nope" } } } }),
                    std::runtime_error);
    }

TEST_CASE("IshikawaDiagram draws every cause on a crowded branch", "[ishikawadiagram][render]")
    {
    // many causes (some of them long) are shrunk and wrapped to fit, but none are dropped
    std::vector<wxString> causes;
    for (int i = 0; i < 40; ++i)
        {
        causes.push_back(
            wxString::Format(L"Cause%d", i) +
            ((i % 5 == 0) ? wxString{ L"_with_a_very_long_name_that_wont_fit" } : wxString{}));
        }
    IshikawaDiagram::Branch crowded{ L"Crowded", wxString{}, {}, causes };
    // 40 twigs, each a line, a label and a dot, plus the rib and the branch's label
    CHECK(ObjectCount({ crowded }, true, true) == FIXED_OBJECTS + 2 + (40 * 3));
    }

TEST_CASE("IshikawaDiagram can be drawn in monochrome", "[ishikawadiagram][render]")
    {
    const auto draw = [](const bool monochrome)
    {
        auto* canvas = MakeCanvas();
        auto chart = std::make_shared<IshikawaDiagram>(canvas);
        chart->SetMonochrome(monochrome);
        CHECK(chart->IsMonochrome() == monochrome);
        chart->SetData(BuildDataset(), L"Effect",
                       { { L"Personnel", L"Man", { { L"Training", L"TrainingDetails" } } },
                         { L"Machines", L"Machine", {} } });
        LayOutOffscreen(canvas, chart);
        return chart->GetObjectCount();
    };
    // only the colors differ, not what is drawn
    CHECK(draw(true) == draw(false));
    }

TEST_CASE("IshikawaDiagram can show cause counts in the branch labels", "[ishikawadiagram][render]")
    {
    const auto draw = [](const bool showCounts)
    {
        auto* canvas = MakeCanvas();
        auto chart = std::make_shared<IshikawaDiagram>(canvas);
        CHECK_FALSE(chart->IsShowingCauseCounts());
        chart->ShowCauseCounts(showCounts);
        CHECK(chart->IsShowingCauseCounts() == showCounts);
        chart->SetData(BuildDataset(), L"Effect",
                       { { L"Personnel", L"Man", {} }, { L"Machines", L"Machine", {} } });
        LayOutOffscreen(canvas, chart);
        return chart->GetObjectCount();
    };
    // only the label text changes, not what is drawn
    CHECK(draw(true) == draw(false));
    }

TEST_CASE("IshikawaDiagram tapers its lines by default", "[ishikawadiagram][render]")
    {
    const auto draw = [](const bool tapered)
    {
        auto* canvas = MakeCanvas();
        auto chart = std::make_shared<IshikawaDiagram>(canvas);
        CHECK(chart->IsTaperedLines());
        chart->SetTaperedLines(tapered);
        CHECK(chart->IsTaperedLines() == tapered);
        chart->SetData(BuildDataset(), L"Effect",
                       { { L"Personnel", L"Man", {} }, { L"Machines", L"Machine", {} } });
        LayOutOffscreen(canvas, chart);
        return chart->GetObjectCount();
    };
    // the spine and ribs are polygons instead of lines, but there are the same number of them
    CHECK(draw(true) == draw(false));
    }

TEST_CASE("IshikawaDiagram can change its fish style", "[ishikawadiagram][render]")
    {
    const auto draw = [](const IshikawaDiagram::FishStyle style)
    {
        auto* canvas = MakeCanvas();
        auto chart = std::make_shared<IshikawaDiagram>(canvas);
        CHECK(chart->GetFishStyle() == IshikawaDiagram::FishStyle::Fish);
        chart->SetFishStyle(style);
        CHECK(chart->GetFishStyle() == style);
        chart->SetData(BuildDataset(), L"Effect",
                       { { L"Personnel", L"Man", {} }, { L"Machines", L"Machine", {} } });
        LayOutOffscreen(canvas, chart);
        return chart->GetObjectCount();
    };
    // a shark only changes the head's outline and mouth, but without a fish
    // there is no tail or head (and so no eye or mouth)
    const size_t fish{ draw(IshikawaDiagram::FishStyle::Fish) };
    CHECK(draw(IshikawaDiagram::FishStyle::Shark) == fish);
    CHECK(draw(IshikawaDiagram::FishStyle::None) == fish - 2);
    }

TEST_CASE("IshikawaDiagram shows a repeated cause once", "[ishikawadiagram][render]")
    {
    // typed causes "Speed Age Speed" are two causes, each a twig and a label
    IshikawaDiagram::Branch repeated{
        L"Personnel", wxString{}, {}, { L"Speed", L"Age", L"Speed" }
    };
    IshikawaDiagram::Branch unique{ L"Personnel", wxString{}, {}, { L"Speed", L"Age" } };
    CHECK(ObjectCount({ repeated }, true, true) == ObjectCount({ unique }, true, true));
    }

TEST_CASE("IshikawaDiagram skips sub-causes of a cause that doesn't exist",
          "[ishikawadiagram][render]")
    {
    const size_t withoutSubs{ ObjectCount({ { L"Man", L"Man", {} } }, true, true) };
    // the cause is missing, so the sub-causes are skipped instead of throwing
    const size_t withOrphan{ ObjectCount(
        { { L"Man", L"Man", { { L"Missing", L"TrainingDetails" } } } }, true, true) };
    CHECK(withOrphan == withoutSubs);
    }
