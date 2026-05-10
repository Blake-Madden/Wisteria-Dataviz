///////////////////////////////////////////////////////////////////////////////
// Name:        pdfreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdfreportprintout.h"
#include "../app/wisteriaapp.h"

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

    wxPrintData printData;
    printData.SetOrientation(
        static_cast<wxPrintOrientation>(wxGetApp().GetAppSettings()->GetPrintOrientation()));
    printData.SetPaperId(wxGetApp().GetAppSettings()->GetPaperId());
    printData.SetFilename(filePath);
    wxPdfDC pdfDC(printData);

    if (!pdfDC.StartDoc(title))
        {
        wxMessageBox(wxString::Format(_(L"Failed to create PDF document \"%s\"."), filePath),
                     _(L"Export Error"), wxOK | wxICON_ERROR);
        return;
        }

    pdfDC.GetPdfDocument()->SetTitle(title);

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
            dipW = dipSize.GetWidth();
            dipH = dipSize.GetHeight();
            }

        if (dipW <= 0 || dipH <= 0)
            {
            // fallback to US Letter
            const wxSize dipSize = pdfDC.ToDIP(wxSize{ wxRound(8.5 * 72.0), wxRound(11.0 * 72.0) });
            dipW = dipSize.GetWidth();
            dipH = dipSize.GetHeight();
            }

        // save original canvas state
        const int origMinWidth = canvas->GetCanvasMinWidthDIPs();
        const int origMinHeight = canvas->GetCanvasMinHeightDIPs();
        const wxSize origSize = canvas->GetSize();

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
            canvas->CalcRowDimensions();
            canvas->SetSize(origSize);
            }
        canvas->ResetResizeDelay();
        canvas->SendSizeEvent();
        canvas->Refresh();
        }

    pdfDC.EndDoc();
    }

#endif // INCLUDE_PDF
