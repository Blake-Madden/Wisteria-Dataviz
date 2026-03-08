///////////////////////////////////////////////////////////////////////////////
// Name:        insertgraphdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertgraphdlg.h"

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertGraphDlg::InsertGraphDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style)
        {
        }

    //-------------------------------------------
    wxChoice* InsertGraphDlg::CreateLegendPlacementChoice(wxWindow* parent,
                                                          const int defaultSelection)
        {
        m_legendPlacementChoice = new wxChoice(parent, wxID_ANY);
        m_legendPlacementChoice->Append(_(L"(None)"));
        m_legendPlacementChoice->Append(_(L"Right"));
        m_legendPlacementChoice->Append(_(L"Left"));
        m_legendPlacementChoice->Append(_(L"Top"));
        m_legendPlacementChoice->Append(_(L"Bottom"));
        m_legendPlacementChoice->SetSelection(defaultSelection);
        return m_legendPlacementChoice;
        }

    //-------------------------------------------
    LegendPlacement InsertGraphDlg::SelectionToLegendPlacement(const int selection)
        {
        switch (selection)
            {
        case 1:
            return LegendPlacement::Right;
        case 2:
            return LegendPlacement::Left;
        case 3:
            return LegendPlacement::Top;
        case 4:
            return LegendPlacement::Bottom;
        default:
            return LegendPlacement::None;
            }
        }

    //-------------------------------------------
    LegendPlacement InsertGraphDlg::GetLegendPlacement() const
        {
        if (m_legendPlacementChoice == nullptr)
            {
            return LegendPlacement::None;
            }
        return SelectionToLegendPlacement(m_legendPlacementChoice->GetSelection());
        }
    } // namespace Wisteria::UI
