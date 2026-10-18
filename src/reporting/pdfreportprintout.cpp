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
#include <array>
#include <wx/paper.h>
#include <wx/pdfdc.h>

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
    {
    if (canvases.empty() || filePath.empty())
        {
        return;
        }

    // refresh auto-generated accessibility text on every page before rendering,
    // so descriptions reflect each canvas's current state
    for (auto* canvas : canvases)
        {
        if (canvas != nullptr)
            {
            canvas->ApplyAutoAccessibilityAttributes();
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

    const size_t pageCount{ canvases.size() };
    int pageNum{ 0 };
    for (auto* canvas : canvases)
        {
        ++pageNum;
        if (canvas == nullptr)
            {
            continue;
            }

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
