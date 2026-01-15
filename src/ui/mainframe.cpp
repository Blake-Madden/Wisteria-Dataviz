///////////////////////////////////////////////////////////////////////////////
// Name:        mainframe.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "mainframe.h"
#include "dialogs/radioboxdlg.h"
#include <algorithm>

// NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)

//----------------------------------------------------------
wxDocTemplate* Wisteria::UI::DocManager::SelectDocumentType(wxDocTemplate** templates,
                                                            const int noTemplates,
                                                            const bool sortDocs)
    {
    std::vector<wxDocTemplate*> data;
    data.reserve(noTemplates);

    // Collect visible + unique (by doc/view pair)
    for (int i = 0; i < noTemplates; ++i)
        {
        if (!templates[i]->IsVisible())
            {
            continue;
            }

        bool want = true;
        for (const auto& datum : data)
            {
            if (templates[i]->GetDocumentName() == datum->GetDocumentName() &&
                templates[i]->GetViewName() == datum->GetViewName())
                {
                want = false;
                break; // stop scanning; we found a duplicate
                }
            }

        if (want)
            {
            data.push_back(templates[i]);
            }
        }

    if (sortDocs && data.size() > 1)
        {
        std::ranges::stable_sort(data, [](const wxDocTemplate* a, const wxDocTemplate* b)
                                 { return a->GetDescription() < b->GetDescription(); });
        }

    // NOLINTNEXTLINE(misc-const-correctness)
    wxDocTemplate* theTemplate{ nullptr };

    switch (data.size())
        {
    case 0:
        theTemplate = nullptr; // nothing to choose from
        break;

    case 1:
        theTemplate = data[0]; // only one choice
        break;

    default:
        {
        wxArrayString docNames;

        for (const auto& datum : data)
            {
            docNames.Add(datum->GetDescription());
            }

        // find a suitable parent window
        wxWindow* parentWindow = [this]()
        {
            if (wxTheApp->GetTopWindow() != nullptr && wxTheApp->GetTopWindow()->IsShown())
                {
                return wxTheApp->GetTopWindow();
                }
            if (GetCurrentDocument() != nullptr &&
                GetCurrentDocument()->GetDocumentWindow() != nullptr)
                {
                return GetCurrentDocument()->GetDocumentWindow();
                }
            return wxTheApp->GetTopWindow();
        }();

        RadioBoxDlg radioDlg(parentWindow, _(L"Select Project Type"), wxString{},
                             _(L"Project types:"), _(L"New Project"), docNames, wxArrayString{});

        if (radioDlg.ShowModal() == wxID_OK)
            {
            const int sel = radioDlg.GetSelection();
            if (sel >= 0 && static_cast<size_t>(sel) < data.size())
                {
                theTemplate = data[sel];
                }
            }
        }
        break;
        }

    return theTemplate;
    }

wxIMPLEMENT_CLASS(Wisteria::UI::BaseMainFrame, wxDocParentFrame);

//----------------------------------------------------------
void Wisteria::UI::BaseMainFrame::DisplayHelp(const wxString& topic /*= wxEmptyString*/) const
    {
    const wxString helpPath =
        !topic.empty() ? GetHelpDirectory() + wxFileName::GetPathSeparator() + topic :
                         GetHelpDirectory() + wxFileName::GetPathSeparator() + L"index.html";
    wxLaunchDefaultBrowser(wxFileName::FileNameToURL(helpPath));
    }

//----------------------------------------------------------
void Wisteria::UI::BaseMainFrame::OnRibbonButtonBarClick(const wxRibbonButtonBarEvent& evt)
    {
    wxCommandEvent event(wxEVT_MENU, evt.GetId());
    ProcessWindowEvent(event);
    }

//----------------------------------------------------------
void Wisteria::UI::BaseMainFrame::OnRibbonToolBarClick(const wxRibbonToolBarEvent& evt)
    {
    wxCommandEvent event(wxEVT_MENU, evt.GetId());
    ProcessWindowEvent(event);
    }

//-------------------------------------------------------
void Wisteria::UI::BaseMainFrame::InitControls(wxRibbonBar* ribbon)
    {
    m_ribbon = ribbon;

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    if (m_ribbon != nullptr)
        {
        mainSizer->Add(m_ribbon, wxSizerFlags{}.Expand());
        }
    SetSizer(mainSizer);
    }

//-------------------------------------------------------
Wisteria::UI::BaseMainFrame::BaseMainFrame(wxDocManager* manager, wxFrame* frame,
                                           wxArrayString defaultFileExtensions,
                                           const wxString& title, const wxPoint& pos,
                                           const wxSize& size, long style)
    : wxDocParentFrame(manager, frame, wxID_ANY, title, pos, size, style),
      m_defaultFileExtensions(std::move(defaultFileExtensions))
    {
    // set up drag 'n' drop
    wxWindow::SetDropTarget(new DropFiles(this));
    // create default printer settings
    GetDocumentManager()->GetPageSetupDialogData().GetPrintData().SetPaperId(
        wxPAPER_LETTER); /*8.5" x 11" (U.S. default)*/
    GetDocumentManager()->GetPageSetupDialogData().GetPrintData().SetOrientation(wxLANDSCAPE);
    GetDocumentManager()->GetPageSetupDialogData().GetPrintData().SetQuality(wxPRINT_QUALITY_HIGH);

    Bind(wxEVT_MENU, &Wisteria::UI::BaseMainFrame::OnHelpContents, this, wxID_HELP);
    Bind(wxEVT_MENU, &Wisteria::UI::BaseMainFrame::OnHelpContents, this, wxID_HELP_CONTENTS);

    Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &Wisteria::UI::BaseMainFrame::OnRibbonButtonBarClick, this);
    Bind(wxEVT_RIBBONTOOLBAR_CLICKED, &Wisteria::UI::BaseMainFrame::OnRibbonToolBarClick, this);
    }

//-------------------------------------------------------
wxDocument* Wisteria::UI::BaseMainFrame::OpenFile(const wxString& path)
    {
    // NOLINTNEXTLINE(misc-const-correctness)
    wxDocument* doc = m_docManager->CreateDocument(path, wxDOC_SILENT);
    if (doc == nullptr)
        {
        m_docManager->OnOpenFileFailure();
        }
    return doc;
    }

//-------------------------------------------------------
void Wisteria::UI::BaseMainFrame::OpenFileNew(const wxString& path)
    {
    m_docManager->CreateDocument(path, wxDOC_NEW);
    }

//-------------------------------------------------------
void Wisteria::UI::BaseMainFrame::OnHelpContents([[maybe_unused]] wxCommandEvent& event)
    {
    DisplayHelp();
    }

//----------------------------------------------------------
bool Wisteria::UI::DropFiles::OnDropFiles([[maybe_unused]] wxCoord x, [[maybe_unused]] wxCoord y,
                                          const wxArrayString& filenames)
    {
    for (size_t n = 0; n < filenames.GetCount(); ++n)
        {
        const wxFileName filename(filenames[n]);
        for (size_t i = 0; i < m_frame->GetDefaultFileExtensions().GetCount(); ++i)
            {
            if (filename.GetExt().CmpNoCase(m_frame->GetDefaultFileExtensions()[i]) == 0)
                {
                m_frame->OpenFile(filenames[n]);
                return true;
                }
            }
        m_frame->OpenFileNew(filenames[n]);
        }

    return true;
    }

// NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
