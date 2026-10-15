///////////////////////////////////////////////////////////////////////////////
// Name:        dataguidectrl.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "dataguidectrl.h"
#include "../../app/wisteriaapp.h"
#include "../../import/html_encode.h"
#include "../../util/donttranslate.h"
#include <algorithm>
#include <array>
#include <wx/base64.h>
#include <wx/mstream.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    wxString DataGuideCtrl::EscapeForHtml(const wxString& text)
        {
        const lily_of_the_valley::html_encode_text encode;
        return encode({ text.wc_str(), text.length() }, false);
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetTypeName(const Data::Dataset::ColumnImportType type)
        {
        switch (type)
            {
        case Data::Dataset::ColumnImportType::Numeric:
            return _(L"Continuous");
        case Data::Dataset::ColumnImportType::String:
            return _(L"Categorical");
        case Data::Dataset::ColumnImportType::Discrete:
            return _(L"Discrete");
        case Data::Dataset::ColumnImportType::Date:
            return _(L"Date");
        case Data::Dataset::ColumnImportType::DichotomousString:
            return _(L"Dichotomous (Categorical)");
        case Data::Dataset::ColumnImportType::DichotomousDiscrete:
            return _(L"Dichotomous (Discrete)");
            }
        return {};
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetSvgDataUri(const wxString& svgName, const wxSize& size)
        {
        if (svgName.empty())
            {
            return {};
            }
        const auto bmpBundle = wxGetApp().GetResourceManager().GetSVG(svgName);
        if (!bmpBundle.IsOk())
            {
            return {};
            }
        const wxImage img = bmpBundle.GetBitmap(size).ConvertToImage();
        wxMemoryOutputStream stream;
        if (!img.IsOk() || !img.SaveFile(stream, wxBITMAP_TYPE_PNG))
            {
            return {};
            }
        const auto* buffer = stream.GetOutputStreamBuffer();
        const auto length = stream.GetLength();
        if (buffer == nullptr || buffer->GetBufferStart() == nullptr || length <= 0)
            {
            return {};
            }
        return _DT(L"data:image/png;base64,") +
               wxBase64Encode(buffer->GetBufferStart(), static_cast<size_t>(length));
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetTypeIconUri(const Data::Dataset::ColumnImportType type)
        {
        wxString svgName;
        switch (type)
            {
        case Data::Dataset::ColumnImportType::Numeric:
            svgName = L"images/scale.svg";
            break;
        case Data::Dataset::ColumnImportType::String:
            svgName = L"images/categorical.svg";
            break;
        case Data::Dataset::ColumnImportType::Discrete:
            svgName = L"images/discrete.svg";
            break;
        case Data::Dataset::ColumnImportType::Date:
            svgName = L"images/date.svg";
            break;
        case Data::Dataset::ColumnImportType::DichotomousString:
            svgName = L"images/dichotomous-categorical.svg";
            break;
        case Data::Dataset::ColumnImportType::DichotomousDiscrete:
            svgName = L"images/dichotomous-discrete.svg";
            break;
            }
        return GetSvgDataUri(svgName, wxSize{ 32, 32 });
        }

    //-------------------------------------------
    wxString DataGuideCtrl::BuildTypeIconHtml(const Data::Dataset::ColumnImportType type)
        {
        const wxString iconUri = GetTypeIconUri(type);
        return iconUri.empty() ?
                   wxString{} :
                   L"<img src='" + iconUri + L"' alt='" + EscapeForHtml(GetTypeName(type)) + L"'>";
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetAccentStyle(const size_t roleIndex)
        {
        struct Rgb
            {
            int m_red;
            int m_green;
            int m_blue;
            };

        constexpr std::array<Rgb, 6> accents{ { { 59, 130, 246 },
                                                { 245, 158, 11 },
                                                { 16, 185, 129 },
                                                { 139, 92, 246 },
                                                { 236, 72, 153 },
                                                { 20, 184, 166 } } };
        const auto& accent = accents[roleIndex % accents.size()];
        return wxString::Format(L"--accent:rgb(%d,%d,%d);--accent-bg:rgba(%d,%d,%d,0.16);",
                                accent.m_red, accent.m_green, accent.m_blue, accent.m_red,
                                accent.m_green, accent.m_blue);
        }

    //-------------------------------------------
    wxString DataGuideCtrl::BuildSectionTitle(const wxString& title, const wxString& iconMarkup,
                                              const wxString& badgeClass)
        {
        return L"<div class='section-title'><span class='badge " + badgeClass +
               L"'><svg viewBox='0 0 24 24' width='16' height='16'>" + iconMarkup +
               L"</svg></span><span>" + EscapeForHtml(title) + L"</span></div>";
        }

    //-------------------------------------------
    wxString DataGuideCtrl::BuildShapeHtml(const DataGuideShape shape)
        {
        if (shape == DataGuideShape::Unspecified)
            {
            return {};
            }

        const bool isLong = (shape == DataGuideShape::Long);
        // table geometry: columns x rows, with the first row being the header
        const int columns = isLong ? 3 : 7;
        const int rows = isLong ? 6 : 3;
        const int cellWidth = isLong ? 22 : 15;
        const int cellHeight = isLong ? 11 : 16;
        const int width = columns * cellWidth;
        const int height = rows * cellHeight;
        const int originX = (120 - width) / 2;
        const int originY = (72 - height) / 2;

        wxString svg = L"<svg viewBox='0 0 120 72' width='120' height='72'>";
        svg += wxString::Format(L"<rect x='%d' y='%d' width='%d' height='%d' rx='3' "
                                L"fill='rgba(59,130,246,0.55)'/>",
                                originX, originY, width, cellHeight);
        svg += wxString::Format(L"<rect x='%d' y='%d' width='%d' height='%d' rx='3' fill='none' "
                                L"stroke='currentColor' stroke-opacity='0.55' stroke-width='1.5'/>",
                                originX, originY, width, height);
        for (int row = 1; row < rows; ++row)
            {
            svg += wxString::Format(L"<line x1='%d' y1='%d' x2='%d' y2='%d' "
                                    L"stroke='currentColor' stroke-opacity='0.35'/>",
                                    originX, originY + (row * cellHeight), originX + width,
                                    originY + (row * cellHeight));
            }
        for (int column = 1; column < columns; ++column)
            {
            svg += wxString::Format(L"<line x1='%d' y1='%d' x2='%d' y2='%d' "
                                    L"stroke='currentColor' stroke-opacity='0.35'/>",
                                    originX + (column * cellWidth), originY,
                                    originX + (column * cellWidth), originY + height);
            }
        svg += L"</svg>";

        const wxString title = isLong ? _(L"Long format") : _(L"Wide format");
        const wxString description =
            isLong ?
                _(L"Each row is one observation, and each variable is its own column.") :
                _(L"Each row is one subject, and repeated measures such as survey questions are "
                  "spread across several columns.");

        return L"<div class='shape'>" + svg + L"<div><div class='shape-title'>" +
               EscapeForHtml(title) + L"</div>" + EscapeForHtml(description) + L"</div></div>";
        }

    //-------------------------------------------
    wxString DataGuideCtrl::BuildBulletCard(const std::vector<wxString>& items,
                                            const wxString& cardClass)
        {
        wxString html = L"<div class='note " + cardClass + L"'><ul>";
        for (const auto& item : items)
            {
            html += L"<li>" + EscapeForHtml(item) + L"</li>";
            }
        html += L"</ul></div>";
        return html;
        }

    //-------------------------------------------
    DataGuideCtrl::DataGuideCtrl(wxWindow* parent, const wxWindowID id, const wxPoint& pos,
                                 const wxSize& size)
        : wxPanel(parent, id, pos, size)
        {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        m_webView = wxWebView::New(this, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize);
        if (m_webView == nullptr)
            {
            wxLogError(L"Failed to create wxWebView. No backend available.");
            }
        else
            {
            m_webView->EnableContextMenu(false);
            m_webView->Bind(wxEVT_WEBVIEW_NAVIGATING, &DataGuideCtrl::OnNavigating, this);
            sizer->Add(m_webView, wxSizerFlags{ 1 }.Expand());
            }

        SetSizer(sizer);

        Bind(wxEVT_SYS_COLOUR_CHANGED, &DataGuideCtrl::OnSysColourChanged, this);
        }

    //-------------------------------------------
    void DataGuideCtrl::SetGuide(const DataGuide& guide)
        {
        m_guide = guide;
        RefreshPage();
        }

    //-------------------------------------------
    void DataGuideCtrl::RefreshPage()
        {
        if (m_webView != nullptr)
            {
            m_webView->SetPage(BuildHtml(), wxString{});
            }
        }

    //-------------------------------------------
    void DataGuideCtrl::OnSysColourChanged([[maybe_unused]] wxSysColourChangedEvent& event)
        {
        RefreshPage();
        }

    //-------------------------------------------
    void DataGuideCtrl::OnNavigating(wxWebViewEvent& event)
        {
        // the page is self-contained, so only the initial load is allowed
        if (const wxString url = event.GetURL();
            url.empty() || url.StartsWith(L"about:") || url.StartsWith(L"data:"))
            {
            event.Skip();
            return;
            }
        event.Veto();
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetBaseStyles()
        {
        return _DT(LR"raw(
* { box-sizing: border-box; }
body { margin: 0; padding: 24px 28px 32px; background: var(--bg); color: var(--fg);
       font: 14px/1.5 "Segoe UI Variable", "Segoe UI", system-ui, sans-serif; }
h1 { margin: 0 0 6px; font-size: 22px; font-weight: 600; letter-spacing: -0.01em; }
h2 { margin: 28px 0 12px; font-size: 12px; font-weight: 600; text-transform: uppercase;
     letter-spacing: 0.08em; opacity: 0.6; }
.summary { margin: 0; opacity: 0.85; }
.requirements { margin: 0; }
)raw");
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetHeroStyles()
        {
        return _DT(LR"raw(
.hero { display: flex; align-items: center; gap: 20px; padding: 22px 26px; margin-bottom: 8px;
        border: 1px solid var(--border); border-radius: 16px;
        background: linear-gradient(120deg, rgba(59,130,246,0.22), rgba(139,92,246,0.16) 55%,
                    rgba(236,72,153,0.12)), var(--card); }
.hero-icon { flex: none; display: flex; align-items: center; justify-content: center;
             width: 72px; height: 72px; border-radius: 18px; background: var(--bg);
             border: 1px solid var(--border); box-shadow: 0 6px 18px rgba(0,0,0,0.22); }
.hero-icon img { width: 44px; height: 44px; }
.hero h1 { font-size: 28px; font-weight: 700; letter-spacing: -0.02em; line-height: 1.15; }
.hero .summary { margin-top: 4px; font-size: 15px; }
)raw");
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetTableStyles()
        {
        return _DT(LR"raw(
.table-wrap { margin-top: 20px; overflow-x: auto; border: 1px solid var(--border);
              border-radius: 10px; background: var(--card); }
table { border-collapse: collapse; width: 100%; font-variant-numeric: tabular-nums; }
th, td { padding: 8px 14px; text-align: left; white-space: nowrap; }
th { font-weight: 600; border-top: 3px solid var(--accent); background: var(--accent-bg); }
td { border-top: 1px solid var(--border); }
.col-role { margin-bottom: 2px; font-size: 11px; font-weight: 700; text-transform: uppercase;
            letter-spacing: 0.06em; color: var(--accent); }
.ref { margin-top: 14px; }
.ref th { border-top: 0; background: transparent; font-size: 11px; text-transform: uppercase;
          letter-spacing: 0.06em; opacity: 0.7; }
.ref td { white-space: normal; vertical-align: middle; }
.ref td.var { font-weight: 600; white-space: nowrap; border-left: 4px solid var(--accent); }
.ref td.types { white-space: nowrap; }
.ref td.types img { width: 16px; height: 16px; margin-right: 6px; vertical-align: middle; }
.ref td.what { opacity: 0.85; min-width: 220px; }
.ref tr.group td { padding: 14px 14px 6px; font-size: 11px; font-weight: 700;
                   text-transform: uppercase; letter-spacing: 0.08em; opacity: 0.65; }
.ref td.values { white-space: nowrap; }
.value { margin: 2px 0; font-size: 12px; }
.value code { display: inline-block; min-width: 22px; padding: 0 6px; margin-right: 4px;
              text-align: center; border-radius: 6px; font-weight: 700;
              border: 1px solid var(--border); background: var(--bg); }
)raw");
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetChipStyles()
        {
        return _DT(LR"raw(
.chip { display: inline-flex; align-items: center; vertical-align: middle; gap: 5px;
        height: 22px; margin: 0 6px 0 0; padding: 0 8px; line-height: 1; border-radius: 999px;
        font-size: 11px; font-weight: 600; border: 1px solid var(--border); }
.chip img { width: 14px; height: 14px; }
.chip.required { background: var(--bg); border-color: var(--accent); }
.chip.conditional { background: var(--bg); border-style: dashed; border-color: var(--accent); }
.cond { margin-top: 4px; font-size: 11px; opacity: 0.8; white-space: normal; max-width: 170px; }
.shape { display: flex; align-items: center; gap: 16px; margin-top: 12px; padding: 10px 16px;
         border: 1px solid var(--border); border-radius: 10px; background: var(--card); }
.shape svg { flex: none; }
.shape-title { font-weight: 600; }
.legend { display: flex; flex-wrap: wrap; gap: 8px 22px; margin: 14px 4px 0; font-size: 12px;
          opacity: 0.85; }
.legend-item { display: inline-flex; align-items: center; gap: 6px; }
.legend-item img { width: 16px; height: 16px; }
)raw");
        }

    //-------------------------------------------
    wxString DataGuideCtrl::GetSectionStyles()
        {
        return _DT(LR"raw(
.section-title { display: flex; align-items: center; gap: 10px; margin: 32px 0 14px;
                 font-size: 17px; font-weight: 600; letter-spacing: -0.01em; }
.section-title::after { content: ''; flex: 1; height: 1px; opacity: 0.3;
                        background: linear-gradient(90deg, currentColor, transparent); }
.badge { display: flex; align-items: center; justify-content: center; width: 28px; height: 28px;
         border-radius: 8px; color: #fff;
         background: linear-gradient(135deg, rgb(59,130,246), rgb(139,92,246)); }
.badge.warn { background: linear-gradient(135deg, rgb(245,158,11), rgb(239,68,68)); }
.badge.missing { background: linear-gradient(135deg, rgb(20,184,166), rgb(59,130,246)); }
.note { --bullet: rgb(139,92,246); margin: 0 0 8px; padding: 8px 14px; border-radius: 10px;
        border: 1px solid var(--border); border-left: 4px solid var(--bullet);
        background: var(--card); }
.note.prereq { --bullet: rgb(245,158,11); }
.note.missing { --bullet: rgb(20,184,166); }
.note ul { margin: 0; padding: 0; list-style: none; }
.note li { position: relative; margin: 6px 0; padding-left: 20px; }
.note li::before { content: ''; position: absolute; left: 2px; top: 0.55em; width: 7px;
                   height: 7px; border-radius: 50%; background: var(--bullet); }
)raw");
        }

    //-------------------------------------------
    wxString DataGuideCtrl::BuildHtml() const
        {
        const wxString bgColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).GetAsString(wxC2S_HTML_SYNTAX);
        const wxString fgColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT).GetAsString(wxC2S_HTML_SYNTAX);
        const wxString cardColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE).GetAsString(wxC2S_HTML_SYNTAX);
        const wxString borderColor =
            wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW).GetAsString(wxC2S_HTML_SYNTAX);

        wxString html = _DT(L"<!DOCTYPE html><html><head><meta charset='utf-8'>"
                            L"<meta name='color-scheme' content='light dark'><style>");
        html += wxString::Format(L":root { --bg:%s; --fg:%s; --card:%s; --border:%s; }", bgColor,
                                 fgColor, cardColor, borderColor);
        html += GetBaseStyles();
        html += GetHeroStyles();
        html += GetTableStyles();
        html += GetChipStyles();
        html += GetSectionStyles();
        html += _DT(L"</style></head><body>");

        if (!m_guide.m_title.empty() || !m_guide.m_summary.empty())
            {
            html += L"<header class='hero'>";
            if (const wxString iconUri = GetSvgDataUri(m_guide.m_iconName, wxSize{ 96, 96 });
                !iconUri.empty())
                {
                html += L"<div class='hero-icon'><img src='" + iconUri + L"' alt=''></div>";
                }
            html += L"<div>";
            if (!m_guide.m_title.empty())
                {
                html += L"<h1>" + EscapeForHtml(m_guide.m_title) + L"</h1>";
                }
            if (!m_guide.m_summary.empty())
                {
                html += L"<p class='summary'>" + EscapeForHtml(m_guide.m_summary) + L"</p>";
                }
            html += L"</div></header>";
            }

        // legend of the column types used by the examples
        const std::array<Data::Dataset::ColumnImportType, 6> legendOrder{
            Data::Dataset::ColumnImportType::Numeric,
            Data::Dataset::ColumnImportType::String,
            Data::Dataset::ColumnImportType::Discrete,
            Data::Dataset::ColumnImportType::Date,
            Data::Dataset::ColumnImportType::DichotomousString,
            Data::Dataset::ColumnImportType::DichotomousDiscrete
        };
        wxString legendHtml;
        for (const auto legendType : legendOrder)
            {
            const bool isUsed = std::ranges::any_of(
                m_guide.m_parameters, [legendType](const auto& param)
                { return std::ranges::find(param.m_types, legendType) != param.m_types.cend(); });
            if (isUsed)
                {
                legendHtml += L"<span class='legend-item'>" + BuildTypeIconHtml(legendType) +
                              EscapeForHtml(GetTypeName(legendType)) + L"</span>";
                }
            }
        if (!legendHtml.empty())
            {
            html += L"<div class='legend'>" + legendHtml + L"</div>";
            }

        if (!m_guide.m_requirements.empty() || m_guide.m_shape != DataGuideShape::Unspecified)
            {
            html += L"<h2>" + EscapeForHtml(_(L"Data Requirements")) + L"</h2>";
            if (!m_guide.m_requirements.empty())
                {
                html +=
                    L"<p class='requirements'>" + EscapeForHtml(m_guide.m_requirements) + L"</p>";
                }
            html += BuildShapeHtml(m_guide.m_shape);
            }

        if (!m_guide.m_prerequisites.empty())
            {
            html += BuildSectionTitle(
                _(L"Before You Start"),
                L"<path d='M12 3 22 20H2Z' fill='none' stroke='currentColor' stroke-width='2' "
                L"stroke-linejoin='round'/>"
                L"<path d='M12 10v5' stroke='currentColor' stroke-width='2' "
                L"stroke-linecap='round'/>"
                L"<circle cx='12' cy='17.5' r='1.1' fill='currentColor'/>",
                L"warn");
            html += BuildBulletCard(m_guide.m_prerequisites, L"prereq");
            }

        // one table explaining every variable the graph can use
        if (!m_guide.m_parameters.empty())
            {
            html += L"<h2>" + EscapeForHtml(_(L"Variables")) + L"</h2>";
            // the Values column is only shown if some variable expects coded values
            const bool hasValues = std::ranges::any_of(m_guide.m_parameters, [](const auto& param)
                                                       { return !param.m_values.empty(); });
            html += L"<div class='table-wrap ref'><table><thead><tr><th>" +
                    EscapeForHtml(_(L"Variable")) + L"</th><th>" + EscapeForHtml(_(L"Accepts")) +
                    L"</th><th>" + EscapeForHtml(_(L"Required")) + L"</th><th>" +
                    EscapeForHtml(_(L"Columns")) + L"</th>";
            if (hasValues)
                {
                html += L"<th>" + EscapeForHtml(_(L"Expected Values")) + L"</th>";
                }
            html += L"<th>" + EscapeForHtml(_(L"What it does")) + L"</th></tr></thead><tbody>";
            for (size_t paramIndex = 0; paramIndex < m_guide.m_parameters.size(); ++paramIndex)
                {
                const auto& param = m_guide.m_parameters[paramIndex];
                if (!param.m_group.empty() &&
                    (paramIndex == 0 ||
                     m_guide.m_parameters[paramIndex - 1].m_group != param.m_group))
                    {
                    html += wxString::Format(L"<tr class='group'><td colspan='%d'>%s</td></tr>",
                                             hasValues ? 6 : 5, EscapeForHtml(param.m_group));
                    }
                html += L"<tr><td class='var' style='" + GetAccentStyle(paramIndex) + L"'>" +
                        EscapeForHtml(param.m_name);
                html += L"</td><td class='types'>";
                for (const auto type : param.m_types)
                    {
                    html += BuildTypeIconHtml(type);
                    }
                html += L"</td><td>";
                switch (param.m_requirement)
                    {
                case DataGuideRequirement::Required:
                    html += L"<span class='chip required' style='" + GetAccentStyle(paramIndex) +
                            L"'>" + EscapeForHtml(_(L"Required")) + L"</span>";
                    break;
                case DataGuideRequirement::Optional:
                    html += L"<span class='chip'>" + EscapeForHtml(_(L"Optional")) + L"</span>";
                    break;
                case DataGuideRequirement::Conditional:
                    html += L"<span class='chip conditional' style='" + GetAccentStyle(paramIndex) +
                            L"'>" + EscapeForHtml(_(L"Conditional")) + L"</span>";
                    if (!param.m_condition.empty())
                        {
                        html +=
                            L"<div class='cond'>" + EscapeForHtml(param.m_condition) + L"</div>";
                        }
                    break;
                    }
                html += L"</td><td>";
                html += L"<span class='chip'>" +
                        EscapeForHtml(param.m_multiple ? _(L"One or more") : _(L"One")) +
                        L"</span>";
                html += L"</td>";
                if (hasValues)
                    {
                    html += L"<td class='values'>";
                    for (const auto& value : param.m_values)
                        {
                        html += L"<div class='value'><code>" + EscapeForHtml(value.m_value) +
                                L"</code> " + EscapeForHtml(value.m_meaning) + L"</div>";
                        }
                    html += L"</td>";
                    }
                html += L"<td class='what'>" + EscapeForHtml(param.m_description) + L"</td></tr>";
                }
            html += L"</tbody></table></div>";
            }

        for (const auto& example : m_guide.m_examples)
            {
            if (!example.m_title.empty())
                {
                html += L"<h2>" + EscapeForHtml(example.m_title) + L"</h2>";
                }
            html += L"<div class='table-wrap'><table><thead><tr>";
            for (const auto& column : example.m_columns)
                {
                // color the column like its variable in the table above
                const auto paramPos =
                    std::ranges::find_if(m_guide.m_parameters, [&column](const auto& param)
                                         { return param.m_name == column.m_parameter; });
                const auto paramIndex =
                    static_cast<size_t>(std::distance(m_guide.m_parameters.cbegin(), paramPos));
                html += L"<th style='" + GetAccentStyle(paramIndex) + L"'>";
                if (paramPos != m_guide.m_parameters.cend())
                    {
                    html += L"<div class='col-role'>&rarr; " + EscapeForHtml(column.m_parameter) +
                            L"</div>";
                    }
                html += EscapeForHtml(column.m_name) + L"</th>";
                }
            html += L"</tr></thead><tbody>";
            for (const auto& row : example.m_rows)
                {
                html += L"<tr>";
                for (size_t colIndex = 0; colIndex < example.m_columns.size(); ++colIndex)
                    {
                    html += L"<td>" +
                            (colIndex < row.size() ? EscapeForHtml(row[colIndex]) : wxString{}) +
                            L"</td>";
                    }
                html += L"</tr>";
                }
            html += L"</tbody></table></div>";
            }

        if (!m_guide.m_missingData.empty())
            {
            // an "empty set" symbol: a circle with a slash through it
            html += BuildSectionTitle(
                _(L"Missing Data"),
                L"<circle cx='12' cy='12' r='8' fill='none' stroke='currentColor' "
                L"stroke-width='2'/>"
                L"<path d='M5.5 18.5 18.5 5.5' stroke='currentColor' stroke-width='2' "
                L"stroke-linecap='round'/>",
                L"missing");
            html += BuildBulletCard(m_guide.m_missingData, L"missing");
            }

        if (!m_guide.m_notes.empty())
            {
            html += BuildSectionTitle(_(L"Notes"),
                                      L"<circle cx='12' cy='12' r='9.5' fill='none' "
                                      L"stroke='currentColor' stroke-width='2'/>"
                                      L"<path d='M12 11v6' stroke='currentColor' stroke-width='2' "
                                      L"stroke-linecap='round'/>"
                                      L"<circle cx='12' cy='7.5' r='1.3' fill='currentColor'/>");
            html += BuildBulletCard(m_guide.m_notes, wxString{});
            }

        html += L"</body></html>";
        return html;
        }
    } // namespace Wisteria::UI
