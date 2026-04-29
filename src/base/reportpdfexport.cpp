///////////////////////////////////////////////////////////////////////////////
// Name:        reportpdfexport.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportpdfexport.h"

#ifdef INCLUDE_PDF

    #include <wx/paper.h>
    #include <wx/pdfdc.h>

//------------------------------------------------------
Wisteria::ReportPDFExport::ReportPDFExport(const std::vector<Canvas*>& canvases,
                                           const wxString& filePath, const wxString& title)
    {
    if (canvases.empty() || filePath.empty())
        {
        return;
        }

    wxPrintData printData = canvases.front()->GetPrinterSettings();
    printData.SetFilename(filePath);
    wxPdfDC pdfDC(printData);

    if (!pdfDC.StartDoc(title))
        {
        wxMessageBox(wxString::Format(_(L"Failed to create PDF document \"%s\"."), filePath),
                     _(L"Export Error"), wxOK | wxICON_ERROR);
        return;
        }

    for (auto* canvas : canvases)
        {
        if (canvas == nullptr)
            {
            continue;
            }

        const wxWindowUpdateLocker updateLocker{ canvas };

        // compute page dimensions from the canvas's paper settings
        const wxPrintData& canvasPrintData = canvas->GetPrinterSettings();
        auto* paper = wxThePrintPaperDatabase->FindPaperType(canvasPrintData.GetPaperId());
        if (paper == nullptr)
            {
            paper = wxThePrintPaperDatabase->FindPaperType(wxPAPER_A4);
            }

        int dipW{ 0 }, dipH{ 0 };
        if (paper != nullptr)
            {
            const bool landscape = (canvasPrintData.GetOrientation() == wxLANDSCAPE);
            const auto paperSzTenthsMM = paper->GetSize();
            const double ptsW =
                (landscape ? paperSzTenthsMM.GetHeight() : paperSzTenthsMM.GetWidth()) / 254.0 *
                72.0;
            const double ptsH =
                (landscape ? paperSzTenthsMM.GetWidth() : paperSzTenthsMM.GetHeight()) / 254.0 *
                72.0;
            dipW = wxRound(ptsW * 96.0 / 72.0);
            dipH = wxRound(ptsH * 96.0 / 72.0);
            }

        if (dipW <= 0 || dipH <= 0)
            {
            // fallback to US Letter at 96 DPI
            dipW = static_cast<int>(8.5 * 96.0);
            dipH = static_cast<int>(11.0 * 96.0);
            }

        // save original canvas state
        const int origMinWidth = canvas->GetCanvasMinWidthDIPs();
        const int origMinHeight = canvas->GetCanvasMinHeightDIPs();
        const wxSize origSize = canvas->GetSize();

        // resize canvas to match page dimensions so CalcAllSizes lays out for the page
        canvas->SetCanvasMinWidthDIPs(dipW);
        canvas->SetCanvasMinHeightDIPs(dipH);
        canvas->SetSize(canvas->FromDIP(wxSize(dipW, dipH)));
        canvas->CalcRowDimensions();
        canvas->SetSize(canvas->FromDIP(wxSize(dipW, dipH)));

        pdfDC.StartPage();
        const wxEventBlocker blocker{ canvas };
        canvas->CalcAllSizes(pdfDC);
        canvas->OnDraw(pdfDC);
        canvas->DrawWatermarkLabel(pdfDC);
        pdfDC.EndPage();

        // restore canvas state
        canvas->SetCanvasMinWidthDIPs(origMinWidth);
        canvas->SetCanvasMinHeightDIPs(origMinHeight);
        canvas->CalcRowDimensions();
        canvas->SetSize(origSize);
        canvas->ResetResizeDelay();
        canvas->SendSizeEvent();
        canvas->Refresh();
        }

    pdfDC.EndDoc();
    }

#endif // INCLUDE_PDF
