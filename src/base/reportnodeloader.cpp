///////////////////////////////////////////////////////////////////////////////
// Name:        reportnodeloader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportnodeloader.h"
#include "reportbuilder.h"

namespace Wisteria
    {
    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableSort(std::shared_ptr<Graphs::Table>& table,
                                          const wxSimpleJSON::Ptr_t& sortNode) const
        {
        const auto sortDirection =
            sortNode->GetProperty(L"direction")->AsString().CmpNoCase(_DT(L"descending")) == 0 ?
                SortDirection::SortDescending :
                SortDirection::SortAscending;
        const std::optional<size_t> sortColumn =
            LoadTablePosition(sortNode->GetProperty(L"column"), table);

        if (sortColumn)
            {
            if (const auto labelsNode = sortNode->GetProperty(L"labels");
                labelsNode->IsOk() && labelsNode->IsValueArray())
                {
                table->Sort(sortColumn.value(), labelsNode->AsStrings(), sortDirection);
                }
            else
                {
                table->Sort(sortColumn.value(), sortDirection);
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableGroupHeader(std::shared_ptr<Graphs::Table>& table,
                                                 const wxSimpleJSON::Ptr_t& node) const
        {
        table->InsertGroupHeader(node->AsStrings());
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableRowGrouping(std::shared_ptr<Graphs::Table>& table,
                                                 const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& rowGrouping : node->AsDoubles())
            {
            table->GroupRow(rowGrouping);
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableColumnGrouping(std::shared_ptr<Graphs::Table>& table,
                                                    const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& columnGrouping : node->AsDoubles())
            {
            table->GroupColumn(columnGrouping);
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableAlternateRowColor(std::shared_ptr<Graphs::Table>& table,
                                                       const wxSimpleJSON::Ptr_t& node) const
        {
        const auto startRow = LoadTablePosition(node->GetProperty(L"start"), table);
        const std::set<size_t> colStops = LoadTableStops(table, node->GetProperty(L"stops"));
        auto rowColor{ m_reportBuilder.ConvertColor(node->GetProperty(L"color")) };
        if (!rowColor.IsOk())
            {
            rowColor = Colors::ColorBrewer::GetColor(Colors::Color::White);
            }
        table->ApplyAlternateRowColors(rowColor, startRow.value_or(0), colStops);
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableRowAdditions(std::shared_ptr<Graphs::Table>& table,
                                                  const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& rowAddCommand : node->AsNodes())
            {
            const std::optional<size_t> position =
                LoadTablePosition(rowAddCommand->GetProperty(L"position"), table);
            if (!position.has_value())
                {
                continue;
                }
            table->InsertRow(position.value());
            const auto values = rowAddCommand->GetProperty(L"values")->AsStrings();
            for (size_t i = 0; i < values.size(); ++i)
                {
                table->GetCell(position.value(), i).SetValue(values[i]);
                }
            const wxColour bgcolor(
                m_reportBuilder.ConvertColor(rowAddCommand->GetProperty(L"background")));
            if (bgcolor.IsOk())
                {
                table->SetRowBackgroundColor(position.value(), bgcolor, std::nullopt);
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableRowSuppression(std::shared_ptr<Graphs::Table>& table,
                                                    const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& rowSuppressionCommand : node->AsNodes())
            {
            const auto [position, startPosition, endPosition] =
                ReadPositions(table, rowSuppressionCommand);
            const auto threshold =
                m_reportBuilder.ConvertNumber(rowSuppressionCommand->GetProperty(L"threshold"));
            const auto suppressionLabel = m_reportBuilder.ExpandAndCache(
                table.get(), L"row-suppression.label",
                rowSuppressionCommand->GetProperty(L"label")->AsString());

            const std::set<size_t> colStops =
                LoadTableStops(table, rowSuppressionCommand->GetProperty(L"stops"));
            if (threshold.has_value())
                {
                if (position.has_value())
                    {
                    table->SetRowSuppression(position.value(), threshold,
                                             !suppressionLabel.empty() ?
                                                 std::optional<wxString>(suppressionLabel) :
                                                 std::nullopt,
                                             colStops);
                    }
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        {
                        table->SetRowSuppression(i, threshold,
                                                 !suppressionLabel.empty() ?
                                                     std::optional<wxString>(suppressionLabel) :
                                                     std::nullopt,
                                                 colStops);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableColumnSuppression(std::shared_ptr<Graphs::Table>& table,
                                                       const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& columnSuppressionCommand : node->AsNodes())
            {
            const auto [position, startPosition, endPosition] =
                ReadPositions(table, columnSuppressionCommand);
            const auto threshold =
                m_reportBuilder.ConvertNumber(columnSuppressionCommand->GetProperty(L"threshold"));
            const auto suppressionLabel = m_reportBuilder.ExpandAndCache(
                table.get(), L"column-suppression.label",
                columnSuppressionCommand->GetProperty(L"label")->AsString());

            const std::set<size_t> rowStops =
                LoadTableStops(table, columnSuppressionCommand->GetProperty(L"stops"));
            if (threshold.has_value())
                {
                if (position.has_value())
                    {
                    table->SetColumnSuppression(position.value(), threshold,
                                                !suppressionLabel.empty() ?
                                                    std::optional<wxString>(suppressionLabel) :
                                                    std::nullopt,
                                                rowStops);
                    }
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        {
                        table->SetColumnSuppression(i, threshold,
                                                    !suppressionLabel.empty() ?
                                                        std::optional<wxString>(suppressionLabel) :
                                                        std::nullopt,
                                                    rowStops);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableColumnHighlight(std::shared_ptr<Graphs::Table>& table,
                                                     const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& columnHighlightsCommand : node->AsNodes())
            {
            const auto [position, startPosition, endPosition] =
                ReadPositions(table, columnHighlightsCommand);

            std::set<size_t> rowStops =
                LoadTableStops(table, columnHighlightsCommand->GetProperty(L"stops"));
            if (position.has_value())
                {
                table->HighlightColumn(position.value(), rowStops);
                }
            if (startPosition.has_value() && endPosition.has_value())
                {
                for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                    {
                    table->HighlightColumn(i, rowStops);
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableAggregates(std::shared_ptr<Graphs::Table>& table,
                                                const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& columnRowAggregate : node->AsNodes())
            {
            const auto aggName = columnRowAggregate->GetProperty(_DT(L"name"))->AsString();
            const auto whereType = columnRowAggregate->GetProperty(L"type")->AsString();
            const auto aggType = columnRowAggregate->GetProperty(L"aggregate-type")->AsString();

            const std::optional<size_t> startColumn =
                LoadTablePosition(columnRowAggregate->GetProperty(L"start"), table);
            const std::optional<size_t> endingColumn =
                LoadTablePosition(columnRowAggregate->GetProperty(L"end"), table);
            const std::optional<size_t> insertionPosition =
                LoadTablePosition(columnRowAggregate->GetProperty(L"position"), table);

            const auto cellBorders = columnRowAggregate->GetProperty(L"borders")->AsBools();
            std::bitset<4> borders{ 0 };
            borders[0] = (!cellBorders.empty() ? cellBorders[0] : table->IsShowingTopBorder());
            borders[1] = (cellBorders.size() > 1 ? cellBorders[1] : table->IsShowingRightBorder());
            borders[2] = (cellBorders.size() > 2 ? cellBorders[2] : table->IsShowingBottomBorder());
            borders[3] = (cellBorders.size() > 3 ? cellBorders[3] : table->IsShowingLeftBorder());

            const wxColour bkColor(
                m_reportBuilder.ConvertColor(columnRowAggregate->GetProperty(L"background")));

            Graphs::Table::AggregateInfo aggInfo;
            if (startColumn.has_value())
                {
                aggInfo.FirstCell(startColumn.value());
                }
            if (endingColumn.has_value())
                {
                aggInfo.LastCell(endingColumn.value());
                }

            if (aggType.CmpNoCase(L"percent-change") == 0)
                {
                aggInfo.Type(AggregateType::ChangePercent);
                }
            else if (aggType.CmpNoCase(L"change") == 0)
                {
                aggInfo.Type(AggregateType::Change);
                }
            else if (aggType.CmpNoCase(L"total") == 0)
                {
                aggInfo.Type(AggregateType::Total);
                }
            else if (aggType.CmpNoCase(L"ratio") == 0)
                {
                aggInfo.Type(AggregateType::Ratio);
                }
            else
                {
                continue;
                }

            if (whereType.CmpNoCase(L"column") == 0)
                {
                table->InsertAggregateColumn(
                    aggInfo, aggName, insertionPosition,
                    columnRowAggregate->GetProperty(L"use-adjacent-color")->AsBool(),
                    (bkColor.IsOk() ? std::optional<wxColour>(bkColor) : std::nullopt),
                    (!cellBorders.empty() ? std::optional<std::bitset<4>>(borders) : std::nullopt));
                }
            else if (whereType.CmpNoCase(L"row") == 0)
                {
                table->InsertAggregateRow(
                    aggInfo, aggName, insertionPosition,
                    (bkColor.IsOk() ? std::optional<wxColour>(bkColor) : std::nullopt),
                    (!cellBorders.empty() ? std::optional<std::bitset<4>>(borders) : std::nullopt));
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableRowTotals(std::shared_ptr<Graphs::Table>& table,
                                               const wxSimpleJSON::Ptr_t& node) const
        {
        const wxColour bkColor(m_reportBuilder.ConvertColor(node->GetProperty(L"background")));
        table->InsertRowTotals(bkColor.IsOk() ? std::optional<wxColour>(bkColor) : std::nullopt);
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableCellUpdates(std::shared_ptr<Graphs::Table>& table,
                                                 const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& cellUpdate : node->AsNodes())
            {
            const std::optional<size_t> rowPosition =
                LoadTablePosition(cellUpdate->GetProperty(L"row"), table);
            const std::optional<size_t> columnPosition =
                LoadTablePosition(cellUpdate->GetProperty(L"column"), table);
            Graphs::Table::TableCell* currentCell =
                ((rowPosition.has_value() && columnPosition.has_value() &&
                  rowPosition.value() < table->GetRowCount() &&
                  columnPosition.value() < table->GetColumnCount()) ?
                     &table->GetCell(rowPosition.value(), columnPosition.value()) :
                     table->FindCell(cellUpdate->GetProperty(L"value-to-find")->AsString()));

            if (currentCell == nullptr)
                {
                continue;
                }

            // column count
            const auto columnCountProperty = cellUpdate->GetProperty(L"column-count");
            if (columnCountProperty->IsOk())
                {
                if (columnCountProperty->IsValueString() &&
                    columnCountProperty->AsString().CmpNoCase(L"all") == 0)
                    {
                    currentCell->SetColumnCount(table->GetColumnCount());
                    }
                else if (columnCountProperty->IsValueNumber())
                    {
                    currentCell->SetColumnCount(columnCountProperty->AsDouble());
                    }
                }
            // row count
            const auto rowCountProperty = cellUpdate->GetProperty(L"row-count");
            if (rowCountProperty->IsOk())
                {
                if (rowCountProperty->IsValueString() &&
                    rowCountProperty->AsString().CmpNoCase(L"all") == 0)
                    {
                    currentCell->SetRowCount(table->GetRowCount());
                    }
                else if (rowCountProperty->IsValueNumber())
                    {
                    currentCell->SetRowCount(rowCountProperty->AsDouble());
                    }
                }
            // value
            const auto valueProperty = cellUpdate->GetProperty(L"value");
            if (valueProperty->IsOk())
                {
                if (valueProperty->IsValueString())
                    {
                    currentCell->SetValue(valueProperty->AsString());
                    }
                else if (valueProperty->IsValueNumber())
                    {
                    currentCell->SetValue(valueProperty->AsDouble());
                    }
                else if (valueProperty->IsValueNull())
                    {
                    currentCell->SetValue(wxEmptyString);
                    }
                }
            // background color
            const wxColour bgcolor(
                m_reportBuilder.ConvertColor(cellUpdate->GetProperty(L"background")));
            if (bgcolor.IsOk())
                {
                currentCell->SetBackgroundColor(bgcolor);
                }

            // an image to the left side of it
            const auto leftSideNode = cellUpdate->GetProperty(L"left-image");
            if (leftSideNode->IsOk())
                {
                auto path = leftSideNode->GetProperty(L"path")->AsString();
                if (!path.empty())
                    {
                    if (!wxFileName::FileExists(path))
                        {
                        path =
                            wxFileName(m_reportBuilder.GetConfigFilePath()).GetPathWithSep() + path;
                        if (!wxFileName::FileExists(path))
                            {
                            throw std::runtime_error(
                                wxString::Format(_(L"%s: label side image not found."), path)
                                    .ToUTF8());
                            }
                        }
                    currentCell->SetLeftImage(GraphItems::Image::LoadFile(path));
                    }
                }

            // prefix
            if (cellUpdate->HasProperty(L"prefix"))
                {
                currentCell->SetPrefix(cellUpdate->GetProperty(L"prefix")->AsString());
                }

            // is it highlighted
            if (cellUpdate->HasProperty(L"highlight"))
                {
                currentCell->Highlight(cellUpdate->GetProperty(L"highlight")->AsBool());
                }

            // font attributes
            if (cellUpdate->HasProperty(L"bold"))
                {
                if (cellUpdate->GetProperty(L"bold")->AsBool())
                    {
                    currentCell->GetFont().MakeBold();
                    }
                else
                    {
                    currentCell->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                    }
                }

            // outer border toggles
            const auto outerBorderToggles = cellUpdate->GetProperty(L"show-borders")->AsBools();
            if (!outerBorderToggles.empty())
                {
                currentCell->ShowTopBorder(outerBorderToggles[0]);
                }
            if (outerBorderToggles.size() >= 2)
                {
                currentCell->ShowRightBorder(outerBorderToggles[1]);
                }
            if (outerBorderToggles.size() >= 3)
                {
                currentCell->ShowBottomBorder(outerBorderToggles[2]);
                }
            if (outerBorderToggles.size() >= 4)
                {
                currentCell->ShowLeftBorder(outerBorderToggles[3]);
                }

            const auto textAlignment = ReportEnumConvert::ConvertTextAlignment(
                cellUpdate->GetProperty(L"text-alignment")->AsString());
            if (textAlignment.has_value())
                {
                currentCell->SetTextAlignment(textAlignment.value());
                }

            // horizontal page alignment
            const auto hPageAlignment =
                cellUpdate->GetProperty(L"horizontal-page-alignment")->AsString();
            if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                {
                currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
                }
            else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                {
                currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned);
                }
            else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                {
                currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableAnnotations(std::shared_ptr<Graphs::Table>& table,
                                                 const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& annotation : node->AsNodes())
            {
            Graphs::Table::CellAnnotation cellAnnotation{
                annotation->GetProperty(L"value")->AsString(),
                std::vector<Graphs::Table::CellPosition>{}, Side::Right, std::nullopt, wxColour{}
            };
            if (annotation->HasProperty(L"side"))
                {
                cellAnnotation.m_side =
                    (annotation->GetProperty(L"side")->AsString().CmpNoCase(L"left") == 0) ?
                        Side::Left :
                        Side::Right;
                }
            cellAnnotation.m_connectionLinePen = table->GetHighlightPen();
            m_reportBuilder.LoadPen(annotation->GetProperty(L"pen"),
                                    cellAnnotation.m_connectionLinePen.value());
            cellAnnotation.m_bgColor =
                m_reportBuilder.ConvertColor(annotation->GetProperty(L"background"));

            const auto cellsNode = annotation->GetProperty(L"cells");
            if (cellsNode->IsOk() && cellsNode->IsValueObject())
                {
                const auto outliersNode = cellsNode->GetProperty(L"column-outliers");
                const auto topNNode = cellsNode->GetProperty(L"column-top-n");
                const auto rangeNode = cellsNode->GetProperty(L"range");
                if (outliersNode->IsOk() && outliersNode->IsValueString())
                    {
                    const auto colIndex = table->FindColumnIndex(outliersNode->AsString());
                    if (colIndex.has_value())
                        {
                        cellAnnotation.m_cells = table->GetOutliers(colIndex.value());
                        table->AddCellAnnotation(cellAnnotation);
                        }
                    }
                else if (topNNode->IsOk() && topNNode->IsValueString())
                    {
                    const auto colIndex = table->FindColumnIndex(topNNode->AsString());
                    if (colIndex.has_value())
                        {
                        cellAnnotation.m_cells = table->GetTopN(
                            colIndex.value(), cellsNode->GetProperty(L"n")->AsDouble(1));
                        table->AddCellAnnotation(cellAnnotation);
                        }
                    }
                else if (rangeNode->IsOk() && rangeNode->HasProperty(L"start") &&
                         rangeNode->HasProperty(L"end"))
                    {
                    const auto startCell =
                        table->FindCellPosition(rangeNode->GetProperty(L"start")->AsString());
                    const auto endCell =
                        table->FindCellPosition(rangeNode->GetProperty(L"end")->AsString());
                    if (startCell && endCell)
                        {
                        cellAnnotation.m_cells = { startCell.value(), endCell.value() };
                        table->AddCellAnnotation(cellAnnotation);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableFootnotes(std::shared_ptr<Graphs::Table>& table,
                                               const wxSimpleJSON::Ptr_t& node) const
        {
        for (const auto& ftNode : node->AsNodes())
            {
            table->AddFootnote(
                m_reportBuilder.ExpandAndCache(table.get(), L"footnote.value",
                                               ftNode->GetProperty(L"value")->AsString()),
                m_reportBuilder.ExpandAndCache(table.get(), L"footnote.text",
                                               ftNode->GetProperty(L"footnote")->AsString()));
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableFeatures(std::shared_ptr<Graphs::Table>& table)
        {
        const auto parse = [](const wxString& tmpl) -> wxSimpleJSON::Ptr_t
        {
            if (tmpl.empty())
                {
                return wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_NULL);
                }
            return wxSimpleJSON::Create(tmpl);
        };

        // helper to parse and call only when a template exists
        const auto applyIfPresent = [&table, &parse](const wxString& prop, const auto& applyFn)
        {
            const auto tmpl = table->GetPropertyTemplate(prop);
            if (!tmpl.empty())
                {
                applyFn(table, parse(tmpl));
                }
        };

        // sorting
        applyIfPresent(L"row-sort",
                       [this](auto& tbl, const auto& node) { ApplyTableSort(tbl, node); });

        // grouping
        applyIfPresent(L"insert-group-header",
                       [this](auto& tbl, const auto& node) { ApplyTableGroupHeader(tbl, node); });
        applyIfPresent(L"row-group",
                       [this](auto& tbl, const auto& node) { ApplyTableRowGrouping(tbl, node); });
        applyIfPresent(L"column-group", [this](auto& tbl, const auto& node)
                       { ApplyTableColumnGrouping(tbl, node); });

        // alternate row color
        applyIfPresent(L"alternate-row-color", [this](auto& tbl, const auto& node)
                       { ApplyTableAlternateRowColor(tbl, node); });

        // row additions
        applyIfPresent(L"row-add",
                       [this](auto& tbl, const auto& node) { ApplyTableRowAdditions(tbl, node); });

        // suppression
        applyIfPresent(L"row-suppression", [this](auto& tbl, const auto& node)
                       { ApplyTableRowSuppression(tbl, node); });
        applyIfPresent(L"column-suppression", [this](auto& tbl, const auto& node)
                       { ApplyTableColumnSuppression(tbl, node); });

        // row formatting
        ApplyTableRowFormatting(table, parse(table->GetPropertyTemplate(L"row-formatting")),
                                parse(table->GetPropertyTemplate(L"row-color")),
                                parse(table->GetPropertyTemplate(L"row-bold")),
                                parse(table->GetPropertyTemplate(L"row-borders")),
                                parse(table->GetPropertyTemplate(L"row-content-align")));

        // column formatting
        ApplyTableColumnFormatting(table, parse(table->GetPropertyTemplate(L"column-formatting")),
                                   parse(table->GetPropertyTemplate(L"column-color")),
                                   parse(table->GetPropertyTemplate(L"column-bold")),
                                   parse(table->GetPropertyTemplate(L"column-borders")));

        // column highlight
        applyIfPresent(L"column-highlight", [this](auto& tbl, const auto& node)
                       { ApplyTableColumnHighlight(tbl, node); });

        // aggregates
        applyIfPresent(L"aggregates",
                       [this](auto& tbl, const auto& node) { ApplyTableAggregates(tbl, node); });

        // row totals
        applyIfPresent(L"row-totals",
                       [this](auto& tbl, const auto& node) { ApplyTableRowTotals(tbl, node); });

        // cell updates
        applyIfPresent(L"cell-update",
                       [this](auto& tbl, const auto& node) { ApplyTableCellUpdates(tbl, node); });

        // annotations
        applyIfPresent(L"cell-annotations",
                       [this](auto& tbl, const auto& node) { ApplyTableAnnotations(tbl, node); });

        // footnotes
        applyIfPresent(L"footnotes",
                       [this](auto& tbl, const auto& node) { ApplyTableFootnotes(tbl, node); });
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableRowFormatting(
        std::shared_ptr<Graphs::Table>& table, const wxSimpleJSON::Ptr_t& formattingNode,
        const wxSimpleJSON::Ptr_t& colorNode, const wxSimpleJSON::Ptr_t& boldNode,
        const wxSimpleJSON::Ptr_t& bordersNode, const wxSimpleJSON::Ptr_t& contentAlignNode) const
        {
        // change the rows' formatting
        const auto rowFormattingCommands = formattingNode->AsNodes();
        if (!rowFormattingCommands.empty())
            {
            for (const auto& rowFormattingCommand : rowFormattingCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, rowFormattingCommand);
                const auto formatValue = ReportEnumConvert::ConvertTableCellFormat(
                    rowFormattingCommand->GetProperty(L"format")->AsString());

                const std::set<size_t> colStops =
                    LoadTableStops(table, rowFormattingCommand->GetProperty(L"stops"));
                if (formatValue.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetRowFormat(position.value(), formatValue.value(), colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowFormat(i, formatValue.value(), colStops);
                            }
                        }
                    }
                }
            }

        // color the rows
        const auto rowColorCommands = colorNode->AsNodes();
        if (!rowColorCommands.empty())
            {
            for (const auto& rowColorCommand : rowColorCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, rowColorCommand);
                const wxColour bgcolor(
                    m_reportBuilder.ConvertColor(rowColorCommand->GetProperty(L"background")));
                const std::set<size_t> colStops =
                    LoadTableStops(table, rowColorCommand->GetProperty(L"stops"));
                if (bgcolor.IsOk())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetRowBackgroundColor(position.value(), bgcolor, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBackgroundColor(i, bgcolor, colStops);
                            }
                        }
                    }
                }
            }

        // bold the rows
        const auto rowBoldCommands = boldNode->AsNodes();
        if (!rowBoldCommands.empty())
            {
            for (const auto& rowBoldCommand : rowBoldCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, rowBoldCommand);
                const std::set<size_t> colStops =
                    LoadTableStops(table, rowBoldCommand->GetProperty(L"stops"));
                if (position.has_value())
                    {
                    table->BoldRow(position.value(), colStops);
                    }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        {
                        table->BoldRow(i, colStops);
                        }
                    }
                }
            }

        // change rows' borders
        const auto rowBordersCommands = bordersNode->AsNodes();
        if (!rowBordersCommands.empty())
            {
            for (const auto& rowBordersCommand : rowBordersCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, rowBordersCommand);
                const auto borderFlags = rowBordersCommand->GetProperty(L"borders")->AsBools();

                const std::set<size_t> rowStops =
                    LoadTableStops(table, rowBordersCommand->GetProperty(L"stops"));
                if (!borderFlags.empty())
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(
                            position.value(),
                            (!borderFlags.empty() ? borderFlags[0] : table->IsShowingTopBorder()),
                            (borderFlags.size() > 1 ? borderFlags[1] :
                                                      table->IsShowingRightBorder()),
                            (borderFlags.size() > 2 ? borderFlags[2] :
                                                      table->IsShowingBottomBorder()),
                            (borderFlags.size() > 3 ? borderFlags[3] :
                                                      table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(
                                i,
                                (!borderFlags.empty() ? borderFlags[0] :
                                                        table->IsShowingTopBorder()),
                                (borderFlags.size() > 1 ? borderFlags[1] :
                                                          table->IsShowingRightBorder()),
                                (borderFlags.size() > 2 ? borderFlags[2] :
                                                          table->IsShowingBottomBorder()),
                                (borderFlags.size() > 3 ? borderFlags[3] :
                                                          table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }

                // borders specified individually
                if (rowBordersCommand->HasProperty(L"top-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(),
                                             rowBordersCommand->GetProperty(L"top-border")
                                                 ->AsBool(table->IsShowingTopBorder()),
                                             std::nullopt, std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i,
                                                 rowBordersCommand->GetProperty(L"top-border")
                                                     ->AsBool(table->IsShowingTopBorder()),
                                                 std::nullopt, std::nullopt, std::nullopt,
                                                 rowStops);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"right-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(), std::nullopt,
                                             rowBordersCommand->GetProperty(L"right-border")
                                                 ->AsBool(table->IsShowingRightBorder()),
                                             std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i, std::nullopt,
                                                 rowBordersCommand->GetProperty(L"right-border")
                                                     ->AsBool(table->IsShowingRightBorder()),
                                                 std::nullopt, std::nullopt, rowStops);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"bottom-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(), std::nullopt, std::nullopt,
                                             rowBordersCommand->GetProperty(L"bottom-border")
                                                 ->AsBool(table->IsShowingBottomBorder()),
                                             std::nullopt);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i, std::nullopt, std::nullopt,
                                                 rowBordersCommand->GetProperty(L"bottom-border")
                                                     ->AsBool(table->IsShowingBottomBorder()),
                                                 std::nullopt);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"left-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(), std::nullopt, std::nullopt,
                                             std::nullopt,
                                             rowBordersCommand->GetProperty(L"left-border")
                                                 ->AsBool(table->IsShowingLeftBorder()),
                                             rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i, std::nullopt, std::nullopt, std::nullopt,
                                                 rowBordersCommand->GetProperty(L"left-border")
                                                     ->AsBool(table->IsShowingLeftBorder()),
                                                 rowStops);
                            }
                        }
                    }
                }
            }

        // change rows' content alignment
        const auto rowContentCommands = contentAlignNode->AsNodes();
        if (!rowContentCommands.empty())
            {
            for (const auto& rowContentCommand : rowContentCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, rowContentCommand);
                const auto hPageAlignment =
                    rowContentCommand->GetProperty(L"horizontal-page-alignment")->AsString();
                const std::set<size_t> colStops =
                    LoadTableStops(table, rowContentCommand->GetProperty(L"stops"));
                if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(
                            position.value(), PageHorizontalAlignment::LeftAligned, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(
                                i, PageHorizontalAlignment::LeftAligned, colStops);
                            }
                        }
                    }
                else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(
                            position.value(), PageHorizontalAlignment::RightAligned, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(
                                i, PageHorizontalAlignment::RightAligned, colStops);
                            }
                        }
                    }
                else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(
                            position.value(), PageHorizontalAlignment::Centered, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(
                                i, PageHorizontalAlignment::Centered, colStops);
                            }
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportNodeLoader::ApplyTableColumnFormatting(std::shared_ptr<Graphs::Table>& table,
                                                      const wxSimpleJSON::Ptr_t& formattingNode,
                                                      const wxSimpleJSON::Ptr_t& colorNode,
                                                      const wxSimpleJSON::Ptr_t& boldNode,
                                                      const wxSimpleJSON::Ptr_t& bordersNode) const
        {
        // change columns' cell formatting
        const auto columnFormattingCommands = formattingNode->AsNodes();
        if (!columnFormattingCommands.empty())
            {
            for (const auto& columnFormattingCommand : columnFormattingCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, columnFormattingCommand);
                const auto formatValue = ReportEnumConvert::ConvertTableCellFormat(
                    columnFormattingCommand->GetProperty(L"format")->AsString());

                const std::set<size_t> rowStops =
                    LoadTableStops(table, columnFormattingCommand->GetProperty(L"stops"));
                if (formatValue.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetColumnFormat(position.value(), formatValue.value(), rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnFormat(i, formatValue.value(), rowStops);
                            }
                        }
                    }
                }
            }

        // color the columns
        const auto colColorCommands = colorNode->AsNodes();
        if (!colColorCommands.empty())
            {
            for (const auto& colColorCommand : colColorCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, colColorCommand);
                const wxColour bgcolor(
                    m_reportBuilder.ConvertColor(colColorCommand->GetProperty(L"background")));
                const std::set<size_t> rowStops =
                    LoadTableStops(table, colColorCommand->GetProperty(L"stops"));
                if (bgcolor.IsOk())
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBackgroundColor(position.value(), bgcolor, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBackgroundColor(i, bgcolor, rowStops);
                            }
                        }
                    }
                }
            }

        // bold the columns
        const auto colBoldCommands = boldNode->AsNodes();
        if (!colBoldCommands.empty())
            {
            for (const auto& colBoldCommand : colBoldCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, colBoldCommand);
                const std::set<size_t> rowStops =
                    LoadTableStops(table, colBoldCommand->GetProperty(L"stops"));
                if (position.has_value())
                    {
                    table->BoldColumn(position.value(), rowStops);
                    }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        {
                        table->BoldColumn(i, rowStops);
                        }
                    }
                }
            }

        // change columns' borders
        const auto columnBordersCommands = bordersNode->AsNodes();
        if (!columnBordersCommands.empty())
            {
            for (const auto& columnBordersCommand : columnBordersCommands)
                {
                const auto [position, startPosition, endPosition] =
                    ReadPositions(table, columnBordersCommand);
                const auto borderFlags = columnBordersCommand->GetProperty(L"borders")->AsBools();

                const std::set<size_t> rowStops =
                    LoadTableStops(table, columnBordersCommand->GetProperty(L"stops"));
                if (!borderFlags.empty())
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(
                            position.value(),
                            (!borderFlags.empty() ? borderFlags[0] : table->IsShowingTopBorder()),
                            (borderFlags.size() > 1 ? borderFlags[1] :
                                                      table->IsShowingRightBorder()),
                            (borderFlags.size() > 2 ? borderFlags[2] :
                                                      table->IsShowingBottomBorder()),
                            (borderFlags.size() > 3 ? borderFlags[3] :
                                                      table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i,
                                (!borderFlags.empty() ? borderFlags[0] :
                                                        table->IsShowingTopBorder()),
                                (borderFlags.size() > 1 ? borderFlags[1] :
                                                          table->IsShowingRightBorder()),
                                (borderFlags.size() > 2 ? borderFlags[2] :
                                                          table->IsShowingBottomBorder()),
                                (borderFlags.size() > 3 ? borderFlags[3] :
                                                          table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }

                // borders specified individually
                if (columnBordersCommand->HasProperty(L"top-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(),
                                                columnBordersCommand->GetProperty(L"top-border")
                                                    ->AsBool(table->IsShowingTopBorder()),
                                                std::nullopt, std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(i,
                                                    columnBordersCommand->GetProperty(L"top-border")
                                                        ->AsBool(table->IsShowingTopBorder()),
                                                    std::nullopt, std::nullopt, std::nullopt,
                                                    rowStops);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"right-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(), std::nullopt,
                                                columnBordersCommand->GetProperty(L"right-border")
                                                    ->AsBool(table->IsShowingRightBorder()),
                                                std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i, std::nullopt,
                                columnBordersCommand->GetProperty(L"right-border")
                                    ->AsBool(table->IsShowingRightBorder()),
                                std::nullopt, std::nullopt, rowStops);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"bottom-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(), std::nullopt, std::nullopt,
                                                columnBordersCommand->GetProperty(L"bottom-border")
                                                    ->AsBool(table->IsShowingBottomBorder()),
                                                std::nullopt);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i, std::nullopt, std::nullopt,
                                columnBordersCommand->GetProperty(L"bottom-border")
                                    ->AsBool(table->IsShowingBottomBorder()),
                                std::nullopt);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"left-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(), std::nullopt, std::nullopt,
                                                std::nullopt,
                                                columnBordersCommand->GetProperty(L"left-border")
                                                    ->AsBool(table->IsShowingLeftBorder()),
                                                rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i, std::nullopt, std::nullopt, std::nullopt,
                                columnBordersCommand->GetProperty(L"left-border")
                                    ->AsBool(table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    std::optional<size_t>
    ReportNodeLoader::LoadTablePosition(const wxSimpleJSON::Ptr_t& positionNode,
                                        std::shared_ptr<Graphs::Table> table)
        {
        if (!positionNode->IsOk())
            {
            return std::nullopt;
            }

        std::optional<size_t> position;

        const auto loadStringToPosition = [&](const auto& originStr)
        {
            if (originStr.CmpNoCase(L"last-column") == 0)
                {
                position = table->GetLastDataColumn();
                }
            else if (originStr.CmpNoCase(L"last-row") == 0)
                {
                position = table->GetLastDataRow();
                }
            else if (originStr.StartsWith(L"column:"))
                {
                if (const auto colPos =
                        (table ? table->FindColumnIndex(originStr.substr(7)) : std::nullopt);
                    colPos.has_value())
                    {
                    position = colPos;
                    }
                }
            else if (originStr.StartsWith(L"row:"))
                {
                if (const auto colPos =
                        (table ? table->FindRowIndex(originStr.substr(4)) : std::nullopt);
                    colPos.has_value())
                    {
                    position = colPos;
                    }
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: unknown table position origin value."), originStr)
                        .ToUTF8());
                }
        };

        const auto origin = positionNode->GetProperty(L"origin");
        if (origin->IsOk())
            {
            if (origin->IsValueString())
                {
                loadStringToPosition(origin->AsString());
                }
            else if (origin->IsValueNumber())
                {
                position = origin->AsDouble();
                }
            }
        else if (positionNode->IsValueString())
            {
            loadStringToPosition(positionNode->AsString());
            }
        else if (positionNode->IsValueNumber())
            {
            position = positionNode->AsDouble();
            }
        const std::optional<double> doubleStartOffset =
            positionNode->HasProperty(L"offset") ?
                std::optional<double>(positionNode->GetProperty(L"offset")->AsDouble()) :
                std::nullopt;
        if (position.has_value() && doubleStartOffset.has_value())
            {
            // value() should work here, but GCC throws an
            // 'uninitialized value' false positive warning
            position = position.value_or(0) + doubleStartOffset.value_or(0);
            }

        return position;
        }
    } // namespace Wisteria
