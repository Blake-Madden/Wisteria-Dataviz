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
        else if (child->GetName() == L"SvgExport")
            {
            const auto boolAttr = [&child](const wxString& name, const bool fallback)
            { return child->GetAttribute(name, fallback ? L"1" : L"0") == L"1"; };
            long val{ 0 };
            if (child
                    ->GetAttribute(L"pageWidth",
                                   std::to_wstring(m_svgExportOptions.m_pageSize.GetWidth()))
                    .ToLong(&val) &&
                val > 0)
                {
                m_svgExportOptions.m_pageSize.SetWidth(static_cast<int>(val));
                }
            if (child
                    ->GetAttribute(L"pageHeight",
                                   std::to_wstring(m_svgExportOptions.m_pageSize.GetHeight()))
                    .ToLong(&val) &&
                val > 0)
                {
                m_svgExportOptions.m_pageSize.SetHeight(static_cast<int>(val));
                }
            m_svgExportOptions.m_includeTransitions =
                boolAttr(L"transitions", m_svgExportOptions.m_includeTransitions);
            m_svgExportOptions.m_includeHighlighting =
                boolAttr(L"highlighting", m_svgExportOptions.m_includeHighlighting);
            m_svgExportOptions.m_includeLayoutOptions =
                boolAttr(L"layoutOptions", m_svgExportOptions.m_includeLayoutOptions);
            m_svgExportOptions.m_includeDarkModeToggle =
                boolAttr(L"darkModeToggle", m_svgExportOptions.m_includeDarkModeToggle);
            m_svgExportOptions.m_includeSlideshow =
                boolAttr(L"slideshow", m_svgExportOptions.m_includeSlideshow);
            m_svgExportOptions.m_includePageShadow =
                boolAttr(L"pageShadow", m_svgExportOptions.m_includePageShadow);
            const wxString colorStr = child->GetAttribute(
                L"themeColor", m_svgExportOptions.m_themeColor.GetAsString(wxC2S_HTML_SYNTAX));
            if (const wxColour color{ colorStr }; color.IsOk())
                {
                m_svgExportOptions.m_themeColor = color;
                }
            const wxString layoutDefault =
                (m_svgExportOptions.m_layout == Wisteria::SVGReportOptions::PageLayout::Stacked) ?
                    L"0" :
                    L"1";
            if (child->GetAttribute(L"layout", layoutDefault).ToLong(&val))
                {
                m_svgExportOptions.m_layout = (val == 0) ?
                                                  Wisteria::SVGReportOptions::PageLayout::Stacked :
                                                  Wisteria::SVGReportOptions::PageLayout::Duplex;
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

    auto* svgNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"SvgExport");
    svgNode->AddAttribute(L"pageWidth",
                          std::to_wstring(std::max(0, m_svgExportOptions.m_pageSize.GetWidth())));
    svgNode->AddAttribute(L"pageHeight",
                          std::to_wstring(std::max(0, m_svgExportOptions.m_pageSize.GetHeight())));
    svgNode->AddAttribute(L"transitions", m_svgExportOptions.m_includeTransitions ? L"1" : L"0");
    svgNode->AddAttribute(L"highlighting", m_svgExportOptions.m_includeHighlighting ? L"1" : L"0");
    svgNode->AddAttribute(L"layoutOptions",
                          m_svgExportOptions.m_includeLayoutOptions ? L"1" : L"0");
    svgNode->AddAttribute(L"darkModeToggle",
                          m_svgExportOptions.m_includeDarkModeToggle ? L"1" : L"0");
    svgNode->AddAttribute(L"slideshow", m_svgExportOptions.m_includeSlideshow ? L"1" : L"0");
    svgNode->AddAttribute(L"pageShadow", m_svgExportOptions.m_includePageShadow ? L"1" : L"0");
    svgNode->AddAttribute(L"themeColor",
                          m_svgExportOptions.m_themeColor.GetAsString(wxC2S_HTML_SYNTAX));
    svgNode->AddAttribute(L"layout", m_svgExportOptions.m_layout ==
                                             Wisteria::SVGReportOptions::PageLayout::Stacked ?
                                         L"0" :
                                         L"1");
    root->AddChild(svgNode);

    if (!doc.Save(filePath))
        {
        wxLogWarning(L"Failed to save settings file: %s", filePath);
        return false;
        }

    wxLogVerbose(L"Settings saved to: %s", filePath);
    return true;
    }
