///////////////////////////////////////////////////////////////////////////////
// Name:        axisoptionspanel.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "axisoptionspanel.h"
#include "../../app/wisteriaapp.h"
#include "../../base/reportenumconvert.h"
#include "insertitemdlg.h"
#include "insertlabeldlg.h"
#include <wx/gbsizer.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    AxisOptionsPanel::AxisOptionsPanel(wxWindow* parent, Canvas* canvas,
                                       const ReportBuilder* reportBuilder)
        : wxPanel(parent), m_canvas(canvas), m_reportBuilder(reportBuilder)
        {
        auto* axisSizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(axisSizer);

        // axis selector
        auto* selectorSizer = new wxBoxSizer(wxHORIZONTAL);
        selectorSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Edit axis:")),
                           wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_axisSelector = new wxChoice(this, wxID_ANY);
        m_axisSelector->Append(_(L"Bottom X"));
        m_axisSelector->Append(_(L"Top X"));
        m_axisSelector->Append(_(L"Left Y"));
        m_axisSelector->Append(_(L"Right Y"));
        m_axisSelector->SetSelection(0);
        m_axisSelector->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { OnAxisSelectionChanged(); });
        selectorSizer->Add(m_axisSelector, wxSizerFlags{ 1 }.Expand());
        axisSizer->Add(selectorSizer, wxSizerFlags{}.Expand().Border());

        auto* gridSizer =
            new wxGridBagSizer(wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder());

        // Group 1: Title / Header / Footer
        //------------------
        auto* titleBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Title / Header / Footer"));
        auto* titleGrid = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                              wxSizerFlags::GetDefaultBorder());
        titleGrid->AddGrowableCol(0, 1);
        titleBox->Add(titleGrid, wxSizerFlags{}.Expand().Border());

        titleGrid->Add(new wxStaticText(titleBox->GetStaticBox(), wxID_ANY, _(L"Title:")),
                       wxSizerFlags{}.CenterVertical());
        auto* editTitleBtn = new wxButton(titleBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editTitleBtn->Bind(wxEVT_BUTTON,
                           [this](wxCommandEvent&)
                           {
                               auto& axis = m_savedAxes.at(m_currentAxisType);
                               GraphItems::Label label = axis.GetTitle();
                               InsertLabelDlg dlg(
                                   m_canvas, m_reportBuilder, this, _(L"Edit Axis Title"), wxID_ANY,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                   label.GetText().empty() ? InsertItemDlg::EditMode::Insert :
                                                             InsertItemDlg::EditMode::Edit,
                                   false /*includePageOptions*/);
                               if (!label.GetText().empty())
                                   {
                                   dlg.LoadFromLabel(label);
                                   }
                               if (dlg.ShowModal() == wxID_OK)
                                   {
                                   dlg.ApplyToLabel(axis.GetTitle());
                                   }
                           });
        titleGrid->Add(editTitleBtn);

        titleGrid->Add(new wxStaticText(titleBox->GetStaticBox(), wxID_ANY, _(L"Header:")),
                       wxSizerFlags{}.CenterVertical());
        auto* editHeaderBtn = new wxButton(titleBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editHeaderBtn->Bind(wxEVT_BUTTON,
                            [this](wxCommandEvent&)
                            {
                                auto& axis = m_savedAxes.at(m_currentAxisType);
                                GraphItems::Label label = axis.GetHeader();
                                InsertLabelDlg dlg(
                                    m_canvas, m_reportBuilder, this, _(L"Edit Axis Header"),
                                    wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                    label.GetText().empty() ? InsertItemDlg::EditMode::Insert :
                                                              InsertItemDlg::EditMode::Edit,
                                    false /*includePageOptions*/);
                                if (!label.GetText().empty())
                                    {
                                    dlg.LoadFromLabel(label);
                                    }
                                if (dlg.ShowModal() == wxID_OK)
                                    {
                                    dlg.ApplyToLabel(axis.GetHeader());
                                    }
                            });
        titleGrid->Add(editHeaderBtn);

        titleGrid->Add(new wxStaticText(titleBox->GetStaticBox(), wxID_ANY, _(L"Footer:")),
                       wxSizerFlags{}.CenterVertical());
        auto* editFooterBtn = new wxButton(titleBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editFooterBtn->Bind(wxEVT_BUTTON,
                            [this](wxCommandEvent&)
                            {
                                auto& axis = m_savedAxes.at(m_currentAxisType);
                                GraphItems::Label label = axis.GetFooter();
                                InsertLabelDlg dlg(
                                    m_canvas, m_reportBuilder, this, _(L"Edit Axis Footer"),
                                    wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                    label.GetText().empty() ? InsertItemDlg::EditMode::Insert :
                                                              InsertItemDlg::EditMode::Edit,
                                    false /*includePageOptions*/);
                                if (!label.GetText().empty())
                                    {
                                    dlg.LoadFromLabel(label);
                                    }
                                if (dlg.ShowModal() == wxID_OK)
                                    {
                                    dlg.ApplyToLabel(axis.GetFooter());
                                    }
                            });
        titleGrid->Add(editFooterBtn);

        gridSizer->Add(titleBox, wxGBPosition(0, 0), wxDefaultSpan, wxEXPAND | wxALL,
                       wxSizerFlags::GetDefaultBorder());

        // Group 2: Axis Line
        //------------------
        auto* lineBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Axis Line"));
        auto* lineGrid = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                             wxSizerFlags::GetDefaultBorder());
        lineGrid->AddGrowableCol(1, 1);
        lineBox->Add(lineGrid, wxSizerFlags{}.Expand().Border());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisLineColorPicker = new wxColourPickerCtrl(lineBox->GetStaticBox(), wxID_ANY);
        lineGrid->Add(m_axisLineColorPicker, wxSizerFlags{}.Expand());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisLineWidthSpin =
            new wxSpinCtrl(lineBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
        lineGrid->Add(m_axisLineWidthSpin, wxSizerFlags{}.Expand());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Style:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisLineStyleChoice = new wxChoice(lineBox->GetStaticBox(), wxID_ANY);
        m_axisLineStyleChoice->Append(_(L"None"));
        m_axisLineStyleChoice->Append(_(L"Solid"));
        m_axisLineStyleChoice->Append(_(L"Dot"));
        m_axisLineStyleChoice->Append(_(L"Long dash"));
        m_axisLineStyleChoice->Append(_(L"Short dash"));
        m_axisLineStyleChoice->Append(_(L"Dot dash"));
        lineGrid->Add(m_axisLineStyleChoice, wxSizerFlags{}.Expand());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Cap:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisCapStyleChoice = new wxChoice(lineBox->GetStaticBox(), wxID_ANY);
        m_axisCapStyleChoice->Append(_(L"No cap"));
        m_axisCapStyleChoice->Append(_(L"Arrow"));
        lineGrid->Add(m_axisCapStyleChoice, wxSizerFlags{}.Expand());

        m_axisReverseCheck = new wxCheckBox(lineBox->GetStaticBox(), wxID_ANY, _(L"Reverse"));
        lineBox->Add(m_axisReverseCheck, wxSizerFlags{}.Border());

        gridSizer->Add(lineBox, wxGBPosition(1, 0), wxDefaultSpan, wxEXPAND | wxALL,
                       wxSizerFlags::GetDefaultBorder());

        // Group 3: Gridlines
        //------------------
        auto* gridlineBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Gridlines"));
        auto* gridlineGrid = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                                 wxSizerFlags::GetDefaultBorder());
        gridlineGrid->AddGrowableCol(1, 1);
        gridlineBox->Add(gridlineGrid, wxSizerFlags{}.Expand().Border());

        gridlineGrid->Add(new wxStaticText(gridlineBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                          wxSizerFlags{}.CenterVertical());
        m_gridlineColorPicker = new wxColourPickerCtrl(gridlineBox->GetStaticBox(), wxID_ANY);
        gridlineGrid->Add(m_gridlineColorPicker, wxSizerFlags{}.Expand());

        gridlineGrid->Add(new wxStaticText(gridlineBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                          wxSizerFlags{}.CenterVertical());
        m_gridlineWidthSpin =
            new wxSpinCtrl(gridlineBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
        gridlineGrid->Add(m_gridlineWidthSpin, wxSizerFlags{}.Expand());

        gridlineGrid->Add(new wxStaticText(gridlineBox->GetStaticBox(), wxID_ANY, _(L"Style:")),
                          wxSizerFlags{}.CenterVertical());
        m_gridlineStyleChoice = new wxChoice(gridlineBox->GetStaticBox(), wxID_ANY);
        m_gridlineStyleChoice->Append(_(L"None"));
        m_gridlineStyleChoice->Append(_(L"Solid"));
        m_gridlineStyleChoice->Append(_(L"Dot"));
        m_gridlineStyleChoice->Append(_(L"Long dash"));
        m_gridlineStyleChoice->Append(_(L"Short dash"));
        m_gridlineStyleChoice->Append(_(L"Dot dash"));
        gridlineGrid->Add(m_gridlineStyleChoice, wxSizerFlags{}.Expand());

        gridSizer->Add(gridlineBox, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND | wxALL,
                       wxSizerFlags::GetDefaultBorder());

        // Group 4: Tickmarks
        //------------------
        auto* tickBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Tickmarks"));
        auto* tickGrid = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                             wxSizerFlags::GetDefaultBorder());
        tickGrid->AddGrowableCol(1, 1);
        tickBox->Add(tickGrid, wxSizerFlags{}.Expand().Border());

        tickGrid->Add(new wxStaticText(tickBox->GetStaticBox(), wxID_ANY, _(L"Display:")),
                      wxSizerFlags{}.CenterVertical());
        m_tickmarkDisplayChoice = new wxChoice(tickBox->GetStaticBox(), wxID_ANY);
        m_tickmarkDisplayChoice->Append(_(L"Inner"));
        m_tickmarkDisplayChoice->Append(_(L"Outer"));
        m_tickmarkDisplayChoice->Append(_(L"Crossed"));
        m_tickmarkDisplayChoice->Append(_(L"No display"));
        tickGrid->Add(m_tickmarkDisplayChoice, wxSizerFlags{}.Expand());

        gridSizer->Add(tickBox, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND | wxALL,
                       wxSizerFlags::GetDefaultBorder());

        // Group 5: Labels
        //------------------
        auto* labelBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Labels"));
        auto* labelGrid = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                              wxSizerFlags::GetDefaultBorder());
        labelGrid->AddGrowableCol(1, 1);
        labelBox->Add(labelGrid, wxSizerFlags{}.Expand().Border());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Display:")),
                       wxSizerFlags{}.CenterVertical());
        m_labelDisplayChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_labelDisplayChoice->Append(_(L"Custom labels or values"));
        m_labelDisplayChoice->Append(_(L"Only custom labels"));
        m_labelDisplayChoice->Append(_(L"Custom labels and values"));
        m_labelDisplayChoice->Append(_(L"No display"));
        m_labelDisplayChoice->Append(_(L"Values"));
        labelGrid->Add(m_labelDisplayChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Number format:")),
                       wxSizerFlags{}.CenterVertical());
        m_numberDisplayChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_numberDisplayChoice->Append(_(L"Value"));
        m_numberDisplayChoice->Append(_(L"Percentage"));
        m_numberDisplayChoice->Append(_(L"Currency"));
        m_numberDisplayChoice->Append(_(L"Value (simple)"));
        labelGrid->Add(m_numberDisplayChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Orientation:")),
                       wxSizerFlags{}.CenterVertical());
        m_labelOrientationChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_labelOrientationChoice->Append(_(L"Parallel"));
        m_labelOrientationChoice->Append(_(L"Perpendicular"));
        labelGrid->Add(m_labelOrientationChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Perp. alignment:")),
                       wxSizerFlags{}.CenterVertical());
        m_perpAlignmentChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_perpAlignmentChoice->Append(_(L"Align with axis line"));
        m_perpAlignmentChoice->Append(_(L"Align with boundary"));
        m_perpAlignmentChoice->Append(_(L"Center on axis line"));
        labelGrid->Add(m_perpAlignmentChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Precision:")),
                       wxSizerFlags{}.CenterVertical());
        m_precisionSpin =
            new wxSpinCtrl(labelBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0);
        labelGrid->Add(m_precisionSpin, wxSizerFlags{}.Expand());

        labelGrid->Add(
            new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Label line length:")),
            wxSizerFlags{}.CenterVertical());
        m_labelLineLengthSpin =
            new wxSpinCtrl(labelBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 10, 500, 100);
        labelGrid->Add(m_labelLineLengthSpin, wxSizerFlags{}.Expand());

        m_doubleSidedCheck =
            new wxCheckBox(labelBox->GetStaticBox(), wxID_ANY, _(L"Double-sided labels"));
        labelBox->Add(m_doubleSidedCheck, wxSizerFlags{}.Border());
        m_showOuterLabelsCheck =
            new wxCheckBox(labelBox->GetStaticBox(), wxID_ANY, _(L"Show outer labels"));
        labelBox->Add(m_showOuterLabelsCheck, wxSizerFlags{}.Border());
        m_stackLabelsCheck = new wxCheckBox(labelBox->GetStaticBox(), wxID_ANY, _(L"Stack labels"));
        labelBox->Add(m_stackLabelsCheck, wxSizerFlags{}.Border());

        gridSizer->Add(labelBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND | wxALL,
                       wxSizerFlags::GetDefaultBorder());

        // Group 6: Brackets
        //------------------
        auto* bracketBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Brackets"));
        m_bracketListBox = new wxEditableListBox(
            bracketBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        RefreshBracketList();
        bracketBox->Add(m_bracketListBox, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

        m_bracketListBox->GetNewButton()->Bind(wxEVT_BUTTON,
                                               [this](wxCommandEvent&) { OnAddBracket(); });
        m_bracketListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"label.svg", wxSize{ 16, 16 }));
        m_bracketListBox->GetEditButton()->Bind(wxEVT_BUTTON,
                                                [this](wxCommandEvent&) { OnEditBracket(); });
        m_bracketListBox->GetDelButton()->Bind(wxEVT_BUTTON,
                                               [this](wxCommandEvent&) { OnRemoveBracket(); });
        m_bracketListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                               [this](wxListEvent&) { OnEditBracket(); });

        auto* addFromDatasetBtn =
            new wxButton(bracketBox->GetStaticBox(), wxID_ANY, _(L"Add from dataset..."));
        addFromDatasetBtn->Bind(wxEVT_BUTTON,
                                [this](wxCommandEvent&) { OnAddBracketsFromDataset(); });
        bracketBox->Add(addFromDatasetBtn, wxSizerFlags{}.Border());

        gridSizer->Add(bracketBox, wxGBPosition(2, 0), wxGBSpan(1, 3), wxEXPAND | wxALL,
                       wxSizerFlags::GetDefaultBorder());

        // global mirror checkboxes
        m_mirrorXAxisCheck = new wxCheckBox(this, wxID_ANY, _(L"Mirror X axis"));
        gridSizer->Add(m_mirrorXAxisCheck, wxGBPosition(3, 0), wxDefaultSpan, wxALL,
                       wxSizerFlags::GetDefaultBorder());

        m_mirrorYAxisCheck = new wxCheckBox(this, wxID_ANY, _(L"Mirror Y axis"));
        gridSizer->Add(m_mirrorYAxisCheck, wxGBPosition(3, 1), wxDefaultSpan, wxALL,
                       wxSizerFlags::GetDefaultBorder());

        axisSizer->Add(gridSizer, wxSizerFlags{ 1 }.Expand());
        }

    //-------------------------------------------
    void AxisOptionsPanel::SelectAxis(AxisType axisType)
        {
        int sel = 0;
        switch (axisType)
            {
        case AxisType::TopXAxis:
            sel = 1;
            break;
        case AxisType::LeftYAxis:
            sel = 2;
            break;
        case AxisType::RightYAxis:
            sel = 3;
            break;
        default:
            sel = 0;
            break;
            }
        m_axisSelector->SetSelection(sel);
        OnAxisSelectionChanged();
        }

    //-------------------------------------------
    void AxisOptionsPanel::SetAxes(const std::map<AxisType, GraphItems::Axis>& axes)
        {
        m_savedAxes = axes;
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            ReadControlsFromAxis(m_savedAxes.at(m_currentAxisType));
            }
        }

    //-------------------------------------------
    std::map<AxisType, GraphItems::Axis> AxisOptionsPanel::GetAxes()
        {
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            WriteControlsToAxis(m_savedAxes.at(m_currentAxisType));
            }
        return m_savedAxes;
        }

    //-------------------------------------------
    void AxisOptionsPanel::SetMirrorX(bool mirror) { m_mirrorXAxisCheck->SetValue(mirror); }

    bool AxisOptionsPanel::GetMirrorX() const { return m_mirrorXAxisCheck->GetValue(); }

    //-------------------------------------------
    void AxisOptionsPanel::SetMirrorY(bool mirror) { m_mirrorYAxisCheck->SetValue(mirror); }

    bool AxisOptionsPanel::GetMirrorY() const { return m_mirrorYAxisCheck->GetValue(); }

    //-------------------------------------------
    namespace
        {
        int PenStyleToIndex(const wxPenStyle style)
            {
            switch (style)
                {
            case wxPENSTYLE_SOLID:
                return 1;
            case wxPENSTYLE_DOT:
                return 2;
            case wxPENSTYLE_LONG_DASH:
                return 3;
            case wxPENSTYLE_SHORT_DASH:
                return 4;
            case wxPENSTYLE_DOT_DASH:
                return 5;
            case wxPENSTYLE_TRANSPARENT:
                [[fallthrough]];
            default:
                return 0;
                }
            }

        wxPenStyle IndexToPenStyle(const int index)
            {
            constexpr wxPenStyle styles[] = { wxPENSTYLE_TRANSPARENT, wxPENSTYLE_SOLID,
                                              wxPENSTYLE_DOT,         wxPENSTYLE_LONG_DASH,
                                              wxPENSTYLE_SHORT_DASH,  wxPENSTYLE_DOT_DASH };
            return (index >= 0 && index < 6) ? styles[index] : wxPENSTYLE_TRANSPARENT;
            }
        } // unnamed namespace

    //-------------------------------------------
    void AxisOptionsPanel::OnAxisSelectionChanged()
        {
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            WriteControlsToAxis(m_savedAxes.at(m_currentAxisType));
            }

        constexpr AxisType axisTypes[] = { AxisType::BottomXAxis, AxisType::TopXAxis,
                                           AxisType::LeftYAxis, AxisType::RightYAxis };
        const int sel = m_axisSelector->GetSelection();
        if (sel >= 0 && sel < 4)
            {
            m_currentAxisType = axisTypes[sel];
            }

        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            ReadControlsFromAxis(m_savedAxes.at(m_currentAxisType));
            }
        }

    //-------------------------------------------
    void AxisOptionsPanel::ReadControlsFromAxis(const GraphItems::Axis& axis)
        {
        if (m_axisLineColorPicker != nullptr)
            {
            const auto& pen = axis.GetAxisLinePen();
            if (pen.IsOk())
                {
                m_axisLineColorPicker->SetColour(pen.GetColour());
                m_axisLineWidthSpin->SetValue(pen.GetWidth());
                m_axisLineStyleChoice->SetSelection(PenStyleToIndex(pen.GetStyle()));
                }
            else
                {
                m_axisLineStyleChoice->SetSelection(0);
                }
            }
        if (m_axisCapStyleChoice != nullptr)
            {
            m_axisCapStyleChoice->SetSelection(axis.GetCapStyle() == AxisCapStyle::Arrow ? 1 : 0);
            }
        if (m_axisReverseCheck != nullptr)
            {
            m_axisReverseCheck->SetValue(axis.IsReversed());
            }

        if (m_gridlineColorPicker != nullptr)
            {
            const auto& pen = axis.GetGridlinePen();
            if (pen.IsOk())
                {
                m_gridlineColorPicker->SetColour(pen.GetColour());
                m_gridlineWidthSpin->SetValue(pen.GetWidth());
                m_gridlineStyleChoice->SetSelection(PenStyleToIndex(pen.GetStyle()));
                }
            else
                {
                m_gridlineStyleChoice->SetSelection(0);
                }
            }

        if (m_tickmarkDisplayChoice != nullptr)
            {
            m_tickmarkDisplayChoice->SetSelection(static_cast<int>(axis.GetTickMarkDisplay()));
            }

        if (m_labelDisplayChoice != nullptr)
            {
            m_labelDisplayChoice->SetSelection(static_cast<int>(axis.GetLabelDisplay()));
            }
        if (m_numberDisplayChoice != nullptr)
            {
            m_numberDisplayChoice->SetSelection(static_cast<int>(axis.GetNumberDisplay()));
            }
        if (m_labelOrientationChoice != nullptr)
            {
            m_labelOrientationChoice->SetSelection(
                static_cast<int>(axis.GetAxisLabelOrientation()));
            }
        if (m_perpAlignmentChoice != nullptr)
            {
            m_perpAlignmentChoice->SetSelection(
                static_cast<int>(axis.GetPerpendicularLabelAxisAlignment()));
            }
        if (m_precisionSpin != nullptr)
            {
            m_precisionSpin->SetValue(axis.GetPrecision());
            }
        if (m_doubleSidedCheck != nullptr)
            {
            m_doubleSidedCheck->SetValue(axis.HasDoubleSidedAxisLabels());
            }
        if (m_showOuterLabelsCheck != nullptr)
            {
            m_showOuterLabelsCheck->SetValue(axis.IsShowingOuterLabels());
            }
        if (m_stackLabelsCheck != nullptr)
            {
            m_stackLabelsCheck->SetValue(axis.IsStackingLabels());
            }
        if (m_labelLineLengthSpin != nullptr)
            {
            m_labelLineLengthSpin->SetValue(static_cast<int>(axis.GetLabelLineLength()));
            }

        // if the axis was loaded from a dataset-driven brackets definition, use its
        // stored templates as hints so "Add Brackets from Dataset" preselects them
        const auto bracketDs = axis.GetPropertyTemplate(L"brackets.dataset");
        if (!bracketDs.empty())
            {
            m_bracketDatasetHint = bracketDs;
            }
        const auto bracketLabel = axis.GetPropertyTemplate(L"bracket.label");
        if (!bracketLabel.empty())
            {
            m_bracketLabelColumnHint = bracketLabel;
            }
        const auto bracketValue = axis.GetPropertyTemplate(L"bracket.value");
        if (!bracketValue.empty())
            {
            m_bracketValueColumnHint = bracketValue;
            }

        RefreshBracketList();
        }

    //-------------------------------------------
    void AxisOptionsPanel::WriteControlsToAxis(GraphItems::Axis& axis)
        {
        if (m_axisLineColorPicker != nullptr)
            {
            // style index 0 means "none"; assign wxNullPen so the axis is drawn
            // without a line, rather than mutating the existing pen (which would
            // promote a previously-null pen into a valid one)
            if (m_axisLineStyleChoice->GetSelection() == 0)
                {
                axis.GetAxisLinePen() = wxNullPen;
                }
            else
                {
                axis.GetAxisLinePen().SetColour(m_axisLineColorPicker->GetColour());
                axis.GetAxisLinePen().SetWidth(m_axisLineWidthSpin->GetValue());
                axis.GetAxisLinePen().SetStyle(
                    IndexToPenStyle(m_axisLineStyleChoice->GetSelection()));
                }
            }
        if (m_axisCapStyleChoice != nullptr)
            {
            axis.SetCapStyle(m_axisCapStyleChoice->GetSelection() == 1 ? AxisCapStyle::Arrow :
                                                                         AxisCapStyle::NoCap);
            }
        if (m_axisReverseCheck != nullptr)
            {
            axis.Reverse(m_axisReverseCheck->GetValue());
            }

        if (m_gridlineColorPicker != nullptr)
            {
            if (m_gridlineStyleChoice->GetSelection() == 0)
                {
                axis.GetGridlinePen() = wxNullPen;
                }
            else
                {
                axis.GetGridlinePen().SetColour(m_gridlineColorPicker->GetColour());
                axis.GetGridlinePen().SetWidth(m_gridlineWidthSpin->GetValue());
                axis.GetGridlinePen().SetStyle(
                    IndexToPenStyle(m_gridlineStyleChoice->GetSelection()));
                }
            }

        if (m_tickmarkDisplayChoice != nullptr)
            {
            const int sel = m_tickmarkDisplayChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetTickMarkDisplay(static_cast<GraphItems::Axis::TickMark::DisplayType>(sel));
                }
            }

        if (m_labelDisplayChoice != nullptr)
            {
            const int sel = m_labelDisplayChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetLabelDisplay(static_cast<AxisLabelDisplay>(sel));
                }
            }
        if (m_numberDisplayChoice != nullptr)
            {
            const int sel = m_numberDisplayChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetNumberDisplay(static_cast<NumberDisplay>(sel));
                }
            }
        if (m_labelOrientationChoice != nullptr)
            {
            const int sel = m_labelOrientationChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetAxisLabelOrientation(static_cast<AxisLabelOrientation>(sel));
                }
            }
        if (m_perpAlignmentChoice != nullptr)
            {
            const int sel = m_perpAlignmentChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetPerpendicularLabelAxisAlignment(static_cast<AxisLabelAlignment>(sel));
                }
            }
        if (m_precisionSpin != nullptr)
            {
            axis.SetPrecision(static_cast<uint8_t>(m_precisionSpin->GetValue()));
            }
        if (m_doubleSidedCheck != nullptr)
            {
            axis.SetDoubleSidedAxisLabels(m_doubleSidedCheck->GetValue());
            }
        if (m_showOuterLabelsCheck != nullptr)
            {
            axis.ShowOuterLabels(m_showOuterLabelsCheck->GetValue());
            }
        if (m_stackLabelsCheck != nullptr)
            {
            axis.StackLabels(m_stackLabelsCheck->GetValue());
            }
        if (m_labelLineLengthSpin != nullptr)
            {
            axis.SetLabelLineLength(static_cast<size_t>(m_labelLineLengthSpin->GetValue()));
            }
        }

    //-------------------------------------------
    void AxisOptionsPanel::RefreshBracketList()
        {
        if (m_bracketListBox == nullptr)
            {
            return;
            }
        wxArrayString items;
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            for (const auto& bracket : m_savedAxes.at(m_currentAxisType).GetBrackets())
                {
                items.Add(wxString::Format(L"%s @ %g\u2013%g", bracket.GetLabel().GetText(),
                                           bracket.GetStartPosition(), bracket.GetEndPosition()));
                }
            }
        m_bracketListBox->SetStrings(items);
        }

    //-------------------------------------------
    void AxisOptionsPanel::OnAddBracket()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0)
            {
            return;
            }
        auto& axis = m_savedAxes.at(m_currentAxisType);

        wxDialog dlg(this, wxID_ANY, _(L"Add Bracket"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                          wxSizerFlags::GetDefaultBorder());
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")), wxSizerFlags{}.CenterVertical());
        auto* labelCombo = new wxComboBox(&dlg, wxID_ANY);
        const auto& customLabels = axis.GetCustomLabels();
        for (const auto& [value, label] : customLabels)
            {
            if (!label.GetText().empty())
                {
                labelCombo->Append(label.GetText(),
                                   new wxStringClientData(wxString::Format(L"%g", value)));
                }
            }
        for (const auto& point : std::as_const(axis).GetAxisPoints())
            {
            if (point.IsShown() && !point.GetDisplayValue().empty())
                {
                labelCombo->Append(point.GetDisplayValue(), new wxStringClientData(wxString::Format(
                                                                L"%g", point.GetValue())));
                }
            }
        sizer->Add(labelCombo, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start:")), wxSizerFlags{}.CenterVertical());
        auto* startSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                               wxDefaultSize, wxSP_ARROW_KEYS, -1e9, 1e9, 0, 0.1);
        sizer->Add(startSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End:")), wxSizerFlags{}.CenterVertical());
        auto* endSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                             wxDefaultSize, wxSP_ARROW_KEYS, -1e9, 1e9, 0, 0.1);
        sizer->Add(endSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")), wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(&dlg, wxID_ANY, *wxBLACK);
        sizer->Add(colorPicker, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Width:")), wxSizerFlags{}.CenterVertical());
        auto* widthSpin = new wxSpinCtrl(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, wxSP_ARROW_KEYS, 1, 5, 2);
        sizer->Add(widthSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")), wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Lines"));
        styleChoice->Append(_(L"Arrow"));
        styleChoice->Append(_(L"Reverse arrow"));
        styleChoice->Append(_(L"No connection lines"));
        styleChoice->Append(_(L"Curly braces"));
        styleChoice->SetSelection(4);
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        labelCombo->Bind(wxEVT_COMBOBOX,
                         [labelCombo, startSpin, endSpin](wxCommandEvent&)
                         {
                             const int sel = labelCombo->GetSelection();
                             if (sel != wxNOT_FOUND)
                                 {
                                 const auto* data = dynamic_cast<wxStringClientData*>(
                                     labelCombo->GetClientObject(sel));
                                 if (data != nullptr)
                                     {
                                     double val = 0;
                                     if (data->GetData().ToDouble(&val))
                                         {
                                         startSpin->SetValue(val);
                                         endSpin->SetValue(val);
                                         }
                                     }
                                 }
                         });

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(mainSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const wxString labelText = labelCombo->GetValue();
        if (labelText.empty())
            {
            return;
            }

        constexpr BracketLineStyle bracketStyles[] = {
            BracketLineStyle::Lines, BracketLineStyle::Arrow, BracketLineStyle::ReverseArrow,
            BracketLineStyle::NoConnectionLines, BracketLineStyle::CurlyBraces
        };
        const auto bracketStyle =
            (styleChoice->GetSelection() >= 0 && styleChoice->GetSelection() < 5) ?
                bracketStyles[styleChoice->GetSelection()] :
                BracketLineStyle::CurlyBraces;

        const double startPos = startSpin->GetValue();
        const double endPos = endSpin->GetValue();
        const double labelPos = (startPos + endPos) / 2.0;

        GraphItems::Axis::AxisBracket bracket(
            startPos, endPos, labelPos, labelText,
            wxPen(colorPicker->GetColour(), widthSpin->GetValue()), bracketStyle);
        axis.AddBracket(std::move(bracket));
        RefreshBracketList();
        }

    //-------------------------------------------
    void AxisOptionsPanel::OnEditBracket()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0 || m_bracketListBox == nullptr)
            {
            return;
            }
        auto& axis = m_savedAxes.at(m_currentAxisType);
        auto& brackets = axis.GetBrackets();

        const long sel = m_bracketListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                      wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, brackets.size()))
            {
            return;
            }
        auto& bracket = brackets[sel];

        wxDialog dlg(this, wxID_ANY, _(L"Edit Bracket"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                          wxSizerFlags::GetDefaultBorder());
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")), wxSizerFlags{}.CenterVertical());
        auto* labelCtrl = new wxTextCtrl(&dlg, wxID_ANY, bracket.GetLabel().GetText());
        sizer->Add(labelCtrl, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start:")), wxSizerFlags{}.CenterVertical());
        auto* startSpin =
            new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
                                 wxSP_ARROW_KEYS, -1e9, 1e9, bracket.GetStartPosition(), 0.1);
        sizer->Add(startSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End:")), wxSizerFlags{}.CenterVertical());
        auto* endSpin =
            new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
                                 wxSP_ARROW_KEYS, -1e9, 1e9, bracket.GetEndPosition(), 0.1);
        sizer->Add(endSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")), wxSizerFlags{}.CenterVertical());
        auto* colorPicker =
            new wxColourPickerCtrl(&dlg, wxID_ANY, bracket.GetLinePen().GetColour());
        sizer->Add(colorPicker, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Width:")), wxSizerFlags{}.CenterVertical());
        auto* widthSpin =
            new wxSpinCtrl(&dlg, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 1, 5, bracket.GetLinePen().GetWidth());
        sizer->Add(widthSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")), wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Lines"));
        styleChoice->Append(_(L"Arrow"));
        styleChoice->Append(_(L"Reverse arrow"));
        styleChoice->Append(_(L"No connection lines"));
        styleChoice->Append(_(L"Curly braces"));
        const int currentStyle = [&bracket]()
        {
            switch (bracket.GetBracketLineStyle())
                {
            case BracketLineStyle::Lines:
                return 0;
            case BracketLineStyle::Arrow:
                return 1;
            case BracketLineStyle::ReverseArrow:
                return 2;
            case BracketLineStyle::NoConnectionLines:
                return 3;
            case BracketLineStyle::CurlyBraces:
                [[fallthrough]];
            default:
                return 4;
                }
        }();
        styleChoice->SetSelection(currentStyle);
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(mainSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        constexpr BracketLineStyle bracketStyles[] = {
            BracketLineStyle::Lines, BracketLineStyle::Arrow, BracketLineStyle::ReverseArrow,
            BracketLineStyle::NoConnectionLines, BracketLineStyle::CurlyBraces
        };
        const auto bracketStyle =
            (styleChoice->GetSelection() >= 0 && styleChoice->GetSelection() < 5) ?
                bracketStyles[styleChoice->GetSelection()] :
                BracketLineStyle::CurlyBraces;

        const double startPos = startSpin->GetValue();
        const double endPos = endSpin->GetValue();
        const double labelPos = (startPos + endPos) / 2.0;

        brackets[sel] = GraphItems::Axis::AxisBracket(
            startPos, endPos, labelPos, labelCtrl->GetValue(),
            wxPen(colorPicker->GetColour(), widthSpin->GetValue()), bracketStyle);
        RefreshBracketList();
        }

    //-------------------------------------------
    void AxisOptionsPanel::OnRemoveBracket()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0 || m_bracketListBox == nullptr)
            {
            return;
            }
        auto& brackets = m_savedAxes.at(m_currentAxisType).GetBrackets();

        const long sel = m_bracketListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                      wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, brackets.size()))
            {
            return;
            }
        brackets.erase(brackets.begin() + sel);
        RefreshBracketList();
        }

    //-------------------------------------------
    void AxisOptionsPanel::OnAddBracketsFromDataset()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0 || m_reportBuilder == nullptr)
            {
            return;
            }
        auto& axis = m_savedAxes.at(m_currentAxisType);
        const auto& datasets = m_reportBuilder->GetDatasets();
        if (datasets.empty())
            {
            wxMessageBox(_(L"No datasets are available."), _(L"No Datasets"), wxOK | wxICON_WARNING,
                         this);
            return;
            }

        // determine the effective hints: prefer whatever is already stored on the axis
        // (set either by JSON load or by a previous "Add from dataset" round) and fall
        // back to the externally-supplied panel hints for fresh inserts
        const wxString axisDatasetTmpl = axis.GetPropertyTemplate(L"brackets.dataset");
        const wxString axisLabelTmpl = axis.GetPropertyTemplate(L"bracket.label");
        const wxString axisValueTmpl = axis.GetPropertyTemplate(L"bracket.value");
        const wxString effectiveDatasetHint =
            !axisDatasetTmpl.empty() ? axisDatasetTmpl : m_bracketDatasetHint;
        const wxString effectiveLabelHint =
            !axisLabelTmpl.empty() ? axisLabelTmpl : m_bracketLabelColumnHint;
        const wxString effectiveValueHint =
            !axisValueTmpl.empty() ? axisValueTmpl : m_bracketValueColumnHint;

        wxDialog dlg(this, wxID_ANY, _(L"Add Brackets from Dataset"), wxDefaultPosition,
                     wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                          wxSizerFlags::GetDefaultBorder());
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Dataset:")),
                   wxSizerFlags{}.CenterVertical());
        auto* datasetChoice = new wxChoice(&dlg, wxID_ANY);
        wxArrayString datasetNames;
        for (const auto& [name, dataset] : datasets)
            {
            datasetNames.Add(name);
            datasetChoice->Append(name);
            }
        int datasetHintIdx = 0;
        if (!effectiveDatasetHint.empty())
            {
            const int found = datasetChoice->FindString(effectiveDatasetHint);
            if (found != wxNOT_FOUND)
                {
                datasetHintIdx = found;
                }
            }
        datasetChoice->SetSelection(datasetHintIdx);
        sizer->Add(datasetChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label column:")),
                   wxSizerFlags{}.CenterVertical());
        auto* labelColChoice = new wxChoice(&dlg, wxID_ANY);
        sizer->Add(labelColChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Value column:")),
                   wxSizerFlags{}.CenterVertical());
        auto* valueColChoice = new wxChoice(&dlg, wxID_ANY);
        sizer->Add(valueColChoice, wxSizerFlags{}.Expand());

        const auto populateColumns = [&datasets, &datasetNames, labelColChoice, valueColChoice,
                                      &effectiveLabelHint, &effectiveValueHint](const int sel)
        {
            labelColChoice->Clear();
            valueColChoice->Clear();
            if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, datasetNames.size()))
                {
                return;
                }
            const auto it = datasets.find(datasetNames[sel]);
            if (it == datasets.cend() || it->second == nullptr)
                {
                return;
                }
            const auto& ds = *(it->second);
            for (const auto& col : ds.GetCategoricalColumnNames())
                {
                labelColChoice->Append(col);
                valueColChoice->Append(col);
                }
            for (const auto& col : ds.GetContinuousColumnNames())
                {
                valueColChoice->Append(col);
                }
            for (const auto& col : ds.GetDateColumnNames())
                {
                valueColChoice->Append(col);
                }

            if (!effectiveLabelHint.empty())
                {
                const int idx = labelColChoice->FindString(effectiveLabelHint);
                if (idx != wxNOT_FOUND)
                    {
                    labelColChoice->SetSelection(idx);
                    }
                else if (labelColChoice->GetCount() > 0)
                    {
                    labelColChoice->SetSelection(0);
                    }
                }
            else if (labelColChoice->GetCount() > 0)
                {
                labelColChoice->SetSelection(0);
                }

            if (!effectiveValueHint.empty())
                {
                const int idx = valueColChoice->FindString(effectiveValueHint);
                if (idx != wxNOT_FOUND)
                    {
                    valueColChoice->SetSelection(idx);
                    }
                else if (valueColChoice->GetCount() > 0)
                    {
                    valueColChoice->SetSelection(0);
                    }
                }
            else if (valueColChoice->GetCount() > 0)
                {
                valueColChoice->SetSelection(0);
                }
        };

        populateColumns(datasetHintIdx);
        datasetChoice->Bind(wxEVT_CHOICE, [&populateColumns, datasetChoice](wxCommandEvent&)
                            { populateColumns(datasetChoice->GetSelection()); });

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")), wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Lines"));
        styleChoice->Append(_(L"Arrow"));
        styleChoice->Append(_(L"Reverse arrow"));
        styleChoice->Append(_(L"No connection lines"));
        styleChoice->Append(_(L"Curly braces"));
        styleChoice->SetSelection(4);
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")), wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(&dlg, wxID_ANY, *wxBLACK);
        sizer->Add(colorPicker, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Width:")), wxSizerFlags{}.CenterVertical());
        auto* widthSpin = new wxSpinCtrl(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, wxSP_ARROW_KEYS, 1, 5, 2);
        sizer->Add(widthSpin, wxSizerFlags{}.Expand());

        auto* simplifyCheck = new wxCheckBox(&dlg, wxID_ANY, _(L"Simplify brackets"));
        simplifyCheck->SetValue(axis.AreBracketsSimplified());

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(simplifyCheck, wxSizerFlags{}.Border());
        mainSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(mainSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const int dsSel = datasetChoice->GetSelection();
        if (dsSel == wxNOT_FOUND || std::cmp_greater_equal(dsSel, datasetNames.size()))
            {
            return;
            }
        const auto dsIt = datasets.find(datasetNames[dsSel]);
        if (dsIt == datasets.cend() || dsIt->second == nullptr)
            {
            return;
            }

        const wxString labelCol = labelColChoice->GetStringSelection();
        const wxString valueCol = valueColChoice->GetStringSelection();
        if (labelCol.empty() || valueCol.empty())
            {
            return;
            }

        axis.ClearBrackets();
        // map category labels to their sorted positions so brackets use the
        // actual axis positions instead of raw category IDs. Needed for
        // common axes, whose children may have sorted or dropped labels.
        std::map<wxString, double> labelPosMap;
            {
            const auto& axisLabels = axis.GetCustomLabels();
            for (const auto& [pos, label] : axisLabels)
                {
                labelPosMap[label.GetText()] = pos;
                }
            }
        if (!labelPosMap.empty())
            {
            axis.AddBrackets(dsIt->second, labelCol, valueCol, labelPosMap);
            }
        else
            {
            axis.AddBrackets(dsIt->second, labelCol, valueCol);
            }

        // store property templates so the bracket source round-trips through save/load
        axis.SetPropertyTemplate(L"brackets.dataset", datasetNames[dsSel]);
        axis.SetPropertyTemplate(L"bracket.label", labelCol);
        axis.SetPropertyTemplate(L"bracket.value", valueCol);

        constexpr BracketLineStyle bracketStyles[] = {
            BracketLineStyle::Lines, BracketLineStyle::Arrow, BracketLineStyle::ReverseArrow,
            BracketLineStyle::NoConnectionLines, BracketLineStyle::CurlyBraces
        };
        const auto bracketStyle =
            (styleChoice->GetSelection() >= 0 && styleChoice->GetSelection() < 5) ?
                bracketStyles[styleChoice->GetSelection()] :
                BracketLineStyle::CurlyBraces;
        const wxPen pen(colorPicker->GetColour(), widthSpin->GetValue());
        for (auto& bracket : axis.GetBrackets())
            {
            bracket.SetBracketLineStyle(bracketStyle);
            bracket.GetLinePen() = pen;
            }

        if (simplifyCheck->GetValue())
            {
            axis.SimplifyBrackets();
            }

        RefreshBracketList();
        }

    } // namespace Wisteria::UI
