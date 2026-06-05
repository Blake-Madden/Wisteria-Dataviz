///////////////////////////////////////////////////////////////////////////////
// Name:        pdfreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdfreportprintout.h"
#include "../app/wisteriaapp.h"
#include <wx/paper.h>
#include <wx/pdfdc.h>

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

    if (!pdfDC.StartDoc(options.m_title))
        {
        wxMessageBox(wxString::Format(_(L"Failed to create PDF document \"%s\"."), filePath),
                     _(L"Export Error"), wxOK | wxICON_ERROR);
        return;
        }

    pdfDC.GetPdfDocument()->SetTitle(options.m_title);
    pdfDC.GetPdfDocument()->SetAuthor(options.m_author);
    pdfDC.GetPdfDocument()->SetSubject(options.m_subject);
    pdfDC.GetPdfDocument()->SetKeywords(options.m_keywords);
    pdfDC.GetPdfDocument()->SetCompression(options.m_compress);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    pdfDC.GetPdfDocument()->SetCreator(wxTheApp->GetAppDisplayName());

    for (auto* canvas : canvases)
        {
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

        int dipW{ 0 }, dipH{ 0 };
        if (paper != nullptr)
            {
            const bool landscape = (printData.GetOrientation() == wxLANDSCAPE);
            const auto paperSzTenthsMM = paper->GetSize();
            const wxSize ptsSize(
                wxRound((landscape ? paperSzTenthsMM.GetHeight() : paperSzTenthsMM.GetWidth()) /
                        254.0 * 72.0),
                wxRound((landscape ? paperSzTenthsMM.GetWidth() : paperSzTenthsMM.GetHeight()) /
                        254.0 * 72.0));
            const wxSize dipSize = pdfDC.ToDIP(ptsSize);
            dipW = dipSize.GetWidth() - leftMargin - rightMargin;
            dipH = dipSize.GetHeight() - topMargin - bottomMargin;
            }

        if (dipW <= 0 || dipH <= 0)
            {
            // fallback to US Letter
            const wxSize dipSize = pdfDC.ToDIP(wxSize{ wxRound(8.5 * 72.0), wxRound(11.0 * 72.0) });
            dipW = dipSize.GetWidth() - leftMargin - rightMargin;
            dipH = dipSize.GetHeight() - topMargin - bottomMargin;
            }

        pdfDC.SetDeviceOrigin(leftMargin, topMargin);

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
            canvas->OnDraw(pdfDC);
            canvas->DrawWatermarkLabel(pdfDC);
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
        canvas->SendSizeEvent();
        canvas->Refresh();
        }

    pdfDC.EndDoc();
    }
