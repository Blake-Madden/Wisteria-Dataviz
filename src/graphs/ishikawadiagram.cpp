///////////////////////////////////////////////////////////////////////////////
// Name:        ishikawadiagram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "ishikawadiagram.h"
#include <algorithm>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::IshikawaDiagram, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    IshikawaDiagram::IshikawaDiagram(Canvas * canvas) : Graph2D(canvas)
        {
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        GetBottomXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetLeftYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetTopXAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        GetRightYAxis().SetLabelDisplay(AxisLabelDisplay::NoDisplay);
        }

    //----------------------------------------------------------------
    wxString IshikawaDiagram::GetDefaultBranchLabel(const size_t index)
        {
        // the 8 Ms
        switch (index)
            {
        case 0:
            return _(L"Manpower");
        case 1:
            return _(L"Machine");
        case 2:
            return _(L"Material");
        case 3:
            return _(L"Method");
        case 4:
            return _(L"Measurement");
        case 5:
            return _(L"Mother Nature");
        case 6:
            return _(L"Management");
        case 7:
            return _(L"Maintenance");
        default:
            return {};
            }
        }

    //----------------------------------------------------------------
    bool IshikawaDiagram::IsDefaultBranchName(const size_t index, const wxString& name)
        {
        const wxString defaultLabel{ GetDefaultBranchLabel(index) };
        return (!defaultLabel.empty() && name.CmpNoCase(defaultLabel) == 0) ||
               (index == 0 && (name.CmpNoCase(L"Man") == 0 || name.CmpNoCase(_(L"People")) == 0)) ||
               (index == 4 && name.CmpNoCase(L"Measure") == 0) ||
               (index == 5 &&
                (name.CmpNoCase(L"Enviro") == 0 || name.CmpNoCase(_(L"Environment")) == 0));
        }

    //----------------------------------------------------------------
    std::vector<wxString> IshikawaDiagram::ParseConstants(const wxString& text)
        {
        std::vector<wxString> constants;
        wxString current;
        bool inQuotes{ false };
        const auto flush = [&constants, &current]()
        {
            if (!current.empty())
                {
                constants.push_back(current);
                current.clear();
                }
        };
        for (size_t i = 0; i < text.length(); ++i)
            {
            const wxUniChar character{ text[i] };
            if (inQuotes)
                {
                if (character != L'"')
                    {
                    current += character;
                    }
                // a doubled quote is a literal quote, and a single one ends the quoted text
                else if (i + 1 < text.length() && text[i + 1] == L'"')
                    {
                    current += L'"';
                    ++i;
                    }
                else
                    {
                    inQuotes = false;
                    flush();
                    }
                }
            else if (character == L'"')
                {
                inQuotes = true;
                }
            else if (character == L' ' || character == L'\t' || character == L'\n' ||
                     character == L'\r')
                {
                flush();
                }
            else
                {
                current += character;
                }
            }
        flush();
        return constants;
        }

    //----------------------------------------------------------------
    wxString IshikawaDiagram::FormatConstants(const std::vector<wxString>& constants)
        {
        wxString text;
        for (const auto& constant : constants)
            {
            if (!text.empty())
                {
                text += L' ';
                }
            // quote anything that would otherwise be split, and double any quotes inside of it
            if (constant.find_first_of(L" \t\r\n\"") != wxString::npos)
                {
                wxString escaped{ constant };
                escaped.Replace(L"\"", L"\"\"");
                text += L'"' + escaped + L'"';
                }
            else
                {
                text += constant;
                }
            }
        return text;
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                  const wxString& effect, const std::vector<Branch>& branches)
        {
        if (data == nullptr)
            {
            return;
            }
        // built separately, so that the diagram is unchanged if a column can't be found
        std::vector<BranchItem> newBranches;
        newBranches.reserve(branches.size());

        for (size_t branchIndex = 0; branchIndex < branches.size(); ++branchIndex)
            {
            const auto& branch{ branches[branchIndex] };
            // a branch without a column is shown without any causes
            const auto causeColumn = data->GetCategoricalColumn(branch.m_column);
            if (!branch.m_column.empty() && causeColumn == data->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': cause column not found for Ishikawa diagram."),
                                     branch.m_column)
                        .ToUTF8());
                }

            BranchItem item;
            item.m_label = branch.m_label.empty() ? (GetDefaultBranchLabel(branchIndex).empty() ?
                                                         branch.m_column :
                                                         GetDefaultBranchLabel(branchIndex)) :
                                                    branch.m_label;
            // a cause that is listed more than once is only shown once,
            // so that sub-causes belong to exactly one cause
            const auto addCause = [&item](const wxString& cause)
            {
                if (!cause.empty() &&
                    std::ranges::none_of(item.m_causes, [&cause](const auto& existing)
                                         { return existing.m_text == cause; }))
                    {
                    item.m_causes.push_back(CauseItem{ cause, {} });
                    }
            };
            if (branch.m_column.empty())
                {
                for (const auto& cause : branch.m_constants)
                    {
                    addCause(cause);
                    }
                }
            else
                {
                for (size_t row = 0; row < data->GetRowCount(); ++row)
                    {
                    addCause(causeColumn->GetValueAsLabel(row));
                    }
                }
            for (const auto& subCauses : branch.m_subCauses)
                {
                const auto subColumn = data->GetCategoricalColumn(subCauses.m_column);
                if (!subCauses.m_column.empty() &&
                    subColumn == data->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"'%s': sub-cause column not found for Ishikawa diagram."),
                            subCauses.m_column)
                            .ToUTF8());
                    }
                const auto parentCause =
                    std::ranges::find_if(item.m_causes, [&subCauses](const auto& cause)
                                         { return cause.m_text == subCauses.m_cause; });
                // the cause may no longer exist (e.g., after the data changed),
                // in which case its sub-causes are skipped
                if (parentCause == item.m_causes.end())
                    {
                    continue;
                    }
                auto& parentSubCauses{ parentCause->m_subCauses };
                const auto addSubCause = [&parentSubCauses](const wxString& subCause)
                {
                    if (!subCause.empty())
                        {
                        parentSubCauses.push_back(subCause);
                        }
                };
                if (subCauses.m_column.empty())
                    {
                    for (const auto& subCause : subCauses.m_constants)
                        {
                        addSubCause(subCause);
                        }
                    continue;
                    }
                for (size_t row = 0; row < data->GetRowCount(); ++row)
                    {
                    addSubCause(subColumn->GetValueAsLabel(row));
                    }
                }
            newBranches.push_back(std::move(item));
            }

        // everything was valid, so apply it
        SetDataset(data);
        GetSelectedIds().clear();
        m_effect = effect;
        m_branchSpecs = branches;
        m_branches = std::move(newBranches);
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::SetAutoAccessibilityAttributes()
        {
        if (m_branches.empty())
            {
            return;
            }

        wxString label{ _(L"A cause-and-effect diagram") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");
        label += wxString::Format(_(L". Effect: %s"), m_effect);
        for (const auto& branch : m_branches)
            {
            label += wxString::Format(
                wxPLURAL(L". %s: %zu cause", L". %s: %zu causes", branch.m_causes.size()),
                branch.m_label, branch.m_causes.size());
            }

        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");
        AddAccessibilityAttribute(label, GetReadableReferenceLines(), L". ");
        AddAccessibilityAttribute(label, GetReadableAnnotations(), L". ");
        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }

    //----------------------------------------------------------------
    IshikawaDiagram::TailFin::TailFin(const wxPoint& spinePoint, const wxPoint& topControl,
                                      const wxPoint& bottomControl, const wxPoint& notchPoint,
                                      const wxColour& color)
        : m_spinePoint(spinePoint), m_topControl(topControl), m_bottomControl(bottomControl),
          m_notchPoint(notchPoint), m_color(color)
        {
        GetGraphItemInfo().Selectable(false);
        }

    //----------------------------------------------------------------
    wxRect IshikawaDiagram::TailFin::GetBoundingBox([[maybe_unused]]
                                                    wxDC &
                                                    dc) const
        {
        const int minX =
            std::min({ m_spinePoint.x, m_topControl.x, m_bottomControl.x, m_notchPoint.x });
        const int maxX =
            std::max({ m_spinePoint.x, m_topControl.x, m_bottomControl.x, m_notchPoint.x });
        const int minY =
            std::min({ m_spinePoint.y, m_topControl.y, m_bottomControl.y, m_notchPoint.y });
        const int maxY =
            std::max({ m_spinePoint.y, m_topControl.y, m_bottomControl.y, m_notchPoint.y });
        return wxRect{ wxPoint{ minX, minY }, wxPoint{ maxX, maxY } };
        }

    //----------------------------------------------------------------
    wxRect IshikawaDiagram::TailFin::Draw(wxDC & dc) const
        {
        const auto boundingBox = GetBoundingBox(dc);
        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return boundingBox;
            }

        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(m_spinePoint.x, m_spinePoint.y);
        // upper lobe: curves out to the top tip and back in to the notch
        path.AddQuadCurveToPoint(m_topControl.x, m_topControl.y, m_notchPoint.x, m_notchPoint.y);
        // lower lobe: mirrors the upper lobe back to the spine
        path.AddQuadCurveToPoint(m_bottomControl.x, m_bottomControl.y, m_spinePoint.x,
                                 m_spinePoint.y);
        path.CloseSubpath();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ m_color });
        gc->FillPath(path);

        return boundingBox;
        }

    //----------------------------------------------------------------
    bool IshikawaDiagram::TailFin::HitTest(const wxPoint pt, wxDC& dc) const
        {
        return GetBoundingBox(dc).Contains(pt);
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::TailFin::Offset(const int xOffset, const int yOffset)
        {
        m_spinePoint.x += xOffset;
        m_spinePoint.y += yOffset;
        m_topControl.x += xOffset;
        m_topControl.y += yOffset;
        m_bottomControl.x += xOffset;
        m_bottomControl.y += yOffset;
        m_notchPoint.x += xOffset;
        m_notchPoint.y += yOffset;
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::TailFin::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                                  [[maybe_unused]] wxDC& dc,
                                                  [[maybe_unused]] const double scaling)
        {
        }

    //----------------------------------------------------------------
    IshikawaDiagram::Head::Head(const wxPoint& topLeft, const wxPoint& bottomLeft,
                                const wxPoint& apex, const wxColour& color,
                                const wxColour& featureColor, const FishStyle fishStyle,
                                const double scaling)
        : m_topLeft(topLeft), m_bottomLeft(bottomLeft), m_apex(apex), m_color(color),
          m_featureColor(featureColor), m_fishStyle(fishStyle)
        {
        GetGraphItemInfo().Selectable(false);
        SetScaling(scaling);
        }

    //----------------------------------------------------------------
    wxRect IshikawaDiagram::Head::GetBoundingBox([[maybe_unused]]
                                                 wxDC &
                                                 dc) const
        {
        const int minX = std::min({ m_topLeft.x, m_bottomLeft.x, m_apex.x });
        const int maxX = std::max({ m_topLeft.x, m_bottomLeft.x, m_apex.x });
        const int minY = std::min({ m_topLeft.y, m_bottomLeft.y, m_apex.y });
        const int maxY = std::max({ m_topLeft.y, m_bottomLeft.y, m_apex.y });
        return wxRect{ wxPoint{ minX, minY }, wxPoint{ maxX, maxY } };
        }

    //----------------------------------------------------------------
    wxRect IshikawaDiagram::Head::Draw(wxDC & dc) const
        {
        const auto boundingBox = GetBoundingBox(dc);
        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return boundingBox;
            }

        const wxPoint topVertex{ m_topLeft };
        const wxPoint bottomVertex{ m_bottomLeft };
        const wxPoint apexVertex{ m_apex };

        const auto normalize = [](const wxPoint& from, const wxPoint& to)
        {
            const double deltaX{ static_cast<double>(to.x - from.x) };
            const double deltaY{ static_cast<double>(to.y - from.y) };
            const double length{ std::hypot(deltaX, deltaY) };
            if (length == 0)
                {
                return std::pair<double, double>{ 0.0, 0.0 };
                }
            return std::pair<double, double>{ safe_divide<double>(deltaX, length),
                                              safe_divide<double>(deltaY, length) };
        };

        const double topEdgeLength{ std::hypot(static_cast<double>(topVertex.x - bottomVertex.x),
                                               static_cast<double>(topVertex.y - bottomVertex.y)) };
        const double bottomEdgeLength{ std::hypot(
            static_cast<double>(bottomVertex.x - apexVertex.x),
            static_cast<double>(bottomVertex.y - apexVertex.y)) };
        const double apexEdgeLength{ std::hypot(static_cast<double>(apexVertex.x - topVertex.x),
                                                static_cast<double>(apexVertex.y - topVertex.y)) };
        double radius{ std::min({ topEdgeLength, bottomEdgeLength, apexEdgeLength }) * 0.35 };
        radius = std::clamp(radius, ScaleToScreenAndCanvas(4), ScaleToScreenAndCanvas(20));

        // cut points on each edge, measured from the vertex they belong to
        const auto topApexUnit = normalize(topVertex, apexVertex);
        const auto topBottomUnit = normalize(topVertex, bottomVertex);
        const wxPoint topTowardApex{
            topVertex.x + static_cast<int>(std::lround(topApexUnit.first * radius)),
            topVertex.y + static_cast<int>(std::lround(topApexUnit.second * radius))
        };
        const wxPoint topTowardBottom{
            topVertex.x + static_cast<int>(std::lround(topBottomUnit.first * radius)),
            topVertex.y + static_cast<int>(std::lround(topBottomUnit.second * radius))
        };

        const auto bottomTopUnit = normalize(bottomVertex, topVertex);
        const auto bottomApexUnit = normalize(bottomVertex, apexVertex);
        const wxPoint bottomTowardTop{
            bottomVertex.x + static_cast<int>(std::lround(bottomTopUnit.first * radius)),
            bottomVertex.y + static_cast<int>(std::lround(bottomTopUnit.second * radius))
        };
        const wxPoint bottomTowardApex{
            bottomVertex.x + static_cast<int>(std::lround(bottomApexUnit.first * radius)),
            bottomVertex.y + static_cast<int>(std::lround(bottomApexUnit.second * radius))
        };

        const auto apexBottomUnit = normalize(apexVertex, bottomVertex);
        const auto apexTopUnit = normalize(apexVertex, topVertex);
        const wxPoint apexTowardBottom{
            apexVertex.x + static_cast<int>(std::lround(apexBottomUnit.first * radius)),
            apexVertex.y + static_cast<int>(std::lround(apexBottomUnit.second * radius))
        };
        const wxPoint apexTowardTop{
            apexVertex.x + static_cast<int>(std::lround(apexTopUnit.first * radius)),
            apexVertex.y + static_cast<int>(std::lround(apexTopUnit.second * radius))
        };

        const int headHeight{ m_bottomLeft.y - m_topLeft.y };
        const int headWidth{ m_apex.x - m_topLeft.x };
        const int mouthY{ m_bottomLeft.y - (headHeight / 3) };
        const int rightBoundaryX{ m_topLeft.x + (2 * headWidth) / 3 };

        // for a shark mouth, a notch is cut directly into the lower-right edge
        // so that the head stays a single (non-self-intersecting) outline
        const auto edgePointX = [this](const int yPos)
        {
            const double deltaY{ static_cast<double>(m_apex.y - m_bottomLeft.y) };
            if (deltaY == 0)
                {
                return m_bottomLeft.x;
                }
            const double fraction{ safe_divide<double>(static_cast<double>(yPos) - m_bottomLeft.y,
                                                       deltaY) };
            return static_cast<int>(
                std::lround(m_bottomLeft.x + (fraction * (m_apex.x - m_bottomLeft.x))));
        };
        const int biteDepth{ headWidth / 2 };
        const int biteHalfHeight{ headHeight / 10 };
        const int lowerLipY{ mouthY + biteHalfHeight };
        const int upperLipY{ mouthY - biteHalfHeight };
        const int lowerLipX{ edgePointX(lowerLipY) };
        const int upperLipX{ edgePointX(upperLipY) };
        const int innerX{ edgePointX(mouthY) - biteDepth };

        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(topTowardBottom.x, topTowardBottom.y);
        path.AddLineToPoint(bottomTowardTop.x, bottomTowardTop.y);
        path.AddQuadCurveToPoint(bottomVertex.x, bottomVertex.y, bottomTowardApex.x,
                                 bottomTowardApex.y);
        if (m_fishStyle == FishStyle::Shark)
            {
            path.AddLineToPoint(lowerLipX, lowerLipY);
            path.AddLineToPoint(innerX, mouthY);
            path.AddLineToPoint(upperLipX, upperLipY);
            }
        path.AddLineToPoint(apexTowardBottom.x, apexTowardBottom.y);
        path.AddQuadCurveToPoint(apexVertex.x, apexVertex.y, apexTowardTop.x, apexTowardTop.y);
        path.AddLineToPoint(topTowardApex.x, topTowardApex.y);
        path.AddQuadCurveToPoint(topVertex.x, topVertex.y, topTowardBottom.x, topTowardBottom.y);
        path.CloseSubpath();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ m_color });
        gc->FillPath(path);

        const double eyeRadius{ ScaleToScreenAndCanvas(8) };
        const double eyeX{ m_topLeft.x + (headWidth * 0.3) };
        const double eyeY{ m_topLeft.y + (headHeight * 0.3) };
        gc->SetBrush(wxBrush{ m_featureColor });
        gc->DrawEllipse(eyeX - eyeRadius, eyeY - eyeRadius, eyeRadius * 2, eyeRadius * 2);

        if (m_fishStyle == FishStyle::Fish)
            {
            const int mouthStartX{ rightBoundaryX - (headWidth / 4) };
            gc->SetPen(wxPen{ m_featureColor, static_cast<int>(std::lround(
                                                  std::max(1.0, ScaleToScreenAndCanvas(2)))) });
            gc->StrokeLine(mouthStartX, mouthY, rightBoundaryX, mouthY);
            }
        else if (m_fishStyle == FishStyle::Shark)
            {
            // teeth along each lip, pointing into the mouth
            const double toothHeight{ static_cast<double>(biteHalfHeight) * 0.45 };

            const auto drawTeeth = [&](const double startX, const double startY, const double endX,
                                       const double endY, const int count, const int verticalSign,
                                       const double startFrac, const double endFrac)
            {
                const double deltaX{ endX - startX };
                const double deltaY{ endY - startY };
                const double len{ std::hypot(deltaX, deltaY) };
                if (len <= 0 || count <= 0)
                    {
                    return;
                    }
                const double unitX{ safe_divide<double>(deltaX, len) };
                const double unitY{ safe_divide<double>(deltaY, len) };
                const double span{ len * (endFrac - startFrac) };
                const double toothWidth{ safe_divide<double>(span, 2.0 * count) };
                for (int i = 0; i < count; ++i)
                    {
                    const double offset{ (len * startFrac) + ((2.0 * i + 1.0) * toothWidth) };
                    const double baseX{ startX + (unitX * offset) };
                    const double baseY{ startY + (unitY * offset) };
                    const double baseEndX{ baseX + (unitX * toothWidth) };
                    const double baseEndY{ baseY + (unitY * toothWidth) };
                    // top row points down-left like the outer teeth; bottom row
                    // points at the upper lip
                    double normalX{ 0.0 };
                    double normalY{ 0.0 };
                    if (verticalSign == 1)
                        {
                        normalX = static_cast<double>(lowerLipX) - upperLipX;
                        normalY = static_cast<double>(lowerLipY) - upperLipY;
                        }
                    else
                        {
                        normalX = static_cast<double>(upperLipX) - baseX;
                        normalY = static_cast<double>(upperLipY) - baseY;
                        }
                    const double normalLength{ std::hypot(normalX, normalY) };
                    if (normalLength > 0)
                        {
                        normalX = safe_divide<double>(normalX, normalLength);
                        normalY = safe_divide<double>(normalY, normalLength);
                        }
                    const double toothLen{ (verticalSign == 1) ? (toothHeight * 1.6) :
                                                                 toothHeight };
                    const double apexX{ baseX + (normalX * toothLen) };
                    const double apexY{ baseY + (normalY * toothLen) };
                    wxGraphicsPath tooth = gc->CreatePath();
                    tooth.MoveToPoint(baseX, baseY);
                    tooth.AddLineToPoint(apexX, apexY);
                    tooth.AddLineToPoint(baseEndX, baseEndY);
                    tooth.CloseSubpath();
                    gc->SetPen(wxPen{
                        wxColour{ 80, 80, 80 },
                        static_cast<int>(std::lround(std::max(1.0, ScaleToScreenAndCanvas(1)))) });
                    gc->SetBrush(wxBrush{ *wxWHITE });
                    gc->FillPath(tooth);
                    gc->StrokePath(tooth);
                    }
            };

            drawTeeth(upperLipX, upperLipY, innerX, mouthY, 4, 1, 0.25, 0.8);
            drawTeeth(lowerLipX, lowerLipY, innerX, mouthY, 4, 0, 0.0, 0.7);
            }

        return boundingBox;
        }

    //----------------------------------------------------------------
    bool IshikawaDiagram::Head::HitTest(const wxPoint pt, wxDC& dc) const
        {
        return GetBoundingBox(dc).Contains(pt);
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::Head::Offset(const int xOffset, const int yOffset)
        {
        m_topLeft.x += xOffset;
        m_topLeft.y += yOffset;
        m_bottomLeft.x += xOffset;
        m_bottomLeft.y += yOffset;
        m_apex.x += xOffset;
        m_apex.y += yOffset;
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::Head::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                               [[maybe_unused]] wxDC& dc,
                                               [[maybe_unused]] const double scaling)
        {
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        const wxRect drawArea{ GetPlotAreaBoundingBox() };
        if (drawArea.IsEmpty() || m_branches.empty())
            {
            return;
            }

        std::vector<const BranchItem*> shownBranches;
        for (const auto& branch : m_branches)
            {
            if (m_showEmptyBranches || !branch.m_causes.empty())
                {
                shownBranches.push_back(&branch);
                }
            }
        if (shownBranches.empty())
            {
            return;
            }

        // the spine and effect use the text color, and each branch gets its own color
        const wxColour mainColor{ GetLeftYAxis().GetFontColor() };
        // a color too close to the plot's background is shaded or tinted so that it stays visible
        const auto ensureVisible = [this](const wxColour& color)
        { return Colors::ColorContrast::ShadeOrTintIfClose(color, GetPlotOrCanvasColor()); };

        // soft gray-blue for the tail fin and spine (contrasted against the background)
        const wxColour finColor{ 155, 180, 195 };
        const wxColour spineColor{ ensureVisible(m_monochrome ? mainColor : finColor) };
        const wxColour bgColor{ GetPlotOrCanvasColor() };
        const wxColour contrastedText{ (bgColor.GetLuminance() > 0.5) ? *wxBLACK : *wxWHITE };

        const auto makeLabel = [this, &contrastedText](const wxString& text, const wxPoint& pt,
                                                       const Anchoring anchoring,
                                                       const double scale = 1.0)
        {
            return std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo{ text }
                                                           .Pen(wxNullPen)
                                                           .FontColor(contrastedText)
                                                           .Scaling(GetScaling() * scale)
                                                           .DPIScaling(GetDPIScaleFactor())
                                                           .Anchoring(anchoring)
                                                           .AnchorPoint(pt)
                                                           .Selectable(false));
        };
        const Colors::Schemes::ColorScheme defaultScheme{ Settings::GetDefaultColorScheme() };
        const std::vector<wxColour> branchColors{ (GetColorScheme() != nullptr &&
                                                   !GetColorScheme()->GetColors().empty()) ?
                                                      GetColorScheme()->GetColors() :
                                                      defaultScheme.GetColors() };
        // monochrome diagram uses the text color for every branch
        const auto getBranchColor =
            [this, &branchColors, &mainColor, &ensureVisible](const size_t index)
        {
            return ensureVisible(m_monochrome ? mainColor :
                                                branchColors[index % branchColors.size()]);
        };

        const auto makeLine =
            [this](const wxPoint& start, const wxPoint& end, const wxColour& color, const int width)
        {
            auto line = std::make_unique<GraphItems::Lines>(wxPen{ color, width }, GetScaling());
            line->AddLine(start, end);
            return line;
        };

        // a spine or rib that is thin at its start and thick at its end if tapering is on,
        // or a plain line of the flat width if it is off
        const auto makeStroke =
            [this, &makeLine](const wxPoint& start, const wxPoint& end, const wxColour& color,
                              const int thinWidth, const int thickWidth,
                              const int flatWidth) -> std::unique_ptr<GraphItems::GraphItemBase>
        {
            if (!m_taperedLines)
                {
                return makeLine(start, end, color, flatWidth);
                }
            const double deltaX{ static_cast<double>(end.x - start.x) };
            const double deltaY{ static_cast<double>(end.y - start.y) };
            const double length{ std::hypot(deltaX, deltaY) };
            const double startHalf{ ScaleToScreenAndCanvas(thinWidth) / 2 };
            const double endHalf{ ScaleToScreenAndCanvas(thickWidth) / 2 };
            // the unit normal of the stroke
            const double normalX{ safe_divide<double>(-deltaY, length) };
            const double normalY{ safe_divide<double>(deltaX, length) };
            const auto offsetPoint = [&](const wxPoint& pt, const double half, const double side)
            {
                return wxPoint{ pt.x + static_cast<int>(std::lround(normalX * half * side)),
                                pt.y + static_cast<int>(std::lround(normalY * half * side)) };
            };
            const std::array<wxPoint, 4> points{ offsetPoint(start, startHalf, 1),
                                                 offsetPoint(end, endHalf, 1),
                                                 offsetPoint(end, endHalf, -1),
                                                 offsetPoint(start, startHalf, -1) };
            return std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo{}
                                                             .Pen(wxPen{ color })
                                                             .Brush(wxBrush{ color })
                                                             .Scaling(GetScaling())
                                                             .Selectable(false),
                                                         points);
        };

        // boxed label, used for the effect and the branch labels
        const auto makeBoxedLabel = [this](const wxString& text, const wxPoint& pt,
                                           const wxColour& fill, const wxColour& border,
                                           const wxColour& textColor)
        {
            auto label =
                std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo{ text }
                                                        .Pen(wxPen{ border, 2 })
                                                        .Brush(wxBrush{ fill })
                                                        .FontColor(textColor)
                                                        .Padding(4, 6, 4, 6)
                                                        .Scaling(GetScaling())
                                                        .DPIScaling(GetDPIScaleFactor())
                                                        .Anchoring(Anchoring::Center)
                                                        .AnchorPoint(pt)
                                                        .LabelAlignment(TextAlignment::Centered)
                                                        .Selectable(false));
            label->SetBoxCorners(BoxCorners::Rounded);
            return label;
        };
        const auto makeBranchLabel =
            [&](const wxString& text, const wxPoint& pt, const wxColour& color)
        {
            // a light tint of the branch color, with a border in the branch color
            const wxColour tint{ Colors::ColorContrast::Tint(color, 0.9) };
            return makeBoxedLabel(text, pt, tint, color, contrastedText);
        };

        // without a fish, there is no head or tail (or eye or mouth), and the effect is boxed
        const bool hasFish{ m_fishStyle != FishStyle::None };
        const wxString effectText{ m_effect.empty() ? _(L"Effect") : m_effect };
        std::unique_ptr<GraphItems::Label> effectLabel;
        if (hasFish)
            {
            // effect label centered inside the fish head; transparent background, no outline
            effectLabel = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo{ effectText }
                    .Pen(wxNullPen)
                    .Brush(wxNullBrush)
                    .FontColor((spineColor.GetLuminance() > 0.5) ? *wxBLACK : *wxWHITE)
                    .Padding(4, 6, 4, 6)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .Anchoring(Anchoring::Center)
                    .LabelAlignment(TextAlignment::Centered)
                    .Selectable(false));
            }
        else
            {
            effectLabel = makeBranchLabel(effectText, wxPoint{ 0, 0 }, spineColor);
            }
        // line breaks in the effect are kept, and long lines are wrapped to fit the head
        const int maxEffectWidth{ static_cast<int>(drawArea.GetWidth() * 0.3) };
        if (effectLabel->GetBoundingBox(dc).GetWidth() > maxEffectWidth)
            {
            effectLabel->SplitTextToFitBoundingBox(
                dc, wxSize{ maxEffectWidth - static_cast<int>(ScaleToScreenAndCanvas(16)),
                            drawArea.GetHeight() / 2 });
            }
        const wxRect effectRect{ effectLabel->GetBoundingBox(dc) };
        const int headPadding{ static_cast<int>(ScaleToScreenAndCanvas(10)) };
        // the head should be substantially larger than the tail fin, so scale the
        // label-derived size up (and keep it inside the plot area)
        const int baseHeadWidth{ std::min(effectRect.GetWidth(), maxEffectWidth) +
                                 (2 * headPadding) };
        const int baseHeadHeight{ effectRect.GetHeight() + (2 * headPadding) };
        const int headHeight{ std::min(baseHeadHeight * 6,
                                       static_cast<int>(drawArea.GetHeight() * 0.6)) };
        // width should track the (good) height, not the (tiny) label width
        // without a fish, the effect's box is only as wide as its text
        const int headWidth{ hasFish ? std::min(std::max(baseHeadWidth,
                                                         static_cast<int>(headHeight * 0.6)),
                                                static_cast<int>(drawArea.GetWidth() * 0.35)) :
                                       std::min(effectRect.GetWidth(), maxEffectWidth) };
        const int spineY{ drawArea.GetY() + (drawArea.GetHeight() / 2) };
        const int spineEndX{ drawArea.GetRight() - headWidth };
        effectLabel->SetAnchorPoint(wxPoint{ spineEndX + (headWidth / 2), spineY });

        // the tail fin takes up the left end of the plot area, and the spine starts after it
        const int tailWidth{
            hasFish ? std::min<int>(ScaleToScreenAndCanvas(72), drawArea.GetWidth() * 0.16) : 0
        };
        // the fin must not reach past the top or bottom of the plot area
        const int tailHalfHeight{ std::min(static_cast<int>(tailWidth * 1.7),
                                           drawArea.GetHeight() / 2) };
        const int spineStartX{ drawArea.GetX() + tailWidth };

        // height of a branch label, used to inset the outer ends of the ribs
        int branchLabelHeight{ 0 };
        if (m_showBranchLabels)
            {
            for (const auto* branch : shownBranches)
                {
                branchLabelHeight =
                    std::max(branchLabelHeight,
                             makeBranchLabel(branch->m_label, wxPoint{ 0, 0 }, getBranchColor(0))
                                 ->GetBoundingBox(dc)
                                 .GetHeight());
                }
            }
        const int edgeMargin{ static_cast<int>(ScaleToScreenAndCanvas(4)) };

        const int pairCount{ static_cast<int>((shownBranches.size() + 1) / 2) };
        // a small plot area must not produce negative sizes, which would flip the ribs
        const int slotWidth{ std::max(0, safe_divide<int>(spineEndX - spineStartX, pairCount)) };
        // the tallest a rib can be; ribs toward the head grow to this, ribs toward
        // the tail are a little shorter, matching the spine's taper
        const int baseHalfHeight{ std::max(0, (drawArea.GetHeight() / 2) - branchLabelHeight -
                                                  edgeMargin) };
        const int ribShift{ std::max(0, std::min(static_cast<int>(baseHalfHeight * 0.4),
                                                 static_cast<int>(slotWidth * 0.35))) };
        const int twigLength{ static_cast<int>(slotWidth * 0.5) };
        // how much taller a rib gets as it approaches the head (at the tail's pair index,
        // the rib is this fraction of the tallest rib)
        constexpr double shortestRibRatio{ 0.62 };
        // the rib height for a pair; pairCount-1 is the pair nearest the head
        const auto ribHalfHeight = [pairCount, baseHalfHeight](const int pairIndex)
        {
            // with one pair there is nothing to grow from, so it's already at full height
            if (pairCount <= 1)
                {
                return baseHalfHeight;
                }
            const double ratio{ shortestRibRatio +
                                ((1.0 - shortestRibRatio) *
                                 safe_divide<double>(static_cast<double>(pairIndex),
                                                     static_cast<double>(pairCount - 1))) };
            return static_cast<int>(baseHalfHeight * ratio);
        };

        // the busiest rib decides how much the cause labels shrink, so that they don't overlap
        size_t mostSlots{ 0 };
        for (const auto* branch : shownBranches)
            {
            size_t slots{ 0 };
            for (const auto& cause : branch->m_causes)
                {
                slots += 1 + cause.m_subCauses.size();
                }
            mostSlots = std::max(mostSlots, slots);
            }
        const int labelGap{ static_cast<int>(ScaleToScreenAndCanvas(7)) };
        const int causeLabelHeight{
            makeLabel(L"Ag", wxPoint{ 0, 0 }, Anchoring::Center)->GetBoundingBox(dc).GetHeight()
        };
        // recomputed per branch, as each rib's height now varies
        double labelScale{ 1.0 };
        int labelBoxHeight{ causeLabelHeight + 1 };
        // a label can't reach past the neighboring rib, so long text is wrapped
        // (and then truncated)
        const int maxCauseWidth{ std::max(1, static_cast<int>((slotWidth - ribShift) * 0.95) -
                                                 labelGap) };
        const int maxSubCauseWidth{ std::max(1,
                                             maxCauseWidth - static_cast<int>(twigLength * 0.4)) };
        const auto makeCauseLabel = [&](const wxString& text, const wxPoint& pt, const int maxWidth)
        {
            auto label = makeLabel(text, pt, Anchoring::BottomRightCorner, labelScale);
            if (label->GetBoundingBox(dc).GetWidth() > maxWidth)
                {
                label->SplitTextToFitBoundingBox(dc, wxSize{ maxWidth, labelBoxHeight });
                }
            return label;
        };

        if (hasFish)
            {
            // forked whale-tail fin: two smooth quad bezier lobes with a concave notch between them
            const int lobeTipX{ drawArea.GetX() };
            const int notchX{ drawArea.GetX() + static_cast<int>(tailWidth * 0.45) };
            AddObject(std::make_unique<TailFin>(wxPoint{ spineStartX, spineY },
                                                wxPoint{ lobeTipX, spineY - tailHalfHeight },
                                                wxPoint{ lobeTipX, spineY + tailHalfHeight },
                                                wxPoint{ notchX, spineY }, spineColor));

            // the eye and mouth: white or black, whichever contrasts with the head
            const wxColour featureColor{
                m_monochrome ? ((spineColor.GetLuminance() > 0.5) ? *wxBLACK : *wxWHITE) : *wxWHITE
            };

            // fish head (with its eye and mouth): triangle with rounded corners,
            // no outline, matching the tail
            AddObject(std::make_unique<Head>(wxPoint{ spineEndX, spineY - (headHeight / 2) },
                                             wxPoint{ spineEndX, spineY + (headHeight / 2) },
                                             wxPoint{ spineEndX + headWidth, spineY }, spineColor,
                                             featureColor, m_fishStyle, GetScaling()));
            }
        AddObject(std::move(effectLabel));

        for (size_t i = 0; i < shownBranches.size(); ++i)
            {
            const auto& branch{ *shownBranches[i] };
            const bool isTop{ i % 2 == 0 };
            const int direction{ isTop ? -1 : 1 };
            // the first branches are nearest the effect, so the pairs run right to left
            const int pairIndex{ pairCount - 1 - static_cast<int>(i / 2) };
            // ribs grow taller as they approach the head
            const int halfHeight{ ribHalfHeight(pairIndex) };
            // labels shrink to fit the spacing between slots on this rib
            const double slotSpacing{ safe_divide<double>(static_cast<double>(halfHeight),
                                                          static_cast<double>(mostSlots + 1)) };
            labelScale =
                std::clamp(safe_divide<double>(
                               slotSpacing, static_cast<double>(causeLabelHeight + (2 * labelGap))),
                           0.5, 1.0);
            labelBoxHeight = std::max(static_cast<int>(slotSpacing),
                                      static_cast<int>(causeLabelHeight * labelScale) + 1);

            const wxPoint attachPt{ spineStartX + ((pairIndex + 1) * slotWidth) -
                                        static_cast<int>(slotWidth * 0.05),
                                    spineY };
            const wxPoint outerPt{ attachPt.x - ribShift, spineY + (direction * halfHeight) };
            const wxColour branchColor{ getBranchColor(i) };
            // sub-causes are a lighter shade of their branch's color
            const wxColour subColor{ ensureVisible(
                Colors::ColorContrast::Tint(branchColor, 0.65)) };
            // the ribs widen toward the spine (more pronounced taper toward head)
            AddObject(makeStroke(outerPt, attachPt, branchColor, 1, 5, 2));

            if (m_showBranchLabels)
                {
                const wxString labelText{ m_showCauseCounts ?
                                              wxString::Format(_DT(L"%s (%zu)"), branch.m_label,
                                                               branch.m_causes.size()) :
                                              branch.m_label };
                // push labels further left (away from the rib)
                const int labelOffset{ static_cast<int>(ScaleToScreenAndCanvas(12)) };
                AddObject(
                    makeBranchLabel(labelText,
                                    wxPoint{ outerPt.x - labelOffset,
                                             outerPt.y + (direction * branchLabelHeight / 2) },
                                    branchColor));
                }

            // each cause and sub-cause gets one evenly spaced slot along the rib,
            // in the order that they are listed when reading from top to bottom
            size_t slotCount{ 0 };
            for (const auto& cause : branch.m_causes)
                {
                slotCount += 1 + cause.m_subCauses.size();
                }
            size_t slot{ 0 };
            const auto getRibPoint = [&]()
            {
                const double fromTop{ safe_divide<double>(static_cast<double>(slot + 1),
                                                          static_cast<double>(slotCount + 1)) };
                // the rib is measured from its outer end, which is at the bottom for lower branches
                const double ratio{ isTop ? fromTop : 1.0 - fromTop };
                ++slot;
                return wxPoint{ outerPt.x + static_cast<int>((attachPt.x - outerPt.x) * ratio),
                                outerPt.y + static_cast<int>((attachPt.y - outerPt.y) * ratio) };
            };
            for (const auto& cause : branch.m_causes)
                {
                const wxPoint causePt{ getRibPoint() };
                AddObject(makeLine(causePt, wxPoint{ causePt.x - twigLength, causePt.y },
                                   branchColor, 1));
                AddObject(makeCauseLabel(cause.m_text, wxPoint{ causePt.x - labelGap, causePt.y },
                                         maxCauseWidth));
                // a dot where the twig meets the rib
                AddObject(std::make_unique<GraphItems::Point2D>(GraphItems::GraphItemInfo{}
                                                                    .Brush(wxBrush{ branchColor })
                                                                    .Pen(wxPen{ branchColor })
                                                                    .Scaling(GetScaling())
                                                                    .DPIScaling(GetDPIScaleFactor())
                                                                    .Anchoring(Anchoring::Center)
                                                                    .AnchorPoint(causePt)
                                                                    .Selectable(false),
                                                                3));

                if (cause.m_subCauses.empty())
                    {
                    continue;
                    }
                // sub-causes hang off of a connector that drops from the cause's twig
                const int connectorX{ causePt.x - static_cast<int>(twigLength * 0.4) };
                const int subLength{ static_cast<int>(twigLength * 0.5) };
                int lastY{ causePt.y };
                for (const auto& subCause : cause.m_subCauses)
                    {
                    const wxPoint subPt{ getRibPoint() };
                    AddObject(makeLine(wxPoint{ connectorX, subPt.y },
                                       wxPoint{ connectorX - subLength, subPt.y }, subColor, 1));
                    AddObject(makeCauseLabel(subCause, wxPoint{ connectorX - labelGap, subPt.y },
                                             maxSubCauseWidth));
                    lastY = subPt.y;
                    }
                AddObject(makeLine(wxPoint{ connectorX, causePt.y }, wxPoint{ connectorX, lastY },
                                   subColor, 1));
                }
            }

        // draw the spine on top of the ribs
        if (m_taperedLines)
            {
            AddObject(makeStroke(wxPoint{ spineStartX, spineY }, wxPoint{ spineEndX, spineY },
                                 spineColor, 2, 6, 3));
            }
        else
            {
            AddObject(makeLine(wxPoint{ spineStartX, spineY }, wxPoint{ spineEndX, spineY },
                               spineColor, 3));
            }
        }
    } // namespace Wisteria::Graphs
