///////////////////////////////////////////////////////////////////////////////
// Name:        icons.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "icons.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Icons::Schemes::IconScheme, wxObject)
    wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Icons::Schemes::StandardShapes,
                              Wisteria::Icons::Schemes::IconScheme)
        wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Icons::Schemes::Semesters,
                                  Wisteria::Icons::Schemes::IconScheme)
