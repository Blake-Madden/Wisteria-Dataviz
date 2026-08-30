///////////////////////////////////////////////////////////////////////////////
// Name:        graphrenderharness.h
// Purpose:     Shared scaffolding for graph layout characterization tests
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

// Shared helpers for the *rendertests.cpp files that lock in the observable
// layout output of a graph (axis ranges and intervals, axis slot counts, and
// the number of render objects produced).
//
// The pieces here are the parts that would otherwise be copied verbatim into
// every characterization test: an offscreen canvas, the layout pass used by the
// SVG and PDF export paths (Canvas::CalcAllSizes), a single-axis fingerprint,
// and small in-memory dataset builders. Each test file still owns its own
// LayoutFingerprint, ChartSpec, BuildChart, AllSpecs, and ExpectedFingerprint.

#ifndef WISTERIA_GRAPH_RENDER_HARNESS_H
#define WISTERIA_GRAPH_RENDER_HARNESS_H

#include <wx/wx.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../../src/base/canvas.h"
#include "../../src/data/dataset.h"
#include "../../src/graphs/graph2d.h"

namespace wisteria_render_tests
    {
    inline constexpr int CANVAS_WIDTH{ 700 };
    inline constexpr int CANVAS_HEIGHT{ 500 };

    /// @returns A canvas sized for a single graph, parented to the test frame.
    [[nodiscard]]
    inline Wisteria::Canvas* MakeCanvas()
        {
        auto* canvas = new Wisteria::Canvas{ wxTheApp->GetTopWindow() };
        canvas->SetSize(CANVAS_WIDTH, CANVAS_HEIGHT);
        canvas->SetCanvasMinWidthDIPs(CANVAS_WIDTH);
        canvas->SetCanvasMinHeightDIPs(CANVAS_HEIGHT);
        return canvas;
        }

    /// @brief Lays the graph out on the canvas with an offscreen DC, exactly as
    ///     the SVG and PDF export paths do (Canvas::CalcAllSizes).
    /// @param canvas The canvas to lay out on.
    /// @param graph The graph to place at cell (0, 0) and lay out.
    inline void LayOutOffscreen(Wisteria::Canvas* canvas,
                                const std::shared_ptr<Wisteria::Graphs::Graph2D>& graph)
        {
        wxLogNull noLog;

        canvas->SetFixedObjectsGridSize(1, 1);
        canvas->SetFixedObject(0, 0, graph);

        wxBitmap bmp{ CANVAS_WIDTH, CANVAS_HEIGHT, 32 };
        wxMemoryDC memDc{ bmp };
        wxGCDC gcdc{ memDc };
        canvas->CalcAllSizes(gcdc);
        }

    /// @brief One drawn snapshot of everything a test observes about a single axis.
    struct AxisFingerprint
        {
        double m_rangeStart{ 0 };
        double m_rangeEnd{ 0 };
        double m_interval{ 0 };
        int m_precision{ 0 };
        bool m_reversed{ false };
        size_t m_pointCount{ 0 };

        [[nodiscard]]
        bool operator==(const AxisFingerprint& that) const
            {
            return m_rangeStart == that.m_rangeStart && m_rangeEnd == that.m_rangeEnd &&
                   m_interval == that.m_interval && m_precision == that.m_precision &&
                   m_reversed == that.m_reversed && m_pointCount == that.m_pointCount;
            }

        [[nodiscard]]
        std::string ToString() const
            {
            std::ostringstream text;
            text << "[" << m_rangeStart << ", " << m_rangeEnd << "] int=" << m_interval
                 << " prec=" << m_precision << " rev=" << (m_reversed ? 1 : 0)
                 << " points=" << m_pointCount;
            return text.str();
            }
        };

    /// @returns A fingerprint of the given axis. Uses GetAxisPointsCount() rather
    ///     than GetAxisPoints().size(); the latter resets the axis label-fit state.
    [[nodiscard]]
    inline AxisFingerprint CaptureAxis(const Wisteria::GraphItems::Axis& axis)
        {
        AxisFingerprint print;
        const auto range = axis.GetRange();
        print.m_rangeStart = range.first;
        print.m_rangeEnd = range.second;
        print.m_interval = axis.GetInterval();
        print.m_precision = axis.GetPrecision();
        print.m_reversed = axis.IsReversed();
        print.m_pointCount = axis.GetAxisPointsCount();
        return print;
        }

    /// @returns A dataset with one continuous column holding @p values.
    [[nodiscard]]
    inline std::shared_ptr<Wisteria::Data::Dataset>
    MakeContinuousDataset(const wxString& columnName, const std::vector<double>& values)
        {
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->AddContinuousColumn(columnName);
        for (size_t idx = 0; idx < values.size(); ++idx)
            {
            dataset->AddRow(Wisteria::Data::RowInfo()
                                .Continuous({ values[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }
        return dataset;
        }

    /// @returns A dataset with one continuous column and one categorical group
    ///     column. @p values and @p groupCodes must be the same length.
    [[nodiscard]]
    inline std::shared_ptr<Wisteria::Data::Dataset> MakeGroupedContinuousDataset(
        const wxString& columnName, const std::vector<double>& values,
        const wxString& groupColumnName,
        const std::vector<Wisteria::Data::GroupIdType>& groupCodes,
        const Wisteria::Data::ColumnWithStringTable::StringTableType& groupLabels)
        {
        wxASSERT_MSG(values.size() == groupCodes.size(),
                     L"value and group-code counts differ in MakeGroupedContinuousDataset()");
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->AddContinuousColumn(columnName);
        dataset->AddCategoricalColumn(groupColumnName, groupLabels);
        for (size_t idx = 0; idx < values.size(); ++idx)
            {
            dataset->AddRow(Wisteria::Data::RowInfo()
                                .Continuous({ values[idx] })
                                .Categoricals({ groupCodes[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }
        return dataset;
        }

    /// @returns A dataset with two continuous columns. @p yValues and @p xValues
    ///     must be the same length.
    [[nodiscard]]
    inline std::shared_ptr<Wisteria::Data::Dataset>
    MakeXYDataset(const wxString& yColumnName, const std::vector<double>& yValues,
                 const wxString& xColumnName, const std::vector<double>& xValues)
        {
        wxASSERT_MSG(yValues.size() == xValues.size(),
                     L"y and x value counts differ in MakeXYDataset()");
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->AddContinuousColumn(yColumnName);
        dataset->AddContinuousColumn(xColumnName);
        for (size_t idx = 0; idx < yValues.size(); ++idx)
            {
            dataset->AddRow(Wisteria::Data::RowInfo()
                                .Continuous({ yValues[idx], xValues[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }
        return dataset;
        }

    /// @returns A dataset with two continuous columns and one categorical group
    ///     column. All three value vectors must be the same length.
    [[nodiscard]]
    inline std::shared_ptr<Wisteria::Data::Dataset> MakeGroupedXYDataset(
        const wxString& yColumnName, const std::vector<double>& yValues,
        const wxString& xColumnName, const std::vector<double>& xValues,
        const wxString& groupColumnName,
        const std::vector<Wisteria::Data::GroupIdType>& groupCodes,
        const Wisteria::Data::ColumnWithStringTable::StringTableType& groupLabels)
        {
        wxASSERT_MSG(yValues.size() == xValues.size() && yValues.size() == groupCodes.size(),
                     L"value counts differ in MakeGroupedXYDataset()");
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->AddContinuousColumn(yColumnName);
        dataset->AddContinuousColumn(xColumnName);
        dataset->AddCategoricalColumn(groupColumnName, groupLabels);
        for (size_t idx = 0; idx < yValues.size(); ++idx)
            {
            dataset->AddRow(Wisteria::Data::RowInfo()
                                .Continuous({ yValues[idx], xValues[idx] })
                                .Categoricals({ groupCodes[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }
        return dataset;
        }

    /// @returns A dataset with one categorical column built from @p labels, one
    ///     row per entry in @p codes.
    [[nodiscard]]
    inline std::shared_ptr<Wisteria::Data::Dataset>
    MakeCategoricalDataset(const wxString& columnName,
                           const Wisteria::Data::ColumnWithStringTable::StringTableType& labels,
                           const std::vector<Wisteria::Data::GroupIdType>& codes)
        {
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->AddCategoricalColumn(columnName, labels);
        for (size_t idx = 0; idx < codes.size(); ++idx)
            {
            dataset->AddRow(Wisteria::Data::RowInfo()
                                .Categoricals({ codes[idx] })
                                .Id(wxString::Format(L"obs%d", static_cast<int>(idx))));
            }
        return dataset;
        }
    } // namespace wisteria_render_tests

#endif // WISTERIA_GRAPH_RENDER_HARNESS_H
