///////////////////////////////////////////////////////////////////////////////
// Name:        insertitemdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertitemdlg.h"
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
                                 const wxPoint& pos, const wxSize& size, const long style)
        : DialogWithHelp(parent, id, caption, pos, size, style), m_canvas(canvas),
          m_reportBuilder(reportBuilder)
        {
        const auto gridSize = m_canvas->GetFixedObjectsGridSize();
        m_rows = std::max<size_t>(gridSize.first, 1);
        m_columns = std::max<size_t>(gridSize.second, 1);
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
    wxString InsertItemDlg::GetItemTypeName(const std::shared_ptr<GraphItems::GraphItemBase>& item)
        {
        if (item == nullptr)
            {
            return {};
            }
        // check leaf types before base types so that derived classes
        // are reported more specifically
        if (item->IsKindOf(wxCLASSINFO(Graphs::Histogram)))
            {
            return _(L"[Histogram]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::GanttChart)))
            {
            return _(L"[Gantt Chart]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::LikertChart)))
            {
            return _(L"[Likert Chart]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::CategoricalBarChart)))
            {
            return _(L"[Bar Chart]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::BarChart)))
            {
            return _(L"[Bar Chart]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::BubblePlot)))
            {
            return _(L"[Bubble Plot]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::ScatterPlot)))
            {
            return _(L"[Scatter Plot]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::MultiSeriesLinePlot)))
            {
            return _(L"[Line Plot]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::WCurvePlot)))
            {
            return _(L"[W-Curve Plot]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::LinePlot)))
            {
            return _(L"[Line Plot]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::BoxPlot)))
            {
            return _(L"[Box Plot]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::CandlestickPlot)))
            {
            return _(L"[Candlestick Plot]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::HeatMap)))
            {
            return _(L"[Heat Map]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::PieChart)))
            {
            return _(L"[Pie Chart]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::Table)))
            {
            return _(L"[Table]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::SankeyDiagram)))
            {
            return _(L"[Sankey Diagram]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::ChernoffFacesPlot)))
            {
            return _(L"[Chernoff Faces]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::WaffleChart)))
            {
            return _(L"[Waffle Chart]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::WordCloud)))
            {
            return _(L"[Word Cloud]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::WinLossSparkline)))
            {
            return _(L"[Sparkline]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::StemAndLeafPlot)))
            {
            return _(L"[Stem & Leaf]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::LRRoadmap)))
            {
            return _(L"[Roadmap]");
            }
        if (item->IsKindOf(wxCLASSINFO(Graphs::ProConRoadmap)))
            {
            return _(L"[Roadmap]");
            }
        if (item->IsKindOf(wxCLASSINFO(GraphItems::Label)))
            {
            return _(L"[Label]");
            }
        if (item->IsKindOf(wxCLASSINFO(GraphItems::Image)))
            {
            return _(L"[Image]");
            }
        // fallback for any Graph2D subclass not listed above
        if (item->IsKindOf(wxCLASSINFO(Graphs::Graph2D)))
            {
            return _(L"[Graph]");
            }
        return _(L"[Item]");
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

        // grid preview (only when there are multiple cells to choose from)
        if (m_rows > 1 || m_columns > 1)
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
            pageSizer->Add(m_gridPanel, wxSizerFlags{ 1 }.Expand().Border());

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
                    const auto labelFont =
                        wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).Smaller();

                    for (size_t row = 0; row < m_rows; ++row)
                        {
                        for (size_t col = 0; col < m_columns; ++col)
                            {
                            const auto cellLeft = static_cast<int>(col * cellWidth);
                            const auto cellTop = static_cast<int>(row * cellHeight);
                            const auto cellW = static_cast<int>((col + 1) * cellWidth) - cellLeft;
                            const auto cellH = static_cast<int>((row + 1) * cellHeight) - cellTop;

                            const auto item = (m_canvas != nullptr) ?
                                                  m_canvas->GetFixedObject(row, col) :
                                                  nullptr;
                            const bool isOccupied = (item != nullptr);

                            if (row == m_selectedRow && col == m_selectedColumn)
                                {
                                dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT),
                                                2, wxPENSTYLE_DOT));
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
                                const auto label = GetItemTypeName(item);
                                if (!label.empty())
                                    {
                                    dc.SetFont(labelFont);
                                    dc.SetTextForeground(
                                        wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
                                    const auto textExtent = dc.GetTextExtent(label);
                                    const auto textLeft = clientRect.x + cellLeft +
                                                          (cellW - textExtent.GetWidth()) / 2;
                                    const auto textTop = clientRect.y + cellTop +
                                                         (cellH - textExtent.GetHeight()) / 2;
                                    dc.DrawText(label, textLeft, textTop);
                                    }
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

        // item placement properties
        auto* propsSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        // horizontal page alignment
        propsSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Horizontal alignment:")),
                        wxSizerFlags{}.CenterVertical());
        m_horizontalAlignChoice = new wxChoice(pagePage, wxID_ANY);
        m_horizontalAlignChoice->Append(_(L"Left"));
        m_horizontalAlignChoice->Append(_(L"Centered"));
        m_horizontalAlignChoice->Append(_(L"Right"));
        m_horizontalAlignChoice->SetSelection(0);
        propsSizer->Add(m_horizontalAlignChoice);

        // vertical page alignment
        propsSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Vertical alignment:")),
                        wxSizerFlags{}.CenterVertical());
        m_verticalAlignChoice = new wxChoice(pagePage, wxID_ANY);
        m_verticalAlignChoice->Append(_(L"Top"));
        m_verticalAlignChoice->Append(_(L"Centered"));
        m_verticalAlignChoice->Append(_(L"Bottom"));
        m_verticalAlignChoice->SetSelection(0);
        propsSizer->Add(m_verticalAlignChoice);

        // scaling
        propsSizer->Add(new wxStaticText(pagePage, wxID_ANY, _(L"Scaling:")),
                        wxSizerFlags{}.CenterVertical());
        m_scalingSpin = new wxSpinCtrlDouble(pagePage, wxID_ANY);
        m_scalingSpin->SetRange(0.1, 10.0);
        m_scalingSpin->SetValue(1.0);
        m_scalingSpin->SetIncrement(0.1);
        m_scalingSpin->SetDigits(1);
        propsSizer->Add(m_scalingSpin);

        pageSizer->Add(propsSizer, wxSizerFlags{}.Border());

        // canvas margins (top, right, bottom, left)
        auto* marginBox = new wxStaticBoxSizer(wxVERTICAL, pagePage, _(L"Canvas margins"));
        auto* marginGrid = new wxFlexGridSizer(4, wxSize{ FromDIP(4), FromDIP(4) });
        marginGrid->Add(new wxStaticText(marginBox->GetStaticBox(), wxID_ANY, _(L"Top:")),
                        wxSizerFlags{}.CenterVertical());
        m_marginTopSpin = new wxSpinCtrl(marginBox->GetStaticBox(), wxID_ANY);
        m_marginTopSpin->SetRange(0, 100);
        m_marginTopSpin->SetValue(0);
        marginGrid->Add(m_marginTopSpin);
        marginGrid->Add(new wxStaticText(marginBox->GetStaticBox(), wxID_ANY, _(L"Right:")),
                        wxSizerFlags{}.CenterVertical());
        m_marginRightSpin = new wxSpinCtrl(marginBox->GetStaticBox(), wxID_ANY);
        m_marginRightSpin->SetRange(0, 100);
        m_marginRightSpin->SetValue(0);
        marginGrid->Add(m_marginRightSpin);
        marginGrid->Add(new wxStaticText(marginBox->GetStaticBox(), wxID_ANY, _(L"Bottom:")),
                        wxSizerFlags{}.CenterVertical());
        m_marginBottomSpin = new wxSpinCtrl(marginBox->GetStaticBox(), wxID_ANY);
        m_marginBottomSpin->SetRange(0, 100);
        m_marginBottomSpin->SetValue(0);
        marginGrid->Add(m_marginBottomSpin);
        marginGrid->Add(new wxStaticText(marginBox->GetStaticBox(), wxID_ANY, _(L"Left:")),
                        wxSizerFlags{}.CenterVertical());
        m_marginLeftSpin = new wxSpinCtrl(marginBox->GetStaticBox(), wxID_ANY);
        m_marginLeftSpin->SetRange(0, 100);
        m_marginLeftSpin->SetValue(0);
        marginGrid->Add(m_marginLeftSpin);
        marginBox->Add(marginGrid, wxSizerFlags{}.Border());
        pageSizer->Add(marginBox, wxSizerFlags{}.Border());

        // padding (top, right, bottom, left)
        auto* paddingBox = new wxStaticBoxSizer(wxVERTICAL, pagePage, _(L"Padding"));
        auto* paddingGrid = new wxFlexGridSizer(4, wxSize{ FromDIP(4), FromDIP(4) });
        paddingGrid->Add(new wxStaticText(paddingBox->GetStaticBox(), wxID_ANY, _(L"Top:")),
                         wxSizerFlags{}.CenterVertical());
        m_paddingTopSpin = new wxSpinCtrl(paddingBox->GetStaticBox(), wxID_ANY);
        m_paddingTopSpin->SetRange(0, 100);
        m_paddingTopSpin->SetValue(0);
        paddingGrid->Add(m_paddingTopSpin);
        paddingGrid->Add(new wxStaticText(paddingBox->GetStaticBox(), wxID_ANY, _(L"Right:")),
                         wxSizerFlags{}.CenterVertical());
        m_paddingRightSpin = new wxSpinCtrl(paddingBox->GetStaticBox(), wxID_ANY);
        m_paddingRightSpin->SetRange(0, 100);
        m_paddingRightSpin->SetValue(0);
        paddingGrid->Add(m_paddingRightSpin);
        paddingGrid->Add(new wxStaticText(paddingBox->GetStaticBox(), wxID_ANY, _(L"Bottom:")),
                         wxSizerFlags{}.CenterVertical());
        m_paddingBottomSpin = new wxSpinCtrl(paddingBox->GetStaticBox(), wxID_ANY);
        m_paddingBottomSpin->SetRange(0, 100);
        m_paddingBottomSpin->SetValue(0);
        paddingGrid->Add(m_paddingBottomSpin);
        paddingGrid->Add(new wxStaticText(paddingBox->GetStaticBox(), wxID_ANY, _(L"Left:")),
                         wxSizerFlags{}.CenterVertical());
        m_paddingLeftSpin = new wxSpinCtrl(paddingBox->GetStaticBox(), wxID_ANY);
        m_paddingLeftSpin->SetRange(0, 100);
        m_paddingLeftSpin->SetValue(0);
        paddingGrid->Add(m_paddingLeftSpin);
        paddingBox->Add(paddingGrid, wxSizerFlags{}.Border());
        pageSizer->Add(paddingBox, wxSizerFlags{}.Border());

        // fit row to content
        m_fitRowToContentCheck = new wxCheckBox(pagePage, wxID_ANY, _(L"Fit row to content"));
        m_fitRowToContentCheck->SetValue(false);
        pageSizer->Add(m_fitRowToContentCheck, wxSizerFlags{}.Border());

        // fixed width
        m_fixedWidthCheck = new wxCheckBox(pagePage, wxID_ANY, _(L"Fixed width"));
        m_fixedWidthCheck->SetValue(false);
        pageSizer->Add(m_fixedWidthCheck, wxSizerFlags{}.Border());

        SetSizer(mainSizer);
        }

    //-------------------------------------------
    PageHorizontalAlignment InsertItemDlg::GetHorizontalPageAlignment() const
        {
        switch (m_horizontalAlignChoice->GetSelection())
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
        switch (m_verticalAlignChoice->GetSelection())
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
        return { m_marginTopSpin->GetValue(), m_marginRightSpin->GetValue(),
                 m_marginBottomSpin->GetValue(), m_marginLeftSpin->GetValue() };
        }

    //-------------------------------------------
    std::array<int, 4> InsertItemDlg::GetPadding() const
        {
        return { m_paddingTopSpin->GetValue(), m_paddingRightSpin->GetValue(),
                 m_paddingBottomSpin->GetValue(), m_paddingLeftSpin->GetValue() };
        }

    //-------------------------------------------
    bool InsertItemDlg::GetFitRowToContent() const { return m_fitRowToContentCheck->GetValue(); }

    //-------------------------------------------
    bool InsertItemDlg::GetFixedWidth() const { return m_fixedWidthCheck->GetValue(); }

    //-------------------------------------------
    void InsertItemDlg::FinalizeControls()
        {
        GetSizer()->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                        wxSizerFlags{}.Expand().Border());
        GetSizer()->SetSizeHints(this);
        }
    } // namespace Wisteria::UI
