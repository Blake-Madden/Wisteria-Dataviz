///////////////////////////////////////////////////////////////////////////////
// Name:        reportslideexportbase.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportslideexportbase.h"
#include "../base/graphitems.h"
#include "../base/settings.h"
#include "../math/safe_math.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/dcsvg.h>
#include <wx/graphics.h>
#include <wx/mstream.h>

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::EscapeXml(const wxString& str)
    {
    wxString result;
    result.reserve(static_cast<size_t>(str.length() * 1.1));
    for (size_t charIndex = 0; charIndex < str.length(); ++charIndex)
        {
        const wxUniChar currentChar = str[charIndex];
        if (currentChar == L'&')
            {
            result += L"&amp;";
            }
        else if (currentChar == L'<')
            {
            result += L"&lt;";
            }
        else if (currentChar == L'>')
            {
            result += L"&gt;";
            }
        else if (currentChar == L'"')
            {
            result += L"&quot;";
            }
        else if (currentChar == L'\'')
            {
            result += L"&apos;";
            }
        else if (currentChar >= 0x20 || currentChar == L'\t' || currentChar == L'\n' ||
                 currentChar == L'\r')
            {
            result += currentChar;
            }
        }
    return result;
    }

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::EscapeXmlAttribute(const wxString& str)
    {
    wxString collapsed{ str };
    collapsed.Replace(L"\r\n", L" ");
    collapsed.Replace(L"\r", L" ");
    collapsed.Replace(L"\n", L" ");
    collapsed.Replace(L"\t", L" ");
    while (collapsed.Replace(L"  ", L" ") > 0)
        {
        }
    return EscapeXml(collapsed.Trim(true).Trim(false));
    }

//------------------------------------------------------
std::vector<Wisteria::ReportSlideExportBase::TableSlide>
Wisteria::ReportSlideExportBase::BuildTableSlides(const ReportExportPage& page,
                                                  const double slideWidthInches,
                                                  const double slideHeightInches)
    {
    constexpr int DEFAULT_FONT_POINTS{ 12 };
    constexpr int MIN_FONT_POINTS{ 8 };
    constexpr int MEASURE_FONT_PIXELS{ 16 }; // 12 points at 96 DPI
    constexpr double DIPS_PER_INCH{ 96.0 };
    constexpr double POINTS_PER_INCH{ 72.0 };
    constexpr double LINE_SPACING{ 1.2 };

    const ReportTableData table{ page.GetTableData(MEASURE_FONT_PIXELS, 0) };
    if (table.m_columns.empty())
        {
        return {};
        }

    const double tableWidth{ std::max(1.0, slideWidthInches - (TABLE_MARGIN_INCHES * 2)) };
    // cell margins get narrower if they would take up more than this share of the table
    constexpr double MAX_MARGIN_SHARE{ 0.5 };
    const double columnCount{ static_cast<double>(table.m_columns.size()) };
    const double cellMarginX{ std::min(
        TABLE_CELL_MARGIN_X_INCHES, safe_divide(tableWidth * MAX_MARGIN_SHARE, columnCount * 2)) };
    const double textSpace{ tableWidth - (columnCount * cellMarginX * 2) };

    double textWidth{ 0.0 };
    for (const auto& column : table.m_columns)
        {
        textWidth += column.m_width / DIPS_PER_INCH;
        }

    // shrink the font to fit the widest text, and let the text wrap if that isn't enough
    const double fitScale{ (textWidth > textSpace) ? safe_divide(textSpace, textWidth) : 1.0 };
    const int fontPoints{ std::max(MIN_FONT_POINTS,
                                   static_cast<int>(std::floor(DEFAULT_FONT_POINTS * fitScale))) };
    const double fontScale{ static_cast<double>(fontPoints) / DEFAULT_FONT_POINTS };

    // Columns get the text width they need, as long as it's within their fair share of the
    // space. That share grows as narrow columns leave some of theirs for the wider ones.
    std::vector<double> neededWidths;
    neededWidths.reserve(table.m_columns.size());
    for (const auto& column : table.m_columns)
        {
        neededWidths.push_back(column.m_width / DIPS_PER_INCH * fontScale);
        }
    std::vector<double> textWidths(neededWidths.size(), 0.0);
    std::vector<bool> isFitted(neededWidths.size(), false);
    double spaceLeft{ textSpace };
    size_t columnsLeft{ neededWidths.size() };
    bool fittedColumn{ true };
    while (fittedColumn && columnsLeft > 0)
        {
        fittedColumn = false;
        const double fairShare{ safe_divide(spaceLeft, static_cast<double>(columnsLeft)) };
        for (size_t col = 0; col < neededWidths.size(); ++col)
            {
            if (!isFitted[col] && neededWidths[col] <= fairShare)
                {
                textWidths[col] = neededWidths[col];
                isFitted[col] = true;
                spaceLeft -= neededWidths[col];
                --columnsLeft;
                fittedColumn = true;
                }
            }
        }

    // The columns that don't fit split what's left, and wrap their text.
    // If they all fit, then the leftover space is shared by every column.
    const double sharedWidth{ safe_divide(
        spaceLeft, static_cast<double>((columnsLeft > 0) ? columnsLeft : neededWidths.size())) };
    std::vector<double> columnWidths;
    columnWidths.reserve(neededWidths.size());
    for (size_t col = 0; col < neededWidths.size(); ++col)
        {
        if (!isFitted[col])
            {
            textWidths[col] = sharedWidth;
            }
        else if (columnsLeft == 0)
            {
            textWidths[col] += sharedWidth;
            }
        columnWidths.push_back(textWidths[col] + (cellMarginX * 2));
        }

    // only the widest text in each column was measured, so the others are estimated from
    // their share of its characters
    std::vector<wxString> labels;
    labels.reserve(table.m_columns.size());
    std::vector<size_t> longestText;
    longestText.reserve(table.m_columns.size());
    for (const auto& column : table.m_columns)
        {
        labels.push_back(column.m_label);
        longestText.push_back(column.m_label.length());
        }
    for (const auto& row : table.m_rows)
        {
        for (size_t col = 0; col < row.size(); ++col)
            {
            longestText[col] = std::max(longestText[col], row[col].length());
            }
        }
    const auto countLines = [&](const std::vector<wxString>& cells)
    {
        int lines{ 1 };
        for (size_t col = 0; col < cells.size(); ++col)
            {
            const double cellWidth{ safe_divide(neededWidths[col] *
                                                    static_cast<double>(cells[col].length()),
                                                static_cast<double>(longestText[col])) };
            if (cellWidth > textWidths[col])
                {
                // words don't break evenly, so allow for an extra line
                lines = std::max(
                    lines,
                    static_cast<int>(std::ceil(safe_divide(cellWidth, textWidths[col]))) + 1);
                }
            }
        return lines;
    };

    const double lineHeight{ fontPoints * LINE_SPACING / POINTS_PER_INCH };
    const double cellMargins{ TABLE_CELL_MARGIN_Y_INCHES * 2 };
    const double headerHeight{ (countLines(labels) * lineHeight) + cellMargins };
    const double titleHeight{ page.m_title.empty() ? 0.0 : TABLE_TITLE_HEIGHT_INCHES };
    const double tableTop{ TABLE_MARGIN_INCHES + titleHeight };
    const double tableHeight{ slideHeightInches - tableTop - TABLE_MARGIN_INCHES };
    const int maxLines{ std::max(1, static_cast<int>(std::floor(safe_divide(
                                        tableHeight - headerHeight - cellMargins, lineHeight)))) };

    // a row can't span slides, so a row taller than a slide continues in further rows
    std::vector<TableSlideRow> slideRows;
    slideRows.reserve(table.m_rows.size());
    for (size_t rowIndex = 0; rowIndex < table.m_rows.size(); ++rowIndex)
        {
        const bool isBanded{ rowIndex % 2 != 0 };
        std::vector<wxString> cells{ table.m_rows[rowIndex] };
        while (countLines(cells) > maxLines)
            {
            // the extra line that countLines() allows must also fit
            const double fullLines{ static_cast<double>(std::max(maxLines - 1, 1)) };
            std::vector<wxString> firstCells(cells.size());
            for (size_t col = 0; col < cells.size(); ++col)
                {
                const double maxChars{ safe_divide(textWidths[col] * fullLines *
                                                       static_cast<double>(longestText[col]),
                                                   neededWidths[col]) };
                if (static_cast<double>(cells[col].length()) <= maxChars)
                    {
                    firstCells[col] = cells[col];
                    cells[col].clear();
                    continue;
                    }
                const size_t hardBreak{ std::max<size_t>(static_cast<size_t>(maxChars), 1) };
                const size_t spacePosition{ cells[col].rfind(L' ', hardBreak) };
                const size_t breakPosition{ (spacePosition != wxString::npos && spacePosition > 0) ?
                                                spacePosition :
                                                hardBreak };
                firstCells[col] = cells[col].substr(0, breakPosition);
                cells[col] = cells[col].substr(breakPosition);
                cells[col].Trim(false);
                }
            const int lines{ countLines(firstCells) };
            slideRows.push_back(TableSlideRow{ .m_cells = std::move(firstCells),
                                               .m_alignments = table.m_cellAlignments[rowIndex],
                                               .m_height = (lines * lineHeight) + cellMargins,
                                               .m_isBanded = isBanded });
            }
        const int lines{ countLines(cells) };
        slideRows.push_back(TableSlideRow{ .m_cells = std::move(cells),
                                           .m_alignments = table.m_cellAlignments[rowIndex],
                                           .m_height = (lines * lineHeight) + cellMargins,
                                           .m_isBanded = isBanded });
        }

    const auto watermark{ page.GetWatermarkLayout(slideWidthInches * POINTS_PER_INCH,
                                                  slideHeightInches * POINTS_PER_INCH) };

    std::vector<TableSlide> slides;
    const auto startSlide = [&]()
    {
        TableSlide slide;
        slide.m_title = (slides.empty() || page.m_title.empty()) ?
                            page.m_title :
                            wxString::Format(_(L"%s (continued)"), page.m_title);
        slide.m_columns = table.m_columns;
        slide.m_columnWidths = columnWidths;
        slide.m_cellMarginX = cellMarginX;
        slide.m_fontPoints = fontPoints;
        slide.m_headerHeight = headerHeight;
        slide.m_top = tableTop;
        slide.m_slideWidth = slideWidthInches;
        slide.m_slideHeight = slideHeightInches;
        slide.m_watermark = watermark;
        slides.push_back(std::move(slide));
    };

    double usedHeight{ 0.0 };
    for (auto& slideRow : slideRows)
        {
        if (slides.empty() ||
            (!slides.back().m_rows.empty() && usedHeight + slideRow.m_height > tableHeight))
            {
            startSlide();
            usedHeight = headerHeight;
            }
        usedHeight += slideRow.m_height;
        slides.back().m_rows.push_back(std::move(slideRow));
        }
    return slides;
    }

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::CollectAccessibilityText(Canvas* canvas)
    {
    if (canvas == nullptr)
        {
        return {};
        }

    wxArrayString lines;
    const auto addTitles = [&lines](const std::vector<GraphItems::Label>& titles)
    {
        for (const auto& title : titles)
            {
            if (!title.GetText().empty())
                {
                lines.Add(title.GetText());
                }
            }
    };
    addTitles(canvas->GetTopTitles());
    addTitles(canvas->GetLeftTitles());
    addTitles(canvas->GetRightTitles());
    addTitles(canvas->GetBottomTitles());

    const auto [rowCount, columnCount] = canvas->GetFixedObjectsGridSize();
    for (size_t row = 0; row < rowCount; ++row)
        {
        for (size_t column = 0; column < columnCount; ++column)
            {
            const auto fixedObject = canvas->GetFixedObject(row, column);
            if (fixedObject == nullptr)
                {
                continue;
                }
            wxString label = fixedObject->GetAccessibilityAttributes().GetAriaLabel();
            if (label.empty())
                {
                label = fixedObject->GetAutoAccessibilityAttributes().GetAriaLabel();
                }
            if (!label.empty())
                {
                wxString buffer;
                Wisteria::GraphItems::GraphItemBase::AddAccessibilityAttribute(buffer, label,
                                                                               wxString{});
                lines.Add(buffer);
                }
            }
        }

    wxString result;
    for (size_t lineIndex = 0; lineIndex < lines.GetCount(); ++lineIndex)
        {
        if (lineIndex > 0)
            {
            result += L"\n\n";
            }
        result += lines[lineIndex];
        }
    return result;
    }

//------------------------------------------------------
void Wisteria::ReportSlideExportBase::RenderCanvas(Canvas* canvas, const wxSize renderSize,
                                                   wxString& svgOut, wxMemoryBuffer* pngOut)
    {
    const wxSize safeSize{ std::max(1, renderSize.GetWidth()),
                           std::max(1, renderSize.GetHeight()) };

    const wxWindowUpdateLocker updateLocker{ canvas };

    // temporarily lay the canvas out at the render size
    const int origMinWidth{ canvas->GetCanvasMinWidthDIPs() };
    const int origMinHeight{ canvas->GetCanvasMinHeightDIPs() };
    const wxSize origSize{ canvas->GetSize() };
    if (canvas->IsFittingToPageWhenPrinting())
        {
        canvas->SetCanvasMinWidthDIPs(safeSize.GetWidth());
        canvas->SetCanvasMinHeightDIPs(safeSize.GetHeight());
        // calling SetSize before CalcRowDimensions() is needed because some
        // internals look at the window size rather than the min width/height
        canvas->SetSize(canvas->FromDIP(safeSize));
        canvas->CalcRowDimensions();
        canvas->SetSize(canvas->FromDIP(safeSize));
        }

    // PNG first, mirroring the raster path in Canvas::Save so the layout the
    // resize produced is drawn without an intervening re-measure.
    if (pngOut != nullptr)
        {
        wxBitmap exportBmp;
        exportBmp.CreateWithDIPSize(safeSize, canvas->GetDPIScaleFactor());
        GraphItems::Image::SetOpacity(exportBmp, wxALPHA_OPAQUE, false);

            {
            wxMemoryDC memDc{ exportBmp };
            memDc.Clear();
            const wxEventBlocker blocker{ canvas };
#ifdef __WXMSW__
            wxGraphicsContext* graphicsContext{ nullptr };
            auto* d2dRenderer = wxGraphicsRenderer::GetDirect2DRenderer();
            if (d2dRenderer != nullptr)
                {
                graphicsContext = d2dRenderer->CreateContext(memDc);
                }
            if (graphicsContext != nullptr)
                {
                wxGCDC gcdc{ graphicsContext };
                canvas->OnDraw(gcdc);
                canvas->DrawWatermarkLabel(gcdc);
                }
            else
                {
                wxGCDC gcdc{ memDc };
                canvas->OnDraw(gcdc);
                canvas->DrawWatermarkLabel(gcdc);
                }
#else
            wxGCDC gcdc{ memDc };
            canvas->OnDraw(gcdc);
            canvas->DrawWatermarkLabel(gcdc);
#endif
            memDc.SelectObject(wxNullBitmap);
            }

        wxImage exportImg{ exportBmp.ConvertToImage() };
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES);
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONX,
                            Settings::GetImageResolutionDPI().GetWidth());
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONY,
                            Settings::GetImageResolutionDPI().GetHeight());
        exportImg.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9);
        if (!canvas->GetLabel().empty())
            {
            exportImg.SetOption(wxIMAGE_OPTION_PNG_DESCRIPTION, canvas->GetLabel());
            }

        wxMemoryOutputStream pngStream;
        exportImg.SaveFile(pngStream, wxBITMAP_TYPE_PNG);

        const size_t pngLength{ static_cast<size_t>(pngStream.GetLength()) };
        *pngOut = wxMemoryBuffer(pngLength);
        pngStream.CopyTo(pngOut->GetData(), pngLength);
        pngOut->SetDataLen(pngLength);
        }

        // SVG
        {
        wxSVGFileDC svgDc{ wxString{}, safeSize.GetWidth(), safeSize.GetHeight(), wxSVG_DEFAULT_DPI,
                           canvas->GetLabel() };
        svgDc.SetBitmapHandler(new wxSVGBitmapEmbedHandler{});
        const wxEventBlocker blocker{ canvas };
        canvas->CalcAllSizes(svgDc);
        canvas->OnDraw(svgDc);
        canvas->DrawWatermarkLabel(svgDc);
        svgOut = svgDc.GetSVGDocument();
        }

    // restore the canvas's original size and layout
    if (canvas->IsFittingToPageWhenPrinting())
        {
        canvas->SetCanvasMinWidthDIPs(origMinWidth);
        canvas->SetCanvasMinHeightDIPs(origMinHeight);
        canvas->SetSize(origSize);
        canvas->CalcRowDimensions();
        canvas->SetSize(origSize);
        }

        // restore the on-screen measurements
        {
        wxGCDC screenDc{ canvas };
        canvas->CalcAllSizes(screenDc);
        }
    canvas->ResetResizeDelay();
    canvas->ZoomReset();
    canvas->SendSizeEvent();
    canvas->Refresh();
    }

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::ColorToHex(const wxColour& color)
    {
    return color.GetAsString(wxC2S_HTML_SYNTAX).AfterFirst(L'#').Upper();
    }
