///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriadoc.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriadoc.h"

wxIMPLEMENT_DYNAMIC_CLASS(WisteriaDoc, wxDocument);

//-------------------------------------------
bool WisteriaDoc::OnNewDocument()
    {
    if (!wxDocument::OnNewDocument())
        {
        return false;
        }

    SetTitle(_(L"Untitled"));
    return true;
    }

//-------------------------------------------
bool WisteriaDoc::OnOpenDocument(const wxString& filename)
    {
    SetFilename(filename, true);
    SetTitle(wxFileName(filename).GetName());
    Modify(false);
    UpdateAllViews();
    return true;
    }
