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
        else if (child->GetName() == L"svg-export")
            {
            const auto boolAttr = [&child](const wxString& name, const bool fallback)
            { return child->GetAttribute(name, fallback ? L"1" : L"0") == L"1"; };
            long val{ 0 };
            if (child
                    ->GetAttribute(L"page-width",
                                   std::to_wstring(m_svgExportOptions.m_pageSize.GetWidth()))
                    .ToLong(&val) &&
                val > 0)
                {
                m_svgExportOptions.m_pageSize.SetWidth(static_cast<int>(val));
                }
            if (child
                    ->GetAttribute(L"page-height",
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
                boolAttr(L"layout-options", m_svgExportOptions.m_includeLayoutOptions);
            m_svgExportOptions.m_includeDarkModeToggle =
                boolAttr(L"dark-mode-toggle", m_svgExportOptions.m_includeDarkModeToggle);
            m_svgExportOptions.m_includeSlideshow =
                boolAttr(L"slideshow", m_svgExportOptions.m_includeSlideshow);
            m_svgExportOptions.m_includePageShadow =
                boolAttr(L"page-shadow", m_svgExportOptions.m_includePageShadow);
            m_svgExportOptions.m_includeLayerControls =
                boolAttr(L"layer-controls", m_svgExportOptions.m_includeLayerControls);
            m_svgExportOptions.m_useGlobalPrintSettings = boolAttr(
                L"svg-use-global-print-settings", m_svgExportOptions.m_useGlobalPrintSettings);
            const wxString colorStr = child->GetAttribute(
                L"themeColor", m_svgExportOptions.m_themeColor.GetAsString(wxC2S_HTML_SYNTAX));
            if (const wxColour color{ colorStr }; color.IsOk())
                {
                m_svgExportOptions.m_themeColor = color;
                }
            const wxString layoutDefault =
                (m_svgExportOptions.m_layout == Wisteria::SVGReportOptions::PageLayout::Single) ?
                    L"0" :
                (m_svgExportOptions.m_layout == Wisteria::SVGReportOptions::PageLayout::Duplex) ?
                    L"1" :
                    L"2";
            if (child->GetAttribute(L"layout", layoutDefault).ToLong(&val))
                {
                if (val == 0)
                    {
                    m_svgExportOptions.m_layout = Wisteria::SVGReportOptions::PageLayout::Single;
                    }
                else if (val == 1)
                    {
                    m_svgExportOptions.m_layout = Wisteria::SVGReportOptions::PageLayout::Duplex;
                    }
                else
                    {
                    m_svgExportOptions.m_layout = Wisteria::SVGReportOptions::PageLayout::Stacked;
                    }
                }
            }
        else if (child->GetName() == L"powerpoint-export")
            {
            const auto boolAttr = [&child](const wxString& name, const bool fallback)
            { return child->GetAttribute(name, fallback ? L"1" : L"0") == L"1"; };
            long val{ 0 };
            if (child
                    ->GetAttribute(L"slide-size", std::to_wstring(static_cast<int>(
                                                      m_powerPointExportOptions.m_slideSize)))
                    .ToLong(&val))
                {
                m_powerPointExportOptions.m_slideSize =
                    (val == 0) ? Wisteria::PowerPointExportOptions::SlideSize::Widescreen16x9 :
                    (val == 1) ? Wisteria::PowerPointExportOptions::SlideSize::Standard4x3 :
                                 Wisteria::PowerPointExportOptions::SlideSize::Custom;
                }
            double dval{ 0 };
            if (child
                    ->GetAttribute(L"custom-width",
                                   std::to_wstring(m_powerPointExportOptions.m_customWidthInches))
                    .ToDouble(&dval) &&
                dval > 0)
                {
                m_powerPointExportOptions.m_customWidthInches = dval;
                }
            if (child
                    ->GetAttribute(L"custom-height",
                                   std::to_wstring(m_powerPointExportOptions.m_customHeightInches))
                    .ToDouble(&dval) &&
                dval > 0)
                {
                m_powerPointExportOptions.m_customHeightInches = dval;
                }
            if (child
                    ->GetAttribute(L"transition", std::to_wstring(static_cast<int>(
                                                      m_powerPointExportOptions.m_transition)))
                    .ToLong(&val) &&
                val >= 0 &&
                val <= static_cast<int>(Wisteria::PowerPointExportOptions::Transition::Morph))
                {
                m_powerPointExportOptions.m_transition =
                    static_cast<Wisteria::PowerPointExportOptions::Transition>(val);
                }
            if (child
                    ->GetAttribute(L"transition-speed",
                                   std::to_wstring(static_cast<int>(
                                       m_powerPointExportOptions.m_transitionSpeed)))
                    .ToLong(&val) &&
                val >= 0 &&
                val <= static_cast<int>(Wisteria::PowerPointExportOptions::TransitionSpeed::Fast))
                {
                m_powerPointExportOptions.m_transitionSpeed =
                    static_cast<Wisteria::PowerPointExportOptions::TransitionSpeed>(val);
                }
            m_powerPointExportOptions.m_advanceOnClick =
                boolAttr(L"advance-on-click", m_powerPointExportOptions.m_advanceOnClick);
            m_powerPointExportOptions.m_advanceAutomatically = boolAttr(
                L"advance-automatically", m_powerPointExportOptions.m_advanceAutomatically);
            if (child
                    ->GetAttribute(L"advance-seconds",
                                   std::to_wstring(m_powerPointExportOptions.m_advanceSeconds))
                    .ToLong(&val) &&
                val > 0)
                {
                m_powerPointExportOptions.m_advanceSeconds = static_cast<int>(val);
                }
            m_powerPointExportOptions.m_loopContinuously =
                boolAttr(L"loop", m_powerPointExportOptions.m_loopContinuously);
            m_powerPointExportOptions.m_includeAccessibilityNotes = boolAttr(
                L"accessibility-notes", m_powerPointExportOptions.m_includeAccessibilityNotes);
            m_powerPointExportOptions.m_includeTitleSlide =
                boolAttr(L"title-slide", m_powerPointExportOptions.m_includeTitleSlide);
            m_powerPointExportOptions.m_titleSlideTheme = child->GetAttribute(
                L"title-slide-theme", m_powerPointExportOptions.m_titleSlideTheme);
            m_powerPointExportOptions.m_author =
                child->GetAttribute(L"author", m_powerPointExportOptions.m_author);
            m_powerPointExportOptions.m_publisher =
                child->GetAttribute(L"publisher", m_powerPointExportOptions.m_publisher);
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
    root->AddChild(printerNode);

    auto* logNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"log");
    logNode->AddAttribute(L"autoRefresh", m_logAutoRefresh ? L"1" : L"0");
    logNode->AddAttribute(L"verbose", m_logVerbose ? L"1" : L"0");
    root->AddChild(logNode);

    auto* svgNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"svg-export");
    svgNode->AddAttribute(L"page-width",
                          std::to_wstring(std::max(0, m_svgExportOptions.m_pageSize.GetWidth())));
    svgNode->AddAttribute(L"page-height",
                          std::to_wstring(std::max(0, m_svgExportOptions.m_pageSize.GetHeight())));
    svgNode->AddAttribute(L"transitions", m_svgExportOptions.m_includeTransitions ? L"1" : L"0");
    svgNode->AddAttribute(L"highlighting", m_svgExportOptions.m_includeHighlighting ? L"1" : L"0");
    svgNode->AddAttribute(L"layout-options",
                          m_svgExportOptions.m_includeLayoutOptions ? L"1" : L"0");
    svgNode->AddAttribute(L"dark-mode-toggle",
                          m_svgExportOptions.m_includeDarkModeToggle ? L"1" : L"0");
    svgNode->AddAttribute(L"slideshow", m_svgExportOptions.m_includeSlideshow ? L"1" : L"0");
    svgNode->AddAttribute(L"page-shadow", m_svgExportOptions.m_includePageShadow ? L"1" : L"0");
    svgNode->AddAttribute(L"layer-controls",
                          m_svgExportOptions.m_includeLayerControls ? L"1" : L"0");
    svgNode->AddAttribute(L"svg-use-global-print-settings",
                          m_svgExportOptions.m_useGlobalPrintSettings ? L"1" : L"0");
    svgNode->AddAttribute(L"themeColor",
                          m_svgExportOptions.m_themeColor.GetAsString(wxC2S_HTML_SYNTAX));
    svgNode->AddAttribute(
        L"layout",
        m_svgExportOptions.m_layout == Wisteria::SVGReportOptions::PageLayout::Single ? L"0" :
        m_svgExportOptions.m_layout == Wisteria::SVGReportOptions::PageLayout::Duplex ? L"1" :
                                                                                        L"2");
    root->AddChild(svgNode);

    auto* pptxNode = new wxXmlNode(wxXML_ELEMENT_NODE, L"powerpoint-export");
    pptxNode->AddAttribute(
        L"slide-size", std::to_wstring(static_cast<int>(m_powerPointExportOptions.m_slideSize)));
    pptxNode->AddAttribute(L"custom-width",
                           std::to_wstring(m_powerPointExportOptions.m_customWidthInches));
    pptxNode->AddAttribute(L"custom-height",
                           std::to_wstring(m_powerPointExportOptions.m_customHeightInches));
    pptxNode->AddAttribute(
        L"transition", std::to_wstring(static_cast<int>(m_powerPointExportOptions.m_transition)));
    pptxNode->AddAttribute(L"transition-speed", std::to_wstring(static_cast<int>(
                                                    m_powerPointExportOptions.m_transitionSpeed)));
    pptxNode->AddAttribute(L"advance-on-click",
                           m_powerPointExportOptions.m_advanceOnClick ? L"1" : L"0");
    pptxNode->AddAttribute(L"advance-automatically",
                           m_powerPointExportOptions.m_advanceAutomatically ? L"1" : L"0");
    pptxNode->AddAttribute(L"advance-seconds",
                           std::to_wstring(m_powerPointExportOptions.m_advanceSeconds));
    pptxNode->AddAttribute(L"loop", m_powerPointExportOptions.m_loopContinuously ? L"1" : L"0");
    pptxNode->AddAttribute(L"accessibility-notes",
                           m_powerPointExportOptions.m_includeAccessibilityNotes ? L"1" : L"0");
    pptxNode->AddAttribute(L"title-slide",
                           m_powerPointExportOptions.m_includeTitleSlide ? L"1" : L"0");
    pptxNode->AddAttribute(L"title-slide-theme", m_powerPointExportOptions.m_titleSlideTheme);
    pptxNode->AddAttribute(L"author", m_powerPointExportOptions.m_author);
    pptxNode->AddAttribute(L"publisher", m_powerPointExportOptions.m_publisher);
    root->AddChild(pptxNode);

    if (!doc.Save(filePath))
        {
        wxLogWarning(L"Failed to save settings file: %s", filePath);
        return false;
        }

    wxLogVerbose(L"Settings saved to: %s", filePath);
    return true;
    }
