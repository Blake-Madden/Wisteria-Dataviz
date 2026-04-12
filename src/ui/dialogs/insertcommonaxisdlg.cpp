///////////////////////////////////////////////////////////////////////////////
// Name:        insertcommonaxisdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertcommonaxisdlg.h"
#include "../../graphs/graph2d.h"
#include <wx/gbsizer.h>
#include <wx/tokenzr.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertCommonAxisDlg::InsertCommonAxisDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                             wxWindow* parent, const wxString& caption,
                                             const wxWindowID id, const wxPoint& pos,
                                             const wxSize& size, const long style,
                                             EditMode editMode)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                        ItemDlgIncludeAllPageOptions)
        {
        CreateControls();
        FinalizeControls();
        TransferDataToWindow();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertCommonAxisDlg::CreateControls()
        {
        InsertItemDlg::CreateControls();

        // Page 2: Common Axis
        //-------------
        auto* commonAxisPage = new wxPanel(GetSideBarBook());
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        commonAxisPage->SetSizer(mainSizer);

        // axis type
        auto* typeSizer = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        typeSizer->AddGrowableCol(1, 1);

        typeSizer->Add(new wxStaticText(commonAxisPage, wxID_ANY, _(L"Axis type:")),
                       wxSizerFlags{}.CenterVertical());
        m_axisTypeChoice = new wxChoice(commonAxisPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxArrayString{}, 0, wxGenericValidator(&m_axisTypeIndex));
        m_axisTypeChoice->Append(_(L"Bottom X"));
        m_axisTypeChoice->Append(_(L"Top X"));
        m_axisTypeChoice->Append(_(L"Left Y"));
        m_axisTypeChoice->Append(_(L"Right Y"));
        m_axisTypeChoice->SetSelection(0);
        m_axisTypeChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { OnAxisTypeChanged(); });
        typeSizer->Add(m_axisTypeChoice, wxSizerFlags{}.Expand());

        mainSizer->Add(typeSizer, wxSizerFlags{}.Expand().Border());

        // connected graphs
        auto* graphBox = new wxStaticBoxSizer(wxVERTICAL, commonAxisPage, _(L"Connected Graphs"));
        m_graphChecklist = new wxCheckListBox(graphBox->GetStaticBox(), wxID_ANY);
        PopulateGraphChecklist();
        graphBox->Add(m_graphChecklist, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(graphBox, wxSizerFlags{ 1 }.Expand().Border());

        // common perpendicular axis
        m_commonPerpAxisCheck = new wxCheckBox(
            commonAxisPage, wxID_ANY, _(L"Use common perpendicular axis"), wxDefaultPosition,
            wxDefaultSize, 0, wxGenericValidator(&m_commonPerpAxis));
        mainSizer->Add(m_commonPerpAxisCheck, wxSizerFlags{}.Border());

        GetSideBarBook()->AddPage(commonAxisPage, _(L"Common Axis"), ID_COMMON_AXIS_SECTION, true);

        // Page 3: Axis
        //-------------
        m_axisOptionsPanel =
            new AxisOptionsPanel(GetSideBarBook(), GetCanvas(), GetReportBuilder());
        GetSideBarBook()->AddPage(m_axisOptionsPanel, _(L"Axis"), ID_AXIS_OPTIONS_SECTION, true);

        // initialize the axis panel with a default axis for the selected type
        OnAxisTypeChanged();
        }

    //-------------------------------------------
    void InsertCommonAxisDlg::PopulateGraphChecklist()
        {
        m_graphChecklist->Clear();
        m_graphIdMap.clear();

        const auto [rows, cols] = GetCanvas()->GetFixedObjectsGridSize();
        for (size_t row = 0; row < rows; ++row)
            {
            for (size_t col = 0; col < cols; ++col)
                {
                const auto item = GetCanvas()->GetFixedObject(row, col);
                if (item != nullptr && dynamic_cast<Graphs::Graph2D*>(item.get()) != nullptr)
                    {
                    const wxString label =
                        wxString::Format(_(L"Row %zu, Col %zu (ID: %ld)"), row, col, item->GetId());
                    m_graphChecklist->Append(label);
                    m_graphIdMap.push_back(item->GetId());
                    }
                }
            }
        }

    //-------------------------------------------
    void InsertCommonAxisDlg::OnAxisTypeChanged()
        {
        TransferDataFromWindow();

        // common perpendicular axis only applies to X axes
        const bool isXAxis = (m_axisTypeIndex == 0 || m_axisTypeIndex == 1);
        m_commonPerpAxisCheck->Enable(isXAxis);
        if (!isXAxis)
            {
            m_commonPerpAxisCheck->SetValue(false);
            m_commonPerpAxis = false;
            }

        // seed the axis options panel with a default axis of the selected type
        const auto axisType = GetAxisType();
        auto currentAxes = m_axisOptionsPanel->GetAxes();
        if (currentAxes.count(axisType) == 0)
            {
            // preserve any existing axis state if only the type is being changed
            GraphItems::Axis defaultAxis(axisType);
            if (!currentAxes.empty())
                {
                defaultAxis = currentAxes.begin()->second;
                }
            currentAxes.clear();
            currentAxes.emplace(axisType, std::move(defaultAxis));
            m_axisOptionsPanel->SetAxes(currentAxes);
            }
        }

    //-------------------------------------------
    bool InsertCommonAxisDlg::Validate()
        {
        TransferDataFromWindow();

        size_t checkedCount = 0;
        for (unsigned int idx = 0; idx < m_graphChecklist->GetCount(); ++idx)
            {
            if (m_graphChecklist->IsChecked(idx))
                {
                ++checkedCount;
                }
            }
        if (checkedCount < 2)
            {
            wxMessageBox(_(L"Please select at least two graphs for the common axis."),
                         _(L"Validation"), wxOK | wxICON_WARNING, this);
            return false;
            }
        return true;
        }

    //-------------------------------------------
    AxisType InsertCommonAxisDlg::GetAxisType() const noexcept
        {
        switch (m_axisTypeIndex)
            {
        case 1:
            return AxisType::TopXAxis;
        case 2:
            return AxisType::LeftYAxis;
        case 3:
            return AxisType::RightYAxis;
        default:
            return AxisType::BottomXAxis;
            }
        }

    //-------------------------------------------
    std::vector<long> InsertCommonAxisDlg::GetChildGraphIds() const
        {
        std::vector<long> ids;
        for (unsigned int idx = 0; idx < m_graphChecklist->GetCount(); ++idx)
            {
            if (m_graphChecklist->IsChecked(idx))
                {
                ids.push_back(m_graphIdMap[idx]);
                }
            }
        return ids;
        }

    //-------------------------------------------
    std::map<AxisType, GraphItems::Axis> InsertCommonAxisDlg::GetAxes()
        {
        return m_axisOptionsPanel->GetAxes();
        }

    //-------------------------------------------
    void InsertCommonAxisDlg::LoadFromAxis(const GraphItems::Axis& axis)
        {
        // axis type
        switch (axis.GetAxisType())
            {
        case AxisType::TopXAxis:
            m_axisTypeIndex = 1;
            break;
        case AxisType::LeftYAxis:
            m_axisTypeIndex = 2;
            break;
        case AxisType::RightYAxis:
            m_axisTypeIndex = 3;
            break;
        default:
            m_axisTypeIndex = 0;
            break;
            }

        // parse child-ids property template ("1,2,3")
        const auto childIdsStr = axis.GetPropertyTemplate(L"child-ids");
        std::vector<long> childIds;
        if (!childIdsStr.empty())
            {
            wxStringTokenizer tokenizer(childIdsStr, L",");
            while (tokenizer.HasMoreTokens())
                {
                long val = 0;
                if (tokenizer.GetNextToken().Trim().Trim(false).ToLong(&val))
                    {
                    childIds.push_back(val);
                    }
                }
            }

        // pre-check matching graphs
        for (unsigned int idx = 0; idx < m_graphChecklist->GetCount(); ++idx)
            {
            if (std::cmp_less(idx, m_graphIdMap.size()) &&
                std::find(childIds.begin(), childIds.end(), m_graphIdMap[idx]) != childIds.end())
                {
                m_graphChecklist->Check(idx, true);
                }
            }

        // common perpendicular axis
        m_commonPerpAxis = (axis.GetPropertyTemplate(L"common-perpendicular-axis") == L"true");

        // sync the panel's axis selector to match, then load the axis data
        m_axisOptionsPanel->SelectAxis(axis.GetAxisType());
        // propagate bracket source templates as panel hints so "Add Brackets from
        // Dataset" pre-selects the dataset/columns even if the axis copy loses them
        m_axisOptionsPanel->SetBracketColumnHints(axis.GetPropertyTemplate(L"brackets.dataset"),
                                                  axis.GetPropertyTemplate(L"bracket.label"),
                                                  axis.GetPropertyTemplate(L"bracket.value"));
        std::map<AxisType, GraphItems::Axis> axisMap;
        axisMap.emplace(axis.GetAxisType(), axis);
        m_axisOptionsPanel->SetAxes(axisMap);

        TransferDataToWindow();
        OnAxisTypeChanged();
        }

    } // namespace Wisteria::UI
