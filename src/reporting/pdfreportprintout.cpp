///////////////////////////////////////////////////////////////////////////////
// Name:        pdfreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdfreportprintout.h"
#include "../app/wisteriaapp.h"
#include "reportprintout.h"
#include <algorithm>
#include <array>
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/paper.h>
#include <wx/pdfdc.h>
#include <wx/pdfgridctrl.h>
#include <wx/pdflistctrl.h>
#include <wx/settings.h>

//------------------------------------------------------
bool Wisteria::ReportPDFExport::HasStripText(const std::array<wxString, 3>& text)
    {
    return std::ranges::any_of(text, [](const wxString& str) { return !str.empty(); });
    }

//------------------------------------------------------
void Wisteria::ReportPDFExport::ApplyTableMargins(wxPdfDocument& doc, const ReportExportPage& page,
                                                  const int stripHeight)
    {
    const double headerSpace{ HasStripText(page.m_headers) ? stripHeight + TABLE_STRIP_GAP : 0.0 };
    const double footerSpace{ HasStripText(page.m_footers) ? stripHeight + TABLE_STRIP_GAP : 0.0 };
    doc.SetMargins(TABLE_SIDE_MARGIN, TABLE_EDGE_MARGIN + headerSpace, TABLE_SIDE_MARGIN);
    // the table starts its own pages, so that the strips can be drawn on each one
    doc.SetAutoPageBreak(false, TABLE_EDGE_MARGIN + footerSpace);
    }

//------------------------------------------------------
void Wisteria::ReportPDFExport::AddTable(wxPdfDocument& doc, const ReportExportPage& page,
                                         const int stripHeight,
                                         const std::function<void()>& pageBreakHandler)
    {
    const double titleSpace{ page.m_title.empty() ? 0.0 : stripHeight + TABLE_STRIP_GAP };
    doc.SetXY(doc.GetLeftMargin(), doc.GetTopMargin() + titleSpace);

    const auto applyOptions = [&pageBreakHandler](auto& tableOptions)
    {
        tableOptions.SetFitToPage(true);
        tableOptions.SetAlternateRowBackgroundColour(wxPdfColour{ 240, 240, 240 });
        tableOptions.SetBorderColour(wxPdfColour{ 200, 200, 200 });
        tableOptions.SetPageBreakHandler(pageBreakHandler);
    };

    if (const auto* const list = std::get_if<const wxListCtrl*>(&page.m_content))
        {
        wxPdfListCtrlOptions listOptions;
        applyOptions(listOptions);
        doc.AddList(*list, listOptions);
        }
    else if (const auto* const grid = std::get_if<const wxGrid*>(&page.m_content))
        {
        wxPdfGridOptions gridOptions;
        applyOptions(gridOptions);
        doc.AddGrid(*grid, gridOptions);
        }
    }

//------------------------------------------------------
size_t Wisteria::ReportPDFExport::CountTablePages(const ReportExportPage& page,
                                                  const wxPrintData& printData,
                                                  const int stripHeight)
    {
    // lay the table out in a throwaway document with the same paper and margins
    wxPdfDocument doc(printData.GetOrientation(), wxString{ L"pt" }, printData.GetPaperId());
    doc.Open();
    ApplyTableMargins(doc, page, stripHeight);
    doc.AddPage();
    AddTable(doc, page, stripHeight, std::function<void()>{});
    return static_cast<size_t>(doc.PageNo());
    }

//------------------------------------------------------
void Wisteria::ReportPDFExport::DrawPrintStrip(wxDC& dc, const wxRect& stripRect,
                                               const double scaling,
                                               const std::array<wxString, 3>& text,
                                               const PageVerticalAlignment vertAlign)
    {
    constexpr std::array<PageHorizontalAlignment, 3> horizAligns{
        PageHorizontalAlignment::LeftAligned, PageHorizontalAlignment::Centered,
        PageHorizontalAlignment::RightAligned
    };
    constexpr std::array<TextAlignment, 3> textAligns{ TextAlignment::FlushLeft,
                                                       TextAlignment::Centered,
                                                       TextAlignment::FlushRight };

    for (size_t i = 0; i < text.size(); ++i)
        {
        if (text[i].empty())
            {
            continue;
            }
        GraphItems::Label label(GraphItems::GraphItemInfo{ text[i] }
                                    .Anchoring(Anchoring::TopLeftCorner)
                                    .Padding(0, 0, 0, 0)
                                    .Pen(wxNullPen)
                                    .LabelAlignment(textAligns[i])
                                    .LabelPageVerticalAlignment(vertAlign)
                                    .LabelPageHorizontalAlignment(horizAligns[i])
                                    .DPIScaling(dc.GetContentScaleFactor()));
        label.SetBoundingBox(stripRect, dc, scaling);
        label.Draw(dc);
        }
    }

//------------------------------------------------------
Wisteria::ReportPDFExport::ReportPDFExport(const std::vector<Canvas*>& canvases,
                                           const wxString& filePath,
                                           const PdfExportOptions& options)
    : ReportPDFExport(ReportExportPage::FromCanvases(canvases), filePath, options)
    {
    }

//------------------------------------------------------
Wisteria::ReportPDFExport::ReportPDFExport(const std::vector<ReportExportPage>& pages,
                                           const wxString& filePath,
                                           const PdfExportOptions& options)
    {
    if (pages.empty() || filePath.empty())
        {
        return;
        }

    // refresh auto-generated accessibility text on every page before rendering,
    // so descriptions reflect each canvas's current state
    for (const auto& page : pages)
        {
        const auto* const canvasHolder = std::get_if<Canvas*>(&page.m_content);
        if (canvasHolder != nullptr && *canvasHolder != nullptr)
            {
            (*canvasHolder)->ApplyAutoAccessibilityAttributes();
            }
        }

    wxPrintData printData;
    printData.SetOrientation(options.m_paperOrientation);
    printData.SetPaperId(options.m_paperSize);
    printData.SetFilename(filePath);
    wxPdfDC pdfDC(printData);
    // render fonts at their point size (rather than scaled to the screen's PPI)
    // so that text matches the DC's DIP scaling
    pdfDC.SetMapModeStyle(wxPDF_MAPMODESTYLE_PDF);
    pdfDC.SetMapMode(wxMM_POINTS);

    if (!pdfDC.StartDoc(options.m_title))
        {
        Settings::ReportError(
            wxString::Format(_(L"Failed to create PDF document \"%s\"."), filePath),
            _(L"Export Error"));
        return;
        }

    pdfDC.GetPdfDocument()->SetTitle(options.m_title);
    pdfDC.GetPdfDocument()->SetAuthor(options.m_author);
    pdfDC.GetPdfDocument()->SetSubject(options.m_subject);
    pdfDC.GetPdfDocument()->SetKeywords(options.m_keywords);
    pdfDC.GetPdfDocument()->SetCompression(options.m_compress);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    pdfDC.GetPdfDocument()->SetCreator(wxTheApp->GetAppDisplayName());

    wxPdfDocument* const pdfDoc{ pdfDC.GetPdfDocument() };
    // the DC's logical units and its document's units are both points at 72 PPI
    const int tableStripHeight{ pdfDC.FromDIP(wxSize{ 0, 24 }).GetHeight() };

    // tables can span multiple pages, so their page counts must be measured
    // for the headers and footers to show the correct total
    size_t pageCount{ 0 };
    for (const auto& page : pages)
        {
        if (std::holds_alternative<Canvas*>(page.m_content))
            {
            ++pageCount;
            }
        else if (page.HasTableData())
            {
            pageCount += CountTablePages(page, printData, tableStripHeight);
            }
        }

    int pageNum{ 0 };
    for (const auto& page : pages)
        {
        const bool isTable{ !std::holds_alternative<Canvas*>(page.m_content) };
        if (isTable && !page.HasTableData())
            {
            continue;
            }

        ++pageNum;
        if (!isTable && std::get<Canvas*>(page.m_content) == nullptr)
            {
            continue;
            }

        if (isTable)
            {
            const auto expandStrip = [&](const std::array<wxString, 3>& text)
            {
                std::array<wxString, 3> expanded;
                std::ranges::transform(text, expanded.begin(),
                                       [&](const wxString& str)
                                       {
                                           return Wisteria::ReportPrintout::ExpandPrintString(
                                               str, pageNum, pageCount, options.m_title);
                                       });
                return expanded;
            };
            const auto drawTableStrips = [&]()
            {
                const int stripLeft{ wxRound(TABLE_SIDE_MARGIN) };
                const int stripWidth{ wxRound(pdfDoc->GetPageWidth() - TABLE_SIDE_MARGIN * 2) };
                // Exporter draws directly on the document, so the DC's cached font may not
                // be what the document is using. Clearing it forces the next font to be applied.
                pdfDC.SetFont(wxNullFont);
                if (HasStripText(page.m_headers))
                    {
                    DrawPrintStrip(pdfDC,
                                   wxRect{ stripLeft, wxRound(TABLE_EDGE_MARGIN), stripWidth,
                                           tableStripHeight },
                                   1.0, expandStrip(page.m_headers),
                                   PageVerticalAlignment::TopAligned);
                    }
                if (HasStripText(page.m_footers))
                    {
                    DrawPrintStrip(pdfDC,
                                   wxRect{ stripLeft,
                                           wxRound(pdfDoc->GetPageHeight() - TABLE_EDGE_MARGIN) -
                                               tableStripHeight,
                                           stripWidth, tableStripHeight },
                                   1.0, expandStrip(page.m_footers),
                                   PageVerticalAlignment::BottomAligned);
                    }
                if (!page.m_watermark.m_label.empty())
                    {
                    const wxPdfColour textColour{ pdfDoc->GetTextColour() };
                    pdfDC.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Bold());
                    Canvas::DrawWatermarkLabel(pdfDC,
                                               wxRect{ 0, 0, wxRound(pdfDoc->GetPageWidth()),
                                                       wxRound(pdfDoc->GetPageHeight()) },
                                               page.m_watermark, 1.0);
                    // the exporter draws directly on the document, so it would inherit these
                    pdfDoc->SetAlpha(1.0, 1.0);
                    pdfDoc->SetTextColour(textColour);
                    pdfDC.SetFont(wxNullFont);
                    }
            };

            const double savedLeftMargin{ pdfDoc->GetLeftMargin() };
            const double savedTopMargin{ pdfDoc->GetTopMargin() };
            const double savedRightMargin{ pdfDoc->GetRightMargin() };
            const double savedBreakMargin{ pdfDoc->GetBreakMargin() };
            // the document has no other getter for this setting
            const bool savedAutoPageBreak{ pdfDoc->AcceptPageBreak() };

            ApplyTableMargins(*pdfDoc, page, tableStripHeight);
            pdfDC.SetDeviceOrigin(0, 0);
            pdfDC.StartPage();
            if (!page.m_title.empty())
                {
                DrawPrintStrip(pdfDC,
                               wxRect{ wxRound(TABLE_SIDE_MARGIN), wxRound(pdfDoc->GetTopMargin()),
                                       wxRound(pdfDoc->GetPageWidth() - TABLE_SIDE_MARGIN * 2),
                                       tableStripHeight },
                               1.0, { wxString{}, page.m_title, wxString{} },
                               PageVerticalAlignment::TopAligned);
                }
            AddTable(*pdfDoc, page, tableStripHeight,
                     [&]()
                     {
                         drawTableStrips();
                         pdfDC.EndPage();
                         pdfDC.StartPage();
                         ++pageNum;
                     });
            // the handler is not called for the table's last page
            drawTableStrips();
            pdfDC.EndPage();
            pdfDoc->SetMargins(savedLeftMargin, savedTopMargin, savedRightMargin);
            pdfDoc->SetAutoPageBreak(savedAutoPageBreak, savedBreakMargin);
            // the strips are optional, so a following page can't rely on them having reset this
            pdfDC.SetFont(wxNullFont);
            continue;
            }

        auto* const canvas{ std::get<Canvas*>(page.m_content) };
        const wxWindowUpdateLocker updateLocker{ canvas };

        // compute page dimensions from the paper settings
        auto* paper = wxThePrintPaperDatabase->FindPaperType(printData.GetPaperId());
        if (paper == nullptr)
            {
            paper = wxThePrintPaperDatabase->FindPaperType(wxPAPER_A4);
            }

        const int leftMargin = pdfDC.ToDIP(wxSize{ 5, 0 }).GetWidth();
        const int topMargin = pdfDC.ToDIP(wxSize{ 0, 0 }).GetHeight();
        const int rightMargin = pdfDC.ToDIP(wxSize{ 10, 0 }).GetWidth();
        const int bottomMargin = pdfDC.ToDIP(wxSize{ 0, 10 }).GetHeight();

        // the DC's logical units per inch (not necessarily PDF points)
        const double pdfPPI{ static_cast<double>(pdfDC.GetPPI().GetWidth()) };

        int dipW{ 0 }, dipH{ 0 };
        if (paper != nullptr)
            {
            const bool landscape = (printData.GetOrientation() == wxLANDSCAPE);
            const auto paperSzTenthsMM = paper->GetSize();
            const wxSize logicalSize(
                wxRound((landscape ? paperSzTenthsMM.GetHeight() : paperSzTenthsMM.GetWidth()) /
                        254.0 * pdfPPI),
                wxRound((landscape ? paperSzTenthsMM.GetWidth() : paperSzTenthsMM.GetHeight()) /
                        254.0 * pdfPPI));
            const wxSize dipSize = pdfDC.ToDIP(logicalSize);
            dipW = dipSize.GetWidth() - leftMargin - rightMargin;
            dipH = dipSize.GetHeight() - topMargin - bottomMargin;
            }

        if (dipW <= 0 || dipH <= 0)
            {
            // fallback to US Letter
            const wxSize dipSize =
                pdfDC.ToDIP(wxSize{ wxRound(8.5 * pdfPPI), wxRound(11.0 * pdfPPI) });
            dipW = dipSize.GetWidth() - leftMargin - rightMargin;
            dipH = dipSize.GetHeight() - topMargin - bottomMargin;
            }

        // reserve room for the header and footer (if being used)
        constexpr int headerFooterStripDIPs{ 24 };
        const bool hasHeader = !canvas->GetLeftPrinterHeader().empty() ||
                               !canvas->GetCenterPrinterHeader().empty() ||
                               !canvas->GetRightPrinterHeader().empty();
        const bool hasFooter = !canvas->GetLeftPrinterFooter().empty() ||
                               !canvas->GetCenterPrinterFooter().empty() ||
                               !canvas->GetRightPrinterFooter().empty();
        const int headerStripDIPs{ hasHeader ? headerFooterStripDIPs : 0 };
        const int footerStripDIPs{ hasFooter ? headerFooterStripDIPs : 0 };
        // the page is laid out at full size and then scaled to fit between the strips
        const int availHeightDIPs{ dipH - (headerStripDIPs + footerStripDIPs) };
        const double contentScale{ safe_divide<double>(availHeightDIPs, dipH) };
        const int contentLeftDIPs{ leftMargin +
                                   static_cast<int>(dipW * (1.0 - contentScale) / 2.0) };
        const int contentTopDIPs{ topMargin + headerStripDIPs };

        pdfDC.SetDeviceOrigin(contentLeftDIPs, contentTopDIPs);

        // save original canvas state
        const int origMinWidth = canvas->GetCanvasMinWidthDIPs();
        const int origMinHeight = canvas->GetCanvasMinHeightDIPs();
        const wxSize origSize = canvas->GetSize();

        // Snapshot row and item proportions before the page-sized layout pass
        // so they can be directly restored afterward. CalcRowDimensions() would
        // re-measure via GetBoundingBox(), but at that point m_frameSize is in
        // PDF-DC coordinates rather than screen coordinates.
        const auto [numRows, numCols] = canvas->GetFixedObjectsGridSize();
        // {heightProportion, rowCount}
        std::vector<std::pair<double, size_t>> savedRowLayout;
        // canvas width proportion per item
        std::vector<std::vector<double>> savedWidths;
        if (canvas->IsFittingToPageWhenPrinting())
            {
            savedRowLayout.reserve(numRows);
            savedWidths.resize(numRows);
            for (size_t row = 0; row < numRows; ++row)
                {
                savedRowLayout.emplace_back(canvas->GetRowInfo(row).GetHeightProportion(),
                                            canvas->GetRowInfo(row).GetRowCount());
                savedWidths[row].resize(numCols);
                for (size_t col = 0; col < numCols; ++col)
                    {
                    const auto obj = canvas->GetFixedObject(row, col);
                    savedWidths[row][col] =
                        (obj != nullptr ? obj->GetCanvasWidthProportion() : 0.0);
                    }
                }
            }

        // resize canvas to match page dimensions so CalcAllSizes lays out for the page
        if (canvas->IsFittingToPageWhenPrinting())
            {
            canvas->SetCanvasMinWidthDIPs(dipW);
            canvas->SetCanvasMinHeightDIPs(dipH);
            canvas->SetSize(canvas->FromDIP(wxSize{ dipW, dipH }));
            // force the internal rect down to the page size; otherwise CalcAllSizes's
            // std::max(minDIPs, currentRectDIPs) leaves a wider landscape rect intact
            // and the canvas overflows the portrait page
            canvas->CalcRowDimensions();
            }

        pdfDC.StartPage();
            {
            const wxEventBlocker blocker{ canvas };
            canvas->CalcAllSizes(pdfDC);

            // the DC's graphics context (used for shapes) ignores the DC's device origin,
            // so apply the same offset to it while drawing this page
            auto* gc = pdfDC.GetGraphicsContext();
            if (gc != nullptr)
                {
                const wxPoint origin{ pdfDC.GetDeviceOrigin() };
                gc->PushState();
                gc->Translate(pdfDC.DeviceToLogicalXRel(origin.x),
                              pdfDC.DeviceToLogicalYRel(origin.y));
                gc->Scale(contentScale, contentScale);
                }
            pdfDC.SetUserScale(contentScale, contentScale);
            canvas->OnDraw(pdfDC);
            canvas->DrawWatermarkLabel(pdfDC);
            pdfDC.SetUserScale(1.0, 1.0);
            if (gc != nullptr)
                {
                gc->PopState();
                }

            if (hasHeader || hasFooter)
                {
                // the strips span the full page width, so use the page's origin
                pdfDC.SetDeviceOrigin(leftMargin, contentTopDIPs);
                // content area and strip size in the DC's units
                const wxSize contentSize{ pdfDC.FromDIP(wxSize{ dipW, availHeightDIPs }) };
                const int stripHeight{
                    pdfDC.FromDIP(wxSize{ 0, headerFooterStripDIPs }).GetHeight()
                };
                const wxRect canvasRect{ 0, 0, contentSize.GetWidth(), contentSize.GetHeight() };
                const auto expand = [&](const wxString& str)
                {
                    return Wisteria::ReportPrintout::ExpandPrintString(str, pageNum, pageCount,
                                                                       options.m_title);
                };
                if (hasHeader)
                    {
                    DrawPrintStrip(pdfDC,
                                   wxRect{ canvasRect.GetX(), canvasRect.GetY() - stripHeight,
                                           canvasRect.GetWidth(), stripHeight },
                                   canvas->GetScaling(),
                                   { expand(canvas->GetLeftPrinterHeader()),
                                     expand(canvas->GetCenterPrinterHeader()),
                                     expand(canvas->GetRightPrinterHeader()) },
                                   PageVerticalAlignment::TopAligned);
                    }
                if (hasFooter)
                    {
                    DrawPrintStrip(pdfDC,
                                   wxRect{ canvasRect.GetX(), canvasRect.GetBottom() + 1,
                                           canvasRect.GetWidth(), stripHeight },
                                   canvas->GetScaling(),
                                   { expand(canvas->GetLeftPrinterFooter()),
                                     expand(canvas->GetCenterPrinterFooter()),
                                     expand(canvas->GetRightPrinterFooter()) },
                                   PageVerticalAlignment::BottomAligned);
                    }
                }
            }
        pdfDC.EndPage();

        // restore original canvas dimensions
        if (canvas->IsFittingToPageWhenPrinting())
            {
            canvas->SetCanvasMinWidthDIPs(origMinWidth);
            canvas->SetCanvasMinHeightDIPs(origMinHeight);
            // restore saved proportions directly instead of calling CalcRowDimensions(),
            // which would use the now-PDF-coordinate m_frameSize values in items
            for (size_t row = 0; row < savedRowLayout.size(); ++row)
                {
                canvas->GetRowInfo(row)
                    .HeightProportion(savedRowLayout[row].first)
                    .RowCount(savedRowLayout[row].second);
                }
            for (size_t row = 0; row < savedWidths.size(); ++row)
                {
                for (size_t col = 0; col < savedWidths[row].size(); ++col)
                    {
                    const auto obj = canvas->GetFixedObject(row, col);
                    if (obj != nullptr)
                        {
                        obj->SetCanvasWidthProportion(savedWidths[row][col]);
                        }
                    }
                }
            canvas->SetSize(origSize);
            }
        canvas->ResetResizeDelay();
        canvas->ZoomReset();
        canvas->SendSizeEvent();
        canvas->Refresh();
        }

    pdfDC.EndDoc();
    }
