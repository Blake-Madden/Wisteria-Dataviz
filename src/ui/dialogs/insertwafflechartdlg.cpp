///////////////////////////////////////////////////////////////////////////////
// Name:        insertwafflechartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertwafflechartdlg.h"
#include "../../app/wisteriaapp.h"
#include "../../base/reportenumconvert.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertWaffleChartDlg::InsertWaffleChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                               wxWindow* parent, const wxString& caption,
                                               const wxWindowID id, const wxPoint& pos,
                                               const wxSize& size, const long style,
                                               EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertWaffleChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Waffle Chart Options"), ID_OPTIONS_SECTION,
                                  true);

        // shapes list
        m_shapeListBox = new wxEditableListBox(
            optionsPage, wxID_ANY, _(L"Shapes:"), wxDefaultPosition,
            wxSize{ FromDIP(400), FromDIP(150) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        optionsSizer->Add(m_shapeListBox, wxSizerFlags{ 1 }.Expand().Border());

        m_shapeListBox->Bind(wxEVT_LIST_BEGIN_LABEL_EDIT, [](wxListEvent& event) { event.Veto(); });
        m_shapeListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED, &InsertWaffleChartDlg::OnEditShape, this);
        m_shapeListBox->Bind(wxEVT_LIST_KEY_DOWN,
                             [this](wxListEvent& evt)
                             {
                                 if (evt.GetKeyCode() == WXK_DELETE ||
                                     evt.GetKeyCode() == WXK_NUMPAD_DELETE ||
                                     evt.GetKeyCode() == WXK_BACK)
                                     {
                                     wxCommandEvent dummy;
                                     OnRemoveShape(dummy);
                                     }
                                 else
                                     {
                                     evt.Skip();
                                     }
                             });

        // override New to open the shape sub-dialog
        m_shapeListBox->GetNewButton()->Bind(wxEVT_BUTTON, &InsertWaffleChartDlg::OnAddShape, this);
        m_shapeListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"shape.svg", wxSize{ 16, 16 }));

        // override Edit to open the shape sub-dialog for the selected item
        m_shapeListBox->GetEditButton()->Bind(wxEVT_BUTTON, &InsertWaffleChartDlg::OnEditShape,
                                              this);

        // override Delete to remove the selected shape from our vector
        m_shapeListBox->GetDelButton()->Bind(wxEVT_BUTTON, &InsertWaffleChartDlg::OnRemoveShape,
                                             this);

        // grid rounding
        auto* gridRoundBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Grid Rounding"));

        auto* gridRoundCheck = new wxCheckBox(
            gridRoundBox->GetStaticBox(), wxID_ANY, _(L"Round grid to a fixed cell count"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&m_useGridRounding));
        gridRoundBox->Add(gridRoundCheck, wxSizerFlags{}.Border());

        auto* gridRoundGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        gridRoundGrid->Add(
            new wxStaticText(gridRoundBox->GetStaticBox(), wxID_ANY, _(L"Cell count:")),
            wxSizerFlags{}.CenterVertical());
        m_gridRoundCellsSpin = new wxSpinCtrl(gridRoundBox->GetStaticBox(), wxID_ANY);
        m_gridRoundCellsSpin->SetRange(1, 10'000);
        m_gridRoundCellsSpin->SetValue(100);
        gridRoundGrid->Add(m_gridRoundCellsSpin);

        gridRoundGrid->Add(
            new wxStaticText(gridRoundBox->GetStaticBox(), wxID_ANY, _(L"Shape index:")),
            wxSizerFlags{}.CenterVertical());
        m_gridRoundIndexSpin = new wxSpinCtrl(gridRoundBox->GetStaticBox(), wxID_ANY);
        m_gridRoundIndexSpin->SetRange(0, 100);
        m_gridRoundIndexSpin->SetValue(0);
        gridRoundGrid->Add(m_gridRoundIndexSpin);

        gridRoundBox->Add(gridRoundGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(gridRoundBox, wxSizerFlags{}.Border());

        // start grid rounding controls disabled
        m_gridRoundCellsSpin->Enable(false);
        m_gridRoundIndexSpin->Enable(false);
        gridRoundCheck->Bind(wxEVT_CHECKBOX,
                             [this](wxCommandEvent& evt)
                             {
                                 m_gridRoundCellsSpin->Enable(evt.IsChecked());
                                 m_gridRoundIndexSpin->Enable(evt.IsChecked());
                             });

        // row count
        auto* rowCountBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Row Count"));

        auto* rowCountCheck =
            new wxCheckBox(rowCountBox->GetStaticBox(), wxID_ANY, _(L"Specify row count"),
                           wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&m_useRowCount));
        rowCountBox->Add(rowCountCheck, wxSizerFlags{}.Border());

        auto* rowCountGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        rowCountGrid->Add(new wxStaticText(rowCountBox->GetStaticBox(), wxID_ANY, _(L"Rows:")),
                          wxSizerFlags{}.CenterVertical());
        m_rowCountSpin = new wxSpinCtrl(rowCountBox->GetStaticBox(), wxID_ANY);
        m_rowCountSpin->SetRange(1, 1'000);
        m_rowCountSpin->SetValue(10);
        rowCountGrid->Add(m_rowCountSpin);

        rowCountBox->Add(rowCountGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(rowCountBox, wxSizerFlags{}.Border());

        // start row count controls disabled
        m_rowCountSpin->Enable(false);
        rowCountCheck->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt)
                            { m_rowCountSpin->Enable(evt.IsChecked()); });

        // legend placement
        auto* legendGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        legendGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                        wxSizerFlags{}.CenterVertical());
        legendGrid->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendGrid, wxSizerFlags{}.Border());
        }

    //-------------------------------------------
    void InsertWaffleChartDlg::BuildShapeFromDlg(const InsertShapeDlg& dlg,
                                                 GraphItems::ShapeInfo& shapeInfo)
        {
        shapeInfo.Shape(dlg.GetIconShape())
            .Pen(wxPen{ dlg.GetPenColor(), dlg.GetPenWidth(), dlg.GetPenStyle() })
            .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
            .Text(dlg.GetLabelText());

        // repeat: parse as number, but preserve the raw string as a property template
        const auto repeatStr = dlg.GetRepeatString();
        long repeatVal{ 1 };
        if (repeatStr.ToLong(&repeatVal) && repeatVal >= 1)
            {
            shapeInfo.Repeat(static_cast<size_t>(repeatVal));
            shapeInfo.SetPropertyTemplate(L"repeat", wxString{});
            }
        else
            {
            // non-numeric (constant reference) — store raw, default repeat to 1
            shapeInfo.Repeat(1);
            shapeInfo.SetPropertyTemplate(L"repeat", repeatStr);
            }

        if (dlg.IsFillable())
            {
            shapeInfo.FillPercent(dlg.GetFillPercent());
            }
        else
            {
            shapeInfo.FillPercent(math_constants::full);
            }
        }

    //-------------------------------------------
    void InsertWaffleChartDlg::OnAddShape([[maybe_unused]] wxCommandEvent& event)
        {
        InsertShapeDlg dlg(GetCanvas(), nullptr, this, _(L"Add Shape"), wxID_ANY, wxDefaultPosition,
                           wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           InsertItemDlg::EditMode::Insert,
                           ShapeDlgIncludePen | ShapeDlgIncludeBrush | ShapeDlgIncludeLabel |
                               ShapeDlgIncludeFillable | ShapeDlgIncludeRepeat);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        GraphItems::ShapeInfo shapeInfo;
        BuildShapeFromDlg(dlg, shapeInfo);

        m_shapes.push_back(shapeInfo);
        RefreshShapeList();
        }

    //-------------------------------------------
    void InsertWaffleChartDlg::OnEditShape([[maybe_unused]] wxCommandEvent& event)
        {
        const auto sel =
            m_shapeListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_shapes.size()))
            {
            return;
            }

        InsertShapeDlg dlg(GetCanvas(), nullptr, this, _(L"Edit Shape"), wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           InsertItemDlg::EditMode::Edit,
                           ShapeDlgIncludePen | ShapeDlgIncludeBrush | ShapeDlgIncludeLabel |
                               ShapeDlgIncludeFillable | ShapeDlgIncludeRepeat);

        dlg.LoadFromShapeInfo(m_shapes[static_cast<size_t>(sel)]);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        auto& shapeInfo = m_shapes[static_cast<size_t>(sel)];
        BuildShapeFromDlg(dlg, shapeInfo);

        RefreshShapeList();
        }

    //-------------------------------------------
    void InsertWaffleChartDlg::OnRemoveShape([[maybe_unused]] wxCommandEvent& event)
        {
        const auto sel =
            m_shapeListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_shapes.size()))
            {
            return;
            }

        if (wxMessageBox(_(L"Delete selected shape?"), _(L"Delete Shape"),
                         wxYES_NO | wxICON_QUESTION, this) != wxYES)
            {
            return;
            }

        m_shapes.erase(m_shapes.begin() + sel);
        RefreshShapeList();
        }

    //-------------------------------------------
    void InsertWaffleChartDlg::RefreshShapeList()
        {
        wxArrayString strings;
        strings.reserve(m_shapes.size());
        for (const auto& shp : m_shapes)
            {
            strings.Add(FormatShapeDescription(shp));
            }
        m_shapeListBox->SetStrings(strings);
        }

    //-------------------------------------------
    wxString
    InsertWaffleChartDlg::FormatShapeDescription(const GraphItems::ShapeInfo& shapeInfo) const
        {
        const auto iconStr = ReportEnumConvert::ConvertIconToString(shapeInfo.GetShape());
        const auto shapeName = iconStr.has_value() ? iconStr.value() : wxString{ L"square" };

        const auto repeatTmpl = shapeInfo.GetPropertyTemplate(L"repeat");
        const auto repeatStr =
            repeatTmpl.empty() ? std::to_wstring(shapeInfo.GetRepeatCount()) : repeatTmpl;

        wxString desc = wxString::Format(L"%s × %s", shapeName, repeatStr);
        if (!shapeInfo.GetText().empty())
            {
            desc += L" [" + shapeInfo.GetText() + L"]";
            }
        if (shapeInfo.GetFillPercent() < math_constants::full)
            {
            // TRANSLATORS: %s is the shape description (e.g., "square × 5"),
            // %.0f is the fill percentage; %% is a literal percent sign.
            // Some locales may reorder or place the percent sign differently.
            desc = wxString::Format(_(L"%s (%.0f%%)"), desc, shapeInfo.GetFillPercent() * 100.0);
            }
        return desc;
        }

    //-------------------------------------------
    bool InsertWaffleChartDlg::Validate()
        {
        if (m_shapes.empty())
            {
            wxMessageBox(_(L"Please add at least one shape."), _(L"Validation"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }
        return true;
        }

    //-------------------------------------------
    std::optional<Graphs::WaffleChart::GridRounding>
    InsertWaffleChartDlg::GetGridRounding() const noexcept
        {
        if (!m_useGridRounding)
            {
            return std::nullopt;
            }
        return Graphs::WaffleChart::GridRounding{
            static_cast<size_t>(m_gridRoundCellsSpin->GetValue()),
            static_cast<size_t>(m_gridRoundIndexSpin->GetValue())
        };
        }

    //-------------------------------------------
    std::optional<size_t> InsertWaffleChartDlg::GetRowCount() const noexcept
        {
        if (!m_useRowCount)
            {
            return std::nullopt;
            }
        return static_cast<size_t>(m_rowCountSpin->GetValue());
        }

    //-------------------------------------------
    void InsertWaffleChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        LoadGraphOptions(graph);

        const auto* waffle = dynamic_cast<const Graphs::WaffleChart*>(&graph);
        if (waffle == nullptr)
            {
            return;
            }

        // shapes
        m_shapes = waffle->GetShapes();
        RefreshShapeList();

        // grid rounding
        if (waffle->GetGridRounding().has_value())
            {
            m_useGridRounding = true;
            const auto& gridRound = waffle->GetGridRounding().value();
            m_gridRoundCellsSpin->SetValue(static_cast<int>(gridRound.m_numberOfCells));
            m_gridRoundIndexSpin->SetValue(static_cast<int>(gridRound.m_shapesIndex));
            m_gridRoundCellsSpin->Enable(true);
            m_gridRoundIndexSpin->Enable(true);
            }

        // row count
        if (waffle->GetRowCount().has_value())
            {
            m_useRowCount = true;
            m_rowCountSpin->SetValue(static_cast<int>(waffle->GetRowCount().value()));
            m_rowCountSpin->Enable(true);
            }

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
