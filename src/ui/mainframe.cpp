///////////////////////////////////////////////////////////////////////////////
// Name:        mainframe.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "mainframe.h"
#include "dialogs/radioboxdlg.h"

//----------------------------------------------------------
wxDocTemplate* Wisteria::UI::DocManager::SelectDocumentType(wxDocTemplate** templates,
                                                            const int noTemplates,
                                                            const bool sortDocs)
    {
    wxArrayString strings;
    auto** data = new wxDocTemplate*[noTemplates];
    int i{ 0 };
    int n{ 0 };

    for (i = 0; i < noTemplates; i++)
        {
        if (templates[i]->IsVisible())
            {
            bool want = true;
            for (int j = 0; j < n; j++)
                {
                // filter out NOT unique documents + view combinations
                if (templates[i]->GetDocumentName() == data[j]->GetDocumentName() &&
                    templates[i]->GetViewName() == data[j]->GetViewName())
                    {
                    want = false;
                    }
                }

            if (want)
                {
                strings.Add(templates[i]->GetDescription());

                data[n] = templates[i];
                n++;
                }
            }
        }

    if (sortDocs)
        {
        strings.Sort(); // ascending sort
        // Yes, this will be slow, but template lists
        // are typically short.
        assert(noTemplates >= static_cast<int>(strings.Count()));
        n = std::min<int>(strings.Count(), noTemplates);
        for (i = 0; i < n; i++)
            {
            for (int j = 0; j < noTemplates; j++)
                {
                if (strings[i] == templates[j]->GetDescription())
                    {
                    data[i] = templates[j];
                    }
                }
            }
        }

    wxDocTemplate* theTemplate{ nullptr };

    switch (n)
        {
    case 0:
        // no visible templates, hence nothing to choose from
        theTemplate = nullptr;
        break;

    case 1:
        // don't propose the user to choose if they have no choice
        theTemplate = data[0];
        break;

    default:
        // wxGetSingleChoiceData is used in the default implementation of this function,
        // but we are overriding it here to use a more advanced selection dialog
        wxArrayString docNames;
        const wxArrayString docDescriptions;
        for (i = 0; i < noTemplates; i++)
            {
            docNames.Add(data[i]->GetDescription());
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
                             _(L"Project types:"), _(L"New Project"), docNames, docDescriptions);
        if (radioDlg.ShowModal() == wxID_OK)
            {
            theTemplate = data[radioDlg.GetSelection()];
            }
        else
            {
            theTemplate = nullptr;
            }
        }

    delete[] data;

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
void Wisteria::UI::BaseMainFrame::OnRibbonButtonBarClick(wxRibbonButtonBarEvent& evt)
    {
    wxCommandEvent event(wxEVT_MENU, evt.GetId());
    ProcessWindowEvent(event);
    }

//----------------------------------------------------------
void Wisteria::UI::BaseMainFrame::OnRibbonToolBarClick(wxRibbonToolBarEvent& evt)
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
