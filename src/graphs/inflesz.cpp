///////////////////////////////////////////////////////////////////////////////
// Name:        inflesz.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "inflesz.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::InfleszScale, Wisteria::Graphs::ScaleChart)

    namespace Wisteria::Graphs
    {
    InfleszScale::InfleszScale(
        Wisteria::Canvas * canvas,
        const std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
        const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes /*= nullptr*/,
        const bool includeSzigriszt /*= true*/, const bool includeFlesch /*= true*/)
        : Wisteria::Graphs::ScaleChart(canvas, colors, shapes)
        {
        SetMainScaleColumnHeader(_DT(L"IFSZ"));
        /*
           This label (and score section) are not on the original graph, but we
           add it to ours to show the results. The original article showed the
           results in a separate table, but this seems more useful to combine them
           with the chart itself (as most other graphical readability test do.)
        */
        SetDataColumnHeader(_DT(L"PUNTUACIÓN"));

        // graph has an odd scale where the "very difficult" section is crunched down,
        // even though it consumes 40% of the range
        GetScalingAxis().GetAxisPoints().clear();
        GetScalingAxis().AddUnevenAxisPoint(-5, L" ");
        GetScalingAxis().AddUnevenAxisPoint(0);
        GetScalingAxis().AddUnevenAxisPoint(15);
        GetScalingAxis().AddUnevenAxisPoint(30);
        GetScalingAxis().AddUnevenAxisPoint(35);
        GetScalingAxis().AddUnevenAxisPoint(40);
        GetScalingAxis().AddUnevenAxisPoint(45);
        GetScalingAxis().AddUnevenAxisPoint(50);
        GetScalingAxis().AddUnevenAxisPoint(55);
        GetScalingAxis().AddUnevenAxisPoint(60);
        GetScalingAxis().AddUnevenAxisPoint(65);
        GetScalingAxis().AddUnevenAxisPoint(70);
        GetScalingAxis().AddUnevenAxisPoint(75);
        GetScalingAxis().AddUnevenAxisPoint(80);
        GetScalingAxis().AddUnevenAxisPoint(85);
        GetScalingAxis().AddUnevenAxisPoint(90);
        GetScalingAxis().AddUnevenAxisPoint(95);
        GetScalingAxis().AddUnevenAxisPoint(100);
        // just extra padding so that the 0 and 100 labels don't overlap the bottom
        // and top axes
        GetScalingAxis().AddUnevenAxisPoint(105, L" ");
        GetScalingAxis().AdjustRangeToLabels();

        GetScalingAxis().Reverse();

        SetMainScaleValues({ 0, 15, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100 },
                           0);

        // INFLESZ scale
        AddScale(
            std::vector<BarChart::BarBlock>{
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(40)
                        .Brush(wxColour{ L"#CBE9E3" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"MUY DIFÍCIL") }
                                                     .LabelFitting(LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(15)
                        .Brush(wxColour{ L"#CCAAD0" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"ALGO DIFÍCIL") }
                                                     .LabelFitting(LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(wxColour{ L"#FFFBD5" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"NORMAL") }
                                                     .LabelFitting(LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(15)
                        .Brush(wxColour{ L"#F7A770" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"BASTANTE FÁCIL") }
                                                     .LabelFitting(LabelFit::DisplayAsIs))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(20)
                        .Brush(wxColour{ L"#67C7C0" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"MUY FÁCIL") }
                                                     .LabelFitting(LabelFit::DisplayAsIs))) } },
            0, _DT(L"INFLESZ"));

        // SZIGRISZT scale
        if (includeSzigriszt)
            {
            AddScale(
                std::vector<BarChart::BarBlock>{
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(14)
                                            .Brush(wxColour{ L"#CBE9E3" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"MUY DIFÍCIL") }
                                                    .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(20)
                            .Brush(wxColour{ L"#F8D3DD" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"DIFÍCIL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(15)
                            .Brush(wxColour{ L"#C4CAE6" })
                            .Decal(GraphItems::Label(
                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE DIFÍCIL") }.LabelFitting(
                                    LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(15)
                            .Brush(wxColour{ L"#FFFBD5" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"NORMAL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(10)
                                            .Brush(wxColour{ L"#F7A770" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE FÁCIL") }
                                                    .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(10)
                            .Brush(wxColour{ L"#F4908B" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"FÁCIL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(16)
                            .Brush(wxColour{ L"#67C7C0" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"MUY FÁCIL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) } },
                0, _DT(L"SZIGRISZT", DTExplanation::ProperNoun));
            }

        // FLESCH scale
        if (includeFlesch)
            {
            AddScale(
                std::vector<BarChart::BarBlock>{
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(29)
                                            .Brush(wxColour{ L"#CBE9E3" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"MUY DIFÍCIL") }
                                                    .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(20)
                            .Brush(wxColour{ L"#F8D3DD" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"DIFÍCIL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(10)
                            .Brush(wxColour{ L"#C4CAE6" })
                            .Decal(GraphItems::Label(
                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE DIFÍCIL") }.LabelFitting(
                                    LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(10)
                            .Brush(wxColour{ L"#FFFBD5" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"NORMAL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(10)
                                            .Brush(wxColour{ L"#F7A770" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE FÁCIL") }
                                                    .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(10)
                            .Brush(wxColour{ L"#F4908B" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"FÁCIL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(11)
                            .Brush(wxColour{ L"#67C7C0" })
                            .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"MUY FÁCIL") }
                                                         .LabelFitting(LabelFit::DisplayAsIs))) } },
                0, _DT(L"FLESCH", DTExplanation::ProperNoun));
            }

        wxString caption =
            _DT(L"IFSZ = Puntuación del Índice de Flesch-Szigriszt.\n"
                "INFLESZ: Escala de interpretación de resultados del Programa INFLESZ.\n",
                DTExplanation::DirectQuote);
        if (includeSzigriszt)
            {
            caption += _DT(L"SZIGRISZT: Escala de Nivel de Perspicuidad de Szigriszt.\n");
            }
        if (includeFlesch)
            {
            caption += _DT(L"FLESCH: Escala RES de Flesch");
            }
        caption.Trim(true);

        GetCaption().GetGraphItemInfo().Padding(2, 0, 2, 5).Text(caption);
        }

    //----------------------------------------------------------------
    void InfleszScale::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"An INFLESZ scale chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");

        // collect finite scores (clamped to the scaling-axis range, matching the plot)
        struct ScoreEntry
            {
            double m_value{ 0 };
            wxString m_idLabel;
            };

        std::vector<ScoreEntry> scores;
        if (GetDataset() != nullptr && !GetScoresColumnName().empty())
            {
            try
                {
                const auto scoresColumn = GetContinuousColumn(GetScoresColumnName());
                const auto [yStart, yEnd] = GetScalingAxis().GetRange();
                for (size_t rowIdx = 0; rowIdx < GetDataset()->GetRowCount(); ++rowIdx)
                    {
                    const double rawVal = scoresColumn->GetValue(rowIdx);
                    if (!std::isfinite(rawVal))
                        {
                        continue;
                        }
                    scores.push_back({ std::clamp<double>(rawVal, yStart, yEnd),
                                       GetDataset()->GetIdColumn().GetValue(rowIdx) });
                    }
                }
            catch (const std::exception&)
                {
                // scores column not available; carry on without score details
                }
            }

        // overall summary of where the score(s) fall numerically
        if (scores.size() == 1)
            {
            const wxString valueStr{ wxNumberFormatter::ToString(
                scores.front().m_value, GetMainScalePrecision(),
                wxNumberFormatter::Style::Style_NoTrailingZeroes) };
            label += L". ";
            if (!scores.front().m_idLabel.empty())
                {
                label += wxString::Format(
                    /* TRANSLATORS: INFLESZ chart accessibility: a single score with its
                       ID label. 1st %s is the ID label, 2nd %s is the score value. */
                    _(L"Score for %s: %s"), scores.front().m_idLabel, valueStr);
                }
            else
                {
                label += wxString::Format(
                    /* TRANSLATORS: INFLESZ chart accessibility: a single score with no ID.
                       %s is the score value. */
                    _(L"Score: %s"), valueStr);
                }
            }
        else if (scores.size() > 1)
            {
            const auto [minIt, maxIt] = std::minmax_element(scores.cbegin(), scores.cend(),
                                                            [](const auto& lhv, const auto& rhv)
                                                            { return lhv.m_value < rhv.m_value; });
            label += L". ";
            label += wxString::Format(
                /* TRANSLATORS: INFLESZ chart accessibility: multiple scores summary.
                   %zu is the score count, 1st %s is the lowest value, 2nd %s is the
                   highest value. */
                _(L"%zu scores ranging from %s to %s"), scores.size(),
                wxNumberFormatter::ToString(minIt->m_value, GetMainScalePrecision(),
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes),
                wxNumberFormatter::ToString(maxIt->m_value, GetMainScalePrecision(),
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes));
            }

        // Only the INFLESZ scale (the first scale with blocks) is used to classify the
        // score. The Szigriszt and Flesch scales are included on the chart for visual
        // comparison only and are not read here.
        const auto infleszBar = std::ranges::find_if(GetBars(), [](const auto& theBar)
                                                     { return !theBar.GetBlocks().empty(); });

        if (infleszBar != GetBars().cend())
            {
            const auto findBlockForScore = [](const Bar& theBar, const double scoreVal)
            {
                double blockStart{ theBar.GetCustomScalingAxisStartPosition().value_or(0) };
                for (const auto& theBlock : theBar.GetBlocks())
                    {
                    const double blockEnd{ blockStart + theBlock.GetLength() };
                    if (is_within(scoreVal, blockStart, blockEnd))
                        {
                        return theBlock.GetDecal().GetText();
                        }
                    blockStart = blockEnd;
                    }
                return wxString{};
            };

            if (scores.size() == 1)
                {
                const wxString blockName{ findBlockForScore(*infleszBar, scores.front().m_value) };
                if (!blockName.empty())
                    {
                    label += L". ";
                    /* TRANSLATORS: INFLESZ chart accessibility: which INFLESZ section
                       the single score lands in. %s is the section's label. */
                    label += wxString::Format(_(L"Classified as %s"), blockName);
                    }
                }
            else if (scores.size() > 1)
                {
                const auto [minIt, maxIt] = std::minmax_element(
                    scores.cbegin(), scores.cend(),
                    [](const auto& lhv, const auto& rhv) { return lhv.m_value < rhv.m_value; });
                const wxString lowBlock{ findBlockForScore(*infleszBar, minIt->m_value) };
                const wxString highBlock{ findBlockForScore(*infleszBar, maxIt->m_value) };
                if (!lowBlock.empty() && !highBlock.empty())
                    {
                    label += L". ";
                    /* TRANSLATORS: INFLESZ chart accessibility: which INFLESZ sections
                       the lowest and highest scores land in. 1st %s is the lowest
                       score's section, 2nd %s is the highest score's section. */
                    label += wxString::Format(_(L"lowest score classified as %s, "
                                                "highest score classified as %s"),
                                              lowBlock, highBlock);
                    }
                }
            }

        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");
        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
