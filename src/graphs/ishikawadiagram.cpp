///////////////////////////////////////////////////////////////////////////////
// Name:        ishikawadiagram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "ishikawadiagram.h"
#include <algorithm>
#include <array>
#include <optional>
#include <ranges>

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
    std::vector<wxString> IshikawaDiagram::GetCauseCategoryLabels(const CauseCategories categories)
        {
        switch (categories)
            {
        case CauseCategories::Marketing:
            return { _(L"Product"),           _(L"Price"),      _(L"Place"),
                     _(L"Promotion"),         _(L"People"),     _(L"Process"),
                     _(L"Physical Evidence"), _(L"Performance") };
        case CauseCategories::Service:
            return { _(L"Surroundings"), _(L"Suppliers"), _(L"Systems"), _(L"Skills"),
                     _(L"Safety") };
        case CauseCategories::Manufacturing:
            [[fallthrough]];
        default:
            return { _(L"Manpower"),    _(L"Machine"),       _(L"Material"),   _(L"Method"),
                     _(L"Measurement"), _(L"Mother Nature"), _(L"Management"), _(L"Maintenance") };
            }
        }

    //----------------------------------------------------------------
    wxString IshikawaDiagram::GetDefaultBranchLabel(const size_t index)
        {
        const auto labels = GetCauseCategoryLabels(CauseCategories::Manufacturing);
        return (index < labels.size()) ? labels[index] : wxString{};
        }

    //----------------------------------------------------------------
    bool IshikawaDiagram::IsDefaultBranchName(const CauseCategories categories, const size_t index,
                                              const wxString& name)
        {
        const auto labels = GetCauseCategoryLabels(categories);
        if (index >= labels.size())
            {
            return false;
            }
        if (name.CmpNoCase(labels[index]) == 0)
            {
            return true;
            }
        // alternative names for each branch, in the same order as the labels
        const auto aliases = [categories]() -> std::vector<std::vector<wxString>>
        {
            switch (categories)
                {
            case CauseCategories::Marketing:
                return { { _(L"Product Elements"), _(L"Products"), _(L"Offering") },
                         { _(L"Pricing"), _(L"Prices") },
                         { _(L"Place and Time"), _(L"Distribution"), _(L"Channel"),
                           _(L"Channels") },
                         { _(L"Promotion and Education"), _(L"Promotions"), _(L"Communication") },
                         { _(L"Participants"), _(L"Personnel"), _(L"Staff") },
                         { _(L"Processes"), _(L"Procedures") },
                         { _(L"Physical Environment"), _(L"Physical"), _(L"Evidence") },
                         { _(L"Productivity"), _(L"Productivity and Quality") } };
            case CauseCategories::Service:
                return { { _(L"Environment") },
                         { _(L"Supplier"), _(L"Vendors") },
                         {},
                         { _(L"Training") },
                         {} };
            case CauseCategories::Manufacturing:
                [[fallthrough]];
            default:
                return { { L"Man", _(L"People") },         {}, {}, {}, { L"Measure" },
                         { L"Enviro", _(L"Environment") }, {}, {} };
                }
        }();
        return (index < aliases.size()) &&
               std::ranges::any_of(aliases[index], [&name](const wxString& alias)
                                   { return name.CmpNoCase(alias) == 0; });
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
        if (!label.ends_with(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }

    //----------------------------------------------------------------
    IshikawaDiagram::TailFin::TailFin(std::vector<wxPoint> outline, const wxColour& color,
                                      const FishStyle fishStyle, const double scaling)
        : m_outline(std::move(outline)), m_color(color), m_fishStyle(fishStyle)
        {
        GraphItemBase::GetGraphItemInfo().Selectable(false);
        GraphItemBase::SetScaling(scaling);
        }

    //----------------------------------------------------------------
    wxRect IshikawaDiagram::TailFin::GetBoundingBox([[maybe_unused]]
                                                    wxDC &
                                                    dc) const
        {
        if (m_outline.empty())
            {
            return {};
            }
        const auto [minX, maxX] = std::ranges::minmax(
            m_outline | std::views::transform([](const wxPoint& pt) { return pt.x; }));
        const auto [minY, maxY] = std::ranges::minmax(
            m_outline | std::views::transform([](const wxPoint& pt) { return pt.y; }));
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

        if (m_outline.empty())
            {
            return boundingBox;
            }
        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(m_outline.front().x, m_outline.front().y);
        for (size_t i = 1; i + 1 < m_outline.size(); i += 2)
            {
            path.AddQuadCurveToPoint(m_outline[i].x, m_outline[i].y, m_outline[i + 1].x,
                                     m_outline[i + 1].y);
            }
        path.CloseSubpath();

        if (m_fishStyle != FishStyle::Skeleton)
            {
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush{ m_color });
            gc->FillPath(path);
            return boundingBox;
            }

        // a skeleton's fin is an outline, with rays fanning out from the spine
        gc->SetPen(wxPen{
            m_color, static_cast<int>(std::lround(std::max(1.0, ScaleToScreenAndCanvas(2)))) });
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->StrokePath(path);

        gc->SetPen(wxPen{ m_color, GetScaledPenWidth() });
        const wxPoint& rayStart{ m_outline.front() };
        // rays stop short of the outline
        for (size_t i = 1; i + 1 < m_outline.size(); i += 2)
            {
            const wxPoint& curveStart{ m_outline[i - 1] };
            const wxPoint& control{ m_outline[i] };
            const wxPoint& curveEnd{ m_outline[i + 1] };
            for (const double tVal : { 0.25, 0.5, 0.75 })
                {
                constexpr double RAY_REACH{ 0.85 };
                const double startWeight{ (1.0 - tVal) * (1.0 - tVal) };
                const double controlWeight{ 2.0 * tVal * (1.0 - tVal) };
                const double endWeight{ tVal * tVal };
                const double curveX{ (startWeight * curveStart.x) + (controlWeight * control.x) +
                                     (endWeight * curveEnd.x) };
                const double curveY{ (startWeight * curveStart.y) + (controlWeight * control.y) +
                                     (endWeight * curveEnd.y) };
                gc->StrokeLine(rayStart.x, rayStart.y,
                               rayStart.x + ((curveX - rayStart.x) * RAY_REACH),
                               rayStart.y + ((curveY - rayStart.y) * RAY_REACH));
                }
            }

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
        for (auto& pt : m_outline)
            {
            pt.x += xOffset;
            pt.y += yOffset;
            }
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::TailFin::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                                  [[maybe_unused]] wxDC& dc,
                                                  [[maybe_unused]] const double scaling)
        {
        }

    //----------------------------------------------------------------
    IshikawaDiagram::DorsalFin::DorsalFin(const wxPoint& frontPoint, const wxPoint& frontControl,
                                          const wxPoint& tipPoint, const wxPoint& rearControl,
                                          const wxPoint& rearPoint, const wxColour& color)
        : m_frontPoint(frontPoint), m_frontControl(frontControl), m_tipPoint(tipPoint),
          m_rearControl(rearControl), m_rearPoint(rearPoint), m_color(color)
        {
        GraphItemBase::GetGraphItemInfo().Selectable(false);
        }

    //----------------------------------------------------------------
    wxRect IshikawaDiagram::DorsalFin::GetBoundingBox([[maybe_unused]]
                                                      wxDC &
                                                      dc) const
        {
        const int minX = std::min(
            { m_frontPoint.x, m_frontControl.x, m_tipPoint.x, m_rearControl.x, m_rearPoint.x });
        const int maxX = std::max(
            { m_frontPoint.x, m_frontControl.x, m_tipPoint.x, m_rearControl.x, m_rearPoint.x });
        const int minY = std::min(
            { m_frontPoint.y, m_frontControl.y, m_tipPoint.y, m_rearControl.y, m_rearPoint.y });
        const int maxY = std::max(
            { m_frontPoint.y, m_frontControl.y, m_tipPoint.y, m_rearControl.y, m_rearPoint.y });
        return wxRect{ wxPoint{ minX, minY }, wxPoint{ maxX, maxY } };
        }

    //----------------------------------------------------------------
    wxRect IshikawaDiagram::DorsalFin::Draw(wxDC & dc) const
        {
        const auto boundingBox = GetBoundingBox(dc);
        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return boundingBox;
            }

        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(m_frontPoint.x, m_frontPoint.y);
        path.AddQuadCurveToPoint(m_frontControl.x, m_frontControl.y, m_tipPoint.x, m_tipPoint.y);
        path.AddQuadCurveToPoint(m_rearControl.x, m_rearControl.y, m_rearPoint.x, m_rearPoint.y);
        path.CloseSubpath();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ m_color });
        gc->FillPath(path);

        return boundingBox;
        }

    //----------------------------------------------------------------
    bool IshikawaDiagram::DorsalFin::HitTest(const wxPoint pt, wxDC& dc) const
        {
        return GetBoundingBox(dc).Contains(pt);
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::DorsalFin::Offset(const int xOffset, const int yOffset)
        {
        m_frontPoint.x += xOffset;
        m_frontPoint.y += yOffset;
        m_frontControl.x += xOffset;
        m_frontControl.y += yOffset;
        m_tipPoint.x += xOffset;
        m_tipPoint.y += yOffset;
        m_rearControl.x += xOffset;
        m_rearControl.y += yOffset;
        m_rearPoint.x += xOffset;
        m_rearPoint.y += yOffset;
        }

    //----------------------------------------------------------------
    void IshikawaDiagram::DorsalFin::SetBoundingBox([[maybe_unused]] const wxRect& rect,
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
        GraphItemBase::GetGraphItemInfo().Selectable(false);
        GraphItemBase::SetScaling(scaling);
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

        // teeth along each lip, pointing into the mouth, drawn before the head so that
        // their bases are hidden in the gums
        if (m_fishStyle == FishStyle::Shark)
            {
            // a tooth stays white with a dark outline, even in dark mode
            std::optional<wxSVGAccessibleGroup> teethGroup;
            if (auto* svgDc = dynamic_cast<wxSVGFileDC*>(&dc); svgDc != nullptr)
                {
                teethGroup.emplace(*svgDc, wxSVGAttributes{}.Class(_DT(L"ink-keep")));
                }
            const GraphItems::GraphicsContextFallback teethGcf{ &dc, boundingBox };
            auto* gc = teethGcf.GetGraphicsContext();
            if (gc != nullptr)
                {
                const double toothHeight{ static_cast<double>(biteHalfHeight) * 0.45 };
                // how far each tooth's base reaches into the gums
                const double rootDepth{ std::max(2.0, ScaleToScreenAndCanvas(2)) };

                const auto drawTeeth = [&](const double startX, const double startY,
                                           const double endX, const double endY, const int count,
                                           const int verticalSign, const double startFrac,
                                           const double endFrac)
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
                        // the base is pushed back into the jaw, which the head covers
                        const double rootX{ normalX * rootDepth };
                        const double rootY{ normalY * rootDepth };
                        wxGraphicsPath tooth = gc->CreatePath();
                        tooth.MoveToPoint(baseX - rootX, baseY - rootY);
                        tooth.AddLineToPoint(apexX, apexY);
                        tooth.AddLineToPoint(baseEndX - rootX, baseEndY - rootY);
                        tooth.CloseSubpath();
                        gc->SetPen(wxPen{ wxColour{ 80, 80, 80 }, GetScaledPenWidth() });
                        gc->SetBrush(wxBrush{ *wxWHITE });
                        gc->FillPath(tooth);
                        gc->StrokePath(tooth);
                        }
                };

                drawTeeth(upperLipX, upperLipY, innerX, mouthY, 4, 1, 0.25, 0.8);
                drawTeeth(lowerLipX, lowerLipY, innerX, mouthY, 4, 0, 0.0, 0.7);
                }
            }

        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return boundingBox;
            }

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

        const double eyeX{ m_topLeft.x + (headWidth * 0.3) };
        const double eyeY{ m_topLeft.y + (headHeight * 0.3) };
        if (m_fishStyle == FishStyle::Skeleton)
            {
            // an outlined skull with a hollow eye socket and a toothed jaw
            const int outlineWidth{ static_cast<int>(
                std::lround(std::max(1.0, ScaleToScreenAndCanvas(2)))) };
            gc->SetPen(wxPen{ m_color, outlineWidth });
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->StrokePath(path);

            const double socketRadius{ ScaleToScreenAndCanvas(10) };
            gc->SetPen(wxPen{ m_featureColor, outlineWidth });
            gc->DrawEllipse(eyeX - socketRadius, eyeY - socketRadius, socketRadius * 2,
                            socketRadius * 2);

            const int jawStartX{ rightBoundaryX - (headWidth / 3) };
            gc->StrokeLine(jawStartX, mouthY, rightBoundaryX, mouthY);
            constexpr int TOOTH_COUNT{ 4 };
            const double toothHalfHeight{ std::max(ScaleToScreenAndCanvas(3),
                                                   static_cast<double>(headHeight) / 30) };
            const double toothSpacing{ safe_divide<double>(rightBoundaryX - jawStartX,
                                                           TOOTH_COUNT + 1) };
            for (int i = 1; i <= TOOTH_COUNT; ++i)
                {
                const double toothX{ jawStartX + (toothSpacing * i) };
                gc->StrokeLine(toothX, mouthY - toothHalfHeight, toothX, mouthY + toothHalfHeight);
                }

            return boundingBox;
            }

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush{ m_color });
        gc->FillPath(path);

        const double eyeRadius{ ScaleToScreenAndCanvas(8) };
        gc->SetBrush(wxBrush{ m_featureColor });
        gc->DrawEllipse(eyeX - eyeRadius, eyeY - eyeRadius, eyeRadius * 2, eyeRadius * 2);

        if (m_fishStyle == FishStyle::Fish)
            {
            const int mouthStartX{ rightBoundaryX - (headWidth / 4) };
            gc->SetPen(wxPen{ m_featureColor, static_cast<int>(std::lround(
                                                  std::max(1.0, ScaleToScreenAndCanvas(2)))) });
            gc->StrokeLine(mouthStartX, mouthY, rightBoundaryX, mouthY);
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
    IshikawaDiagram::CauseParts IshikawaDiagram::SplitCauseText(const wxString& text)
        {
        // split at every '|', which makes the first piece the problem
        std::vector<wxString> pieces;
        size_t start{ 0 };
        while (true)
            {
            const size_t barPos{ text.find(L'|', start) };
            const size_t pieceEnd{ (barPos == wxString::npos) ? text.length() : barPos };
            wxString piece{ text.substr(start, pieceEnd - start) };
            piece.Trim(true).Trim(false);
            pieces.push_back(piece);
            if (barPos == wxString::npos)
                {
                break;
                }
            start = barPos + 1;
            }

        CauseParts parts;
        // a leading '|' doesn't separate a problem from an idea, so the text isn't split
        if (pieces.size() < 2 || pieces.front().empty())
            {
            parts.m_problem = text;
            return parts;
            }

        for (size_t i = 1; i < pieces.size(); ++i)
            {
            // a '|' with nothing after it adds no idea, and a stray '||' is skipped
            if (!pieces[i].empty())
                {
                parts.m_ideas.push_back(pieces[i]);
                }
            }
        // nothing after any of the '|'s means that there was nothing to separate
        if (parts.m_ideas.empty())
            {
            parts.m_problem = text;
            return parts;
            }
        parts.m_problem = pieces.front();
        return parts;
        }

    //----------------------------------------------------------------
    IshikawaDiagram::IdeaMarker IshikawaDiagram::ParseIdeaMarker(const wxString& ideaText)
        {
        IdeaMarker marker;
        marker.m_text = ideaText;
        if (ideaText.empty())
            {
            return marker;
            }

        size_t markerLength{ 0 };
        IdeaStatus status{ IdeaStatus::OfInterest };
        if (ideaText[0] == L'*')
            {
            // one star per level of the 3-Dot method, so that '*', '**', and '***'
            // count as one, two, and three dots
            while (markerLength < ideaText.length() && ideaText[markerLength] == L'*')
                {
                ++markerLength;
                }
            status = (markerLength >= 3) ? IdeaStatus::UnderTest :
                     (markerLength == 2) ? IdeaStatus::InPrep :
                                           IdeaStatus::OfInterest;
            }
        else if (ideaText[0] == L'!')
            {
            markerLength = 1;
            status = IdeaStatus::Selected;
            }
        else if (ideaText[0] == L'-')
            {
            markerLength = 1;
            status = IdeaStatus::Rejected;
            }

        // the marker starts the idea, with or without a space after it
        marker.m_status = status;
        marker.m_text = ideaText.substr(markerLength);
        marker.m_text.Trim(true).Trim(false);
        return marker;
        }

    //----------------------------------------------------------------
    bool IshikawaDiagram::IsDrawingCards() const noexcept
        {
        return m_cedacCardStyle != CedacCardStyle::None &&
               std::ranges::none_of(m_branches,
                                    [](const BranchItem& branch)
                                    {
                                        return std::ranges::any_of(
                                            branch.m_causes, [](const CauseItem& cause)
                                            { return !cause.m_subCauses.empty(); });
                                    });
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> IshikawaDiagram::CreateLegend(const LegendOptions& options)
        {
        // the status markers are drawn on idea cards, so there is nothing to explain
        // without them
        const auto isDrawnIdea = [](const wxString& ideaText)
        { return !ParseIdeaMarker(ideaText).m_text.empty(); };
        const bool hasIdeas{ std::ranges::any_of(
            m_branches,
            [&isDrawnIdea](const BranchItem& branch)
            {
                return std::ranges::any_of(branch.m_causes,
                                           [&isDrawnIdea](const CauseItem& cause)
                                           {
                                               return std::ranges::any_of(
                                                   SplitCauseText(cause.m_text).m_ideas,
                                                   isDrawnIdea);
                                           });
            }) };
        if (!IsDrawingCards() || !hasIdeas)
            {
            return nullptr;
            }

        SetLegendInfo(options);

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{}
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()))
                .DPIScaling(GetDPIScaleFactor()));

        // the marker drawn for each status, in the order that IdeaStatus lists them
        const std::map<wxString, wxString> markers{ { L"•", _(L"Of interest") },
                                                    { L"••", _(L"In preparation") },
                                                    { L"•••", _(L"Under test") },
                                                    { L"✓", _(L"Selected") },
                                                    { L"✗", _(L"Rejected") } };

        wxString legendText;
        for (const auto& marker : markers)
            {
            if (!legendText.empty())
                {
                legendText += L'\n';
                }
            legendText += marker.first + L"  " + marker.second;
            }

        if (options.IsIncludingHeader())
            {
            const wxString headerText{ options.GetTitle().empty() ? _(L"Status") :
                                                                    options.GetTitle() };
            legendText.Prepend(headerText + L'\n');
            legend->GetHeaderInfo()
                .Enable(true)
                .LabelAlignment(TextAlignment::Centered)
                .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()))
                .GetFont()
                .MakeBold()
                .MakeLarger();
            }
        legend->SetText(legendText);

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> IshikawaDiagram::MakeCauseLabel(
        wxDC & dc, const wxString& text, const wxPoint& pt, const int maxWidth,
        const int labelBoxHeight, const wxColour& branchColor, const CauseLabelLayout& layout,
        const CedacCardKind kind, const bool strikeThroughText) const
        {
        auto label = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{ text }
                .Pen(wxNullPen)
                .FontColor(layout.m_textColor)
                // a straight-cornered box draws each side separately, and all four are
                // off by default
                .Outline(true, true, true, true)
                .Scaling(layout.m_scaling * layout.m_labelScale)
                .DPIScaling(layout.m_dpiScaling)
                // the problem grows leftward from the rib and the idea rightward,
                // so that the two sit across from each other
                .Anchoring((kind == CedacCardKind::Idea) ? Anchoring::BottomLeftCorner :
                                                           Anchoring::BottomRightCorner)
                .AnchorPoint(pt)
                .Selectable(false));
        if (strikeThroughText)
            {
            // CEDAC strikes through a rejected idea, so that it reads as discarded
            label->GetFont().SetStrikethrough(true);
            }
        if (!layout.IsDrawingCards())
            {
            if (label->GetBoundingBox(dc).GetWidth() > maxWidth)
                {
                label->SplitTextToFitBoundingBox(dc, wxSize{ maxWidth, labelBoxHeight });
                }
            return label;
            }

        // the text starts past the marker's column, whose size doesn't scale with the text
        const bool hasMarker{ kind == CedacCardKind::Idea };
        const int markerIndent{ hasMarker ?
                                    static_cast<int>(CARD_MARKER_WIDTH_DIPS * layout.m_scaling *
                                                     layout.m_dpiScaling) :
                                    0 };
        const int markerIndentDIPs{ hasMarker ? static_cast<int>(std::lround(
                                                    CARD_MARKER_WIDTH_DIPS / layout.m_labelScale)) :
                                                0 };
        // the text is wrapped (and then truncated) to fit inside the card
        const wxSize innerSize{ std::max(1, layout.m_cardWidth - (2 * layout.m_cardPadding) -
                                                markerIndent),
                                std::max(1, layout.m_cardHeight - (2 * layout.m_cardPadding)) };
        if (const wxRect textBox{ label->GetBoundingBox(dc) };
            textBox.GetWidth() > innerSize.GetWidth() ||
            textBox.GetHeight() > innerSize.GetHeight())
            {
            label->SplitTextToFitBoundingBox(dc, innerSize);
            }
        // every card is the same size, so the text sits in its top-left corner
        label->SetMinimumUserSizeDIPs(layout.m_cardWidthDIPs, layout.m_cardHeightDIPs);
        label->SetPadding(CARD_PADDING_DIPS, CARD_PADDING_DIPS, CARD_PADDING_DIPS,
                          CARD_PADDING_DIPS + markerIndentDIPs);
        label->SetShadowType(ShadowType::RightSideAndBottomShadow);
        label->SetTextAlignment(TextAlignment::FlushLeft);
        label->SetPageVerticalAlignment(PageVerticalAlignment::TopAligned);
        label->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
        if (layout.m_cedacCardStyle == CedacCardStyle::StickyNote)
            {
            const wxColour noteColor{ m_monochrome ? *wxWHITE :
                                                     ((kind == CedacCardKind::Idea) ?
                                                          wxColour{ 200, 230, 201 } :
                                                          wxColour{ 255, 205, 210 }) };
            label->SetFontBackgroundColor(noteColor);
            label->GetGraphItemInfo().Pen(wxPen{ branchColor, 1 });
            label->SetFontColor(*wxBLACK);
            }
        else
            {
            // an index card is ruled in translucent ink, so it needs a light surface
            label->SetFontBackgroundColor(*wxWHITE);
            label->GetGraphItemInfo().Pen(wxPen{ wxColour{ 190, 190, 190 }, 1 });
            label->SetFontColor(*wxBLACK);
            label->SetLabelStyle(LabelStyle::IndexCard);
            }
        label->GetAccessibilityAttributes().Class(_DT(L"ink-keep"));
        return label;
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
            // (a skeleton's head is hollow, so its text contrasts with the background)
            effectLabel = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo{ effectText }
                    .Pen(wxNullPen)
                    .Brush(wxNullBrush)
                    .FontColor((m_fishStyle == FishStyle::Skeleton) ? contrastedText :
                               (spineColor.GetLuminance() > 0.5)    ? *wxBLACK :
                                                                      *wxWHITE)
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
        const auto getBranchLabelText = [this](const BranchItem& branch)
        {
            return m_showCauseCounts ?
                       wxString::Format(_DT(L"%s (%zu)"), branch.m_label, branch.m_causes.size()) :
                       branch.m_label;
        };
        // branch labels are pushed left of their rib's outer end (away from the rib)
        const int branchLabelOffset{ static_cast<int>(ScaleToScreenAndCanvas(12)) };

        // a card shares its slot with sub-causes, so a diagram with them uses plain labels
        const CedacCardStyle cardStyle{ IsDrawingCards() ? m_cedacCardStyle :
                                                           CedacCardStyle::None };
        // the card styles draw every cause as a card of the same size
        const bool drawCards{ cardStyle != CedacCardStyle::None };
        // only cards split a cause at each '|', so a plain label keeps the whole text
        const auto splitCause = [drawCards](const wxString& text)
        { return drawCards ? SplitCauseText(text) : CauseParts{ text, {} }; };
        // a cause's ideas, skipping any that are only a status marker
        const auto getIdeas = [&splitCause](const wxString& causeText)
        {
            std::vector<IdeaMarker> ideas;
            for (const auto& ideaText : splitCause(causeText).m_ideas)
                {
                if (auto idea{ ParseIdeaMarker(ideaText) }; !idea.m_text.empty())
                    {
                    ideas.push_back(std::move(idea));
                    }
                }
            return ideas;
        };
        // a problem and its first idea share a rib slot, and each later idea takes another
        const auto getSlotWeight = [&getIdeas](const CauseItem& cause)
        { return std::max<size_t>(1, getIdeas(cause.m_text).size()) + cause.m_subCauses.size(); };
        // an idea is drawn on the far side of its rib, which needs room past the last rib
        const bool hasIdeas{ std::ranges::any_of(
            shownBranches,
            [&getIdeas](const auto* branch)
            {
                return std::ranges::any_of(branch->m_causes, [&getIdeas](const auto& cause)
                                           { return !getIdeas(cause.m_text).empty(); });
            }) };
        const int pairCount{ static_cast<int>((shownBranches.size() + 1) / 2) };
        // half a slot past the last rib holds the ideas of the branches nearest the head
        const double slotDivisor{ static_cast<double>(pairCount) + (hasIdeas ? 0.5 : 0.0) };
        // a small plot area must not produce negative sizes, which would flip the ribs
        const int slotWidth{ std::max(
            0, static_cast<int>(safe_divide<double>(static_cast<double>(spineEndX - spineStartX),
                                                    slotDivisor))) };
        // the tallest a rib can be, with a fish's ribs shortening toward the tail
        const int baseHalfHeight{ std::max(0, (drawArea.GetHeight() / 2) - branchLabelHeight -
                                                  edgeMargin) };
        const int ribShift{ std::max(0, std::min(static_cast<int>(baseHalfHeight * 0.4),
                                                 static_cast<int>(slotWidth * 0.35))) };
        const int twigLength{ static_cast<int>(slotWidth * 0.5) };
        // how much taller a rib gets as it approaches the head (at the tail's pair index,
        // the rib is this fraction of the tallest rib)
        constexpr double shortestRibRatio{ 0.62 };
        // the rib height for a pair (pairCount-1 is nearest the head), constant without a fish
        const auto ribHalfHeight = [pairCount, baseHalfHeight, hasFish](const int pairIndex)
        {
            // with one pair there is nothing to grow from, so it's already at full height
            if (!hasFish || pairCount <= 1)
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
                slots += getSlotWeight(cause);
                }
            mostSlots = std::max(mostSlots, slots);
            }
        const int labelGap{ static_cast<int>(ScaleToScreenAndCanvas(7)) };
        const int causeLabelHeight{
            makeLabel(L"Ag", wxPoint{ 0, 0 }, Anchoring::Center)->GetBoundingBox(dc).GetHeight()
        };
        // a label can't reach past the neighboring rib, so long text is wrapped
        // (and then truncated)
        const int maxCauseWidth{ std::max(1, static_cast<int>(slotWidth * 0.9) - labelGap) };
        const int maxSubCauseWidth{ std::max(1,
                                             maxCauseWidth - static_cast<int>(twigLength * 0.4)) };
        // the first branches are nearest the effect, so the pairs run right to left
        const auto getPairIndex = [pairCount](const size_t branchIndex)
        { return pairCount - 1 - static_cast<int>(branchIndex / 2); };
        // where a pair's ribs meet the spine
        const auto getAttachPoint = [spineStartX, slotWidth, spineY](const int pairIndex)
        {
            return wxPoint{ spineStartX + ((pairIndex + 1) * slotWidth) -
                                static_cast<int>(slotWidth * 0.05),
                            spineY };
        };
        // the spacing between the slots on a rib, which grows as the ribs approach the head
        const auto getSlotSpacing = [&ribHalfHeight, mostSlots](const int pairIndex)
        {
            return safe_divide<double>(static_cast<double>(ribHalfHeight(pairIndex)),
                                       static_cast<double>(mostSlots + 1));
        };
        // the height of a label's box on a rib, which may let its text wrap
        const auto getLabelBoxHeight =
            [&getSlotSpacing, causeLabelHeight](const int pairIndex, const double scale)
        {
            return std::max(static_cast<int>(getSlotSpacing(pairIndex)),
                            static_cast<int>(causeLabelHeight * scale) + 1);
        };

        constexpr double MIN_LABEL_SCALE{ 0.5 };
        const int cardPadding{ static_cast<int>(ScaleToScreenAndCanvas(CARD_PADDING_DIPS)) };
        // an idea card holds its status marker in a column at the start of its text
        const int cardMarkerWidth{ static_cast<int>(
            ScaleToScreenAndCanvas(CARD_MARKER_WIDTH_DIPS)) };
        int cardWidth{ 0 };
        int cardHeight{ 0 };
        if (drawCards)
            {
            // a note has room for a couple of lines of text
            const int preferredHeight{ (2 * causeLabelHeight) + (2 * cardPadding) };
            // the shortest rib has the least room per slot, and sets the height
            // for every card on the diagram
            const int slotSpacing{ static_cast<int>(std::lround(getSlotSpacing(0))) };
            // a dense rib still keeps room for one line, with the text shrunk to fit
            const int smallestHeight{ static_cast<int>((causeLabelHeight * MIN_LABEL_SCALE) +
                                                       (2 * cardPadding)) };
            cardHeight = std::max(smallestHeight, std::min(preferredHeight, slotSpacing));

            // the longest cause sets the width for every card, so that a short cause
            // sits in a card with room to spare
            const auto measureText = [&dc, &makeLabel](const wxString& text)
            {
                return makeLabel(text, wxPoint{ 0, 0 }, Anchoring::Center)
                    ->GetBoundingBox(dc)
                    .GetWidth();
            };
            int widestCause{ 0 };
            for (const auto* branch : shownBranches)
                {
                for (const auto& cause : branch->m_causes)
                    {
                    // a problem and its ideas sit in separate cards, so each is measured alone
                    widestCause =
                        std::max(widestCause, measureText(splitCause(cause.m_text).m_problem));
                    for (const auto& idea : getIdeas(cause.m_text))
                        {
                        // the marker's column is part of what the card has to hold
                        widestCause =
                            std::max(widestCause, measureText(idea.m_text) + cardMarkerWidth);
                        }
                    }
                }
            // cards on neighboring ribs share the gap, so each gets half of it, down to a minimum
            const int halfGap{ safe_divide<int>(slotWidth - (2 * labelGap), 2) };
            const int minCardWidth{ static_cast<int>(ScaleToScreenAndCanvas(44)) };
            const int cardMaxWidth{ hasIdeas ? std::max(halfGap, minCardWidth) : maxCauseWidth };
            cardWidth = std::min(cardMaxWidth, widestCause + (2 * cardPadding));
            }

        // all cause labels share one scale, starting with what fits the shortest rib's spacing
        double fitScale{ drawCards ?
                             1.0 :
                             std::clamp(safe_divide<double>(
                                            getSlotSpacing(0),
                                            static_cast<double>(causeLabelHeight + (2 * labelGap))),
                                        MIN_LABEL_SCALE, 1.0) };
        if (drawCards)
            {
            // a card's height is fixed, so a line of text has to fit inside it
            fitScale =
                std::min(fitScale, safe_divide<double>(cardHeight - (2 * cardPadding),
                                                       static_cast<double>(causeLabelHeight)));
            }
        // causes too long for the lines that fit shrink every label
        // (down to a point), rather than being truncated
        const double spacingScale{ fitScale };
        for (size_t i = 0; i < shownBranches.size(); ++i)
            {
            const int pairIndex{ getPairIndex(i) };
            // a card holds a fixed number of lines; a plain label holds as many as its rib allows
            const int linesThatFit{ std::max(
                1, safe_divide<int>(drawCards ? (cardHeight - (2 * cardPadding)) :
                                                getLabelBoxHeight(pairIndex, spacingScale),
                                    static_cast<int>(causeLabelHeight * spacingScale))) };
            const int causeWidth{ drawCards ? (cardWidth - (2 * cardPadding)) : maxCauseWidth };
            const auto fitText = [&](const wxString& text, const int maxWidth)
            {
                const int textWidth{ makeLabel(text, wxPoint{ 0, 0 }, Anchoring::Center,
                                               spacingScale)
                                         ->GetBoundingBox(dc)
                                         .GetWidth() };
                if (const int availableWidth{ maxWidth * linesThatFit }; textWidth > availableWidth)
                    {
                    fitScale = std::min(
                        fitScale, spacingScale * safe_divide<double>(availableWidth, textWidth));
                    }
            };
            for (const auto& cause : shownBranches[i]->m_causes)
                {
                // a problem and its ideas each get their own card, so each is fitted alone
                fitText(splitCause(cause.m_text).m_problem, causeWidth);
                for (const auto& idea : getIdeas(cause.m_text))
                    {
                    // the marker's column is not available to the text
                    fitText(idea.m_text, std::max(1, causeWidth - cardMarkerWidth));
                    }
                for (const auto& subCause : cause.m_subCauses)
                    {
                    fitText(subCause, maxSubCauseWidth);
                    }
                }
            }
        const double labelScale{ std::max(MIN_LABEL_SCALE, fitScale) };
        // stacked idea cards overlap by their scaled bottom padding, keeping the text visible
        const int ideaStackStep{ std::max(
            1, cardHeight - static_cast<int>(std::lround(cardPadding * labelScale))) };
        // how far a card may be nudged off of its slot without covering a neighbor's text
        const int jitterAmplitude{ std::clamp((cardPadding - 1) / 2, 1, 3) };
        // each card in a stack is also nudged right, so that it reads as a separate card
        const int ideaStackNudge{ jitterAmplitude };
        // recomputed per branch, as each rib's height varies
        int labelBoxHeight{ causeLabelHeight + 1 };
        const CauseLabelLayout causeLabelLayout{
            .m_cedacCardStyle = cardStyle,
            .m_labelScale = labelScale,
            .m_cardWidth = cardWidth,
            .m_cardHeight = cardHeight,
            .m_cardPadding = cardPadding,
            .m_cardWidthDIPs = static_cast<wxCoord>(std::lround(
                DownscaleFromScreenAndCanvas(safe_divide<double>(cardWidth, labelScale)))),
            .m_cardHeightDIPs = static_cast<wxCoord>(std::lround(
                DownscaleFromScreenAndCanvas(safe_divide<double>(cardHeight, labelScale)))),
            .m_scaling = GetScaling(),
            .m_dpiScaling = GetDPIScaleFactor(),
            .m_textColor = contrastedText
        };
        // fixed nudges look hand-assembled, while keeping the screen and exports identical
        const auto getCardOffset = [&](const size_t slotIndex)
        {
            if (!drawCards)
                {
                return wxPoint{ 0, 0 };
                }
            // alternating, with a two-step magnitude so that no two seams line up
            const int verticalOffset{ (((slotIndex % 2) == 0) ? -1 : 1) *
                                      (jitterAmplitude + static_cast<int>(slotIndex % 2)) };
            // the cards are also shifted sideways, so that their right edges don't line up
            const int horizontalOffset{ (((slotIndex % 4) < 2) ? -1 : 1) * jitterAmplitude };
            return wxPoint{ horizontalOffset, verticalOffset };
        };
        // an idea's status is shown at the start of its card as dots, a checkmark, or an X
        const auto addIdeaMarker = [this, cardHeight, cardPadding, causeLabelHeight,
                                    labelScale](const wxPoint& cardAnchor, const IdeaMarker& idea)
        {
            // the marker sits on the card's first line and doesn't scale with the text
            const int lineHeight{ static_cast<int>(causeLabelHeight * labelScale) };
            const int topPadding{ static_cast<int>(cardPadding * labelScale) };
            const int lineCenterY{ cardAnchor.y - cardHeight + topPadding + (lineHeight / 2) };
            const int markerLeft{ cardAnchor.x + topPadding };
            if (idea.IsSelected() || idea.IsRejected())
                {
                // a checkmark or an X, drawn in black if the diagram is monochrome
                const wxColour markerColor{ m_monochrome ?
                                                *wxBLACK :
                                                (idea.IsSelected() ? wxColour{ 46, 125, 50 } :
                                                                     wxColour{ 200, 45, 45 }) };
                const int glyphSize{ static_cast<int>(ScaleToScreenAndCanvas(8)) };
                const int left{ markerLeft };
                const int right{ markerLeft + glyphSize };
                const int top{ lineCenterY - (glyphSize / 2) };
                const int bottom{ lineCenterY + (glyphSize / 2) };
                auto strokes =
                    std::make_unique<GraphItems::Lines>(wxPen{ markerColor, 2 }, GetScaling());
                // a marker is ink on the card, so it keeps its color in dark mode
                strokes->GetAccessibilityAttributes().Class(_DT(L"ink-keep"));
                if (idea.IsRejected())
                    {
                    strokes->AddLine(wxPoint{ left, top }, wxPoint{ right, bottom });
                    strokes->AddLine(wxPoint{ left, bottom }, wxPoint{ right, top });
                    }
                else
                    {
                    // a short stroke down to the bottom, then a long one up to the right
                    const int elbowX{ left + ((glyphSize * 2) / 5) };
                    strokes->AddLine(wxPoint{ left, lineCenterY }, wxPoint{ elbowX, bottom });
                    strokes->AddLine(wxPoint{ elbowX, bottom }, wxPoint{ right, top });
                    }
                AddObject(std::move(strokes));
                return;
                }

            // one dot per level, filled in like the stickers on a CEDAC board
            const int dotRadius{ static_cast<int>(ScaleToScreenAndCanvas(2)) };
            const int dotGap{ static_cast<int>(ScaleToScreenAndCanvas(1)) };
            for (int dot = 0; dot < idea.GetDotCount(); ++dot)
                {
                AddObject(std::make_unique<GraphItems::Point2D>(
                    GraphItems::GraphItemInfo{}
                        .Brush(wxBrush{ *wxBLACK })
                        .Pen(wxPen{ *wxBLACK })
                        // ink on the card, so it keeps its color in dark mode
                        .Accessibility(wxSVGAttributes{}.Class(_DT(L"ink-keep")))
                        .Scaling(GetScaling())
                        .DPIScaling(GetDPIScaleFactor())
                        .Anchoring(Anchoring::Center)
                        .AnchorPoint(
                            wxPoint{ markerLeft + dotRadius + (dot * ((2 * dotRadius) + dotGap)),
                                     lineCenterY })
                        .Selectable(false),
                    2));
                }
        };

        if (hasFish)
            {
            // a position in the tail, as fractions of its width (from the left) and
            // its half height (from the spine, downward)
            const auto tailPoint = [&](const double across, const double down)
            {
                return wxPoint{ drawArea.GetX() + static_cast<int>(std::lround(tailWidth * across)),
                                spineY + static_cast<int>(std::lround(tailHalfHeight * down)) };
            };
            const wxPoint spinePt{ spineStartX, spineY };
            if (m_fishStyle == FishStyle::Shark)
                {
                // crescent shark tail: pointed tips, with the upper lobe longer than the lower
                AddObject(std::make_unique<TailFin>(
                    std::vector<wxPoint>{ spinePt,
                                          // upper leading edge, bowed slightly outward
                                          tailPoint(0.55, -0.65), tailPoint(0.0, -1.0),
                                          // upper trailing edge, curving in to the notch
                                          tailPoint(0.45, -0.45), tailPoint(0.5, 0.15),
                                          // lower trailing edge, curving out to the lower tip
                                          tailPoint(0.5, 0.45), tailPoint(0.2, 0.65),
                                          // lower leading edge, bowed slightly outward
                                          tailPoint(0.7, 0.5), spinePt },
                    spineColor, m_fishStyle, GetScaling()));
                }
            else
                {
                // forked whale-tail fin: two smooth quad bezier lobes with a concave notch
                // between them
                AddObject(std::make_unique<TailFin>(
                    std::vector<wxPoint>{ spinePt, tailPoint(0.0, -1.0), tailPoint(0.45, 0.0),
                                          tailPoint(0.0, 1.0), spinePt },
                    spineColor, m_fishStyle, GetScaling()));
                }

            // a dorsal fin above the middle top ribs, sitting on the line running along
            // their outer ends and fitted between their branch labels
            if (m_fishStyle == FishStyle::Shark && pairCount > 1 && slotWidth > 0)
                {
                const int frontPair{ pairCount / 2 };
                const int rearPair{ frontPair - 1 };
                const auto getOuterPoint = [&](const int pairIndex)
                {
                    return wxPoint{ getAttachPoint(pairIndex).x - ribShift,
                                    spineY - ribHalfHeight(pairIndex) };
                };
                const wxPoint frontOuter{ getOuterPoint(frontPair) };
                const wxPoint rearOuter{ getOuterPoint(rearPair) };
                // the top branch of a pair comes first
                const auto getLabelWidth = [&](const int pairIndex)
                {
                    if (!m_showBranchLabels)
                        {
                        return 0;
                        }
                    const size_t branchIndex{ static_cast<size_t>(2 *
                                                                  (pairCount - 1 - pairIndex)) };
                    return makeBranchLabel(getBranchLabelText(*shownBranches[branchIndex]),
                                           wxPoint{ 0, 0 }, spineColor)
                        ->GetBoundingBox(dc)
                        .GetWidth();
                };
                const int finMargin{ static_cast<int>(ScaleToScreenAndCanvas(6)) };
                const int labelShift{ m_showBranchLabels ? branchLabelOffset : 0 };
                const int gapLeft{ rearOuter.x - labelShift + (getLabelWidth(rearPair) / 2) +
                                   finMargin };
                const int gapRight{ frontOuter.x - labelShift - (getLabelWidth(frontPair) / 2) -
                                    finMargin };
                const int baseLength{ std::min(gapRight - gapLeft,
                                               static_cast<int>(slotWidth * 0.5)) };
                // the y position along the line through the ribs' outer ends
                const auto getBackY = [&](const double xPos)
                {
                    return rearOuter.y +
                           ((xPos - rearOuter.x) * safe_divide<double>(frontOuter.y - rearOuter.y,
                                                                       frontOuter.x - rearOuter.x));
                };
                const double rearX{ gapLeft + ((gapRight - gapLeft - baseLength) / 2.0) };
                const double frontX{ rearX + baseLength };
                const double rearY{ getBackY(rearX) };
                const double frontY{ getBackY(frontX) };
                // unit vector along the base (toward the head) and its upward normal
                const double baseDist{ std::hypot(frontX - rearX, frontY - rearY) };
                const double unitX{ safe_divide<double>(frontX - rearX, baseDist) };
                const double unitY{ safe_divide<double>(frontY - rearY, baseDist) };
                const double normalX{ unitY };
                const double normalY{ -unitX };
                // the tip is swept back toward the tail, and must stay inside the plot area
                constexpr double tipAlong{ 0.15 };
                const double finHeight{ std::min(baseDist * 1.4,
                                                 getBackY(rearX + (baseDist * tipAlong)) -
                                                     drawArea.GetTop() - edgeMargin) };
                // a position on the fin, measured along its base and up from it
                const auto finPoint = [&](const double along, const double upward)
                {
                    return wxPoint{
                        static_cast<int>(std::lround(rearX + (unitX * baseDist * along) +
                                                     (normalX * finHeight * upward))),
                        static_cast<int>(std::lround(rearY + (unitY * baseDist * along) +
                                                     (normalY * finHeight * upward)))
                    };
                };
                if (baseLength >= ScaleToScreenAndCanvas(12) &&
                    finHeight >= ScaleToScreenAndCanvas(8))
                    {
                    AddObject(std::make_unique<DorsalFin>(
                        finPoint(1.0, 0.0),
                        // bowed out to make the leading edge convex
                        finPoint(0.55, 0.75), finPoint(tipAlong, 1.0),
                        // pulled in toward the leading edge to make the trailing edge concave
                        finPoint(0.35, 0.3), finPoint(0.0, 0.0), spineColor));
                    }
                }

            // the eye and mouth: white or black, whichever contrasts with the head
            // (a colored shark has cold, black eyes, and a skeleton's match its bones)
            const wxColour featureColor{
                (m_fishStyle == FishStyle::Skeleton) ? spineColor :
                m_monochrome ? ((spineColor.GetLuminance() > 0.5) ? *wxBLACK : *wxWHITE) :
                (m_fishStyle == FishStyle::Shark) ? *wxBLACK :
                                                    *wxWHITE
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
            const int pairIndex{ getPairIndex(i) };
            // ribs grow taller as they approach the head, unless there is no fish to
            // make a rib cage of
            const int halfHeight{ ribHalfHeight(pairIndex) };
            labelBoxHeight = getLabelBoxHeight(pairIndex, labelScale);

            const wxPoint attachPt{ getAttachPoint(pairIndex) };
            const wxPoint outerPt{ attachPt.x - ribShift, spineY + (direction * halfHeight) };
            const wxColour branchColor{ getBranchColor(i) };
            // sub-causes are a lighter shade of their branch's color
            const wxColour subColor{ ensureVisible(
                Colors::ColorContrast::Tint(branchColor, 0.65)) };
            // the ribs are drawn with a constant width
            AddObject(makeLine(outerPt, attachPt, branchColor, 2));

            if (m_showBranchLabels)
                {
                AddObject(
                    makeBranchLabel(getBranchLabelText(branch),
                                    wxPoint{ outerPt.x - branchLabelOffset,
                                             outerPt.y + (direction * branchLabelHeight / 2) },
                                    branchColor));
                }

            // each cause and sub-cause gets an evenly spaced slot, in top-to-bottom order
            size_t slotCount{ 0 };
            for (const auto& cause : branch.m_causes)
                {
                slotCount += getSlotWeight(cause);
                }
            size_t slot{ 0 };
            // a cause takes a slot per card, which leaves room for its stack of ideas
            const auto getRibPoint = [&](const size_t weight)
            {
                const double fromTop{ safe_divide<double>(static_cast<double>(slot + 1),
                                                          static_cast<double>(slotCount + 1)) };
                // the rib is measured from its outer end, which is at the bottom for lower branches
                const double ratio{ isTop ? fromTop : 1.0 - fromTop };
                slot += weight;
                return wxPoint{ outerPt.x + static_cast<int>((attachPt.x - outerPt.x) * ratio),
                                outerPt.y + static_cast<int>((attachPt.y - outerPt.y) * ratio) };
            };
            for (const auto& cause : branch.m_causes)
                {
                const size_t slotIndex{ slot };
                const wxPoint causePt{ getRibPoint(getSlotWeight(cause)) };
                // a card covers its twig, so there is nothing to draw under it
                if (!drawCards)
                    {
                    AddObject(makeLine(causePt, wxPoint{ causePt.x - twigLength, causePt.y },
                                       branchColor, 1));
                    }
                const wxPoint cardOffset{ getCardOffset(slotIndex) };
                AddObject(MakeCauseLabel(dc, splitCause(cause.m_text).m_problem,
                                         wxPoint{ causePt.x - labelGap, causePt.y } + cardOffset,
                                         maxCauseWidth, labelBoxHeight, branchColor,
                                         causeLabelLayout, CedacCardKind::Problem));
                // the ideas are stacked across the rib from their problem
                const auto ideas{ getIdeas(cause.m_text) };
                for (size_t ideaIndex = 0; ideaIndex < ideas.size(); ++ideaIndex)
                    {
                    const wxPoint ideaAnchor{
                        wxPoint{ causePt.x + labelGap +
                                     (static_cast<int>(ideaIndex) * ideaStackNudge),
                                 causePt.y + (static_cast<int>(ideaIndex) * ideaStackStep) } +
                        cardOffset
                    };
                    AddObject(MakeCauseLabel(dc, ideas[ideaIndex].m_text, ideaAnchor, maxCauseWidth,
                                             labelBoxHeight, branchColor, causeLabelLayout,
                                             CedacCardKind::Idea, ideas[ideaIndex].IsRejected()));
                    // the marker is drawn on top of the card that was just added
                    addIdeaMarker(ideaAnchor, ideas[ideaIndex]);
                    }
                // a dot where the cause meets the rib
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
                    const wxPoint subPt{ getRibPoint(1) };
                    AddObject(makeLine(wxPoint{ connectorX, subPt.y },
                                       wxPoint{ connectorX - subLength, subPt.y }, subColor, 1));
                    AddObject(MakeCauseLabel(
                        dc, subCause, wxPoint{ connectorX - labelGap, subPt.y }, maxSubCauseWidth,
                        labelBoxHeight, subColor, causeLabelLayout, CedacCardKind::Problem));
                    lastY = subPt.y;
                    }
                AddObject(makeLine(wxPoint{ connectorX, causePt.y }, wxPoint{ connectorX, lastY },
                                   subColor, 1));
                }
            }

        // a skeleton's vertebrae cross the spine at even intervals, growing toward the head
        if (m_fishStyle == FishStyle::Skeleton)
            {
            const int vertebraSpacing{ static_cast<int>(ScaleToScreenAndCanvas(16)) };
            const int vertebraCount{ safe_divide<int>(spineEndX - spineStartX, vertebraSpacing) };
            if (vertebraCount > 1)
                {
                const double vertebraGap{ safe_divide<double>(spineEndX - spineStartX,
                                                              vertebraCount) };
                const double smallestHalfHeight{ ScaleToScreenAndCanvas(4) };
                const double largestHalfHeight{ ScaleToScreenAndCanvas(8) };
                auto vertebrae =
                    std::make_unique<GraphItems::Lines>(wxPen{ spineColor, 2 }, GetScaling());
                for (int i = 1; i < vertebraCount; ++i)
                    {
                    const int vertebraX{ spineStartX +
                                         static_cast<int>(std::lround(vertebraGap * i)) };
                    const double headward{ safe_divide<double>(i, vertebraCount) };
                    const int vertebraHalfHeight{ static_cast<int>(
                        std::lround(smallestHalfHeight +
                                    ((largestHalfHeight - smallestHalfHeight) * headward))) };
                    vertebrae->AddLine(wxPoint{ vertebraX, spineY - vertebraHalfHeight },
                                       wxPoint{ vertebraX, spineY + vertebraHalfHeight });
                    }
                AddObject(std::move(vertebrae));
                }
            }

        // the spine is drawn over the ribs, in red if there is no fish and it isn't monochrome
        const wxColour spineLineColor{ (hasFish || m_monochrome) ? spineColor :
                                                                   wxColour{ 220, 40, 45 } };
        // without a fish, the spine ends at the base of an arrow head to hide its round cap
        const int arrowLength{ hasFish ? 0 : static_cast<int>(ScaleToScreenAndCanvas(14)) };
        const int arrowHalfHeight{ static_cast<int>(ScaleToScreenAndCanvas(6)) };
        AddObject(makeLine(wxPoint{ spineStartX, spineY },
                           wxPoint{ spineEndX - arrowLength, spineY }, spineLineColor, 3));
        if (!hasFish)
            {
            const std::array<wxPoint, 3> arrowPoints{
                wxPoint{ spineEndX, spineY },
                wxPoint{ spineEndX - arrowLength, spineY - arrowHalfHeight },
                wxPoint{ spineEndX - arrowLength, spineY + arrowHalfHeight }
            };
            AddObject(std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo{}
                                                                .Pen(wxNullPen)
                                                                .Brush(spineLineColor)
                                                                .Scaling(GetScaling())
                                                                .DPIScaling(GetDPIScaleFactor()),
                                                            arrowPoints));
            }
        }
    } // namespace Wisteria::Graphs
