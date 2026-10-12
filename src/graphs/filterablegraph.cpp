///////////////////////////////////////////////////////////////////////////////
// Name:        filterablegraph.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "filterablegraph.h"
#include "../base/canvas.h"
#include <set>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::FilterableGraph, Wisteria::GraphItems::GraphItemBase)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    FilterableGraph::FilterableGraph(const std::shared_ptr<const Data::Dataset>& data,
                                     const wxString& filterColumn, const wxString& defaultValue,
                                     const GraphFactory& factory,
                                     const bool includeAllOption /*= false*/)
        : m_filterColumn(filterColumn), m_includeAll(includeAllOption)
        {
        if (data == nullptr || !factory)
            {
            throw std::runtime_error(
                _(L"A dataset and graph builder are required for a filterable graph.").ToUTF8());
            }

        if (data->GetCategoricalColumn(filterColumn) == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': filter column not found for graph.").ToUTF8(), filterColumn));
            }
        const std::vector<wxString> labels{ GetFilterValues(*data, filterColumn) };

        const auto buildGraph =
            [&factory](const std::shared_ptr<const Data::Dataset>& subset, const wxString& label)
        {
            auto graph = factory(subset);
            if (graph == nullptr)
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': unable to build graph for filter value.").ToUTF8(), label));
                }
            return graph;
        };

        if (m_includeAll)
            {
            const wxString allLabel{ GetDefaultAllLabel() };
            m_variants.push_back(Variant{ allLabel, buildGraph(data, allLabel) });
            }
        for (const auto& label : labels)
            {
            Data::Subset dataSubset;
            const auto subset = dataSubset.SubsetSimple(
                data, Data::ColumnFilterInfo{ filterColumn, Comparison::Equals, { label } });
            m_variants.push_back(Variant{ label, buildGraph(subset, label) });
            }

        if (m_variants.empty())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': filter column does not contain any values.").ToUTF8(), filterColumn));
            }

        m_activeIndex = FindVariant(defaultValue).value_or(0);
        AdoptLayoutFrom(*GetActiveGraph());
        }

    //----------------------------------------------------------------
    std::vector<wxString> FilterableGraph::GetFilterValues(const Data::Dataset& data,
                                                           const wxString& filterColumn)
        {
        std::vector<wxString> labels;
        const auto filterCol = data.GetCategoricalColumn(filterColumn);
        if (filterCol == data.GetCategoricalColumns().cend())
            {
            return labels;
            }

        // only the labels actually used in the data
        const std::set<Data::GroupIdType> usedIds{ filterCol->GetValues().cbegin(),
                                                   filterCol->GetValues().cend() };
        for (const auto& usedId : usedIds)
            {
            if (usedId == Data::ColumnWithStringTable::MISSING_DATA_CODE)
                {
                continue;
                }
            const wxString label{ filterCol->GetLabelFromID(usedId) };
            if (!label.empty())
                {
                labels.push_back(label);
                }
            }
        std::ranges::sort(labels,
                          [](const auto& lhv, const auto& rhv) { return lhv.CmpNoCase(rhv) < 0; });
        return labels;
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetAllLabel(const wxString& label)
        {
        if (m_includeAll && !m_variants.empty())
            {
            m_variants.front().m_label = label;
            }
        }

    //----------------------------------------------------------------
    std::optional<size_t> FilterableGraph::FindVariant(const wxString& label) const
        {
        const auto foundPos =
            std::ranges::find_if(m_variants, [&label](const auto& filteredGraph)
                                 { return filteredGraph.m_label.CmpNoCase(label) == 0; });
        return (foundPos != m_variants.cend()) ?
                   std::optional<size_t>{ static_cast<size_t>(
                       std::distance(m_variants.cbegin(), foundPos)) } :
                   std::nullopt;
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetActiveIndex(const size_t index)
        {
        if (index >= m_variants.size() || index == m_activeIndex)
            {
            return;
            }
        m_activeIndex = index;
        SyncToGraph(*GetActiveGraph());
        UpdateLegend();
        }

    //----------------------------------------------------------------
    bool FilterableGraph::SetActiveValue(const wxString& label)
        {
        const auto index = FindVariant(label);
        if (!index.has_value())
            {
            return false;
            }
        SetActiveIndex(index.value());
        return true;
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> FilterableGraph::CreateLegend(const LegendOptions& options)
        {
        return std::dynamic_pointer_cast<GraphItems::Label>(CreateLegend(options, LegendFactory{}));
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::GraphItemBase> FilterableGraph::CreateLegend(
        const LegendOptions& options, const LegendFactory& factory)
        {
        m_legendOptions = options;
        m_legendFactory = factory;
        // every graph has the legend settings, whichever one is shown (or saved)
        for (auto& filteredGraph : m_variants)
            {
            filteredGraph.m_graph->SetLegendInfo(options);
            }
        std::shared_ptr<GraphItems::GraphItemBase> legend{ BuildLegend() };
        m_legend = legend;
        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::GraphItemBase> FilterableGraph::BuildLegend() const
        {
        Graph2D& graph{ *GetActiveGraph() };
        std::unique_ptr<GraphItems::GraphItemBase> legend;
        if (m_legendFactory)
            {
            legend = m_legendFactory(graph, m_legendOptions.value());
            }
        else
            {
            legend = graph.CreateLegend(m_legendOptions.value());
            }
        if (legend == nullptr)
            {
            return legend;
            }
        // this object tracks its own legend (and the one built here may be temporary),
        // so the graph shouldn't keep a pointer to it
        graph.UnregisterCreatedLegend(legend.get());
        if (auto* legendLabel = dynamic_cast<GraphItems::Label*>(legend.get());
            legendLabel != nullptr)
            {
            if (!m_legendOptions->GetTitle().empty())
                {
                legendLabel->SetLine(0, m_legendOptions->GetTitle());
                }
            legendLabel->SetIsLegend(true);
            }
        return legend;
        }

    //----------------------------------------------------------------
    void FilterableGraph::UpdateLegend()
        {
        const auto legend = m_legend.lock();
        if (legend == nullptr || !m_legendOptions.has_value())
            {
            return;
            }
        auto newLegend = BuildLegend();
        if (newLegend == nullptr)
            {
            legend->Show(false);
            return;
            }

        // keep the settings that the canvas assigned to the legend already on it
        const auto legendId{ legend->GetId() };
        const auto scaling{ legend->GetScaling() };
        const auto originalScaling{ legend->GetOriginalCanvasScaling() };
        const auto widthProportion{ legend->GetCanvasWidthProportion() };
        const auto heightProportion{ legend->GetCanvasHeightProportion() };
        const wxSVGAttributes accessibilityAttributes{ legend->GetAutoAccessibilityAttributes() };
        const auto restoreSettings = [&](GraphItems::GraphItemBase& target)
        {
            target.SetId(legendId);
            target.SetScaling(scaling);
            target.SetOriginalCanvasScaling(originalScaling);
            target.SetCanvasWidthProportion(widthProportion);
            target.SetCanvasHeightProportion(heightProportion);
            target.GetAutoAccessibilityAttributes() = accessibilityAttributes;
        };

        // a label legend is updated in place
        auto* legendLabel = dynamic_cast<GraphItems::Label*>(legend.get());
        const auto* newLabel = dynamic_cast<const GraphItems::Label*>(newLegend.get());
        if (legendLabel != nullptr && newLabel != nullptr)
            {
            *legendLabel = *newLabel;
            restoreSettings(*legendLabel);
            return;
            }

        // other legends can't be copied, so the new one replaces the old one in its canvas cell
        Canvas* canvas{ GetActiveGraph()->GetCanvas() };
        if (canvas == nullptr)
            {
            return;
            }
        const auto [gridRows, gridCols] = canvas->GetFixedObjectsGridSize();
        for (size_t row = 0; row < gridRows; ++row)
            {
            for (size_t col = 0; col < gridCols; ++col)
                {
                if (canvas->GetFixedObject(row, col) == legend)
                    {
                    std::shared_ptr<GraphItems::GraphItemBase> replacement{ std::move(newLegend) };
                    replacement->SetScaling(scaling);
                    canvas->SetFixedObject(row, col, replacement);
                    // the canvas recalculates these when an object is placed
                    restoreSettings(*replacement);
                    m_legend = replacement;
                    return;
                    }
                }
            }
        }

    //----------------------------------------------------------------
    void FilterableGraph::AdoptLayoutFrom(const Graph2D& graph)
        {
        GraphItemBase::SetScaling(graph.GetScaling());
        GraphItemBase::SetDPIScaleFactor(graph.GetDPIScaleFactor());
        GraphItemBase::SetSelectable(graph.IsSelectable());
        Show(graph.IsShown());
        SetId(graph.GetId());
        SetCanvasMargins(graph.GetTopCanvasMargin(), graph.GetRightCanvasMargin(),
                         graph.GetBottomCanvasMargin(), graph.GetLeftCanvasMargin());
        SetCanvasWidthProportion(graph.GetCanvasWidthProportion());
        SetCanvasHeightProportion(graph.GetCanvasHeightProportion());
        FitCanvasRowHeightToContent(graph.IsFittingCanvasRowHeightToContent());
        SetFixedWidthOnCanvas(graph.IsFixedWidthOnCanvas());
        SetPageVerticalAlignment(graph.GetPageVerticalAlignment());
        SetPageHorizontalAlignment(graph.GetPageHorizontalAlignment());
        }

    //----------------------------------------------------------------
    void FilterableGraph::SyncToGraph(Graph2D & graph) const
        {
        graph.Show(IsShown());
        graph.SetId(GetId());
        graph.SetContentTop(GetContentTop());
        graph.SetContentBottom(GetContentBottom());
        graph.SetContentLeft(GetContentLeft());
        graph.SetContentRight(GetContentRight());
        }

    //----------------------------------------------------------------
    wxRect FilterableGraph::GetBoundingBox(wxDC & dc) const
        {
        return GetActiveItem().GetBoundingBox(dc);
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetBoundingBox(const wxRect& rect, wxDC& dc, const double parentScaling)
        {
        for (auto& filteredGraph : m_variants)
            {
            static_cast<GraphItemBase&>(*filteredGraph.m_graph)
                .SetBoundingBox(rect, dc, parentScaling);
            }
        }

    //----------------------------------------------------------------
    void FilterableGraph::Offset(const int xToMove, const int yToMove)
        {
        for (auto& filteredGraph : m_variants)
            {
            static_cast<GraphItemBase&>(*filteredGraph.m_graph).Offset(xToMove, yToMove);
            }
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetScaling(const double scaling)
        {
        GraphItemBase::SetScaling(scaling);
        for (auto& filteredGraph : m_variants)
            {
            static_cast<GraphItemBase&>(*filteredGraph.m_graph).SetScaling(scaling);
            }
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetDPIScaleFactor(const double scaling)
        {
        GraphItemBase::SetDPIScaleFactor(scaling);
        for (auto& filteredGraph : m_variants)
            {
            static_cast<GraphItemBase&>(*filteredGraph.m_graph).SetDPIScaleFactor(scaling);
            }
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetSelected(const bool selected)
        {
        GraphItemBase::SetSelected(selected);
        if (!m_variants.empty())
            {
            GetActiveItem().SetSelected(selected);
            }
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetSelectable(const bool selectable) noexcept
        {
        GraphItemBase::SetSelectable(selectable);
        for (auto& filteredGraph : m_variants)
            {
            static_cast<GraphItemBase&>(*filteredGraph.m_graph).SetSelectable(selectable);
            }
        }

    //----------------------------------------------------------------
    void FilterableGraph::SetFreeFloating(const bool freeFloat)
        {
        GraphItemBase::SetFreeFloating(freeFloat);
        for (auto& filteredGraph : m_variants)
            {
            static_cast<GraphItemBase&>(*filteredGraph.m_graph).SetFreeFloating(freeFloat);
            }
        }

    //----------------------------------------------------------------
    wxBitmap FilterableGraph::ToBitmap(wxDC & dc) const { return GetActiveItem().ToBitmap(dc); }

    //----------------------------------------------------------------
    void FilterableGraph::SetAutoAccessibilityAttributes()
        {
        for (auto& filteredGraph : m_variants)
            {
            static_cast<GraphItemBase&>(*filteredGraph.m_graph).SetAutoAccessibilityAttributes();
            }
        }

    //----------------------------------------------------------------
    bool FilterableGraph::IsOk() const noexcept
        {
        return !m_variants.empty() && GraphItemBase::IsOk() && GetActiveGraph()->IsOk();
        }

    //----------------------------------------------------------------
    wxRect FilterableGraph::Draw(wxDC & dc) const
        {
        SyncToGraph(*GetActiveGraph());
        // a hidden graph keeps its place in the layout but leaves it blank
        if (!IsShown())
            {
            return GetActiveItem().GetBoundingBox(dc);
            }
        return GetActiveItem().Draw(dc);
        }

    //----------------------------------------------------------------
    void FilterableGraph::DrawSelectionLabel(wxDC & dc, const double scaling,
                                             const wxRect boundingBox) const
        {
        GetActiveItem().DrawSelectionLabel(dc, scaling, boundingBox);
        }

    //----------------------------------------------------------------
    void FilterableGraph::RecalcSizes(wxDC & dc)
        {
        SyncToGraph(*GetActiveGraph());
        GetActiveItem().RecalcSizes(dc);
        }

    //----------------------------------------------------------------
    void FilterableGraph::UpdateSelectedItems() { GetActiveItem().UpdateSelectedItems(); }

    //----------------------------------------------------------------
    void FilterableGraph::ClearSelections()
        {
        GraphItemBase::SetSelected(false);
        GetActiveItem().ClearSelections();
        }

    //----------------------------------------------------------------
    bool FilterableGraph::HitTest(const wxPoint pt, wxDC& dc) const
        {
        return GetActiveItem().HitTest(pt, dc);
        }

    //----------------------------------------------------------------
    bool FilterableGraph::SelectObjectAtPoint(const wxPoint& pt, wxDC& dc)
        {
        auto& activeItem = GetActiveItem();
        const bool hitObject = activeItem.SelectObjectAtPoint(pt, dc);
        // a selected part of the graph (e.g., a bar) also selects this object for editing
        GraphItemBase::SetSelected(activeItem.IsSelected() || !activeItem.GetSelectedIds().empty());
        return hitObject;
        }

    //----------------------------------------------------------------
    wxRect FilterableGraph::GetContentRect() const noexcept
        {
        return m_variants.empty() ? wxRect{} : GetActiveItem().GetContentRect();
        }
    } // namespace Wisteria::Graphs
