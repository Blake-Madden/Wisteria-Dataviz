///////////////////////////////////////////////////////////////////////////////
// Name:        aboutdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "aboutdlg.h"
#include "../../base/colorbrewer.h"
#include "../../base/label.h"
#include "../../import/html_extract_text.h"
#include "../../util/donttranslate.h"
#include "../../util/hardwareinfo.h"
#include "../../util/textstream.h"
#include <algorithm>
#include <utility>
#include <wx/artprov.h>
#include <wx/clipbrd.h>
#include <wx/filename.h>
#include <wx/html/htmlwin.h>
#include <wx/uilocale.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    AboutDlgInfo& AboutDlgInfo::AddCitation(wxString label, wxString text,
                                            const CitationFormat format)
        {
        if (!wxString{ text }.Trim().Trim(false).empty())
            {
            m_citations.push_back({ std::move(label), std::move(text), format });
            }
        return *this;
        }

    //------------------------------------------------------
    AboutDlgInfo& AboutDlgInfo::AddCitationFile(wxString label, wxString filePath,
                                                const CitationFormat format)
        {
        if (filePath.empty() || !wxFileName::FileExists(filePath) ||
            wxFileName::GetSize(filePath) == 0)
            {
            return *this;
            }

        wxString citation;
        if (TextStream::ReadFile(filePath, citation, wxString{},
                                 TextStream::ReadFileInteractivityMode::NoInteractivity))
            {
            AddCitation(std::move(label), std::move(citation), format);
            }
        return *this;
        }

    //------------------------------------------------------
    AboutDlg::AboutDlg(wxWindow* parent, AboutDlgInfo info, wxWindowID id, const wxPoint& pos,
                       const wxSize& size, long style)
        : m_info(std::move(info))
        {
        Create(parent, id, pos, size, style);
        }

    //------------------------------------------------------
    void AboutDlg::CopyToClipboard(const wxString& source, const bool stripHtml)
        {
        if (wxTheClipboard->Open())
            {
            if (!source.empty())
                {
                wxString textToCopy{ source };
                if (stripHtml)
                    {
                    lily_of_the_valley::html_extract_text htmlExtract;
                    textToCopy = htmlExtract(source.wc_str(), source.length(), true, true);
                    }
                wxTheClipboard->Clear();
                auto* obj = new wxDataObjectComposite();
                obj->Add(new wxTextDataObject(textToCopy));
                wxTheClipboard->SetData(obj);
                }
            wxTheClipboard->Close();
            }
        }

    //------------------------------------------------------
    bool AboutDlg::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                          const long style)
        {
        DialogWithHelp::Create(
            parent, id,
            wxString::Format(/* TRANSLATORS: %s is the application name */ _(L"About %s"),
                             m_info.m_appName),
            pos, size, style);

        CreateControls();
        Centre();

        Bind(
            wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&)
            { CopyToClipboard(m_components, true); }, ID_COPY_COMPONENTS);

        Bind(
            wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&)
            { CopyToClipboard(m_info.m_eula, false); }, ID_COPY_LICENSE);

        Bind(
            wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&)
            { CopyToClipboard(m_productInfo, false); }, ID_COPY_PRODUCT_INFO);

        return true;
        }

    //------------------------------------------------------
    void AboutDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        m_sideBarBook = new SideBarBook(this, wxID_ANY);
        mainSizer->Add(m_sideBarBook, wxSizerFlags{ 1 }.Expand().Border());

            // version info page
            {
            auto* mainPage = new wxPanel(m_sideBarBook);
            auto* mainPanelSizer = new wxBoxSizer(wxVERTICAL);
            mainPage->SetSizer(mainPanelSizer);
            m_sideBarBook->AddPage(mainPage, _(L"Product Info"), ID_VERSION_PAGE, true);

            auto* productInfoGrid =
                new wxFlexGridSizer(2, wxSize(wxSizerFlags::GetDefaultBorder(), 0));
            productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, _(L"Version:")));
            productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, m_info.m_appVersion));
            if (!m_info.m_buildVersion.empty())
                {
                productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, _(L"Build:")));
                productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, m_info.m_buildVersion));
                }

            if (wxGetMouseState().ShiftDown())
                {
                productInfoGrid->Add(
                    new wxStaticText(mainPage, wxID_ANY,
                                     // TRANSLATORS: Options used to compile the program.
                                     _(L"Build Info:")));
                wxString buildInfo;
#ifndef NDEBUG
                buildInfo += _(L"DEBUG version");
#else
                // TRANSLATORS: Release version of the program.
                buildInfo += _(L"Release version");
#endif
#if defined(__apple_build_version__)
                // TRANSLATORS: Compiler used to build the program.
                buildInfo +=
                    wxString::Format(_(L"\nCompiler: Apple Clang %d.%d.%d"), __clang_major__,
                                     __clang_minor__, __clang_patchlevel__);
#elif defined(__clang__)
                // TRANSLATORS: Compiler used to build the program.
                buildInfo += wxString::Format(_(L"\nCompiler: Clang %d.%d.%d"), __clang_major__,
                                              __clang_minor__, __clang_patchlevel__);
#elif defined(_MSC_FULL_VER)
                // TRANSLATORS: Compiler used to build the program.
                buildInfo +=
                    wxString::Format(_(L"\nCompiler: MSVC %d.%d.%d"), _MSC_FULL_VER / 10000000,
                                     (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000);
#elif defined(__MINGW32__) || defined(__MINGW64__)
                // TRANSLATORS: Compiler used to build the program.
                buildInfo += wxString::Format(_(L"\nCompiler: MinGW GCC %d.%d.%d"), __GNUC__,
                                              __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(__GNUC__)
                // TRANSLATORS: Compiler used to build the program.
                buildInfo += wxString::Format(_(L"\nCompiler: GCC %d.%d.%d"), __GNUC__,
                                              __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
                // TRANSLATORS: C++ language standard used to build the program.
                // (__cplusplus encodes the standard's finalization year as YYYYMM;
                // C++98 is the sole exception, using 199711L.)
                buildInfo +=
                    wxString::Format(_(L"\nC++ Standard: C++%02ld"),
                                     (__cplusplus <= 199711L) ? 98L : ((__cplusplus / 100) % 100));
#ifdef COMPILE_FLAGS_SUMMARY_STR
                wxString compileFlagsInfo = wxString::FromUTF8(COMPILE_FLAGS_SUMMARY_STR);
                compileFlagsInfo.Replace(L"\\n", L"\n");
                compileFlagsInfo.Trim();
                compileFlagsInfo.Trim(false);
                if (!compileFlagsInfo.empty())
                    {
                    // TRANSLATORS: Options used to compile the program.
                    buildInfo +=
                        _(L"\n\nCompile options:\n--------------------\n") + compileFlagsInfo;
                    }
#endif
#if defined(ENABLED_SANITIZERS_STR) && defined(SANITIZER_ENV_HINTS_STR)
                wxString sanitizerInfo = wxString::FromUTF8(ENABLED_SANITIZERS_STR) + L"\n" +
                                         wxString::FromUTF8(SANITIZER_ENV_HINTS_STR);

                sanitizerInfo.Replace(L"\\n", L"\n");
                sanitizerInfo.Trim();
                sanitizerInfo.Trim(false);

                if (!sanitizerInfo.empty())
                    {
                    if (const size_t subSectionStart = sanitizerInfo.find(L"\n  ");
                        subSectionStart != wxString::npos)
                        {
                        sanitizerInfo.insert(subSectionStart, 1, L'\n');
                        if (size_t subSectionEnd = sanitizerInfo.find(L'\n', subSectionStart + 4);
                            subSectionEnd != wxString::npos)
                            {
                            sanitizerInfo.insert(++subSectionEnd, L"  --------------------\n");
                            }
                        }

                    buildInfo
                        .append(
                            // TRANSLATORS: AddressSanitizer options used to compile the program.
                            _(L"\n\nAddressSanitizer options:\n--------------------\n"))
                        .append(sanitizerInfo);
                    }
#endif
                productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, buildInfo));
                }

            productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, _(L"Build Date:")));
            wxDateTime buildDate;
            buildDate.ParseDate(__DATE__);
            productInfoGrid->Add(new wxStaticText(
                mainPage, wxID_ANY,
                buildDate.Format(wxUILocale::GetCurrent().GetInfo(wxLOCALE_LONG_DATE_FMT))));
            productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY,
                                                  // TRANSLATORS: Operating System
                                                  _(L"Platform:")));
            productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, wxGetOsDescription()));
            productInfoGrid->Add(
                new wxStaticText(mainPage, wxID_ANY,
                                 /* TRANSLATORS: Computer CPU information */ _(L"Architecture:")));
            productInfoGrid->Add(new wxStaticText(
                mainPage, wxID_ANY,
                wxString::Format(/* TRANSLATORS: CPU type, and then threads count */
                                 _(L"%s, %d CPU threads"),
                                 wxSystemHardwareInfo::GetNativeCPUArchitectureName(),
                                 wxSystemHardwareInfo::GetCPUCount())));

            productInfoGrid->Add(new wxStaticText(mainPage, wxID_ANY, _(L"Memory:")));
            productInfoGrid->Add(new wxStaticText(
                mainPage, wxID_ANY,
                wxFileName::GetHumanReadableSize(wxSystemHardwareInfo::GetMemory().GetValue(),
                                                 _(L"Not available"))));

            if (wxGetMouseState().ShiftDown())
                {
                productInfoGrid->Add(
                    new wxStaticText(mainPage, wxID_ANY, _(L"Peak Memory Usage:")));
                productInfoGrid->Add(
                    new wxStaticText(mainPage, wxID_ANY,
                                     wxFileName::GetHumanReadableSize(
                                         wxSystemHardwareInfo::GetPeakUsedMemory().GetValue(),
                                         _(L"Not available"))));
                }

            // put it all together
            const wxString titleFont{ GraphItems::Label::GetFirstAvailableFont(
                { DONTTRANSLATE(L"Inter"), DONTTRANSLATE(L"Roboto"), DONTTRANSLATE(L"Orbitron"),
                  DONTTRANSLATE(L"Georgia") }) };
            auto* appLabelSizer = new wxBoxSizer(wxVERTICAL);
            auto* appLabel = new wxStaticText(mainPage, wxID_ANY, m_info.m_appName);
            appLabel->SetFont(
                wxFontInfo{ appLabel->GetFont().GetFractionalPointSize() * 2 }.FaceName(titleFont));
            appLabel->SetForegroundColour(wxSystemSettings::SelectLightDark(
                Colors::ColorBrewer::GetEclipseBlack(), Colors::ColorBrewer::GetEclipseOrange()));
            appLabelSizer->Add(appLabel);
            appLabel = new wxStaticText(mainPage, wxID_ANY, m_info.m_appVersion);
            appLabel->SetFont(
                wxFontInfo{ appLabel->GetFont().GetFractionalPointSize() * 1.5 }.FaceName(
                    titleFont));
            appLabelSizer->Add(appLabel);
            auto* appTitleSizer = new wxBoxSizer(wxHORIZONTAL);
            if (m_info.m_logo.IsOk())
                {
                appTitleSizer->Add(new wxStaticBitmap(
                    mainPage, wxID_ANY, FixStaticBitmapImage(m_info.m_logo, wxSize{ 128, 128 })));
                }
            appTitleSizer->Add(appLabelSizer,
                               wxSizerFlags{}.CenterVertical().Left().Border(wxLEFT));
            mainPanelSizer->Add(appTitleSizer, wxSizerFlags{}.Left().Border(wxLEFT));

            auto* productArea = new wxBoxSizer(wxHORIZONTAL);
            productArea->Add(productInfoGrid);
            productArea->AddStretchSpacer();
            productArea->Add(
                new wxBitmapButton(mainPage, ID_COPY_PRODUCT_INFO,
                                   wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON)));
            mainPanelSizer->Add(productArea, wxSizerFlags{}.Expand().Border());
            mainPanelSizer->AddSpacer(wxSizerFlags::GetDefaultBorder() * 5);
            mainPanelSizer->AddStretchSpacer();
            mainPanelSizer->Add(new wxStaticText(mainPage, wxID_ANY, m_info.m_copyright),
                                wxSizerFlags{}.Left().Border(wxLEFT));
            mainPanelSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

            m_productInfo = m_info.m_appName + L"\n";
            for (size_t i = 0; i < productInfoGrid->GetChildren().size(); ++i)
                {
                const auto* currentWindow = productInfoGrid->GetChildren()[i]->GetWindow();
                if (currentWindow != nullptr)
                    {
                    m_productInfo.append(currentWindow->GetLabel());
                    m_productInfo.append((i % 2 == 0) ? L'\t' : L'\n');
                    }
                }
            }

        // License page
        if (!m_info.m_eula.empty())
            {
            auto* eulaPage = new wxPanel(m_sideBarBook);
            auto* mainPanelSizer = new wxBoxSizer(wxVERTICAL);
            eulaPage->SetSizer(mainPanelSizer);
            m_sideBarBook->AddPage(eulaPage, _(L"License"), ID_LICENSING_PAGE, false);

            auto* textRowSizer = new wxBoxSizer(wxHORIZONTAL);
            auto* eulaWindow = new wxTextCtrl(
                eulaPage, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
                wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY, wxGenericValidator(&m_info.m_eula));
            textRowSizer->Add(eulaWindow, wxSizerFlags{ 1 }.Expand());
            textRowSizer->Add(new wxBitmapButton(
                eulaPage, ID_COPY_LICENSE, wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON)));
            mainPanelSizer->Add(textRowSizer, wxSizerFlags{ 1 }.Expand().Border());
            }

        // components
        if (!m_info.m_libraries.empty())
            {
            auto* componentsPage = new wxPanel(m_sideBarBook);
            auto* mainPanelSizer = new wxBoxSizer(wxVERTICAL);
            componentsPage->SetSizer(mainPanelSizer);
            m_sideBarBook->AddPage(
                componentsPage,
                _DT(L"SBOM", DTExplanation::ProperNoun, L"This is an industry standard acronym."),
                ID_COMPONENTS_PAGE, false);

            // consistently format library info with the name and version number
            const auto formatLibInfo = [](const AboutLibraryInfo& lib) -> wxString
            {
                const wxVersionInfo& libInfo = lib.m_versionInfo;
                if (!libInfo.IsOk())
                    {
                    return {};
                    }
                wxString copyright = libInfo.GetCopyright();
                copyright.Replace(L"\n", L"<br />");

                const wxString commitHash =
                    lib.m_commitHash.empty() ?
                        wxString{} :
                        wxString::Format(_(L"Commit hash: %s<br />"), lib.m_commitHash);
                const wxString origin = lib.m_origin.empty() ?
                                            wxString{} :
                                            wxString::Format(_(L"Origin: %s<br />"), lib.m_origin);

                return wxString::Format(
                    L"<span style='font-weight: bold;'>%s</span> - %s<br />%s%s%s<br />",
                    libInfo.GetName(),
                    (libInfo.AtLeast(0, 0, 1) ?
                         wxString::Format(_(L"version %s"), libInfo.GetNumericVersionString()) :
                         _(L"unversioned")),
                    (libInfo.HasCopyright() ? copyright + L"<br />" : wxString{}), commitHash,
                    origin);
            };

            std::ranges::sort(
                m_info.m_libraries, [](const auto& lhv, const auto& rhv)
                { return lhv.m_versionInfo.GetName().CmpNoCase(rhv.m_versionInfo.GetName()) < 0; });

            m_components.clear();
            for (const auto& lib : m_info.m_libraries)
                {
                m_components.append(formatLibInfo(lib));
                }
            m_components.Trim();

            mainPanelSizer->Add(
                new wxStaticText(
                    componentsPage, wxID_ANY,
                    /* TRANSLATORS: program libraries */ _(L"Software bill of materials:")),
                wxSizerFlags{}.Border(wxLEFT));
            auto* textRowSizer = new wxBoxSizer(wxHORIZONTAL);
            auto* textWindow = new wxHtmlWindow(
                componentsPage, wxID_ANY, wxDefaultPosition, wxSize{ -1, FromDIP(75) },
                wxHW_SCROLLBAR_AUTO | wxBORDER_THEME | wxHW_NO_SELECTION);
            textWindow->SetPage(m_components);
            textRowSizer->Add(textWindow, wxSizerFlags{ 1 }.Expand());
            textRowSizer->Add(
                new wxBitmapButton(componentsPage, ID_COPY_COMPONENTS,
                                   wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON)));
            mainPanelSizer->Add(textRowSizer, wxSizerFlags{ 1 }.Expand().Border());
            }

        if (!m_info.m_citations.empty())
            {
            auto* citationPage = new wxPanel(m_sideBarBook);
            auto* mainPanelSizer = new wxBoxSizer(wxVERTICAL);
            citationPage->SetSizer(mainPanelSizer);
            m_sideBarBook->AddPage(citationPage, _(L"Citation"), ID_CITATION_PAGE, false);

            mainPanelSizer->Add(new wxStaticText(citationPage, wxID_ANY,
                                                 wxString::Format(_(L"To cite %s in publications:"),
                                                                  m_info.m_appName)),
                                wxSizerFlags{}.Border(wxLEFT));

            mainPanelSizer->AddSpacer(wxSizerFlags::GetDefaultBorder() * 2);

            for (auto& citation : m_info.m_citations)
                {
                mainPanelSizer->Add(new wxStaticText(citationPage, wxID_ANY, citation.m_label),
                                    wxSizerFlags{}.Border(wxLEFT));
                auto* textRowSizer = new wxBoxSizer(wxHORIZONTAL);
                const bool isHtml{ citation.m_format == CitationFormat::Html };
                if (isHtml)
                    {
                    auto* textWindow = new wxHtmlWindow(
                        citationPage, wxID_ANY, wxDefaultPosition, wxSize{ -1, FromDIP(75) },
                        wxHW_SCROLLBAR_AUTO | wxBORDER_THEME | wxHW_NO_SELECTION);
                    textWindow->SetPage(citation.m_text);
                    textRowSizer->Add(textWindow, wxSizerFlags{ 1 }.Expand());
                    }
                else
                    {
                    auto* textWindow = new wxTextCtrl(citationPage, wxID_ANY, wxString{},
                                                      wxDefaultPosition, wxSize{ -1, FromDIP(175) },
                                                      wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY,
                                                      wxGenericValidator(&citation.m_text));
                    textWindow->SetFont(wxFontInfo{}.Family(wxFontFamily::wxFONTFAMILY_TELETYPE));
                    // Force the text color to reset to the system text color.
                    // Setting the font changes the font color to black, and simply setting it
                    // back to wxSYS_COLOUR_WINDOWTEXT will have no effect because the control
                    // still thinks the font color is that, although it really isn't. So set it
                    // to the dialog color and then to the text color for it to take effect.
                    textWindow->SetForegroundColour(
                        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
                    textWindow->SetForegroundColour(
                        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
                    textRowSizer->Add(textWindow, wxSizerFlags{ 1 }.Expand());
                    }
                auto* copyButton = new wxBitmapButton(
                    citationPage, wxID_ANY, wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON));
                copyButton->Bind(wxEVT_BUTTON,
                                 [text = citation.m_text, isHtml]([[maybe_unused]] wxCommandEvent&)
                                 { CopyToClipboard(text, isHtml); });
                textRowSizer->Add(copyButton);
                mainPanelSizer->Add(textRowSizer, wxSizerFlags{ isHtml ? 0 : 1 }.Expand().Border());
                }
            }

        mainSizer->Add(CreateSeparatedButtonSizer(wxCLOSE), wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);
        }
    } // namespace Wisteria::UI
