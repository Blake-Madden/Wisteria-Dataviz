///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriadoc.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_DOC_H
#define WISTERIA_DOC_H

#include <wx/docview.h>
#include <wx/filename.h>
#include <wx/wx.h>

/// @brief Document class for Wisteria Dataviz projects.
class WisteriaDoc : public wxDocument
    {
  public:
    WisteriaDoc() = default;
    WisteriaDoc(const WisteriaDoc&) = delete;
    WisteriaDoc& operator=(const WisteriaDoc&) = delete;

    bool OnOpenDocument(const wxString& filename) override;

    wxDECLARE_DYNAMIC_CLASS(WisteriaDoc);
    };

#endif // WISTERIA_DOC_H
