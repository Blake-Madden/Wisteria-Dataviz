///////////////////////////////////////////////////////////////////////////////
// Name:        insertshapedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertshapedlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertShapeDlg::InsertShapeDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style,
                                   EditMode editMode, const int options)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode),
          m_options(options)
        {
        CreateControls();
        FinalizeControls();
        TransferDataToWindow();

        const auto currentSize = GetSize();
        SetSize(currentSize.GetWidth() * 2, currentSize.GetHeight());
        SetMinSize(wxSize{ currentSize.GetWidth() * 2, currentSize.GetHeight() });

        Centre();
        }

    //-------------------------------------------
    void InsertShapeDlg::PopulateShapeChoice()
        {
        // pairs of human-readable name and IconShape enum value,
        // sorted alphabetically by display name
        const std::vector<std::pair<wxString, Icons::IconShape>> shapes = {
            { _(L"Apple"), Icons::IconShape::Apple },
            { _(L"Arrow (right)"), Icons::IconShape::ArrowRight },
            { _(L"Asterisk"), Icons::IconShape::Asterisk },
            { _(L"Banner"), Icons::IconShape::Banner },
            { _(L"Barn"), Icons::IconShape::Barn },
            { _(L"Blackboard"), Icons::IconShape::Blackboard },
            { _(L"Blank"), Icons::IconShape::Blank },
            { _(L"Book"), Icons::IconShape::Book },
            { _(L"Box plot"), Icons::IconShape::BoxPlot },
            { _(L"Car"), Icons::IconShape::Car },
            { _(L"Cheese pizza"), Icons::IconShape::CheesePizza },
            { _(L"Chevron (downward)"), Icons::IconShape::ChevronDownward },
            { _(L"Chevron (upward)"), Icons::IconShape::ChevronUpward },
            { _(L"Chocolate chip cookie"), Icons::IconShape::ChocolateChipCookie },
            { _(L"Circle"), Icons::IconShape::Circle },
            { _(L"Clock"), Icons::IconShape::Clock },
            { _(L"Coffee shop cup"), Icons::IconShape::CoffeeShopCup },
            { _(L"Crescent (bottom)"), Icons::IconShape::CrescentBottom },
            { _(L"Crescent (right)"), Icons::IconShape::CrescentRight },
            { _(L"Crescent (top)"), Icons::IconShape::CrescentTop },
            { _(L"Crossed out"), Icons::IconShape::CrossedOut },
            { _(L"Curly brace (bottom)"), Icons::IconShape::BottomCurlyBrace },
            { _(L"Curly brace (left)"), Icons::IconShape::LeftCurlyBrace },
            { _(L"Curly brace (right)"), Icons::IconShape::RightCurlyBrace },
            { _(L"Curly brace (top)"), Icons::IconShape::TopCurlyBrace },
            { _(L"Curving road"), Icons::IconShape::CurvingRoad },
            { _(L"Diamond"), Icons::IconShape::Diamond },
            { _(L"Factory"), Icons::IconShape::Factory },
            { _(L"Fall leaf"), Icons::IconShape::FallLeaf },
            { _(L"Farm"), Icons::IconShape::Farm },
            { _(L"Flame"), Icons::IconShape::Flame },
            { _(L"Flower"), Icons::IconShape::Flower },
            { _(L"Go road sign"), Icons::IconShape::GoRoadSign },
            { _(L"Graduation cap"), Icons::IconShape::GraduationCap },
            { _(L"Granny Smith apple"), Icons::IconShape::GrannySmithApple },
            { _(L"Hawaiian pizza"), Icons::IconShape::HawaiianPizza },
            { _(L"Heart"), Icons::IconShape::Heart },
            { _(L"Hexagon"), Icons::IconShape::Hexagon },
            { _(L"Horizontal line"), Icons::IconShape::HorizontalLine },
            { _(L"House"), Icons::IconShape::House },
            { _(L"Hundred dollar bill"), Icons::IconShape::HundredDollarBill },
            { _(L"Immaculate Heart"), Icons::IconShape::ImmaculateHeart },
            { _(L"Immaculate Heart (with sword)"), Icons::IconShape::ImmaculateHeartWithSword },
            { _(L"IV bag"), Icons::IconShape::IVBag },
            { _(L"Jack-o'-lantern"), Icons::IconShape::JackOLantern },
            { _(L"Location marker"), Icons::IconShape::LocationMarker },
            { _(L"Man"), Icons::IconShape::Man },
            { _(L"Marker rectangle"), Icons::IconShape::MarkerRectangle },
            { _(L"Monitor"), Icons::IconShape::Monitor },
            { _(L"Newspaper"), Icons::IconShape::Newspaper },
            { _(L"Number range"), Icons::IconShape::NumberRange },
            { _(L"Office"), Icons::IconShape::Office },
            { _(L"Pencil rectangle"), Icons::IconShape::PencilRectangle },
            { _(L"Pepperoni pizza"), Icons::IconShape::PepperoniPizza },
            { _(L"Pill"), Icons::IconShape::Pill },
            { _(L"Plus"), Icons::IconShape::Plus },
            { _(L"Pumpkin"), Icons::IconShape::Pumpkin },
            { _(L"Ruler"), Icons::IconShape::Ruler },
            { _(L"Snowflake"), Icons::IconShape::Snowflake },
            { _(L"Square"), Icons::IconShape::Square },
            { _(L"Sun"), Icons::IconShape::Sun },
            { _(L"Sunflower"), Icons::IconShape::Sunflower },
            { _(L"Sword"), Icons::IconShape::Sword },
            { _(L"Tack"), Icons::IconShape::Tack },
            { _(L"Thermometer (cold)"), Icons::IconShape::ColdThermometer },
            { _(L"Thermometer (hot)"), Icons::IconShape::HotThermometer },
            { _(L"Tire"), Icons::IconShape::Tire },
            { _(L"Tractor"), Icons::IconShape::Tractor },
            { _(L"Triangle (downward)"), Icons::IconShape::TriangleDownward },
            { _(L"Triangle (left)"), Icons::IconShape::TriangleLeft },
            { _(L"Triangle (right)"), Icons::IconShape::TriangleRight },
            { _(L"Triangle (upward)"), Icons::IconShape::TriangleUpward },
            { _(L"Vertical line"), Icons::IconShape::VerticalLine },
            { _(L"Warning road sign"), Icons::IconShape::WarningRoadSign },
            { _(L"Watercolor rectangle"), Icons::IconShape::WaterColorRectangle },
            { _(L"Watercolor rectangle (thick)"), Icons::IconShape::ThickWaterColorRectangle },
            { _(L"Woman"), Icons::IconShape::Woman },
            { _(L"Woman (business)"), Icons::IconShape::BusinessWoman }
        };

        m_shapeMap.clear();
        m_shapeMap.reserve(shapes.size());
        for (const auto& [name, shape] : shapes)
            {
            m_shapeChoice->Append(name);
            m_shapeMap.push_back(shape);
            }

        // default to Square
        for (size_t i = 0; i < m_shapeMap.size(); ++i)
            {
            if (m_shapeMap[i] == Icons::IconShape::Square)
                {
                m_shapeIndex = static_cast<int>(i);
                break;
                }
            }
        }

    //-------------------------------------------
    void InsertShapeDlg::CreateControls()
        {
        InsertItemDlg::CreateControls();

        auto* shapePage = new wxPanel(GetSideBarBook());
        auto* shapeSizer = new wxBoxSizer(wxVERTICAL);
        shapePage->SetSizer(shapeSizer);
        GetSideBarBook()->AddPage(shapePage, _(L"Shape Options"), ID_SHAPE_SECTION, true);

        // page options (background color, outline, etc.) are not relevant for shapes,
        // which are self-contained items with their own pen/brush controls.
        GetSideBarBook()->DeletePage(0);

        // shape type
        auto* shapeGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        shapeGrid->Add(new wxStaticText(shapePage, wxID_ANY, _(L"Shape:")),
                       wxSizerFlags{}.CenterVertical());
        m_shapeChoice = new wxChoice(shapePage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                     nullptr, 0, wxGenericValidator(&m_shapeIndex));
        PopulateShapeChoice();
        shapeGrid->Add(m_shapeChoice);

        shapeSizer->Add(shapeGrid, wxSizerFlags{}.Border());

        // size
        if ((m_options & ShapeDlgIncludeSize) != 0)
            {
            auto* sizeBox = new wxStaticBoxSizer(wxVERTICAL, shapePage, _(L"Size"));
            auto* sizeGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

            sizeGrid->Add(new wxStaticText(sizeBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                          wxSizerFlags{}.CenterVertical());
            m_widthSpin = new wxSpinCtrl(sizeBox->GetStaticBox(), wxID_ANY);
            m_widthSpin->SetRange(1, 10'000);
            m_widthSpin->SetValue(32);
            sizeGrid->Add(m_widthSpin);

            sizeGrid->Add(new wxStaticText(sizeBox->GetStaticBox(), wxID_ANY, _(L"Height:")),
                          wxSizerFlags{}.CenterVertical());
            m_heightSpin = new wxSpinCtrl(sizeBox->GetStaticBox(), wxID_ANY);
            m_heightSpin->SetRange(1, 10'000);
            m_heightSpin->SetValue(32);
            sizeGrid->Add(m_heightSpin);

            sizeBox->Add(sizeGrid, wxSizerFlags{}.Border());
            shapeSizer->Add(sizeBox, wxSizerFlags{}.Expand().Border());
            }

        // pen options
        if ((m_options & ShapeDlgIncludePen) != 0)
            {
            auto* penBox = new wxStaticBoxSizer(wxVERTICAL, shapePage, _(L"Pen (outline)"));
            auto* penGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

            penGrid->Add(new wxStaticText(penBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                         wxSizerFlags{}.CenterVertical());
            m_penColorPicker = new wxColourPickerCtrl(penBox->GetStaticBox(), wxID_ANY, *wxBLACK);
            penGrid->Add(m_penColorPicker);

            penGrid->Add(new wxStaticText(penBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                         wxSizerFlags{}.CenterVertical());
            m_penWidthSpin = new wxSpinCtrl(penBox->GetStaticBox(), wxID_ANY);
            m_penWidthSpin->SetRange(1, 20);
            m_penWidthSpin->SetValue(1);
            penGrid->Add(m_penWidthSpin);

            penGrid->Add(new wxStaticText(penBox->GetStaticBox(), wxID_ANY, _(L"Style:")),
                         wxSizerFlags{}.CenterVertical());
            auto* penStyleChoice =
                new wxChoice(penBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                             nullptr, 0, wxGenericValidator(&m_penStyle));
            penStyleChoice->Append(_(L"Solid"));
            penStyleChoice->Append(_(L"Dot"));
            penStyleChoice->Append(_(L"Long dash"));
            penStyleChoice->Append(_(L"Short dash"));
            penStyleChoice->Append(_(L"Dot dash"));
            penGrid->Add(penStyleChoice);

            penBox->Add(penGrid, wxSizerFlags{}.Border());
            shapeSizer->Add(penBox, wxSizerFlags{}.Expand().Border());
            }

        // brush options
        if ((m_options & ShapeDlgIncludeBrush) != 0)
            {
            auto* brushBox = new wxStaticBoxSizer(wxVERTICAL, shapePage, _(L"Brush (fill)"));
            auto* brushGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

            brushGrid->Add(new wxStaticText(brushBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                           wxSizerFlags{}.CenterVertical());
            m_brushColorPicker =
                new wxColourPickerCtrl(brushBox->GetStaticBox(), wxID_ANY, *wxWHITE);
            brushGrid->Add(m_brushColorPicker);

            brushGrid->Add(new wxStaticText(brushBox->GetStaticBox(), wxID_ANY, _(L"Style:")),
                           wxSizerFlags{}.CenterVertical());
            auto* brushStyleChoice =
                new wxChoice(brushBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_brushStyle));
            brushStyleChoice->Append(_(L"Solid"));
            brushStyleChoice->Append(_(L"Transparent"));
            brushStyleChoice->Append(_(L"Backward diagonal hatch"));
            brushStyleChoice->Append(_(L"Cross-diagonal hatch"));
            brushStyleChoice->Append(_(L"Forward diagonal hatch"));
            brushStyleChoice->Append(_(L"Cross hatch"));
            brushStyleChoice->Append(_(L"Horizontal hatch"));
            brushStyleChoice->Append(_(L"Vertical hatch"));
            brushGrid->Add(brushStyleChoice);

            brushBox->Add(brushGrid, wxSizerFlags{}.Border());
            shapeSizer->Add(brushBox, wxSizerFlags{}.Expand().Border());
            }

        // label options
        if ((m_options & ShapeDlgIncludeLabel) != 0)
            {
            auto* labelBox = new wxStaticBoxSizer(wxVERTICAL, shapePage, _(L"Label"));
            auto* labelGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

            labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Text:")),
                           wxSizerFlags{}.CenterVertical());
            m_labelTextCtrl = new wxTextCtrl(labelBox->GetStaticBox(), wxID_ANY, wxString{},
                                             wxDefaultPosition, wxSize{ FromDIP(200), -1 });
            labelGrid->Add(m_labelTextCtrl, wxSizerFlags{}.Expand());

            labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                           wxSizerFlags{}.CenterVertical());
            m_labelColorPicker =
                new wxColourPickerCtrl(labelBox->GetStaticBox(), wxID_ANY, *wxBLACK);
            labelGrid->Add(m_labelColorPicker);

            labelBox->Add(labelGrid, wxSizerFlags{}.Border());
            shapeSizer->Add(labelBox, wxSizerFlags{}.Expand().Border());
            }

        // fillable options
        if ((m_options & ShapeDlgIncludeFillable) != 0)
            {
            auto* fillBox = new wxStaticBoxSizer(wxVERTICAL, shapePage, _(L"Fillable"));

            auto* fillableCheck = new wxCheckBox(fillBox->GetStaticBox(), wxID_ANY,
                                                 _(L"Make shape fillable"), wxDefaultPosition,
                                                 wxDefaultSize, 0, wxGenericValidator(&m_fillable));
            fillBox->Add(fillableCheck, wxSizerFlags{}.Border());

            auto* fillGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
            fillGrid->Add(new wxStaticText(fillBox->GetStaticBox(), wxID_ANY, _(L"Fill percent:")),
                          wxSizerFlags{}.CenterVertical());
            m_fillPercentSpin = new wxSpinCtrlDouble(
                fillBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                wxSP_ARROW_KEYS, 0.0, 1.0, 0.0, 0.05);
            fillGrid->Add(m_fillPercentSpin);

            fillBox->Add(fillGrid, wxSizerFlags{}.Border());
            shapeSizer->Add(fillBox, wxSizerFlags{}.Expand().Border());

            // fill controls start disabled
            OnEnableFillable(false);

            fillableCheck->Bind(wxEVT_CHECKBOX,
                                [this](wxCommandEvent& evt) { OnEnableFillable(evt.IsChecked()); });
            }

        // repeat count
        if ((m_options & ShapeDlgIncludeRepeat) != 0)
            {
            auto* repeatBox = new wxStaticBoxSizer(wxVERTICAL, shapePage, _(L"Repeat"));
            auto* repeatGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

            repeatGrid->Add(new wxStaticText(repeatBox->GetStaticBox(), wxID_ANY, _(L"Count:")),
                            wxSizerFlags{}.CenterVertical());
            m_repeatTextCtrl = new wxTextCtrl(repeatBox->GetStaticBox(), wxID_ANY, L"1",
                                              wxDefaultPosition, wxSize{ FromDIP(200), -1 });
            repeatGrid->Add(m_repeatTextCtrl, wxSizerFlags{}.Expand());

            repeatBox->Add(repeatGrid, wxSizerFlags{}.Border());
            shapeSizer->Add(repeatBox, wxSizerFlags{}.Expand().Border());
            }

        // alignment
        if ((m_options & ShapeDlgIncludeAlignment) != 0)
            {
            auto* alignBox = new wxStaticBoxSizer(wxVERTICAL, shapePage, _(L"Alignment"));
            auto* alignGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

            alignGrid->Add(new wxStaticText(alignBox->GetStaticBox(), wxID_ANY, _(L"Horizontal:")),
                           wxSizerFlags{}.CenterVertical());
            auto* hAlignChoice =
                new wxChoice(alignBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_horizontalAlign));
            hAlignChoice->Append(_(L"Left aligned"));
            hAlignChoice->Append(_(L"Centered"));
            hAlignChoice->Append(_(L"Right aligned"));
            alignGrid->Add(hAlignChoice);

            alignGrid->Add(new wxStaticText(alignBox->GetStaticBox(), wxID_ANY, _(L"Vertical:")),
                           wxSizerFlags{}.CenterVertical());
            auto* vAlignChoice =
                new wxChoice(alignBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_verticalAlign));
            vAlignChoice->Append(_(L"Top aligned"));
            vAlignChoice->Append(_(L"Centered"));
            vAlignChoice->Append(_(L"Bottom aligned"));
            alignGrid->Add(vAlignChoice);

            alignBox->Add(alignGrid, wxSizerFlags{}.Border());
            shapeSizer->Add(alignBox, wxSizerFlags{}.Expand().Border());
            }
        }

    //-------------------------------------------
    void InsertShapeDlg::OnEnableFillable(const bool enable)
        {
        if (m_fillPercentSpin != nullptr)
            {
            m_fillPercentSpin->Enable(enable);
            }
        }

    //-------------------------------------------
    bool InsertShapeDlg::Validate() { return true; }

    //-------------------------------------------
    Icons::IconShape InsertShapeDlg::GetIconShape() const noexcept
        {
        if (m_shapeIndex >= 0 && static_cast<size_t>(m_shapeIndex) < m_shapeMap.size())
            {
            return m_shapeMap[static_cast<size_t>(m_shapeIndex)];
            }
        return Icons::IconShape::Square;
        }

    //-------------------------------------------
    int InsertShapeDlg::GetShapeWidth() const
        {
        return m_widthSpin != nullptr ? m_widthSpin->GetValue() : 32;
        }

    //-------------------------------------------
    int InsertShapeDlg::GetShapeHeight() const
        {
        return m_heightSpin != nullptr ? m_heightSpin->GetValue() : 32;
        }

    //-------------------------------------------
    wxColour InsertShapeDlg::GetPenColor() const
        {
        return m_penColorPicker != nullptr ? m_penColorPicker->GetColour() : *wxBLACK;
        }

    //-------------------------------------------
    int InsertShapeDlg::GetPenWidth() const
        {
        return m_penWidthSpin != nullptr ? m_penWidthSpin->GetValue() : 1;
        }

    //-------------------------------------------
    wxPenStyle InsertShapeDlg::GetPenStyle() const noexcept
        {
        switch (m_penStyle)
            {
        case 1:
            return wxPENSTYLE_DOT;
        case 2:
            return wxPENSTYLE_LONG_DASH;
        case 3:
            return wxPENSTYLE_SHORT_DASH;
        case 4:
            return wxPENSTYLE_DOT_DASH;
        default:
            return wxPENSTYLE_SOLID;
            }
        }

    //-------------------------------------------
    wxColour InsertShapeDlg::GetBrushColor() const
        {
        return m_brushColorPicker != nullptr ? m_brushColorPicker->GetColour() : *wxWHITE;
        }

    //-------------------------------------------
    wxBrushStyle InsertShapeDlg::GetBrushStyle() const noexcept
        {
        switch (m_brushStyle)
            {
        case 1:
            return wxBRUSHSTYLE_TRANSPARENT;
        case 2:
            return wxBRUSHSTYLE_BDIAGONAL_HATCH;
        case 3:
            return wxBRUSHSTYLE_CROSSDIAG_HATCH;
        case 4:
            return wxBRUSHSTYLE_FDIAGONAL_HATCH;
        case 5:
            return wxBRUSHSTYLE_CROSS_HATCH;
        case 6:
            return wxBRUSHSTYLE_HORIZONTAL_HATCH;
        case 7:
            return wxBRUSHSTYLE_VERTICAL_HATCH;
        default:
            return wxBRUSHSTYLE_SOLID;
            }
        }

    //-------------------------------------------
    wxString InsertShapeDlg::GetLabelText() const
        {
        return m_labelTextCtrl != nullptr ? m_labelTextCtrl->GetValue() : wxString{};
        }

    //-------------------------------------------
    wxColour InsertShapeDlg::GetLabelFontColor() const
        {
        return m_labelColorPicker != nullptr ? m_labelColorPicker->GetColour() : *wxBLACK;
        }

    //-------------------------------------------
    double InsertShapeDlg::GetFillPercent() const
        {
        return m_fillPercentSpin != nullptr ? m_fillPercentSpin->GetValue() : 0.0;
        }

    //-------------------------------------------
    wxString InsertShapeDlg::GetRepeatString() const
        {
        return m_repeatTextCtrl != nullptr ? m_repeatTextCtrl->GetValue() : wxString{ L"1" };
        }

    //-------------------------------------------
    void InsertShapeDlg::SetRepeatString(const wxString& repeat)
        {
        if (m_repeatTextCtrl != nullptr)
            {
            m_repeatTextCtrl->SetValue(repeat);
            }
        }

    //-------------------------------------------
    PageHorizontalAlignment InsertShapeDlg::GetHorizontalAlignment() const noexcept
        {
        switch (m_horizontalAlign)
            {
        case 0:
            return PageHorizontalAlignment::LeftAligned;
        case 2:
            return PageHorizontalAlignment::RightAligned;
        default:
            return PageHorizontalAlignment::Centered;
            }
        }

    //-------------------------------------------
    PageVerticalAlignment InsertShapeDlg::GetVerticalAlignment() const noexcept
        {
        switch (m_verticalAlign)
            {
        case 0:
            return PageVerticalAlignment::TopAligned;
        case 2:
            return PageVerticalAlignment::BottomAligned;
        default:
            return PageVerticalAlignment::Centered;
            }
        }

    //-------------------------------------------
    void InsertShapeDlg::SetIconShape(const Icons::IconShape shape)
        {
        for (size_t i = 0; i < m_shapeMap.size(); ++i)
            {
            if (m_shapeMap[i] == shape)
                {
                m_shapeIndex = static_cast<int>(i);
                if (m_shapeChoice != nullptr)
                    {
                    m_shapeChoice->SetSelection(m_shapeIndex);
                    }
                break;
                }
            }
        }

    //-------------------------------------------
    void InsertShapeDlg::SetBrushColor(const wxColour& color)
        {
        if (m_brushColorPicker != nullptr && color.IsOk())
            {
            m_brushColorPicker->SetColour(color);
            }
        }

    //-------------------------------------------
    void InsertShapeDlg::LoadFromShape(const Wisteria::GraphItems::Shape& shape)
        {
        // alignment
        switch (shape.GetPageHorizontalAlignment())
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
        switch (shape.GetPageVerticalAlignment())
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

        // find the shape in the map
        const auto iconShape = shape.GetShape();
        for (size_t i = 0; i < m_shapeMap.size(); ++i)
            {
            if (m_shapeMap[i] == iconShape)
                {
                m_shapeIndex = static_cast<int>(i);
                break;
                }
            }

        // size (prefer cached original values from the JSON)
        const auto widthTmpl = shape.GetPropertyTemplate(L"size.width");
        const auto heightTmpl = shape.GetPropertyTemplate(L"size.height");
        const auto sz = shape.GetSizeDIPS();
        if (m_widthSpin != nullptr)
            {
            m_widthSpin->SetValue(widthTmpl.empty() ? sz.GetWidth() : wxAtoi(widthTmpl));
            }
        if (m_heightSpin != nullptr)
            {
            m_heightSpin->SetValue(heightTmpl.empty() ? sz.GetHeight() : wxAtoi(heightTmpl));
            }

        // pen
        const auto& pen = shape.GetPen();
        if (pen.IsOk() && pen != wxNullPen)
            {
            if (m_penColorPicker != nullptr)
                {
                m_penColorPicker->SetColour(pen.GetColour());
                }
            if (m_penWidthSpin != nullptr)
                {
                m_penWidthSpin->SetValue(pen.GetWidth());
                }
            switch (pen.GetStyle())
                {
            case wxPENSTYLE_DOT:
                m_penStyle = 1;
                break;
            case wxPENSTYLE_LONG_DASH:
                m_penStyle = 2;
                break;
            case wxPENSTYLE_SHORT_DASH:
                m_penStyle = 3;
                break;
            case wxPENSTYLE_DOT_DASH:
                m_penStyle = 4;
                break;
            default:
                m_penStyle = 0;
                break;
                }
            }

        // brush
        const auto& brush = shape.GetBrush();
        if (brush.IsOk() && brush != wxNullBrush)
            {
            if (m_brushColorPicker != nullptr)
                {
                m_brushColorPicker->SetColour(brush.GetColour());
                }
            switch (brush.GetStyle())
                {
            case wxBRUSHSTYLE_TRANSPARENT:
                m_brushStyle = 1;
                break;
            case wxBRUSHSTYLE_BDIAGONAL_HATCH:
                m_brushStyle = 2;
                break;
            case wxBRUSHSTYLE_CROSSDIAG_HATCH:
                m_brushStyle = 3;
                break;
            case wxBRUSHSTYLE_FDIAGONAL_HATCH:
                m_brushStyle = 4;
                break;
            case wxBRUSHSTYLE_CROSS_HATCH:
                m_brushStyle = 5;
                break;
            case wxBRUSHSTYLE_HORIZONTAL_HATCH:
                m_brushStyle = 6;
                break;
            case wxBRUSHSTYLE_VERTICAL_HATCH:
                m_brushStyle = 7;
                break;
            default:
                m_brushStyle = 0;
                break;
                }
            }

        // label
        if (m_labelTextCtrl != nullptr)
            {
            const auto textTmpl = shape.GetPropertyTemplate(L"label.text");
            m_labelTextCtrl->SetValue(textTmpl.empty() ? shape.GetText() : textTmpl);
            }
        if (m_labelColorPicker != nullptr && shape.GetFontColor().IsOk())
            {
            m_labelColorPicker->SetColour(shape.GetFontColor());
            }

        // not fillable
        m_fillable = false;
        OnEnableFillable(false);

        TransferDataToWindow();
        }

    //-------------------------------------------
    void InsertShapeDlg::LoadFromFillableShape(const Wisteria::GraphItems::FillableShape& shape)
        {
        // load the Shape base properties
        LoadFromShape(shape);

        // enable fillable and set the fill percent
        m_fillable = true;
        if (m_fillPercentSpin != nullptr)
            {
            m_fillPercentSpin->SetValue(shape.GetFillPercent());
            }
        OnEnableFillable(true);

        TransferDataToWindow();
        }

    //-------------------------------------------
    void InsertShapeDlg::LoadFromShapeInfo(const Wisteria::GraphItems::ShapeInfo& shapeInfo)
        {
        // find the shape in the map
        const auto iconShape = shapeInfo.GetShape();
        for (size_t i = 0; i < m_shapeMap.size(); ++i)
            {
            if (m_shapeMap[i] == iconShape)
                {
                m_shapeIndex = static_cast<int>(i);
                break;
                }
            }

        // size
        const auto sz = shapeInfo.GetSizeDIPs();
        if (m_widthSpin != nullptr)
            {
            m_widthSpin->SetValue(sz.GetWidth());
            }
        if (m_heightSpin != nullptr)
            {
            m_heightSpin->SetValue(sz.GetHeight());
            }

        // pen
        const auto& pen = shapeInfo.GetPen();
        if (pen.IsOk() && pen != wxNullPen)
            {
            if (m_penColorPicker != nullptr)
                {
                m_penColorPicker->SetColour(pen.GetColour());
                }
            if (m_penWidthSpin != nullptr)
                {
                m_penWidthSpin->SetValue(pen.GetWidth());
                }
            switch (pen.GetStyle())
                {
            case wxPENSTYLE_DOT:
                m_penStyle = 1;
                break;
            case wxPENSTYLE_LONG_DASH:
                m_penStyle = 2;
                break;
            case wxPENSTYLE_SHORT_DASH:
                m_penStyle = 3;
                break;
            case wxPENSTYLE_DOT_DASH:
                m_penStyle = 4;
                break;
            default:
                m_penStyle = 0;
                break;
                }
            }

        // brush
        const auto& brush = shapeInfo.GetBrush();
        if (brush.IsOk() && brush != wxNullBrush)
            {
            if (m_brushColorPicker != nullptr)
                {
                m_brushColorPicker->SetColour(brush.GetColour());
                }
            switch (brush.GetStyle())
                {
            case wxBRUSHSTYLE_TRANSPARENT:
                m_brushStyle = 1;
                break;
            case wxBRUSHSTYLE_BDIAGONAL_HATCH:
                m_brushStyle = 2;
                break;
            case wxBRUSHSTYLE_CROSSDIAG_HATCH:
                m_brushStyle = 3;
                break;
            case wxBRUSHSTYLE_FDIAGONAL_HATCH:
                m_brushStyle = 4;
                break;
            case wxBRUSHSTYLE_CROSS_HATCH:
                m_brushStyle = 5;
                break;
            case wxBRUSHSTYLE_HORIZONTAL_HATCH:
                m_brushStyle = 6;
                break;
            case wxBRUSHSTYLE_VERTICAL_HATCH:
                m_brushStyle = 7;
                break;
            default:
                m_brushStyle = 0;
                break;
                }
            }

        // label
        if (m_labelTextCtrl != nullptr)
            {
            m_labelTextCtrl->SetValue(shapeInfo.GetText());
            }

        // fill percent
        const auto fillPct = shapeInfo.GetFillPercent();
        if (fillPct < 1.0)
            {
            m_fillable = true;
            if (m_fillPercentSpin != nullptr)
                {
                m_fillPercentSpin->SetValue(fillPct);
                }
            OnEnableFillable(true);
            }
        else
            {
            m_fillable = false;
            OnEnableFillable(false);
            }

        // repeat count
        if (m_repeatTextCtrl != nullptr)
            {
            const auto repeatTmpl = shapeInfo.GetPropertyTemplate(L"repeat");
            m_repeatTextCtrl->SetValue(
                repeatTmpl.empty() ? std::to_wstring(shapeInfo.GetRepeatCount()) : repeatTmpl);
            }

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
