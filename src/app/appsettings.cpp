///////////////////////////////////////////////////////////////////////////////
// Name:        appsettings.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "appsettings.h"
#include <wx/log.h>
#include <wx/xml/xml.h>

//-------------------------------------------
bool AppSettings::LoadSettingsFile(const wxString& filePath)
    {
    m_settingsFilePath = filePath;

    if (!wxFileName::FileExists(filePath))
        {
        return false;
        }

    wxXmlDocument doc;
    if (!doc.Load(filePath))
        {
        wxLogWarning(L"Failed to load settings file: %s", filePath);
        return false;
        }

    const auto* root = doc.GetRoot();
    if (root == nullptr || root->GetName() != L"WisteriaSettings")
        {
        wxLogWarning(L"Invalid settings file format: %s", filePath);
        return false;
        }

    for (auto* child = root->GetChildren(); child != nullptr; child = child->GetNext())
        {
        if (child->GetName() == L"Window")
            {
            long val{ 0 };
            if (child->GetAttribute(L"maximized", L"1").ToLong(&val))
                {
                m_appWindowMaximized = (val != 0);
                }
            if (child->GetAttribute(L"width", L"800").ToLong(&val))
                {
                m_appWindowWidth = (val > 0) ? static_cast<int>(val) : 800;
                }
            if (child->GetAttribute(L"height", L"700").ToLong(&val))
                {
                m_appWindowHeight = (val > 0) ? static_cast<int>(val) : 700;
                }
            }
        }

    wxLogVerbose(L"Settings loaded from: %s", filePath);
    return true;
    }

//-------------------------------------------
bool AppSettings::SaveSettingsFile() { return SaveSettingsFile(m_settingsFilePath); }

//-------------------------------------------
bool AppSettings::SaveSettingsFile(const wxString& filePath)
    {
    if (filePath.empty())
        {
        wxLogWarning(L"Cannot save settings: no file path specified.");
        return false;
        }

    m_settingsFilePath = filePath;

    wxXmlDocument doc;
    auto* root = new wxXmlNode(wxXML_ELEMENT_NODE, L"WisteriaSettings");
    doc.SetRoot(root);

    auto* windowNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"Window");
    windowNode->AddAttribute(L"maximized", m_appWindowMaximized ? L"1" : L"0");
    windowNode->AddAttribute(L"width", std::to_wstring(m_appWindowWidth));
    windowNode->AddAttribute(L"height", std::to_wstring(m_appWindowHeight));
    root->AddChild(windowNode);

    if (!doc.Save(filePath))
        {
        wxLogWarning(L"Failed to save settings file: %s", filePath);
        return false;
        }

    wxLogVerbose(L"Settings saved to: %s", filePath);
    return true;
    }
