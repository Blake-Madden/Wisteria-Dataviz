///////////////////////////////////////////////////////////////////////////////
// Name:        appsettings.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "appsettings.h"
#include <wx/log.h>
#include <wx/tokenzr.h>
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
    if (root == nullptr || root->GetName() != L"wisteria-settings")
        {
        wxLogWarning(L"Invalid settings file format: %s", filePath);
        return false;
        }

    for (auto* child = root->GetChildren(); child != nullptr; child = child->GetNext())
        {
        if (child->GetName() == L"window")
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
        else if (child->GetName() == L"printer")
            {
            long val{ 0 };
            if (child->GetAttribute(L"orientation", L"1").ToLong(&val))
                {
                m_printOrientation = static_cast<int>(val);
                }
            if (child->GetAttribute(L"paperId", L"1").ToLong(&val))
                {
                m_paperId = static_cast<wxPaperSize>(val);
                }
            if (child->GetAttribute(L"duplex", L"0").ToLong(&val) && val >= wxDUPLEX_SIMPLEX &&
                val <= wxDUPLEX_VERTICAL)
                {
                m_printDuplex = static_cast<wxDuplexMode>(val);
                }
            if (child->GetAttribute(L"collate", L"1").ToLong(&val))
                {
                m_printCollate = (val != 0);
                }
            if (child->GetAttribute(L"color", L"1").ToLong(&val))
                {
                m_printColor = (val != 0);
                }
            }
        else if (child->GetName() == L"log")
            {
            long val{ 0 };
            if (child->GetAttribute(L"autoRefresh", L"0").ToLong(&val))
                {
                m_logAutoRefresh = (val != 0);
                }
            if (child->GetAttribute(L"verbose", L"0").ToLong(&val))
                {
                m_logVerbose = (val != 0);
                }
            }
        else if (child->GetName() == L"project")
            {
            long val{ 0 };
            if (child->GetAttribute(L"autoRefresh", L"0").ToLong(&val))
                {
                m_projectAutoRefresh = (val != 0);
                }
            }
        else if (child->GetName() == L"object-gallery")
            {
            m_collapsedGalleryGroups.clear();
            wxStringTokenizer tokenizer(child->GetAttribute(L"collapsed-groups"), L",");
            while (tokenizer.HasMoreTokens())
                {
                long groupId{ 0 };
                if (tokenizer.GetNextToken().ToLong(&groupId))
                    {
                    m_collapsedGalleryGroups.insert(static_cast<Wisteria::GalleryGroup>(groupId));
                    }
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
    auto* root = new wxXmlNode(wxXML_ELEMENT_NODE, L"wisteria-settings");
    doc.SetRoot(root);

    auto* windowNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"window");
    windowNode->AddAttribute(L"maximized", m_appWindowMaximized ? L"1" : L"0");
    windowNode->AddAttribute(L"width", std::to_wstring(m_appWindowWidth));
    windowNode->AddAttribute(L"height", std::to_wstring(m_appWindowHeight));
    root->AddChild(windowNode);

    auto* printerNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"printer");
    printerNode->AddAttribute(L"orientation", std::to_wstring(m_printOrientation));
    printerNode->AddAttribute(L"paperId", std::to_wstring(static_cast<int>(m_paperId)));
    printerNode->AddAttribute(L"duplex", std::to_wstring(static_cast<int>(m_printDuplex)));
    printerNode->AddAttribute(L"collate", m_printCollate ? L"1" : L"0");
    printerNode->AddAttribute(L"color", m_printColor ? L"1" : L"0");
    root->AddChild(printerNode);

    auto* logNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"log");
    logNode->AddAttribute(L"autoRefresh", m_logAutoRefresh ? L"1" : L"0");
    logNode->AddAttribute(L"verbose", m_logVerbose ? L"1" : L"0");
    root->AddChild(logNode);

    auto* projectNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"project");
    projectNode->AddAttribute(L"autoRefresh", m_projectAutoRefresh ? L"1" : L"0");
    root->AddChild(projectNode);

    auto* galleryNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"object-gallery");
    wxString collapsedGroupsCsv;
    for (const auto& group : m_collapsedGalleryGroups)
        {
        if (!collapsedGroupsCsv.empty())
            {
            collapsedGroupsCsv += L",";
            }
        collapsedGroupsCsv += std::to_wstring(static_cast<int>(group));
        }
    galleryNode->AddAttribute(L"collapsed-groups", collapsedGroupsCsv);
    root->AddChild(galleryNode);

    if (!doc.Save(filePath))
        {
        wxLogWarning(L"Failed to save settings file: %s", filePath);
        return false;
        }

    wxLogVerbose(L"Settings saved to: %s", filePath);
    return true;
    }
