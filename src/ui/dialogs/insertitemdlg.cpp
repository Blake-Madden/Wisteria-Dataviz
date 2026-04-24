///////////////////////////////////////////////////////////////////////////////
// Name:        insertitemdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertitemdlg.h"
#include "../../app/wisteriaapp.h"
#include "../../base/image.h"
#include "../../base/label.h"
#include "../../graphs/barchart.h"
#include "../../graphs/boxplot.h"
#include "../../graphs/bubbleplot.h"
#include "../../graphs/candlestickplot.h"
#include "../../graphs/categoricalbarchart.h"
#include "../../graphs/chernoffplot.h"
#include "../../graphs/ganttchart.h"
#include "../../graphs/graph2d.h"
#include "../../graphs/heatmap.h"
#include "../../graphs/histogram.h"
#include "../../graphs/likertchart.h"
#include "../../graphs/lineplot.h"
#include "../../graphs/lrroadmap.h"
#include "../../graphs/multi_series_lineplot.h"
#include "../../graphs/piechart.h"
#include "../../graphs/proconroadmap.h"
#include "../../graphs/sankeydiagram.h"
#include "../../graphs/scatterplot.h"
#include "../../graphs/stemandleafplot.h"
#include "../../graphs/table.h"
#include "../../graphs/waffle_chart.h"
#include "../../graphs/wcurveplot.h"
#include "../../graphs/win_loss_sparkline.h"
#include "../../graphs/wordcloud.h"
#include <wx/dcbuffer.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertItemDlg::InsertItemDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                 wxWindow* parent, const wxString& caption, const wxWindowID id,
                                 const wxPoint& pos, const wxSize& size, const long style,
                                 EditMode editMode, const int pageOptions)
        : DialogWithHelp(parent, id, caption, pos, size, style), m_canvas(canvas),
          m_reportBuilder(reportBuilder), m_editMode(editMode), m_pageOptions(pageOptions)
        {
        const auto gridSize = m_canvas->GetFixedObjectsGridSize();
        m_rows = std::max<size_t>(gridSize.first, 1);
        m_columns = std::max<size_t>(gridSize.second, 1);

        // select the first empty cell (scanning in reading order),
        // falling back to (0, 0) if all cells are occupied
        for (size_t row = 0; row < gridSize.first; ++row)
            {
            for (size_t col = 0; col < gridSize.second; ++col)
                {
                if (m_canvas->GetFixedObject(row, col) == nullptr)
                    {
                    m_selectedRow = row;
                    m_selectedColumn = col;
                    return;
                    }
                }
            }
        }

    //-------------------------------------------
    void InsertItemDlg::SelectCell(size_t row, size_t column)
        {
        if (row < m_rows && column < m_columns)
            {
            m_selectedRow = row;
            m_selectedColumn = column;
            if (m_gridPanel != nullptr)
                {
                m_gridPanel->Refresh();
                }
            }
        }

    //-------------------------------------------
    void InsertItemDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        m_sideBarBook = new SideBarBook(this, wxID_ANY);
        mainSizer->Add(m_sideBarBook, wxSizerFlags{ 1 }.Expand().Border());

        // Page section
        auto* pagePage = new wxPanel(m_sideBarBook);
        auto* pageSizer = new wxBoxSizer(wxVERTICAL);
        pagePage->SetSizer(pageSizer);
        m_sideBarBook->AddPage(pagePage, _(L"Page"), ID_PAGE_SECTION, true);

        // grid size controls (only for new items being added to the canvas)
        if (GetEditMode() == EditMode::Insert)
            {
            auto* gridSizeSizer =
                new wxFlexGridSizer(4, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                               wxSizerFlags::GetDefaultBorder() });
            gridSizeSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Rows:")),
                               wxSizerFlags{}.CenterVertical());
            m_rowsSpin = new wxSpinCtrl(pagePage, wxID_ANY);
            m_rowsSpin->SetRange(static_cast<int>(m_rows), 50);
            m_rowsSpin->SetValue(static_cast<int>(m_rows));
            gridSizeSizer->Add(m_rowsSpin);
            gridSizeSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Columns:")),
                               wxSizerFlags{}.CenterVertical());
            m_columnsSpin = new wxSpinCtrl(pagePage, wxID_ANY);
            m_columnsSpin->SetRange(static_cast<int>(m_columns), 50);
            m_columnsSpin->SetValue(static_cast<int>(m_columns));
            gridSizeSizer->Add(m_columnsSpin);
            pageSizer->Add(gridSizeSizer, wxSizerFlags{}.Border());

            m_rowsSpin->Bind(wxEVT_SPINCTRL,
                             [this]([[maybe_unused]] wxSpinEvent&)
                             {
                                 m_rows = static_cast<size_t>(m_rowsSpin->GetValue());
                                 if (m_gridPanel != nullptr)
                                     {
                                     m_gridPanel->Refresh();
                                     }
                             });
            m_columnsSpin->Bind(wxEVT_SPINCTRL,
                                [this]([[maybe_unused]] wxSpinEvent&)
                                {
                                    m_columns = static_cast<size_t>(m_columnsSpin->GetValue());
                                    if (m_gridPanel != nullptr)
                                        {
                                        m_gridPanel->Refresh();
                                        }
                                });
            }

        // grid preview (only for new items being added to the canvas)
        if (GetEditMode() == EditMode::Insert)
            {
            pageSizer->Add(new wxStaticText(pagePage, wxID_ANY,
                                            _(L"Select the cell where the item will be placed:")),
                           wxSizerFlags{}.Border(wxLEFT));

            const int previewHeight = FromDIP(200);
            const int previewWidth = static_cast<int>(previewHeight * math_constants::golden_ratio);
            m_gridPanel =
                new wxPanel(pagePage, wxID_ANY, wxDefaultPosition,
                            wxSize{ previewWidth, previewHeight }, wxTAB_TRAVERSAL | wxWANTS_CHARS);
            m_gridPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
            m_gridPanel->SetMinSize(wxSize{ previewWidth, previewHeight });
            pageSizer->Add(m_gridPanel, wxSizerFlags{ 1 }.CenterHorizontal().Border());

            // paint handler
            m_gridPanel->Bind(
                wxEVT_PAINT,
                [this]([[maybe_unused]] wxPaintEvent&)
                {
                    wxAutoBufferedPaintDC dc(m_gridPanel);
                    dc.SetBackground(*wxWHITE_BRUSH);
                    dc.Clear();

                    const auto clientRect = m_gridPanel->GetClientRect();
                    if (clientRect.IsEmpty())
                        {
                        return;
                        }

                    const auto cellWidth = safe_divide<double>(clientRect.GetWidth(), m_columns);
                    const auto cellHeight = safe_divide<double>(clientRect.GetHeight(), m_rows);

                    const wxBrush occupiedBrush(wxColour{ 220, 220, 220 });

                    for (size_t row = 0; row < m_rows; ++row)
                        {
                        for (size_t col = 0; col < m_columns; ++col)
                            {
                            const auto cellLeft = static_cast<int>(col * cellWidth);
                            const auto cellTop = static_cast<int>(row * cellHeight);
                            const auto cellW = static_cast<int>((col + 1) * cellWidth) - cellLeft;
                            const auto cellH = static_cast<int>((row + 1) * cellHeight) - cellTop;

                            const auto canvasGrid = (m_canvas != nullptr) ?
                                                        m_canvas->GetFixedObjectsGridSize() :
                                                        std::make_pair<size_t, size_t>(0, 0);
                            const auto item = (m_canvas != nullptr && row < canvasGrid.first &&
                                               col < canvasGrid.second) ?
                                                  m_canvas->GetFixedObject(row, col) :
                                                  nullptr;
                            const bool isOccupied = (item != nullptr);

                            if (row == m_selectedRow && col == m_selectedColumn)
                                {
                                dc.SetPen(
                                    wxPen{ wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT), 2,
                                           wxPENSTYLE_DOT });
                                dc.SetBrush(isOccupied ? occupiedBrush : *wxWHITE_BRUSH);
                                }
                            else
                                {
                                dc.SetPen(*wxBLACK_PEN);
                                dc.SetBrush(isOccupied ? occupiedBrush : *wxWHITE_BRUSH);
                                }
                            dc.DrawRectangle(clientRect.x + cellLeft, clientRect.y + cellTop, cellW,
                                             cellH);

                            if (isOccupied)
                                {
                                const auto svgName = WisteriaApp::GetItemIconName(item.get());
                                if (!svgName.empty())
                                    {
                                    const auto iconSize = std::min(cellW, cellH) * 3 / 4;
                                    const auto bmpBundle =
                                        wxGetApp().GetResourceManager().GetSVG(svgName);
                                    if (bmpBundle.IsOk())
                                        {
                                        const auto bmp =
                                            bmpBundle.GetBitmap(wxSize{ iconSize, iconSize });
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
                                dc.SetFont(
                                    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Smaller());
                                dc.SetTextForeground(
                                    wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
                                const auto textExtent = dc.GetTextExtent(emptyLabel);
                                const auto textLeft =
                                    clientRect.x + cellLeft + (cellW - textExtent.GetWidth()) / 2;
                                const auto textTop =
                                    clientRect.y + cellTop + (cellH - textExtent.GetHeight()) / 2;
                                dc.DrawText(emptyLabel, textLeft, textTop);
                                }
                            }
                        }
                });

            // resize handler
            m_gridPanel->Bind(wxEVT_SIZE,
                              [this](wxSizeEvent& evt)
                              {
                                  m_gridPanel->Refresh();
                                  evt.Skip();
                              });

            // click handler
            m_gridPanel->Bind(
                wxEVT_LEFT_DOWN,
                [this](wxMouseEvent& evt)
                {
                    m_gridPanel->SetFocus();
                    const auto clientRect = m_gridPanel->GetClientRect();
                    if (clientRect.IsEmpty())
                        {
                        return;
                        }
                    const auto cellWidth = safe_divide<double>(clientRect.GetWidth(), m_columns);
                    const auto cellHeight = safe_divide<double>(clientRect.GetHeight(), m_rows);

                    if (cellWidth <= 0 || cellHeight <= 0)
                        {
                        return;
                        }

                    const auto col = safe_divide<size_t>(evt.GetX() - clientRect.x, cellWidth);
                    const auto row = safe_divide<size_t>(evt.GetY() - clientRect.y, cellHeight);

                    SelectCell(std::min(row, m_rows - 1), std::min(col, m_columns - 1));
                });

            // keyboard handler (arrow keys navigate cells,
            // Tab/Shift+Tab pass through for normal focus traversal)
            m_gridPanel->Bind(wxEVT_KEY_DOWN,
                              [this](wxKeyEvent& evt)
                              {
                                  const auto keyCode = evt.GetKeyCode();
                                  if (keyCode == WXK_RIGHT)
                                      {
                                      if (m_selectedColumn + 1 < m_columns)
                                          {
                                          SelectCell(m_selectedRow, m_selectedColumn + 1);
                                          }
                                      else if (m_selectedRow + 1 < m_rows)
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
                                          SelectCell(m_selectedRow - 1, m_columns - 1);
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
                                      if (m_selectedRow + 1 < m_rows)
                                          {
                                          SelectCell(m_selectedRow + 1, m_selectedColumn);
                                          }
                                      }
                                  else
                                      {
                                      evt.Skip();
                                      }
                              });

            // focus handlers
            m_gridPanel->Bind(wxEVT_SET_FOCUS,
                              [this]([[maybe_unused]]
                                     wxFocusEvent& evt)
                              {
                                  m_gridPanel->Refresh();
                                  evt.Skip();
                              });
            m_gridPanel->Bind(wxEVT_KILL_FOCUS,
                              [this]([[maybe_unused]]
                                     wxFocusEvent& evt)
                              {
                                  m_gridPanel->Refresh();
                                  evt.Skip();
                              });
            }

        // page-level settings (alignment, scaling, margins, padding, outline, etc.)
        if ((m_pageOptions & ItemDlgIncludePageSettings) != 0)
            {
            // two-column layout for the remaining controls
            auto* columnsSizer = new wxBoxSizer(wxHORIZONTAL);
            pageSizer->Add(columnsSizer, wxSizerFlags{}.Expand());

            auto* leftColumnSizer = new wxBoxSizer(wxVERTICAL);
            columnsSizer->Add(leftColumnSizer, wxSizerFlags{}.Expand());

            auto* rightColumnSizer = new wxBoxSizer(wxVERTICAL);
            columnsSizer->Add(rightColumnSizer, wxSizerFlags{}.Expand().Border(wxLEFT));

            // item placement properties
            auto* propsSizer = new wxFlexGridSizer(2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                              wxSizerFlags::GetDefaultBorder() });

            // horizontal page alignment
            propsSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Horizontal alignment:")),
                            wxSizerFlags{}.CenterVertical());
                {
                auto* hAlignChoice =
                    new wxChoice(pagePage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr,
                                 0, wxGenericValidator(&m_horizontalAlign));
                hAlignChoice->Append(_(L"Left"));
                hAlignChoice->Append(_(L"Centered"));
                hAlignChoice->Append(_(L"Right"));
                propsSizer->Add(hAlignChoice);
                }

            // vertical page alignment
            propsSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Vertical alignment:")),
                            wxSizerFlags{}.CenterVertical());
                {
                auto* vAlignChoice =
                    new wxChoice(pagePage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr,
                                 0, wxGenericValidator(&m_verticalAlign));
                vAlignChoice->Append(_(L"Top"));
                vAlignChoice->Append(_(L"Centered"));
                vAlignChoice->Append(_(L"Bottom"));
                propsSizer->Add(vAlignChoice);
                }

            // scaling
            propsSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Scaling:")),
                            wxSizerFlags{}.CenterVertical());
            m_scalingSpin = new wxSpinCtrlDouble(pagePage, wxID_ANY);
            m_scalingSpin->SetRange(0.1, 10.0);
            m_scalingSpin->SetValue(1.0);
            m_scalingSpin->SetIncrement(0.1);
            m_scalingSpin->SetDigits(1);
            propsSizer->Add(m_scalingSpin);

            leftColumnSizer->Add(propsSizer, wxSizerFlags{}.Border());

            // canvas margins (top, right, bottom, left)
            auto* marginBox = new wxStaticBoxSizer(wxVERTICAL, pagePage, _(L"Canvas margins"));
            auto* marginGrid = new wxFlexGridSizer(
                4, wxSize{ wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder() });

            const auto addMarginSpin = [&](const wxString& label, int* value)
            {
                marginGrid->Add(new wxStaticText(marginBox->GetStaticBox(), wxID_ANY, label),
                                wxSizerFlags{}.CenterVertical());
                auto* spin = new wxSpinCtrl(marginBox->GetStaticBox(), wxID_ANY);
                spin->SetRange(0, 100);
                spin->SetValidator(wxGenericValidator(value));
                marginGrid->Add(spin);
            };
            addMarginSpin(_(L"Top:"), &m_marginTop);
            addMarginSpin(_(L"Right:"), &m_marginRight);
            addMarginSpin(_(L"Bottom:"), &m_marginBottom);
            addMarginSpin(_(L"Left:"), &m_marginLeft);

            marginBox->Add(marginGrid, wxSizerFlags{}.Border());
            rightColumnSizer->Add(marginBox, wxSizerFlags{}.Border());

            // padding (top, right, bottom, left)
            auto* paddingBox = new wxStaticBoxSizer(wxVERTICAL, pagePage, _(L"Padding"));
            auto* paddingGrid = new wxFlexGridSizer(
                4, wxSize{ wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder() });

            const auto addPaddingSpin = [&](const wxString& label, int* value)
            {
                paddingGrid->Add(new wxStaticText(paddingBox->GetStaticBox(), wxID_ANY, label),
                                 wxSizerFlags{}.CenterVertical());
                auto* spin = new wxSpinCtrl(paddingBox->GetStaticBox(), wxID_ANY);
                spin->SetRange(0, 100);
                spin->SetValidator(wxGenericValidator(value));
                paddingGrid->Add(spin);
            };
            addPaddingSpin(_(L"Top:"), &m_paddingTop);
            addPaddingSpin(_(L"Right:"), &m_paddingRight);
            addPaddingSpin(_(L"Bottom:"), &m_paddingBottom);
            addPaddingSpin(_(L"Left:"), &m_paddingLeft);

            paddingBox->Add(paddingGrid, wxSizerFlags{}.Border());
            rightColumnSizer->Add(paddingBox, wxSizerFlags{}.Border());

            // fit row to content
            leftColumnSizer->Add(new wxCheckBox(pagePage, wxID_ANY, _(L"Fit row to content"),
                                                wxDefaultPosition, wxDefaultSize, 0,
                                                wxGenericValidator(&m_fitRowToContent)),
                                 wxSizerFlags{}.Border());

            // fixed width
            leftColumnSizer->Add(new wxCheckBox(pagePage, wxID_ANY, _(L"Fixed width"),
                                                wxDefaultPosition, wxDefaultSize, 0,
                                                wxGenericValidator(&m_fixedWidth)),
                                 wxSizerFlags{}.Border());

            // outline pen
            auto* outlineBox = new wxStaticBoxSizer(wxVERTICAL, pagePage, _(L"Outline"));
            auto* outlineGrid = new wxFlexGridSizer(2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                               wxSizerFlags::GetDefaultBorder() });
            outlineGrid->Add(new wxStaticText(outlineBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                             wxSizerFlags{}.CenterVertical());
            m_outlineColorPicker =
                new wxColourPickerCtrl(outlineBox->GetStaticBox(), wxID_ANY, *wxBLACK);
            outlineGrid->Add(m_outlineColorPicker);
            outlineGrid->Add(new wxStaticText(outlineBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                             wxSizerFlags{}.CenterVertical());
                {
                auto* outlineWidthSpin = new wxSpinCtrl(outlineBox->GetStaticBox(), wxID_ANY);
                outlineWidthSpin->SetRange(1, 20);
                outlineWidthSpin->SetValidator(wxGenericValidator(&m_outlineWidth));
                outlineGrid->Add(outlineWidthSpin);
                }
            outlineGrid->Add(new wxStaticText(outlineBox->GetStaticBox(), wxID_ANY, _(L"Style:")),
                             wxSizerFlags{}.CenterVertical());
                {
                auto* outlineStyleChoice =
                    new wxChoice(outlineBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, 0, nullptr, 0, wxGenericValidator(&m_outlineStyle));
                outlineStyleChoice->Append(_(L"Solid"));
                outlineStyleChoice->Append(_(L"Dot"));
                outlineStyleChoice->Append(_(L"Long dash"));
                outlineStyleChoice->Append(_(L"Short dash"));
                outlineStyleChoice->Append(_(L"Dot dash"));
                outlineGrid->Add(outlineStyleChoice);
                }
            outlineBox->Add(outlineGrid, wxSizerFlags{}.Border());

            // border sides
            auto* borderSizer = new wxBoxSizer(wxHORIZONTAL);
            borderSizer->Add(new wxCheckBox(outlineBox->GetStaticBox(), wxID_ANY, _(L"Top"),
                                            wxDefaultPosition, wxDefaultSize, 0,
                                            wxGenericValidator(&m_outlineTop)),
                             wxSizerFlags{}.Border(wxRIGHT));
            borderSizer->Add(new wxCheckBox(outlineBox->GetStaticBox(), wxID_ANY, _(L"Right"),
                                            wxDefaultPosition, wxDefaultSize, 0,
                                            wxGenericValidator(&m_outlineRight)),
                             wxSizerFlags{}.Border(wxRIGHT));
            borderSizer->Add(new wxCheckBox(outlineBox->GetStaticBox(), wxID_ANY, _(L"Bottom"),
                                            wxDefaultPosition, wxDefaultSize, 0,
                                            wxGenericValidator(&m_outlineBottom)),
                             wxSizerFlags{}.Border(wxRIGHT));
            borderSizer->Add(new wxCheckBox(outlineBox->GetStaticBox(), wxID_ANY, _(L"Left"),
                                            wxDefaultPosition, wxDefaultSize, 0,
                                            wxGenericValidator(&m_outlineLeft)));
            outlineBox->Add(borderSizer, wxSizerFlags{}.Border());

            leftColumnSizer->Add(outlineBox, wxSizerFlags{}.Border());
            }

        SetSizer(mainSizer);
        }

    //-------------------------------------------
    PageHorizontalAlignment InsertItemDlg::GetHorizontalPageAlignment() const
        {
        switch (m_horizontalAlign)
            {
        case 1:
            return PageHorizontalAlignment::Centered;
        case 2:
            return PageHorizontalAlignment::RightAligned;
        default:
            return PageHorizontalAlignment::LeftAligned;
            }
        }

    //-------------------------------------------
    PageVerticalAlignment InsertItemDlg::GetVerticalPageAlignment() const
        {
        switch (m_verticalAlign)
            {
        case 1:
            return PageVerticalAlignment::Centered;
        case 2:
            return PageVerticalAlignment::BottomAligned;
        default:
            return PageVerticalAlignment::TopAligned;
            }
        }

    //-------------------------------------------
    double InsertItemDlg::GetItemScaling() const { return m_scalingSpin->GetValue(); }

    //-------------------------------------------
    std::array<int, 4> InsertItemDlg::GetCanvasMargins() const
        {
        return { m_marginTop, m_marginRight, m_marginBottom, m_marginLeft };
        }

    //-------------------------------------------
    std::array<int, 4> InsertItemDlg::GetPadding() const
        {
        return { m_paddingTop, m_paddingRight, m_paddingBottom, m_paddingLeft };
        }

    //-------------------------------------------
    bool InsertItemDlg::GetFitRowToContent() const { return m_fitRowToContent; }

    //-------------------------------------------
    bool InsertItemDlg::GetFixedWidth() const { return m_fixedWidth; }

    //-------------------------------------------
    wxPen InsertItemDlg::GetOutlinePen() const
        {
        constexpr wxPenStyle penStyles[] = { wxPENSTYLE_SOLID, wxPENSTYLE_DOT, wxPENSTYLE_LONG_DASH,
                                             wxPENSTYLE_SHORT_DASH, wxPENSTYLE_DOT_DASH };
        const auto styleIndex = static_cast<size_t>(m_outlineStyle);
        const auto penStyle =
            (styleIndex < std::size(penStyles)) ? penStyles[styleIndex] : wxPENSTYLE_SOLID;
        return { m_outlineColorPicker->GetColour(), m_outlineWidth, penStyle };
        }

    //-------------------------------------------
    std::bitset<4> InsertItemDlg::GetOutlineSides() const
        {
        std::bitset<4> sides;
        sides.set(0, m_outlineTop);
        sides.set(1, m_outlineRight);
        sides.set(2, m_outlineBottom);
        sides.set(3, m_outlineLeft);
        return sides;
        }

    //-------------------------------------------
    void InsertItemDlg::ApplyPageOptions(GraphItems::GraphItemBase& item) const
        {
        item.SetPageHorizontalAlignment(GetHorizontalPageAlignment());
        item.SetPageVerticalAlignment(GetVerticalPageAlignment());
        item.SetScaling(GetItemScaling());

        const auto margins = GetCanvasMargins();
        item.SetCanvasMargins(margins[0], margins[1], margins[2], margins[3]);

        const auto padding = GetPadding();
        item.SetPadding(padding[0], padding[1], padding[2], padding[3]);

        item.SetFixedWidthOnCanvas(GetFixedWidth());
        item.FitCanvasRowHeightToContent(GetFitRowToContent());

        const auto sides = GetOutlineSides();
        item.GetPen() = GetOutlinePen();
        item.GetGraphItemInfo().Outline(sides[0], sides[1], sides[2], sides[3]);
        }

    //-------------------------------------------
    void InsertItemDlg::SetSelectedCell(const size_t row, const size_t column)
        {
        SelectCell(row, column);
        }

    //-------------------------------------------
    void InsertItemDlg::LoadPageOptions(const GraphItems::GraphItemBase& item)
        {
        // alignment
        switch (item.GetPageHorizontalAlignment())
            {
        case PageHorizontalAlignment::Centered:
            m_horizontalAlign = 1;
            break;
        case PageHorizontalAlignment::RightAligned:
            m_horizontalAlign = 2;
            break;
        default:
            m_horizontalAlign = 0;
            break;
            }
        switch (item.GetPageVerticalAlignment())
            {
        case PageVerticalAlignment::Centered:
            m_verticalAlign = 1;
            break;
        case PageVerticalAlignment::BottomAligned:
            m_verticalAlign = 2;
            break;
        default:
            m_verticalAlign = 0;
            break;
            }

        // scaling — use the original value cached when the item was placed on the
        // canvas, since the canvas overwrites GetScaling() with its own scaling
        m_scalingSpin->SetValue(item.GetGraphItemInfo().GetOriginalCanvasScaling());

        // canvas margins
        m_marginTop = item.GetTopCanvasMargin();
        m_marginRight = item.GetRightCanvasMargin();
        m_marginBottom = item.GetBottomCanvasMargin();
        m_marginLeft = item.GetLeftCanvasMargin();

        // padding
        m_paddingTop = item.GetTopPadding();
        m_paddingRight = item.GetRightPadding();
        m_paddingBottom = item.GetBottomPadding();
        m_paddingLeft = item.GetLeftPadding();

        // sizing options
        m_fitRowToContent = item.IsFittingCanvasRowHeightToContent();
        m_fixedWidth = item.IsFixedWidthOnCanvas();

        // outline — Label's constructors default to Outline(true,true,true,true) but
        // with wxNullPen, so no outline is actually drawn. Treat the sides as disabled
        // when the pen is null to avoid misleadingly showing all checkboxes checked.
        const auto& pen = item.GetPen();
        const bool hasOutlinePen = pen.IsOk() && pen != wxNullPen;

        const auto& info = item.GetGraphItemInfo();
        m_outlineTop = hasOutlinePen && info.IsShowingTopOutline();
        m_outlineRight = hasOutlinePen && info.IsShowingRightOutline();
        m_outlineBottom = hasOutlinePen && info.IsShowingBottomOutline();
        m_outlineLeft = hasOutlinePen && info.IsShowingLeftOutline();

        if (hasOutlinePen)
            {
            m_outlineColorPicker->SetColour(pen.GetColour());
            m_outlineWidth = pen.GetWidth();

            switch (pen.GetStyle())
                {
            case wxPENSTYLE_DOT:
                m_outlineStyle = 1;
                break;
            case wxPENSTYLE_LONG_DASH:
                m_outlineStyle = 2;
                break;
            case wxPENSTYLE_SHORT_DASH:
                m_outlineStyle = 3;
                break;
            case wxPENSTYLE_DOT_DASH:
                m_outlineStyle = 4;
                break;
            default:
                m_outlineStyle = 0;
                break;
                }
            }

        TransferDataToWindow();
        }

    //-------------------------------------------
    bool InsertItemDlg::ConfirmOverwrite()
        {
        if (GetEditMode() == EditMode::Insert && m_canvas != nullptr)
            {
            const auto [canvasRows, canvasCols] = m_canvas->GetFixedObjectsGridSize();
            if (m_selectedRow < canvasRows && m_selectedColumn < canvasCols &&
                m_canvas->GetFixedObject(m_selectedRow, m_selectedColumn) != nullptr)
                {
                if (wxMessageBox(_(L"The selected cell already contains an "
                                   "item. Do you want to replace it?"),
                                 _(L"Replace Item"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
                    {
                    return false;
                    }
                }
            }
        return true;
        }

    //-------------------------------------------
    void InsertItemDlg::FinalizeControls()
        {
        GetSizer()->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                        wxSizerFlags{}.Expand().Border());
        GetSizer()->SetSizeHints(this);

        Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent& evt)
            {
                ApplyGridSize();
                TransferDataFromWindow();

                if (!ConfirmOverwrite())
                    {
                    return;
                    }

                evt.Skip();
            },
            wxID_OK);
        }
    } // namespace Wisteria::UI
