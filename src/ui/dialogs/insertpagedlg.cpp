///////////////////////////////////////////////////////////////////////////////
// Name:        insertpagedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertpagedlg.h"
#include <wx/dcbuffer.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertPageDlg::InsertPageDlg(Canvas* canvas, wxWindow* parent, const wxWindowID id,
                                 const wxString& caption, const wxPoint& pos, const wxSize& size,
                                 const long style)
        : DialogWithHelp(parent, id, caption, pos, size, style), m_canvas(canvas)
        {
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
    void InsertPageDlg::CreateControls()
        {
        if (m_canvas != nullptr)
            {
            const auto [currentRows, currentCols] = m_canvas->GetFixedObjectsGridSize();
            m_rowCount = currentRows;
            m_columnCount = currentCols;
            }

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* gridSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        gridSizer->AddGrowableCol(1, 1);

        // rows
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Rows:")),
                       wxSizerFlags{}.CenterVertical());
        auto* rowsSpin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                        wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
        rowsSpin->SetValidator(wxGenericValidator(&m_rowCount));
        gridSizer->Add(rowsSpin, wxSizerFlags{}.Expand());

        // columns
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Columns:")),
                       wxSizerFlags{}.CenterVertical());
        auto* columnsSpin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                           wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
        columnsSpin->SetValidator(wxGenericValidator(&m_columnCount));
        gridSizer->Add(columnsSpin, wxSizerFlags{}.Expand());

        // name
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Name:")),
                       wxSizerFlags{}.CenterVertical());
        auto* nameCtrl = new wxTextCtrl(this, wxID_ANY, m_pageName, wxDefaultPosition,
                                        wxDefaultSize, 0, wxGenericValidator(&m_pageName));
        gridSizer->Add(nameCtrl, wxSizerFlags{}.Expand());

        contentSizer->Add(gridSizer, wxSizerFlags{}.Expand().Border());

        // preview panel
        const int previewHeight = FromDIP(400);
        const int previewWidth = static_cast<int>(previewHeight * math_constants::golden_ratio);
        m_previewPanel =
            new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize{ previewWidth, previewHeight },
                        wxTAB_TRAVERSAL | wxWANTS_CHARS);
        m_previewPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
        m_previewPanel->SetMinSize(wxSize{ previewWidth, previewHeight });
        contentSizer->Add(m_previewPanel, wxSizerFlags{ 1 }.Expand().Border());

        mainSizer->Add(contentSizer, wxSizerFlags{ 1 }.Expand());

        // OK/Cancel buttons
        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);

        const auto paintPreview = [this]([[maybe_unused]]
                                         wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(m_previewPanel);
            dc.SetBackground(*wxWHITE_BRUSH);
            dc.Clear();

            const auto clientRect = m_previewPanel->GetClientRect();
            if (clientRect.IsEmpty())
                {
                return;
                }

            const auto cellWidth = safe_divide<double>(clientRect.GetWidth(), m_columnCount);
            const auto cellHeight = safe_divide<double>(clientRect.GetHeight(), m_rowCount);

            const wxBrush occupiedBrush(wxColour{ 220, 220, 220 });
            const auto labelFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Smaller();

            for (size_t row = 0; std::cmp_less(row, m_rowCount); ++row)
                {
                for (size_t col = 0; std::cmp_less(col, m_columnCount); ++col)
                    {
                    const auto cellLeft = static_cast<int>(col * cellWidth);
                    const auto cellTop = static_cast<int>(row * cellHeight);
                    const auto cellW = static_cast<int>((col + 1) * cellWidth) - cellLeft;
                    const auto cellH = static_cast<int>((row + 1) * cellHeight) - cellTop;

                    const auto canvasGrid = (m_canvas != nullptr) ?
                                                m_canvas->GetFixedObjectsGridSize() :
                                                std::make_pair<size_t, size_t>(0, 0);
                    const auto item =
                        (m_canvas != nullptr && row < canvasGrid.first && col < canvasGrid.second) ?
                            m_canvas->GetFixedObject(row, col) :
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
                        const auto label = item->GetClassName();
                        if (!label.empty())
                            {
                            dc.SetFont(labelFont);
                            dc.SetTextForeground(
                                wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
                            const auto textExtent = dc.GetTextExtent(label);
                            const auto textLeft =
                                clientRect.x + cellLeft + (cellW - textExtent.GetWidth()) / 2;
                            const auto textTop =
                                clientRect.y + cellTop + (cellH - textExtent.GetHeight()) / 2;
                            dc.DrawText(label, textLeft, textTop);
                            }
                        }
                    }
                }
        };

        // click handler
        m_previewPanel->Bind(
            wxEVT_LEFT_DOWN,
            [this](wxMouseEvent& evt)
            {
                m_previewPanel->SetFocus();
                const auto clientRect = m_previewPanel->GetClientRect();
                if (clientRect.IsEmpty())
                    {
                    return;
                    }
                const auto cellWidth = safe_divide<double>(clientRect.GetWidth(), m_columnCount);
                const auto cellHeight = safe_divide<double>(clientRect.GetHeight(), m_rowCount);

                if (cellWidth <= 0 || cellHeight <= 0)
                    {
                    return;
                    }

                const auto col = safe_divide<size_t>(evt.GetX() - clientRect.x, cellWidth);
                const auto row = safe_divide<size_t>(evt.GetY() - clientRect.y, cellHeight);

                SelectCell(std::min(row, static_cast<size_t>(m_rowCount - 1)),
                           std::min(col, static_cast<size_t>(m_columnCount - 1)));
            });

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
                    const auto [canvasRows, canvasColumns] = m_canvas->GetFixedObjectsGridSize();
                    if (m_selectedRow < canvasRows && m_selectedColumn < canvasColumns &&
                        m_canvas->GetFixedObject(m_selectedRow, m_selectedColumn) != nullptr)
                        {
                        if (wxMessageBox(
                                wxString::Format(
                                    _(L"Are you sure you want to delete the selected %s?"),
                                    m_canvas->GetFixedObject(m_selectedRow, m_selectedColumn)
                                        ->GetClassName()),
                                _(L"Delete Item"), wxYES_NO | wxICON_QUESTION, this) == wxYES)
                            {
                            m_canvas->SetFixedObject(m_selectedRow, m_selectedColumn, nullptr);
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

        m_previewPanel->Bind(wxEVT_PAINT, paintPreview);
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
                           m_previewPanel->Refresh();
                       });
        columnsSpin->Bind(wxEVT_SPINCTRL,
                          [this]([[maybe_unused]] wxSpinEvent&)
                          {
                              TransferDataFromWindow();
                              m_previewPanel->Refresh();
                          });
        }
    } // namespace Wisteria::UI
