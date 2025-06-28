///////////////////////////////////////////////////////////////////////////////
// Name:        inflesz.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "inflesz.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::InfleszChart, Wisteria::Graphs::ScaleChart)

    namespace Wisteria::Graphs
    {
    InfleszChart::InfleszChart(
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

        // graph has an odd scale where the "very difficult" section and crunched down,
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
                    Wisteria::Graphs::BarChart::BarBlockInfo(39)
                        .Brush(wxColour{ L"#CBE9E3" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"MUY DIFÍCIL") }
                                                     .LabelFitting(LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(15)
                        .Brush(wxColour{ L"#CCAAD0" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"ALGO DIFÍCIL") }
                                                     .LabelFitting(LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(wxColour{ L"#FFFBD5" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"NORMAL") }
                                                     .LabelFitting(LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(15)
                        .Brush(wxColour{ L"#F7A770" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"BASTANTE FÁCIL") }
                                                     .LabelFitting(LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(21)
                        .Brush(wxColour{ L"#67C7C0" })
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ _DT(L"MUY FÁCIL") }
                                                     .LabelFitting(LabelFit::ScaleFontToFit))) } },
            0, L"INFLESZ");

        // SZIGRISZT scale
        if (includeSzigriszt)
            {
            AddScale(
                std::vector<BarChart::BarBlock>{
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(14)
                                            .Brush(wxColour{ L"#CBE9E3" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"MUY DIFÍCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(20)
                                            .Brush(wxColour{ L"#F8D3DD" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"DIFÍCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(15)
                            .Brush(wxColour{ L"#C4CAE6" })
                            .Decal(GraphItems::Label(
                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE DIFÍCIL") }.LabelFitting(
                                    LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(15)
                                            .Brush(wxColour{ L"#FFFBD5" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ L"NORMAL" }.LabelFitting(
                                                    LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(10)
                                            .Brush(wxColour{ L"#F7A770" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE FÁCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(10)
                                            .Brush(wxColour{ L"#F4908B" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"FÁCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(16)
                                            .Brush(wxColour{ L"#67C7C0" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"MUY FÁCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) } },
                0, L"SZIGRISZT");
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
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(20)
                                            .Brush(wxColour{ L"#F8D3DD" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"DIFÍCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{
                        Wisteria::Graphs::BarChart::BarBlockInfo(10)
                            .Brush(wxColour{ L"#C4CAE6" })
                            .Decal(GraphItems::Label(
                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE DIFÍCIL") }.LabelFitting(
                                    LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(10)
                                            .Brush(wxColour{ L"#FFFBD5" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"NORMAL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(10)
                                            .Brush(wxColour{ L"#F7A770" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"BASTANTE FÁCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(10)
                                            .Brush(wxColour{ L"#F4908B" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"FÁCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) },
                    BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(11)
                                            .Brush(wxColour{ L"#67C7C0" })
                                            .Decal(GraphItems::Label(
                                                GraphItems::GraphItemInfo{ _DT(L"MUY FÁCIL") }
                                                    .LabelFitting(LabelFit::ScaleFontToFit))) } },
                0, L"FLESCH");
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
    } // namespace Wisteria::Graphs
