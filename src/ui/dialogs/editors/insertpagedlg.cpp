///////////////////////////////////////////////////////////////////////////////
// Name:        insertpagedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertpagedlg.h"
#include "../../app/wisteriaapp.h"
#include "../../app/wisteriadoc.h"
#include "../../app/wisteriaview.h"
#include "../../graphs/graph2d.h"
#include "insert_bullet_chart_dlg.h"
#include "insert_nightingale_rose_chart_dlg.h"
#include "insert_waterfallchart_dlg.h"
#include "insertboxplotdlg.h"
#include "insertbubbleplotdlg.h"
#include "insertcandlestickplotdlg.h"
#include "insertcatbarchartdlg.h"
#include "insertchernoffdlg.h"
#include "insertchoroplethmapdlg.h"
#include "insertcommonaxisdlg.h"
#include "insertganttchartdlg.h"
#include "insertheatmapdlg.h"
#include "inserthistogramdlg.h"
#include "insertimgdlg.h"
#include "insertkpicarddlg.h"
#include "insertlabeldlg.h"
#include "insertlikertdlg.h"
#include "insertlineplotdlg.h"
#include "insertlrroadmapdlg.h"
#include "insertmultiserieslineplotdlg.h"
#include "insertpiechartdlg.h"
#include "insertproconroadmapdlg.h"
#include "insertracetrackchartdlg.h"
#include "insertsankeydiagramdlg.h"
#include "insertscalechartdlg.h"
#include "insertscatterplotdlg.h"
#include "insertshapedlg.h"
#include "insertstemandleafdlg.h"
#include "inserttabledlg.h"
#include "insertwafflechartdlg.h"
#include "insertwcurvedlg.h"
#include "insertwilmarthbridgeplotdlg.h"
#include "insertwlsparklinedlg.h"
#include "insertwordclouddlg.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertPageDlg::InsertPageDlg(Canvas* canvas, const wxArrayString& pageNames,
                                 const Wisteria::ReportBuilder* reportBuilder, WisteriaDoc* doc,
                                 wxWindow* parent, const wxWindowID id, const wxString& caption,
                                 const wxPoint& pos, const wxSize& size, const long style,
                                 EditMode editMode)
        : DialogWithHelp(parent, id, caption, pos, size, style), m_canvas(canvas),
          m_reportBuilder(reportBuilder), m_doc(doc), m_editMode(editMode), m_pageNames(pageNames)
        {
        if (!m_pageNames.empty())
            {
            m_relativePageIndex = pageNames.size() - 1;
            }
        if (m_canvas != nullptr && m_editMode == EditMode::Edit)
            {
            const auto& watermark = m_canvas->GetWatermark();
            m_watermarkLabel = watermark.m_label;
            if (watermark.m_color.IsOk())
                {
                m_watermarkColor = watermark.m_color;
                }
            const auto& bgColor = m_canvas->GetBackgroundColor();
            if (bgColor.IsOk())
                {
                m_backgroundColor = bgColor;
                }
            m_backgroundImageOpacity = m_canvas->GetBackgroundImageOpacity();
            m_resetPageNumbering = m_canvas->IsResettingPageNumbering();
            m_layer = m_canvas->GetLayer();
            }
        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //-------------------------------------------
    void InsertPageDlg::SelectCell(size_t row, size_t column)
        {
        if (std::cmp_less(row, m_rowCount) && std::cmp_less(column, m_columnCount))
            {
            m_selectedRow = row;
            m_selectedColumn = column;
            if (m_previewPanel != nullptr)
                {
                m_previewPanel->Refresh();
                }
            }
        }

    //-------------------------------------------
    void InsertPageDlg::ResizeFixedObjectsGrid()
        {
        m_fixedObjectsGrid.resize(static_cast<size_t>(m_rowCount));
        for (auto& row : m_fixedObjectsGrid)
            {
            row.resize(static_cast<size_t>(m_columnCount), nullptr);
            }
        // grid dimensions changed, so the cell size (and thus the fixed icon size) is stale
        m_iconPixelSize = 0;
        }

    //-------------------------------------------
    std::pair<size_t, size_t> InsertPageDlg::CellFromPoint(const wxPoint& pt) const
        {
        const auto clientRect = m_previewPanel->GetClientRect();
        const auto cellWidth = safe_divide<double>(clientRect.GetWidth(), m_columnCount);
        const auto cellHeight = safe_divide<double>(clientRect.GetHeight(), m_rowCount);
        if (cellWidth <= 0 || cellHeight <= 0)
            {
            return { 0, 0 };
            }
        const auto col = safe_divide<size_t>(pt.x - clientRect.x, cellWidth);
        const auto row = safe_divide<size_t>(pt.y - clientRect.y, cellHeight);
        return { std::min(row, static_cast<size_t>(m_rowCount - 1)),
                 std::min(col, static_cast<size_t>(m_columnCount - 1)) };
        }

    //-------------------------------------------
    void InsertPageDlg::ApplyGridEdits(Canvas* canvas) const
        {
        if (canvas == nullptr)
            {
            return;
            }
        const auto [rows, columns] = canvas->GetFixedObjectsGridSize();
        for (size_t row = 0; row < m_fixedObjectsGrid.size() && row < rows; ++row)
            {
            for (size_t col = 0; col < m_fixedObjectsGrid[row].size() && col < columns; ++col)
                {
                canvas->SetFixedObject(row, col, m_fixedObjectsGrid[row][col]);
                }
            }
        }

    //-------------------------------------------
    void InsertPageDlg::PaintPreview([[maybe_unused]] wxPaintEvent& event)
        {
        wxAutoBufferedPaintDC pdc(m_previewPanel);
        pdc.SetBackground(*wxWHITE_BRUSH);
        pdc.Clear();

#ifdef __WXMSW__
        wxGraphicsContext* context{ nullptr };
        auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
        if (renderer != nullptr)
            {
            context = renderer->CreateContext(pdc);
            }

        if (context != nullptr)
            {
            wxGCDC dc(context);
            DrawPreview(dc);
            }
        else
            {
            wxGCDC dc(pdc);
            DrawPreview(dc);
            }
#else
        wxGCDC dc(pdc);
        DrawPreview(dc);
#endif
        }

    //-------------------------------------------
    void InsertPageDlg::DrawPreview(wxGCDC& dc)
        {
        const auto clientRect = m_previewPanel->GetClientRect();
        if (clientRect.IsEmpty())
            {
            return;
            }

        const auto cellWidth = safe_divide<double>(clientRect.GetWidth(), m_columnCount);
        const auto cellHeight = safe_divide<double>(clientRect.GetHeight(), m_rowCount);

        // fix the icon size to the grid's initial cell size so that resizing the
        // dialog repositions (but doesn't upscale) the icons
        if (m_iconPixelSize <= 0)
            {
            m_iconPixelSize = std::max(1, static_cast<int>(std::min(cellWidth, cellHeight) *
                                                           math_constants::three_fourths));
            }

        const wxBrush occupiedBrush(wxColour{ 220, 220, 220 });

        for (size_t row = 0; std::cmp_less(row, m_rowCount); ++row)
            {
            for (size_t col = 0; std::cmp_less(col, m_columnCount); ++col)
                {
                const auto cellLeft = static_cast<int>(col * cellWidth);
                const auto cellTop = static_cast<int>(row * cellHeight);
                const auto cellW = static_cast<int>((col + 1) * cellWidth) - cellLeft;
                const auto cellH = static_cast<int>((row + 1) * cellHeight) - cellTop;

                const auto item =
                    (row < m_fixedObjectsGrid.size() && col < m_fixedObjectsGrid[row].size()) ?
                        m_fixedObjectsGrid[row][col] :
                        nullptr;
                const bool isOccupied = (item != nullptr);

                dc.SetPen(*wxBLACK_PEN);
                dc.SetBrush(isOccupied ? occupiedBrush : *wxWHITE_BRUSH);

                if (row == m_selectedRow && col == m_selectedColumn)
                    {
                    dc.SetPen(wxPen{ wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT), 2,
                                     wxPENSTYLE_DOT });
                    dc.SetBrush(isOccupied ? occupiedBrush : *wxWHITE_BRUSH);
                    }
                else
                    {
                    dc.SetPen(*wxBLACK_PEN);
                    dc.SetBrush(isOccupied ? occupiedBrush : *wxWHITE_BRUSH);
                    }
                dc.DrawRectangle(clientRect.x + cellLeft, clientRect.y + cellTop, cellW, cellH);

                if (isOccupied)
                    {
                    const auto svgName = WisteriaApp::GetItemIconName(item.get());
                    if (!svgName.empty())
                        {
                        const auto bmpBundle = wxGetApp().GetResourceManager().GetSVG(svgName);
                        if (bmpBundle.IsOk())
                            {
                            const auto bmp =
                                bmpBundle.GetBitmap(wxSize(m_iconPixelSize, m_iconPixelSize));
                            const auto iconLeft =
                                clientRect.x + cellLeft + (cellW - bmp.GetWidth()) / 2;
                            const auto iconTop =
                                clientRect.y + cellTop + (cellH - bmp.GetHeight()) / 2;
                            dc.DrawBitmap(bmp, iconLeft, iconTop, true);
                            }
                        }
                    }
                else
                    {
                    const auto emptyLabel = _(L"[Empty]");
                    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Smaller());
                    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
                    const auto textExtent = dc.GetTextExtent(emptyLabel);
                    const auto textLeft =
                        clientRect.x + cellLeft + (cellW - textExtent.GetWidth()) / 2;
                    const auto textTop =
                        clientRect.y + cellTop + (cellH - textExtent.GetHeight()) / 2;
                    dc.DrawText(emptyLabel, textLeft, textTop);
                    }
                }
            }

        if (m_isDraggingItem)
            {
            const auto targetLeft = static_cast<int>(m_dragTargetColumn * cellWidth);
            const auto targetTop = static_cast<int>(m_dragTargetRow * cellHeight);
            const auto targetW =
                static_cast<int>((m_dragTargetColumn + 1) * cellWidth) - targetLeft;
            const auto targetH = static_cast<int>((m_dragTargetRow + 1) * cellHeight) - targetTop;
            const wxColour dropColor{ 0, 150, 0 };

            // highlight the drop target cell
            dc.SetPen(wxPen{ dropColor, FromDIP(4), wxPENSTYLE_SHORT_DASH });
            dc.SetBrush(wxBrush{ wxColour(0, 150, 0, 30) });
            dc.DrawRectangle(clientRect.x + targetLeft, clientRect.y + targetTop, targetW, targetH);

            // arrow pointing down at the drop target
            const auto arrowCenterX = clientRect.x + targetLeft + (targetW / 2);
            const auto arrowTop = clientRect.y + targetTop + (targetH / 4);
            const auto arrowHalfWidth = FromDIP(20);
            const auto arrowHeight = FromDIP(32);

            auto* gc = dc.GetGraphicsContext();

            wxGraphicsPath arrowPath = gc->CreatePath();
            arrowPath.MoveToPoint(arrowCenterX - arrowHalfWidth, arrowTop);
            arrowPath.AddLineToPoint(arrowCenterX + arrowHalfWidth, arrowTop);
            arrowPath.AddLineToPoint(arrowCenterX, arrowTop + arrowHeight);
            arrowPath.CloseSubpath();

            const wxColour lightSheen{ 170, 235, 170 };
            const wxColour deepShade{ 0, 90, 0 };
            gc->SetBrush(gc->CreateLinearGradientBrush(arrowCenterX, arrowTop, arrowCenterX,
                                                       arrowTop + arrowHeight, lightSheen,
                                                       deepShade));
            gc->SetPen(wxPen{ deepShade, FromDIP(1) });
            gc->DrawPath(arrowPath);

            // glossy highlight running parallel to the triangle's left edge, fading from
            // white into the arrow's own gradient rather than sitting on top of it
            const double sheenStartT{ 0.05 };
            const double sheenEndT{ 0.7 };
            const double sheenInset{ arrowHalfWidth * 0.4 };
            const double outerTopX{ (arrowCenterX - arrowHalfWidth) +
                                    arrowHalfWidth * sheenStartT };
            const double outerTopY{ arrowTop + arrowHeight * sheenStartT };
            const double outerBottomX{ (arrowCenterX - arrowHalfWidth) +
                                       arrowHalfWidth * sheenEndT };
            const double outerBottomY{ arrowTop + arrowHeight * sheenEndT };
            const double innerTopX{ outerTopX + sheenInset };
            const double innerBottomX{ outerBottomX + sheenInset };

            wxGraphicsPath sheenPath = gc->CreatePath();
            sheenPath.MoveToPoint(outerTopX, outerTopY);
            sheenPath.AddLineToPoint(outerBottomX, outerBottomY);
            sheenPath.AddLineToPoint(innerBottomX, outerBottomY);
            sheenPath.AddLineToPoint(innerTopX, outerTopY);
            sheenPath.CloseSubpath();

            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(gc->CreateLinearGradientBrush(
                (outerTopX + outerBottomX) / 2.0, (outerTopY + outerBottomY) / 2.0,
                (innerTopX + innerBottomX) / 2.0, (outerTopY + outerBottomY) / 2.0,
                wxColour(255, 255, 255, 190), wxColour(255, 255, 255, 0)));
            gc->DrawPath(sheenPath);

            // ghost of the dragged item, following the mouse cursor
            const auto& draggedItem = m_fixedObjectsGrid[m_dragSourceRow][m_dragSourceColumn];
            if (draggedItem != nullptr)
                {
                const auto svgName = WisteriaApp::GetItemIconName(draggedItem.get());
                if (!svgName.empty())
                    {
                    const auto bmpBundle = wxGetApp().GetResourceManager().GetSVG(svgName);
                    if (bmpBundle.IsOk())
                        {
                        const auto bmp =
                            bmpBundle.GetBitmap(wxSize(m_iconPixelSize, m_iconPixelSize));
                        dc.DrawBitmap(bmp, m_dragPosition.x - (bmp.GetWidth() / 2),
                                      m_dragPosition.y - (bmp.GetHeight() / 2), true);
                        }
                    }
                }
            }
        }

    //-------------------------------------------
    void InsertPageDlg::BindPreviewPanelMouseEvents()
        {
        // click handler (also arms a possible drag if the clicked cell is occupied)
        m_previewPanel->Bind(wxEVT_LEFT_DOWN,
                             [this](wxMouseEvent& evt)
                             {
                                 m_previewPanel->SetFocus();
                                 const auto clientRect = m_previewPanel->GetClientRect();
                                 if (clientRect.IsEmpty())
                                     {
                                     return;
                                     }

                                 const auto [row, col] = CellFromPoint(evt.GetPosition());
                                 SelectCell(row, col);

                                 if (row < m_fixedObjectsGrid.size() &&
                                     col < m_fixedObjectsGrid[row].size() &&
                                     m_fixedObjectsGrid[row][col] != nullptr)
                                     {
                                     m_dragSourceRow = row;
                                     m_dragSourceColumn = col;
                                     m_dragTargetRow = row;
                                     m_dragTargetColumn = col;
                                     m_dragPosition = evt.GetPosition();
                                     m_previewPanel->CaptureMouse();
                                     }
                             });

        // drag handler: updates the drop target and ghost position while the
        // mouse moves with the button held down
        m_previewPanel->Bind(wxEVT_MOTION,
                             [this](wxMouseEvent& evt)
                             {
                                 if (!m_previewPanel->HasCapture())
                                     {
                                     return;
                                     }
                                 m_isDraggingItem = true;
                                 m_dragPosition = evt.GetPosition();
                                 const auto dropCell = CellFromPoint(evt.GetPosition());
                                 m_dragTargetRow = dropCell.first;
                                 m_dragTargetColumn = dropCell.second;
                                 m_previewPanel->Refresh();
                             });

        // drop handler: moves (swaps) the dragged item into the target cell
        m_previewPanel->Bind(
            wxEVT_LEFT_UP,
            [this]([[maybe_unused]]
                   wxMouseEvent& evt)
            {
                if (m_previewPanel->HasCapture())
                    {
                    m_previewPanel->ReleaseMouse();
                    }
                if (m_isDraggingItem)
                    {
                    if ((m_dragTargetRow != m_dragSourceRow ||
                         m_dragTargetColumn != m_dragSourceColumn) &&
                        m_dragTargetRow < m_fixedObjectsGrid.size() &&
                        m_dragTargetColumn < m_fixedObjectsGrid[m_dragTargetRow].size())
                        {
                        std::swap(m_fixedObjectsGrid[m_dragSourceRow][m_dragSourceColumn],
                                  m_fixedObjectsGrid[m_dragTargetRow][m_dragTargetColumn]);
                        SelectCell(m_dragTargetRow, m_dragTargetColumn);
                        }
                    m_isDraggingItem = false;
                    m_previewPanel->Refresh();
                    }
            });

        m_previewPanel->Bind(wxEVT_MOUSE_CAPTURE_LOST,
                             [this]([[maybe_unused]] wxMouseCaptureLostEvent&)
                             {
                                 m_isDraggingItem = false;
                                 m_previewPanel->Refresh();
                             });
        }

    //-------------------------------------------
    void InsertPageDlg::CreateControls()
        {
        if (m_canvas != nullptr)
            {
            const auto [currentRows, currentCols] = m_canvas->GetFixedObjectsGridSize();
            m_rowCount = currentRows;
            m_columnCount = currentCols;
            }
        ResizeFixedObjectsGrid();
        if (m_canvas != nullptr)
            {
            for (size_t row = 0; std::cmp_less(row, m_rowCount); ++row)
                {
                for (size_t col = 0; std::cmp_less(col, m_columnCount); ++col)
                    {
                    m_fixedObjectsGrid[row][col] = m_canvas->GetFixedObject(row, col);
                    }
                }
            }

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* gridSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        gridSizer->AddGrowableCol(1, 1);

        // name
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Name:")),
                       wxSizerFlags{}.CenterVertical());
        auto* nameCtrl =
            new wxTextCtrl(this, wxID_ANY, m_pageName, wxDefaultPosition, wxDefaultSize,
                           wxTE_MULTILINE | wxTE_RICH2, wxGenericValidator{ &m_pageName });
#if wxUSE_SPELLCHECK
        nameCtrl->EnableProofCheck(wxTextProofOptions::Default().GrammarCheck());
#endif
        gridSizer->Add(nameCtrl, wxSizerFlags{}.Expand());

        // layer
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Layer:")),
                       wxSizerFlags{}.CenterVertical());
        auto* layerCtrl = new wxTextCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, wxTE_RICH2, wxGenericValidator{ &m_layer });
#if wxUSE_SPELLCHECK
        layerCtrl->EnableProofCheck(wxTextProofOptions::Default().GrammarCheck());
#endif
        gridSizer->Add(layerCtrl, wxSizerFlags{}.Expand());

        if (m_editMode == EditMode::Insert)
            {
            auto* posSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Position"));
            auto* rbBefore =
                new wxRadioButton(posSizer->GetStaticBox(), wxID_ANY, _(L"Insert before"),
                                  wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
            auto* rbAfter =
                new wxRadioButton(posSizer->GetStaticBox(), wxID_ANY, _(L"Insert after"));
            posSizer->Add(rbBefore, wxSizerFlags{}.Border());
            posSizer->Add(rbAfter, wxSizerFlags{}.Border());

            auto* comboPages =
                new wxComboBox(posSizer->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                               wxDefaultSize, m_pageNames, wxCB_DROPDOWN | wxCB_READONLY);
            comboPages->SetValidator(wxGenericValidator{ &m_relativePageIndex });
            posSizer->Add(comboPages, wxSizerFlags{}.Expand().Border());

            if (m_pageNames.empty())
                {
                rbBefore->Disable();
                rbAfter->Disable();
                comboPages->Disable();
                }
            else
                {
                rbAfter->SetValidator(wxGenericValidator{ &m_insertAfter });
                }

            gridSizer->AddSpacer(0);
            gridSizer->Add(posSizer, wxSizerFlags{}.Expand());
            }

        // rows
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Rows:")),
                       wxSizerFlags{}.CenterVertical());
        auto* rowsSpin = new wxSpinCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                        wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
        rowsSpin->SetValidator(wxGenericValidator{ &m_rowCount });
        gridSizer->Add(rowsSpin, wxSizerFlags{}.Expand());

        // columns
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Columns:")),
                       wxSizerFlags{}.CenterVertical());
        auto* columnsSpin = new wxSpinCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                           wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
        columnsSpin->SetValidator(wxGenericValidator{ &m_columnCount });
        gridSizer->Add(columnsSpin, wxSizerFlags{}.Expand());

        // left-side sizer holds the grid controls and the appearance section
        auto* leftSizer = new wxBoxSizer(wxVERTICAL);
        leftSizer->Add(gridSizer, wxSizerFlags{}.Expand().Border());

        // appearance section
        auto* appearanceBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Appearance"));
        auto* appearanceGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        appearanceGrid->AddGrowableCol(1, 1);

        // watermark label
        appearanceGrid->Add(
            new wxStaticText(appearanceBox->GetStaticBox(), wxID_STATIC, _(L"Watermark:")),
            wxSizerFlags{}.CenterVertical());
        auto* wmTextCtrl =
            new wxTextCtrl(appearanceBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_watermarkLabel });
        appearanceGrid->Add(wmTextCtrl, wxSizerFlags{}.Expand());

        // watermark color
        appearanceGrid->Add(
            new wxStaticText(appearanceBox->GetStaticBox(), wxID_STATIC, _(L"Watermark color:")),
            wxSizerFlags{}.CenterVertical());
        m_watermarkColorPicker =
            new wxColourPickerCtrl(appearanceBox->GetStaticBox(), wxID_ANY, m_watermarkColor);
        appearanceGrid->Add(m_watermarkColorPicker, wxSizerFlags{}.Expand());

        // background color
        appearanceGrid->Add(
            new wxStaticText(appearanceBox->GetStaticBox(), wxID_STATIC, _(L"Background color:")),
            wxSizerFlags{}.CenterVertical());
        m_bgColorPicker =
            new wxColourPickerCtrl(appearanceBox->GetStaticBox(), wxID_ANY, m_backgroundColor);
        appearanceGrid->Add(m_bgColorPicker, wxSizerFlags{}.Expand());

        // background image thumbnail (click or drag-and-drop to select)
        appearanceGrid->Add(
            new wxStaticText(appearanceBox->GetStaticBox(), wxID_STATIC, _(L"Background image:")),
            wxSizerFlags{}.CenterVertical());
        m_bgThumbnail = new Thumbnail(
            appearanceBox->GetStaticBox(), wxNullBitmap, ClickMode::BrowseForImageFile, true,
            wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxBORDER_SIMPLE);
        auto* bgClearBtn = new wxButton(appearanceBox->GetStaticBox(), wxID_ANY, _(L"Clear"));
        bgClearBtn->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                         { m_bgThumbnail->SetBitmap(wxNullBitmap); });
        auto* bgImgHBox = new wxBoxSizer(wxHORIZONTAL);
        bgImgHBox->Add(m_bgThumbnail, wxSizerFlags{}.Border(wxTOP | wxBOTTOM));
        bgImgHBox->Add(bgClearBtn, wxSizerFlags{}.Top().Border());
        appearanceGrid->Add(bgImgHBox, wxSizerFlags{}.Expand());

        // background image opacity
        appearanceGrid->Add(
            new wxStaticText(appearanceBox->GetStaticBox(), wxID_STATIC, _(L"Image opacity:")),
            wxSizerFlags{}.CenterVertical());
        auto* bgOpacitySizer = new wxBoxSizer(wxHORIZONTAL);
        auto* bgOpacitySlider =
            new wxSlider(appearanceBox->GetStaticBox(), wxID_ANY, m_backgroundImageOpacity, 0, 255,
                         wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
        bgOpacitySlider->SetValidator(wxGenericValidator{ &m_backgroundImageOpacity });
        m_bgOpacityLabel = new wxStaticText(
            appearanceBox->GetStaticBox(), wxID_STATIC, std::to_wstring(m_backgroundImageOpacity),
            wxDefaultPosition, FromDIP(wxSize{ 30, wxDefaultCoord }));
        bgOpacitySizer->Add(bgOpacitySlider, wxSizerFlags{ 1 }.CenterVertical());
        bgOpacitySizer->Add(m_bgOpacityLabel, wxSizerFlags{}.CenterVertical().Border(wxLEFT));
        appearanceGrid->Add(bgOpacitySizer, wxSizerFlags{}.Expand());

        appearanceBox->Add(appearanceGrid, wxSizerFlags{}.Expand().Border());

        // reset page numbering
        auto* resetPageNumCheck =
            new wxCheckBox(appearanceBox->GetStaticBox(), wxID_ANY, _(L"Reset page numbering"));
        resetPageNumCheck->SetValidator(wxGenericValidator{ &m_resetPageNumbering });
        appearanceBox->Add(resetPageNumCheck, wxSizerFlags{}.Border());

        leftSizer->Add(appearanceBox, wxSizerFlags{}.Expand().Border());
        contentSizer->Add(leftSizer, wxSizerFlags{}.Expand());

        // pre-load existing background image when editing
        if (m_canvas != nullptr && m_editMode == EditMode::Edit)
            {
            const auto bgImagePath = m_canvas->GetBackgroundImagePath();
            if (!bgImagePath.empty())
                {
                m_bgThumbnail->LoadImage(bgImagePath);
                }
            }

        // opacity slider: keep the label in sync
        bgOpacitySlider->Bind(wxEVT_SLIDER,
                              [this]([[maybe_unused]] wxCommandEvent&)
                              {
                                  TransferDataFromWindow();
                                  if (m_bgOpacityLabel != nullptr)
                                      {
                                      m_bgOpacityLabel->SetLabel(
                                          std::to_wstring(m_backgroundImageOpacity));
                                      }
                              });

        // preview panel
        const int previewWidth = FromDIP(400);
        const int previewHeight = static_cast<int>(previewWidth * math_constants::golden_ratio);
        m_previewPanel =
            new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize{ previewWidth, previewHeight },
                        wxTAB_TRAVERSAL | wxWANTS_CHARS);
        m_previewPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
        m_previewPanel->SetMinSize(wxSize{ previewWidth, previewHeight });
        contentSizer->Add(m_previewPanel, wxSizerFlags{ 1 }.Expand().Border());

        // object gallery
        const int galleryWidth = FromDIP(340);
        m_galleryPanel =
            new Wisteria::UI::ObjectGalleryCtrl(this, m_previewPanel, wxID_ANY, wxDefaultPosition,
                                                wxSize{ galleryWidth, previewHeight });
        m_galleryPanel->SetMinSize(wxSize{ galleryWidth, previewHeight });
        contentSizer->Add(m_galleryPanel, wxSizerFlags{}.Expand().Border());

        m_previewPanel->Bind(Wisteria::UI::wxEVT_OBJECTGALLERY_ITEM_DROPPED,
                             &InsertPageDlg::OnGalleryItemDropped, this);
        UpdateAxisGalleryState();

        mainSizer->Add(contentSizer, wxSizerFlags{ 1 }.Expand());

        // OK/Cancel buttons
        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);

        BindPreviewPanelMouseEvents();

        // keyboard handler (arrow keys navigate cells,
        // Tab/Shift+Tab pass through for normal focus traversal)
        m_previewPanel->Bind(
            wxEVT_KEY_DOWN,
            [this](wxKeyEvent& evt)
            {
                const auto keyCode = evt.GetKeyCode();
                if (keyCode == WXK_RIGHT)
                    {
                    if (std::cmp_less(m_selectedColumn + 1, m_columnCount))
                        {
                        SelectCell(m_selectedRow, m_selectedColumn + 1);
                        }
                    else if (std::cmp_less(m_selectedRow + 1, m_rowCount))
                        {
                        SelectCell(m_selectedRow + 1, 0);
                        }
                    }
                else if (keyCode == WXK_LEFT)
                    {
                    if (m_selectedColumn > 0)
                        {
                        SelectCell(m_selectedRow, m_selectedColumn - 1);
                        }
                    else if (m_selectedRow > 0)
                        {
                        SelectCell(m_selectedRow - 1, m_columnCount - 1);
                        }
                    }
                else if (keyCode == WXK_UP)
                    {
                    if (m_selectedRow > 0)
                        {
                        SelectCell(m_selectedRow - 1, m_selectedColumn);
                        }
                    }
                else if (keyCode == WXK_DOWN)
                    {
                    if (std::cmp_less(m_selectedRow + 1, m_rowCount))
                        {
                        SelectCell(m_selectedRow + 1, m_selectedColumn);
                        }
                    }
                if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE || keyCode == WXK_BACK)
                    {
                    if (m_selectedRow < m_fixedObjectsGrid.size() &&
                        m_selectedColumn < m_fixedObjectsGrid[m_selectedRow].size() &&
                        m_fixedObjectsGrid[m_selectedRow][m_selectedColumn] != nullptr)
                        {
                        if (wxMessageBox(wxString::Format(
                                             _(L"Are you sure you want to delete the selected %s?"),
                                             m_fixedObjectsGrid[m_selectedRow][m_selectedColumn]
                                                 ->GetClassName()),
                                         _(L"Delete Item"), wxYES_NO | wxICON_QUESTION,
                                         this) == wxYES)
                            {
                            m_fixedObjectsGrid[m_selectedRow][m_selectedColumn] = nullptr;
                            m_previewPanel->Refresh();
                            }
                        }
                    }
                else
                    {
                    evt.Skip();
                    }
            });

        // focus handlers
        m_previewPanel->Bind(wxEVT_SET_FOCUS,
                             [this]([[maybe_unused]]
                                    wxFocusEvent& evt)
                             {
                                 m_previewPanel->Refresh();
                                 evt.Skip();
                             });
        m_previewPanel->Bind(wxEVT_KILL_FOCUS,
                             [this]([[maybe_unused]]
                                    wxFocusEvent& evt)
                             {
                                 m_previewPanel->Refresh();
                                 evt.Skip();
                             });

        m_previewPanel->Bind(wxEVT_PAINT, &InsertPageDlg::PaintPreview, this);
        m_previewPanel->Bind(wxEVT_SIZE,
                             [this](wxSizeEvent& evt)
                             {
                                 m_previewPanel->Refresh();
                                 evt.Skip();
                             });

        rowsSpin->Bind(wxEVT_SPINCTRL,
                       [this]([[maybe_unused]] wxSpinEvent&)
                       {
                           TransferDataFromWindow();
                           ResizeFixedObjectsGrid();
                           m_previewPanel->Refresh();
                       });
        columnsSpin->Bind(wxEVT_SPINCTRL,
                          [this]([[maybe_unused]] wxSpinEvent&)
                          {
                              TransferDataFromWindow();
                              ResizeFixedObjectsGrid();
                              m_previewPanel->Refresh();
                          });
        }

    //-------------------------------------------
    Canvas* InsertPageDlg::CreateStagingCanvas()
        {
        auto* stagingCanvas = new Canvas(this, wxID_ANY);
        stagingCanvas->Hide();
        stagingCanvas->SetFixedObjectsGridSize(static_cast<size_t>(m_rowCount),
                                               static_cast<size_t>(m_columnCount));
        for (size_t row = 0; row < m_fixedObjectsGrid.size(); ++row)
            {
            for (size_t col = 0; col < m_fixedObjectsGrid[row].size(); ++col)
                {
                stagingCanvas->SetFixedObject(row, col, m_fixedObjectsGrid[row][col]);
                }
            }
        return stagingCanvas;
        }

    //-------------------------------------------
    void InsertPageDlg::SyncFromCanvas(Canvas* stagingCanvas)
        {
        const auto [rows, cols] = stagingCanvas->GetFixedObjectsGridSize();
        if (std::cmp_greater(rows, m_rowCount) || std::cmp_greater(cols, m_columnCount))
            {
            m_rowCount = std::max(m_rowCount, static_cast<int>(rows));
            m_columnCount = std::max(m_columnCount, static_cast<int>(cols));
            ResizeFixedObjectsGrid();
            // update row/column spin controls if new legends needed to add those to the canvas
            TransferDataToWindow();
            }

        for (size_t row = 0; row < rows; ++row)
            {
            for (size_t col = 0; col < cols; ++col)
                {
                m_fixedObjectsGrid[row][col] = stagingCanvas->GetFixedObject(row, col);
                }
            }
        }

    //-------------------------------------------
    void InsertPageDlg::UpdateAxisGalleryState()
        {
        if (m_galleryPanel == nullptr)
            {
            return;
            }

        size_t graphCount{ 0 };
        for (const auto& row : m_fixedObjectsGrid)
            {
            for (const auto& item : row)
                {
                if (item != nullptr &&
                    std::dynamic_pointer_cast<Wisteria::Graphs::Graph2D>(item) != nullptr)
                    {
                    ++graphCount;
                    }
                }
            }

        m_galleryPanel->EnableItem(Wisteria::GalleryItemType::Axis, graphCount >= 2,
                                   _(L"Requires two or more graphs already on this page"));
        }

    //-------------------------------------------
    void InsertPageDlg::OnGalleryItemDropped(Wisteria::UI::ObjectGalleryItemDroppedEvent& event)
        {
        const auto clientPos = m_previewPanel->ScreenToClient(event.GetDropScreenPosition());
        const auto [row, col] = CellFromPoint(clientPos);

        auto* stagingCanvas = CreateStagingCanvas();

        bool placed{ false };
        switch (event.GetItemType())
            {
        case Wisteria::GalleryItemType::Label:
            placed = DropLabel(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::KpiCard:
            placed = DropKpiCard(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::Spacer:
            placed = DropSpacer(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::DividerHorizontalSingle:
            placed =
                DropDivider(stagingCanvas, row, col, Wisteria::DividerType::HorizontalSingleLine);
            break;
        case Wisteria::GalleryItemType::DividerHorizontalDouble:
            placed =
                DropDivider(stagingCanvas, row, col, Wisteria::DividerType::HorizontalDoubleLine);
            break;
        case Wisteria::GalleryItemType::DividerVerticalSingle:
            placed =
                DropDivider(stagingCanvas, row, col, Wisteria::DividerType::VerticalSingleLine);
            break;
        case Wisteria::GalleryItemType::DividerVerticalDouble:
            placed =
                DropDivider(stagingCanvas, row, col, Wisteria::DividerType::VerticalDoubleLine);
            break;
        case Wisteria::GalleryItemType::Shape:
            placed = DropShape(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::Image:
            placed = DropImage(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::Axis:
            placed = DropAxis(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::Histogram:
            placed = DropHistogram(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::BoxPlot:
            placed = DropBoxPlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::StemAndLeafPlot:
            placed = DropStemAndLeaf(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::HeatMap:
            placed = DropHeatMap(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::ScatterPlot:
            placed = DropScatterPlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::BubblePlot:
            placed = DropBubblePlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::ChernoffFacesPlot:
            placed = DropChernoffFacesPlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::WilmarthBridgePlot:
            placed = DropWilmarthBridgePlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::WinLossSparkline:
            placed = DropWinLossSparkline(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::GanttChart:
            placed = DropGanttChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::CandlestickPlot:
            placed = DropCandlestickPlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::ScaleChart:
            placed = DropScaleChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::WCurvePlot:
            placed = DropWCurvePlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::LRRoadmap:
            placed = DropLRRoadmap(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::LikertChart:
            placed = DropLikertChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::WordCloud:
            placed = DropWordCloud(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::ProConRoadmap:
            placed = DropProConRoadmap(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::BarChart:
            placed = DropBarChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::PieChart:
            placed = DropPieChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::LinePlot:
            placed = DropLinePlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::MultiSeriesLinePlot:
            placed = DropMultiSeriesLinePlot(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::Table:
            placed = DropTable(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::ChoroplethMap:
            placed = DropChoroplethMap(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::SankeyDiagram:
            placed = DropSankeyDiagram(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::WaffleChart:
            placed = DropWaffleChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::RaceTrackChart:
            placed = DropRaceTrackChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::NightingaleRoseChart:
            placed = DropNightingaleRoseChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::BulletChart:
            placed = DropBulletChart(stagingCanvas, row, col);
            break;
        case Wisteria::GalleryItemType::WaterfallChart:
            placed = DropWaterfallChart(stagingCanvas, row, col);
            break;
        default:
            // remaining graph types are not wired up yet
            break;
            }

        if (placed)
            {
            SyncFromCanvas(stagingCanvas);
            SelectCell(row, col);
            UpdateAxisGalleryState();
            m_previewPanel->Refresh();
            }

        stagingCanvas->Destroy();
        }

    //-------------------------------------------
    bool InsertPageDlg::DropLabel(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertLabelDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"label.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        dlg.ApplyGridSize();
        stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                                      dlg.BuildLabel());
        return true;
        }

    //-------------------------------------------
    bool InsertPageDlg::DropKpiCard(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertKpiCardDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"kpi-card.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        dlg.ApplyGridSize();
        stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                                      dlg.BuildKpiCard());
        return true;
        }

    //-------------------------------------------
    bool InsertPageDlg::DropSpacer(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertLabelDlg dlg(stagingCanvas, m_reportBuilder, this, _(L"Insert Spacer"),
                                         wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Insert,
                                         Wisteria::UI::LabelDlgIncludePageOptions);
        WisteriaView::SetDialogIcon(dlg, L"spacer.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        dlg.ApplyGridSize();

        stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                                      Wisteria::UI::InsertLabelDlg::BuildSpacerLabel(
                                          stagingCanvas, Wisteria::SpacerType::Spacer));
        return true;
        }

    //-------------------------------------------
    bool InsertPageDlg::DropDivider(Canvas* stagingCanvas, const size_t row, const size_t col,
                                    const Wisteria::DividerType type)
        {
        const wxString iconName = (type == Wisteria::DividerType::HorizontalSingleLine) ?
                                      L"divider-horizontal-single.svg" :
                                  (type == Wisteria::DividerType::HorizontalDoubleLine) ?
                                      L"divider-horizontal-double.svg" :
                                  (type == Wisteria::DividerType::VerticalSingleLine) ?
                                      L"divider-vertical-single.svg" :
                                      L"divider-vertical-double.svg";

        auto label = Wisteria::UI::InsertLabelDlg::BuildDividerLabel(stagingCanvas, type);

        Wisteria::UI::InsertLabelDlg dlg(stagingCanvas, m_reportBuilder, this, _(L"Insert Divider"),
                                         wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                                         Wisteria::UI::InsertItemDlg::EditMode::Insert,
                                         Wisteria::UI::LabelDlgIncludePageOptions);
        WisteriaView::SetDialogIcon(dlg, iconName);
        dlg.LoadFromLabel(*label);
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        dlg.ApplyGridSize();
        dlg.ApplyPageOptions(*label);

        stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), label);
        return true;
        }

    //-------------------------------------------
    bool InsertPageDlg::DropShape(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertShapeDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"shape.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        dlg.ApplyGridSize();
        stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                                      dlg.BuildShape());
        return true;
        }

    //-------------------------------------------
    bool InsertPageDlg::DropImage(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertImageDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"image.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        dlg.ApplyGridSize();

        auto image = dlg.BuildImage(m_doc);
        if (image == nullptr)
            {
            return false;
            }

        stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), image);
        return true;
        }

    //-------------------------------------------
    bool InsertPageDlg::DropAxis(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertCommonAxisDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"axis.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        dlg.ApplyGridSize();

        auto commonAxis = dlg.BuildCommonAxis();
        if (commonAxis == nullptr)
            {
            return false;
            }

        stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                                      std::move(commonAxis));
        return true;
        }

    //-------------------------------------------
    bool InsertPageDlg::DropHistogram(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertHistogramDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"histogram.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildHistogram();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropBoxPlot(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertBoxPlotDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"boxplot.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildBoxPlot(m_doc);
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropStemAndLeaf(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertStemAndLeafDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"stem-leaf.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildStemAndLeafPlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropHeatMap(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertHeatMapDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"heatmap.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildHeatMap();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropScatterPlot(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertScatterPlotDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"scatterplot.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildScatterPlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropBubblePlot(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertBubblePlotDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"bubbleplot.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildBubblePlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropChernoffFacesPlot(Canvas* stagingCanvas, const size_t row,
                                              const size_t col)
        {
        Wisteria::UI::InsertChernoffDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"chernoffplot.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildChernoffFacesPlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            auto legend = WisteriaView::BuildLegend(
                dlg, legendPlacement,
                [&plot, &dlg](const Wisteria::Graphs::LegendOptions& options)
                    -> std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
                {
                    return dlg.GetUseEnhancedLegend() ?
                               std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                   plot->CreateEnhancedLegend(options)) :
                               std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                   plot->CreateLegend(options));
                });
            WisteriaView::PlaceGraphAndLegendInGrid(stagingCanvas, plot, std::move(legend),
                                                    dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                                                    legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropWilmarthBridgePlot(Canvas* stagingCanvas, const size_t row,
                                               const size_t col)
        {
        Wisteria::UI::InsertWilmarthBridgePlotDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"wilmarth-bridge.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildWilmarthBridgePlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropWinLossSparkline(Canvas* stagingCanvas, const size_t row,
                                             const size_t col)
        {
        Wisteria::UI::InsertWLSparklineDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"sparkline.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildWinLossSparkline();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropGanttChart(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertGanttChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"gantt.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildGanttChart();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropCandlestickPlot(Canvas* stagingCanvas, const size_t row,
                                            const size_t col)
        {
        Wisteria::UI::InsertCandlestickPlotDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"candlestick.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildCandlestickPlot();
            stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropScaleChart(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertScaleChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"scale.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildScaleChart();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropWCurvePlot(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertWCurveDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"wcurve.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildWCurvePlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropLRRoadmap(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertLRRoadmapDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"roadmap.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildLRRoadmap();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropLikertChart(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertLikertDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"likert7.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildLikertChart();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropWordCloud(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertWordCloudDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"wordcloud.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildWordCloud();
            // word clouds do not support legends
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), Wisteria::UI::LegendPlacement::None);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropProConRoadmap(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertProConRoadmapDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"roadmap.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildProConRoadmap();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropBarChart(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertCatBarChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"barchart.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildCatBarChart(m_doc);
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropPieChart(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertPieChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"piechart.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildPieChart(m_doc);
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropLinePlot(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertLinePlotDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"lineplot.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildLinePlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropMultiSeriesLinePlot(Canvas* stagingCanvas, const size_t row,
                                                const size_t col)
        {
        Wisteria::UI::InsertMultiSeriesLinePlotDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"lineplot.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildMultiSeriesLinePlot();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropTable(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertTableDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"table.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto table = dlg.BuildTable();
            // tables do not support legends
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, table, std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), Wisteria::UI::LegendPlacement::None);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropChoroplethMap(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertChoroplethMapDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"choropleth.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildChoroplethMap();

            // uses a specialized legend
            const bool wantsLegend = dlg.IsMappingData() || dlg.IsUsingProportionalSymbols();
            const auto legendPlacement =
                wantsLegend ? dlg.GetLegendPlacement() : Wisteria::UI::LegendPlacement::None;

            auto legend = WisteriaView::BuildLegend(
                dlg, legendPlacement,
                [&plot, &dlg](const Wisteria::Graphs::LegendOptions& options)
                    -> std::unique_ptr<Wisteria::GraphItems::GraphItemBase>
                {
                    return dlg.GetSymbolColumn().empty() ?
                               std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                   plot->CreateLegend(options)) :
                               std::unique_ptr<Wisteria::GraphItems::GraphItemBase>(
                                   plot->CreateChoroplethLegend(options));
                });
            WisteriaView::PlaceGraphAndLegendInGrid(stagingCanvas, plot, std::move(legend),
                                                    dlg.GetSelectedRow(), dlg.GetSelectedColumn(),
                                                    legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropSankeyDiagram(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertSankeyDiagramDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"sankey.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildSankeyDiagram();
            // Sankey diagrams do not support legends
            stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropWaffleChart(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertWaffleChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"waffle.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildWaffleChart();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropRaceTrackChart(Canvas* stagingCanvas, const size_t row,
                                           const size_t col)
        {
        Wisteria::UI::InsertRaceTrackChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"racetrack.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildRaceTrackChart();
            // race track charts do not support legends
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, std::unique_ptr<Wisteria::GraphItems::GraphItemBase>{},
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), Wisteria::UI::LegendPlacement::None);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropNightingaleRoseChart(Canvas* stagingCanvas, const size_t row,
                                                 const size_t col)
        {
        Wisteria::UI::InsertNightingaleRoseChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"rose.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildNightingaleRoseChart();
            const auto legendPlacement = dlg.GetLegendPlacement();
            WisteriaView::PlaceGraphAndLegendInGrid(
                stagingCanvas, plot, WisteriaView::BuildLegend(dlg, *plot, legendPlacement),
                dlg.GetSelectedRow(), dlg.GetSelectedColumn(), legendPlacement);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropBulletChart(Canvas* stagingCanvas, const size_t row, const size_t col)
        {
        Wisteria::UI::InsertBulletChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"bulletchart.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildBulletChart();
            stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }

    //-------------------------------------------
    bool InsertPageDlg::DropWaterfallChart(Canvas* stagingCanvas, const size_t row,
                                           const size_t col)
        {
        Wisteria::UI::InsertWaterfallChartDlg dlg(stagingCanvas, m_reportBuilder, this);
        WisteriaView::SetDialogIcon(dlg, L"waterfallchart.svg");
        dlg.SetSelectedCell(row, col);
        if (dlg.ShowModal() != wxID_OK)
            {
            return false;
            }

        try
            {
            auto plot = dlg.BuildWaterfallChart();
            stagingCanvas->SetFixedObject(dlg.GetSelectedRow(), dlg.GetSelectedColumn(), plot);
            return true;
            }
        catch (const std::exception&)
            {
            return false;
            }
        }
    } // namespace Wisteria::UI
