///////////////////////////////////////////////////////////////////////////////
// Name:        ishikawadiagram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "ishikawadiagram.h"

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
        // the 6 Ms
        switch (index)
            {
        case 0:
            return _(L"People"); // "Man"
        case 1:
            return _(L"Machine");
        case 2:
            return _(L"Material");
        case 3:
            return _(L"Method");
        case 4:
            return _(L"Measurement");
        case 5:
            return _(L"Environment"); // "Mother Nature"
        default:
            return {};
            }
        }

    //----------------------------------------------------------------
    bool IshikawaDiagram::IsDefaultBranchName(const size_t index, const wxString& name)
        {
        const wxString defaultLabel{ GetDefaultBranchLabel(index) };
        return (!defaultLabel.empty() && name.CmpNoCase(defaultLabel) == 0) ||
               (index == 0 && name.CmpNoCase(L"Man") == 0);
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
            label +=
                wxString::Format(_(L". %s: %zu causes"), branch.m_label, branch.m_causes.size());
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

        const auto makeLabel = [this](const wxString& text, const wxPoint& pt,
                                      const Anchoring anchoring, const double scale = 1.0)
        {
            return std::make_unique<GraphItems::Label>(GraphItems::GraphItemInfo{ text }
                                                           .Pen(wxNullPen)
                                                           .Scaling(GetScaling() * scale)
                                                           .DPIScaling(GetDPIScaleFactor())
                                                           .Anchoring(anchoring)
                                                           .AnchorPoint(pt)
                                                           .Selectable(false));
        };
        // the spine and effect use the text color, and each branch gets its own color
        const wxColour mainColor{ GetLeftYAxis().GetFontColor() };
        const Colors::Schemes::ColorScheme defaultScheme{ Settings::GetDefaultColorScheme() };
        const std::vector<wxColour> branchColors{ (GetColorScheme() != nullptr &&
                                                   !GetColorScheme()->GetColors().empty()) ?
                                                      GetColorScheme()->GetColors() :
                                                      defaultScheme.GetColors() };
        // monochrome diagram uses the text color for every branch
        const auto getBranchColor = [this, &branchColors, &mainColor](const size_t index)
        { return m_monochrome ? mainColor : branchColors[index % branchColors.size()]; };

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
            return makeBoxedLabel(text, pt, tint, color, Colors::ColorContrast::Shade(color, 0.2));
        };

        // effect box at the right end of the spine, styled like the branch labels
        // (it is sized by the effect's text, or by a placeholder if there isn't any)
        auto effectLabel =
            makeBranchLabel(m_effect.empty() ? _(L"Effect") : m_effect, wxPoint{ 0, 0 }, mainColor);
        // line breaks in the effect are kept, and long lines are wrapped to fit the box
        const int maxEffectWidth{ static_cast<int>(drawArea.GetWidth() * 0.3) };
        if (effectLabel->GetBoundingBox(dc).GetWidth() > maxEffectWidth)
            {
            effectLabel->SplitTextToFitBoundingBox(
                dc, wxSize{ maxEffectWidth - static_cast<int>(ScaleToScreenAndCanvas(16)),
                            drawArea.GetHeight() / 2 });
            }
        const wxRect effectRect{ effectLabel->GetBoundingBox(dc) };
        const int effectWidth{ std::min(effectRect.GetWidth(), maxEffectWidth) };
        const int spineY{ drawArea.GetY() + (drawArea.GetHeight() / 2) };
        const int spineEndX{ drawArea.GetRight() - effectWidth };
        effectLabel->SetAnchorPoint(wxPoint{ spineEndX + (effectWidth / 2), spineY });

        // the tail fin takes up the left end of the plot area, and the spine starts after it
        const int tailWidth{ std::min<int>(ScaleToScreenAndCanvas(28),
                                           drawArea.GetWidth() * 0.06) };
        const int tailHalfHeight{ static_cast<int>(tailWidth * 0.7) };
        const int spineStartX{ drawArea.GetX() + tailWidth };

        // without text, the box is drawn as a plain shape so that the spine still has a head
        std::unique_ptr<GraphItems::Polygon> effectBox;
        if (m_effect.empty())
            {
            const int effectHalfHeight{ effectRect.GetHeight() / 2 };
            const std::array<wxPoint, 4> effectBoxPoints{
                wxPoint{ spineEndX, spineY - effectHalfHeight },
                wxPoint{ spineEndX + effectWidth, spineY - effectHalfHeight },
                wxPoint{ spineEndX + effectWidth, spineY + effectHalfHeight },
                wxPoint{ spineEndX, spineY + effectHalfHeight }
            };
            effectBox = std::make_unique<GraphItems::Polygon>(
                GraphItems::GraphItemInfo{}
                    .Pen(wxPen{ mainColor, 2 })
                    .Brush(wxBrush{ Colors::ColorContrast::Tint(mainColor, 0.9) })
                    .Scaling(GetScaling())
                    .Selectable(false),
                effectBoxPoints);
            }

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
        const int halfHeight{ std::max(0, (drawArea.GetHeight() / 2) - branchLabelHeight -
                                              edgeMargin) };
        const int ribShift{ std::max(
            0, std::min(static_cast<int>(halfHeight * 0.4), static_cast<int>(slotWidth * 0.35))) };
        const int twigLength{ static_cast<int>(slotWidth * 0.5) };

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
        const int labelGap{ static_cast<int>(ScaleToScreenAndCanvas(2)) };
        const int causeLabelHeight{
            makeLabel(L"Ag", wxPoint{ 0, 0 }, Anchoring::Center)->GetBoundingBox(dc).GetHeight()
        };
        const double slotSpacing{ safe_divide<double>(static_cast<double>(halfHeight),
                                                      static_cast<double>(mostSlots + 1)) };
        const double labelScale{ std::clamp(
            safe_divide<double>(slotSpacing,
                                static_cast<double>(causeLabelHeight + (2 * labelGap))),
            0.5, 1.0) };
        const int labelBoxHeight{ std::max(static_cast<int>(slotSpacing),
                                           static_cast<int>(causeLabelHeight * labelScale) + 1) };
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

        // forked tail fin, styled like the effect box
        const std::array<wxPoint, 4> tailPoints{
            wxPoint{ spineStartX, spineY }, wxPoint{ drawArea.GetX(), spineY - tailHalfHeight },
            wxPoint{ drawArea.GetX() + static_cast<int>(tailWidth * 0.35), spineY },
            wxPoint{ drawArea.GetX(), spineY + tailHalfHeight }
        };
        AddObject(std::make_unique<GraphItems::Polygon>(
            GraphItems::GraphItemInfo{}
                .Pen(wxPen{ mainColor, 2 })
                .Brush(wxBrush{ Colors::ColorContrast::Tint(mainColor, 0.9) })
                .Scaling(GetScaling())
                .Selectable(false),
            tailPoints));

        // the spine is thicker than the ribs, and it either widens toward the effect
        // or ends with an arrowhead pointing at it
        if (m_taperedLines)
            {
            AddObject(makeStroke(wxPoint{ spineStartX, spineY }, wxPoint{ spineEndX, spineY },
                                 mainColor, 2, 6, 3));
            }
        else
            {
            auto spine = makeLine(wxPoint{ spineStartX, spineY }, wxPoint{ spineEndX, spineY },
                                  mainColor, 3);
            spine->SetLineStyle(LineStyle::Arrows);
            spine->SetArrowheadScale(0.75);
            AddObject(std::move(spine));
            }
        if (effectBox != nullptr)
            {
            AddObject(std::move(effectBox));
            }
        else
            {
            AddObject(std::move(effectLabel));
            }

        for (size_t i = 0; i < shownBranches.size(); ++i)
            {
            const auto& branch{ *shownBranches[i] };
            const bool isTop{ i % 2 == 0 };
            const int direction{ isTop ? -1 : 1 };
            // the first branches are nearest the effect, so the pairs run right to left
            const int pairIndex{ pairCount - 1 - static_cast<int>(i / 2) };

            const wxPoint attachPt{ spineStartX + ((pairIndex + 1) * slotWidth) -
                                        static_cast<int>(slotWidth * 0.05),
                                    spineY };
            const wxPoint outerPt{ attachPt.x - ribShift, spineY + (direction * halfHeight) };
            const wxColour branchColor{ getBranchColor(i) };
            // sub-causes are a lighter shade of their branch's color
            const wxColour subColor{ Colors::ColorContrast::Tint(branchColor, 0.65) };
            // the ribs widen toward the spine
            AddObject(makeStroke(outerPt, attachPt, branchColor, 1, 3, 2));

            if (m_showBranchLabels)
                {
                const wxString labelText{ m_showCauseCounts ?
                                              wxString::Format(
                                                  /* TRANSLATORS: branch label of a cause-and-effect
                                                     diagram. 1st %s is the branch's name, 2nd is
                                                     the number of causes in the branch. */
                                                  _(L"%s (%zu)"),
                                                  branch.m_label, branch.m_causes.size()) :
                                              branch.m_label };
                AddObject(makeBranchLabel(
                    labelText,
                    wxPoint{ outerPt.x, outerPt.y + (direction * branchLabelHeight / 2) },
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
        }
    } // namespace Wisteria::Graphs
