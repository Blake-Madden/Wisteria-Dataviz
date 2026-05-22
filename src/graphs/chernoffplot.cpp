///////////////////////////////////////////////////////////////////////////////
// Name:        chernoffplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "chernoffplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ChernoffFacesPlot, Wisteria::Graphs::Graph2D)
    wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ChernoffFacesPlot::ChernoffLegend,
                              GraphItems::GraphItemBase)

        namespace Wisteria::Graphs
    {
    static_assert(static_cast<int>(FacialHair::FACIAL_HAIR_COUNT) ==
                      static_cast<int>(HairAccessory::HAIR_ACCESSORY_COUNT),
                  "FacialHair and HairAccessory must have the same number of values");

    static_assert(static_cast<int>(HairStyleFemale::FEMALE_HAIR_STYLE_COUNT) ==
                      static_cast<int>(HairStyleMale::MALE_HAIR_STYLE_COUNT),
                  "HairStyleFemale and HairStyleMale must have the same number of values");

    namespace
        {
        // internal unified representation of every distinct hair style that DrawFace knows
        // how to render. The public API exposes gender-specific HairStyleFemale and
        // HairStyleMale enums; this helper collapses both to a single kind so the
        // drawing branches don't need to be duplicated per gender.
        enum class HairStyleKind
            {
            Bob,           // female only
            Pixie,         // female only
            Bun,           // female only
            PartiallyBald, // male only
            BaldCombOver,  // male only
            CombOver,      // male only
            LongStraight,  // shared
            HighTopFade,   // shared
            FlatTop,       // shared
            Curly,         // shared
            LongCurly,     // shared
            Bald           // shared
            };

        [[nodiscard]]
        HairStyleKind ToHairStyleKind(const HairStyleFemale hsFemale, const HairStyleMale hsMale,
                                      const Gender gender) noexcept
            {
            if (gender == Gender::Female)
                {
                switch (hsFemale)
                    {
                case HairStyleFemale::Bob:
                    return HairStyleKind::Bob;
                case HairStyleFemale::Pixie:
                    return HairStyleKind::Pixie;
                case HairStyleFemale::Bun:
                    return HairStyleKind::Bun;
                case HairStyleFemale::LongStraight:
                    return HairStyleKind::LongStraight;
                case HairStyleFemale::HighTopFade:
                    return HairStyleKind::HighTopFade;
                case HairStyleFemale::FlatTop:
                    return HairStyleKind::FlatTop;
                case HairStyleFemale::Curly:
                    return HairStyleKind::Curly;
                case HairStyleFemale::LongCurly:
                    return HairStyleKind::LongCurly;
                case HairStyleFemale::Bald:
                    return HairStyleKind::Bald;
                case HairStyleFemale::FEMALE_HAIR_STYLE_COUNT:
                    [[fallthrough]];
                default:
                    return HairStyleKind::Bob;
                    }
                }
            switch (hsMale)
                {
            case HairStyleMale::PartiallyBald:
                return HairStyleKind::PartiallyBald;
            case HairStyleMale::BaldCombOver:
                return HairStyleKind::BaldCombOver;
            case HairStyleMale::CombOver:
                return HairStyleKind::CombOver;
            case HairStyleMale::LongStraight:
                return HairStyleKind::LongStraight;
            case HairStyleMale::HighTopFade:
                return HairStyleKind::HighTopFade;
            case HairStyleMale::FlatTop:
                return HairStyleKind::FlatTop;
            case HairStyleMale::Curly:
                return HairStyleKind::Curly;
            case HairStyleMale::LongCurly:
                return HairStyleKind::LongCurly;
            case HairStyleMale::Bald:
                return HairStyleKind::Bald;
            case HairStyleMale::MALE_HAIR_STYLE_COUNT:
                [[fallthrough]];
            default:
                return HairStyleKind::PartiallyBald;
                }
            }
        } // namespace

    //----------------------------------------------------------------
    wxString ChernoffFacesPlot::GetFeatureDisplayName(FeatureId id)
        {
        switch (id)
            {
        case FeatureId::FaceWidth:
            return _(L"Face width");
        case FeatureId::FaceHeight:
            return _(L"Face height");
        case FeatureId::EyeSize:
            return _(L"Eye size");
        case FeatureId::EyePosition:
            return _(L"Eye position");
        case FeatureId::EyebrowSlant:
            return _(L"Eyebrow slant");
        case FeatureId::PupilDirection:
            return _(L"Pupil direction");
        case FeatureId::NoseSize:
            return _(L"Nose size");
        case FeatureId::MouthWidth:
            return _(L"Mouth width");
        case FeatureId::SmileFrown:
            return _(L"Smile/frown");
        case FeatureId::FaceColor:
            return _(L"Face color");
        case FeatureId::EarSize:
            return _(L"Ear size");
        case FeatureId::HairStyle:
            return _(L"Hair style");
        case FeatureId::HairAddition:
            return _(L"Hair addition");
        default:
            return wxString{};
            }
        }

    //----------------------------------------------------------------
    wxString ChernoffFacesPlot::GetFeatureColumnName(const FeatureId id) const
        {
        switch (id)
            {
        case FeatureId::FaceWidth:
            return m_faceWidthColumnName;
        case FeatureId::FaceHeight:
            return m_faceHeightColumnName;
        case FeatureId::EyeSize:
            return m_eyeSizeColumnName;
        case FeatureId::EyePosition:
            return m_eyePositionColumnName;
        case FeatureId::EyebrowSlant:
            return m_eyebrowSlantColumnName;
        case FeatureId::PupilDirection:
            return m_pupilPositionColumnName;
        case FeatureId::NoseSize:
            return m_noseSizeColumnName;
        case FeatureId::MouthWidth:
            return m_mouthWidthColumnName;
        case FeatureId::SmileFrown:
            return m_mouthCurvatureColumnName;
        case FeatureId::FaceColor:
            return m_faceSaturationColumnName;
        case FeatureId::EarSize:
            return m_earSizeColumnName;
        case FeatureId::HairStyle:
            return m_hairStyleColumnName;
        case FeatureId::HairAddition:
            return m_hairAdditionColumnName;
        default:
            return {};
            }
        }

    //----------------------------------------------------------------
    wxRect ChernoffFacesPlot::FaceObject::GetBoundingBox([[maybe_unused]]
                                                         wxDC &
                                                         dc) const
        {
        const auto scaledSize = wxSize(ScaleToScreenAndCanvas(m_size.GetWidth()),
                                       ScaleToScreenAndCanvas(m_size.GetHeight()));

        switch (GetAnchoring())
            {
        case Anchoring::TopLeftCorner:
            return { GetAnchorPoint(), scaledSize };
        case Anchoring::TopRightCorner:
            return { wxPoint(GetAnchorPoint().x - scaledSize.GetWidth(), GetAnchorPoint().y),
                     scaledSize };
        case Anchoring::BottomLeftCorner:
            return { wxPoint(GetAnchorPoint().x, GetAnchorPoint().y - scaledSize.GetHeight()),
                     scaledSize };
        case Anchoring::BottomRightCorner:
            return { wxPoint(GetAnchorPoint().x - scaledSize.GetWidth(),
                             GetAnchorPoint().y - scaledSize.GetHeight()),
                     scaledSize };
        case Anchoring::Center:
            [[fallthrough]];
        default:
            return { wxPoint(GetAnchorPoint().x - scaledSize.GetWidth() / 2,
                             GetAnchorPoint().y - scaledSize.GetHeight() / 2),
                     scaledSize };
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::FaceObject::SetBoundingBox(
        const wxRect& rect, [[maybe_unused]] wxDC& dc, [[maybe_unused]] double parentScaling)
        {
        SetAnchorPoint(rect.GetTopLeft());
        SetAnchoring(Anchoring::TopLeftCorner);
        m_size = rect.GetSize();
        }

    //----------------------------------------------------------------
    wxRect ChernoffFacesPlot::FaceObject::Draw(wxDC & dc) const
        {
        if (!IsShown())
            {
            return {};
            }

        const wxRect boundingBox = GetBoundingBox(dc);

            {
            const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
            auto* gc = gcf.GetGraphicsContext();
            if (gc != nullptr)
                {
                ChernoffFacesPlot::DrawFace(gc, boundingBox, m_features, m_faceColorLighter,
                                            m_faceColorDarker, m_outlineColor, m_lipstickColor,
                                            m_eyeColor, m_hairColor, m_hairStyleFemale,
                                            m_hairStyleMale, m_gender);
                }
            }

        // draw hair accessory on top (female only)
        if (m_gender == Gender::Female && m_features.m_hasHairAccessory)
            {
            Icons::IconShape accShape{ Icons::IconShape::Blank };
            switch (m_features.m_hairAccessory)
                {
            case HairAccessory::Butterfly:
                accShape = Icons::IconShape::Butterfly;
                break;
            case HairAccessory::Flower:
                accShape = Icons::IconShape::Flower;
                break;
            case HairAccessory::SunFlower:
                accShape = Icons::IconShape::Sunflower;
                break;
            case HairAccessory::Heart:
                accShape = Icons::IconShape::Heart;
                break;
            case HairAccessory::Leaf:
                accShape = Icons::IconShape::FallLeaf;
                break;
            case HairAccessory::Snowflake:
                accShape = Icons::IconShape::Snowflake;
                break;
            case HairAccessory::Star:
                accShape = Icons::IconShape::Star;
                break;
            case HairAccessory::Pumpkin:
                accShape = Icons::IconShape::Pumpkin;
                break;
            default:
                break;
                }

            if (accShape != Icons::IconShape::Blank)
                {
                // position accessory in the hair area (upper portion of head,
                // above the eyes/forehead where bangs/hair are drawn). uses the
                // same face geometry as DrawFace() so it follows face shape.
                const double cx =
                    boundingBox.GetX() + boundingBox.GetWidth() * math_constants::half;
                const double cy =
                    boundingBox.GetY() + boundingBox.GetHeight() * math_constants::half;
                const double baseRadius =
                    std::min(boundingBox.GetWidth(), boundingBox.GetHeight()) * 0.4;
                const double faceWidth = baseRadius * (0.7 + 0.6 * m_features.m_faceWidth);
                const double faceHeight = baseRadius * (0.8 + 0.4 * m_features.m_faceHeight);

                const int accSize = static_cast<int>(faceHeight * 0.5);
                const int accCenterX = static_cast<int>(cx + faceWidth * 0.5);
                const int accCenterY = static_cast<int>(cy - faceHeight * 0.7);
                const wxRect accRect{ accCenterX - accSize / 2, accCenterY - accSize / 2, accSize,
                                      accSize };
                GraphItems::Shape sh(GraphItems::GraphItemInfo{}
                                         .Pen(wxPen{ m_outlineColor, 1 })
                                         .Anchoring(Anchoring::TopLeftCorner)
                                         .Scaling(GetScaling())
                                         .DPIScaling(GetDPIScaleFactor()),
                                     accShape, accRect.GetSize());

                // some accessories tilt 25° clockwise so they don't sit perfectly
                // upright against the hair
                const bool tiltAccessory =
                    (m_features.m_hairAccessory == HairAccessory::Butterfly ||
                     m_features.m_hairAccessory == HairAccessory::Heart ||
                     m_features.m_hairAccessory == HairAccessory::Pumpkin ||
                     m_features.m_hairAccessory == HairAccessory::Star);
                auto* accGc = dc.GetGraphicsContext();
                if (tiltAccessory && accGc != nullptr)
                    {
                    const double rotationRad = (25.0 * std::numbers::pi) / 180.0;
                    accGc->PushState();
                    accGc->Translate(accCenterX, accCenterY);
                    accGc->Rotate(rotationRad);
                    accGc->Translate(-accCenterX, -accCenterY);
                    sh.Draw(accRect, dc);
                    accGc->PopState();
                    }
                else
                    {
                    sh.Draw(accRect, dc);
                    }
                }
            }

        return boundingBox;
        }

    //----------------------------------------------------------------
    wxRect ChernoffFacesPlot::ChernoffLegend::GetBoundingBox([[maybe_unused]]
                                                             wxDC &
                                                             dc) const
        {
        return m_rect;
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::ChernoffLegend::SetBoundingBox(
        const wxRect& rect, [[maybe_unused]] wxDC& dc, [[maybe_unused]] double parentScaling)
        {
        m_rect = rect;
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::ChernoffLegend::RecalcSizes(wxDC & dc)
        {
        // calculate minimum size needed for the legend, preserving current position
        const wxPoint currentPos = m_rect.GetPosition();

        const auto lineGap = ScaleToScreenAndCanvas(5);
        // Face is elongated: height = 1.5 * width, and we want it 2x larger.
        // When either bottom section has many entries, halve the face so the
        // legend stays compact.
        const auto maxBottomEntries =
            std::max(m_hairStyleLabels.size(), m_hairAdditionLabels.size());
        const auto minFaceHeight =
            maxBottomEntries > 4 ? ScaleToScreenAndCanvas(80) : ScaleToScreenAndCanvas(160);
        const auto minFaceWidth = minFaceHeight * math_constants::two_thirds;

        // measure label widths at reduced scaling (labels will be scaled to fit)
        const double labelScaling = GetScaling() * 0.6;
        int maxLeftLabelWidth = 0;
        int maxRightLabelWidth = 0;
        int leftCount = 0;
        int rightCount = 0;

        // pre-count features per side so we can force single-line labels when there
        // are many on a side (otherwise two-line labels stack too tightly to read)
        for (const auto& feature : m_features)
            {
            if (!feature.m_columnName.empty())
                {
                if (feature.m_leftSide)
                    {
                    ++leftCount;
                    }
                else
                    {
                    ++rightCount;
                    }
                }
            }
        const bool forceSingleLine = (leftCount > 4 || rightCount > 4);

        for (const auto& feature : m_features)
            {
            if (!feature.m_columnName.empty())
                {
                const wxString joiner =
                    (forceSingleLine || feature.m_columnName.length() <= 8) ? L": " : L":\n";
                GraphItems::Label label(GraphItems::GraphItemInfo{
                    GetFeatureDisplayName(feature.m_featureId) + joiner +
                    L"<span style='font-style:italic;'>" + feature.m_columnName + L"</span>" }
                                            .Pen(wxNullPen)
                                            .Scaling(labelScaling)
                                            .DPIScaling(GetDPIScaleFactor()));
                label.EnableMarkup(true);
                const auto labelSize = label.GetBoundingBox(dc).GetSize();

                if (feature.m_leftSide)
                    {
                    maxLeftLabelWidth = std::max(maxLeftLabelWidth, labelSize.GetWidth());
                    }
                else
                    {
                    maxRightLabelWidth = std::max(maxRightLabelWidth, labelSize.GetWidth());
                    }
                }
            }

        // calculate minimum dimensions
        // width: left labels + gap + face + gap + right labels
        const int minWidthForFace =
            maxLeftLabelWidth + lineGap + minFaceWidth + lineGap + maxRightLabelWidth;

        // height: face uses 2/3 of height, so total = faceHeight * 3/2
        const auto labelHeight = ScaleToScreenAndCanvas(14);
        const int labelsHeight = std::max(leftCount, rightCount) * labelHeight + (labelHeight / 2);
        const auto minHeightForFace = std::max<int>(minFaceHeight * 3 / 2, labelsHeight);

        // additional space needed for the hair-style and hair-addition key sections
        // (rendered side-by-side below the face)
        const auto hairIconSize = ScaleToScreenAndCanvas(28);
        const auto hairEntryGap = ScaleToScreenAndCanvas(4);
        const auto sectionGap = ScaleToScreenAndCanvas(20);

        const auto measureSection = [&](const std::vector<wxString>& labels,
                                        const wxString& columnName, int& outWidth, int& outHeight)
        {
            outWidth = 0;
            outHeight = 0;
            if (labels.empty())
                {
                return;
                }
            int maxHairLabelWidth{ 0 };
            for (const auto& hairLabel : labels)
                {
                const GraphItems::Label widthLbl(GraphItems::GraphItemInfo{ hairLabel }
                                                     .Pen(wxNullPen)
                                                     .Scaling(labelScaling)
                                                     .DPIScaling(GetDPIScaleFactor()));
                maxHairLabelWidth =
                    std::max(maxHairLabelWidth, widthLbl.GetBoundingBox(dc).GetWidth());
                }
            int headerWidth{ 0 };
            int headerHeight{ 0 };
            if (!columnName.empty())
                {
                GraphItems::Label headerLbl(GraphItems::GraphItemInfo{
                    L"<span style='font-style:italic;'>" + columnName + L"</span>" }
                                                .Pen(wxNullPen)
                                                .Scaling(labelScaling)
                                                .DPIScaling(GetDPIScaleFactor()));
                headerLbl.EnableMarkup(true);
                const auto headerBox = headerLbl.GetBoundingBox(dc);
                headerWidth = headerBox.GetWidth();
                headerHeight = headerBox.GetHeight() + ScaleToScreenAndCanvas(4);
                }
            outWidth = std::max<int>(hairIconSize + lineGap + maxHairLabelWidth, headerWidth);
            outHeight = headerHeight +
                        static_cast<int>(labels.size()) * (hairIconSize + hairEntryGap) +
                        ScaleToScreenAndCanvas(10);
        };

        int hairStyleSectionWidth{ 0 };
        int hairStyleSectionHeight{ 0 };
        measureSection(m_hairStyleLabels, m_hairStyleColumnName, hairStyleSectionWidth,
                       hairStyleSectionHeight);

        int hairAdditionSectionWidth{ 0 };
        int hairAdditionSectionHeight{ 0 };
        measureSection(m_hairAdditionLabels, m_hairAdditionColumnName, hairAdditionSectionWidth,
                       hairAdditionSectionHeight);

        const bool bothSections = !m_hairStyleLabels.empty() && !m_hairAdditionLabels.empty();
        const int bottomSectionWidth =
            hairStyleSectionWidth + (bothSections ? sectionGap : 0) + hairAdditionSectionWidth;
        const int bottomSectionHeight = std::max(hairStyleSectionHeight, hairAdditionSectionHeight);

        const int minWidth = std::max(minWidthForFace, bottomSectionWidth);
        const auto minHeight = minHeightForFace + bottomSectionHeight;

        m_rect = wxRect{ currentPos, wxSize{ minWidth, minHeight } };
        }

    //----------------------------------------------------------------
    wxRect ChernoffFacesPlot::ChernoffLegend::Draw(wxDC & dc) const
        {
        if (!IsShown() || m_rect.IsEmpty())
            {
            return {};
            }

        // collect features for each side with their IDs for connection points
        struct FeatureInfo
            {
            wxString labelText;
            FeatureId featureId;
            };

        std::vector<FeatureInfo> leftFeatures;
        leftFeatures.reserve(m_features.size());
        std::vector<FeatureInfo> rightFeatures;
        rightFeatures.reserve(m_features.size());

        // count features per side so a side with many entries can force single-line
        // labels (two-line labels stack too tightly to read past ~4 per side)
        int leftFeatureCount{ 0 };
        int rightFeatureCount{ 0 };
        for (const auto& [columnName, featureId, leftSide] : m_features)
            {
            if (!columnName.empty())
                {
                if (leftSide)
                    {
                    ++leftFeatureCount;
                    }
                else
                    {
                    ++rightFeatureCount;
                    }
                }
            }
        const bool forceSingleLineLabels = (leftFeatureCount > 4 || rightFeatureCount > 4);

        for (const auto& [columnName, featureId, leftSide] : m_features)
            {
            if (!columnName.empty())
                {
                const wxString displayName = GetFeatureDisplayName(featureId);
                const wxString joiner =
                    (forceSingleLineLabels || columnName.length() <= 8) ? L": " : L":\n";
                const FeatureInfo info{ displayName + joiner +
                                            L"<span style='font-style:italic;'>" + columnName +
                                            L"</span>",
                                        featureId };
                if (leftSide)
                    {
                    leftFeatures.push_back(info);
                    }
                else
                    {
                    rightFeatures.push_back(info);
                    }
                }
            }

        // height reserved for the hair-style and hair-addition key sections drawn
        // below the face (rendered as side-by-side columns)
        const auto hairEntryGap = ScaleToScreenAndCanvas(4);
        const auto hairBottomPadding = ScaleToScreenAndCanvas(10);
        const auto minHairIconSize = ScaleToScreenAndCanvas(8);
        auto hairIconSize = ScaleToScreenAndCanvas(28);
        const double headerScaling = GetScaling() * 0.6;

        const auto measureHeaderHeight = [&](const std::vector<wxString>& labels,
                                             const wxString& columnName) -> int
        {
            if (labels.empty() || columnName.empty())
                {
                return 0;
                }
            GraphItems::Label hdrLbl(GraphItems::GraphItemInfo{
                L"<span style='font-style:italic;'>" + columnName + L"</span>" }
                                         .Pen(wxNullPen)
                                         .Scaling(headerScaling)
                                         .DPIScaling(GetDPIScaleFactor()));
            hdrLbl.EnableMarkup(true);
            return hdrLbl.GetBoundingBox(dc).GetHeight() + ScaleToScreenAndCanvas(4);
        };

        const int hairStyleHeaderHeight =
            measureHeaderHeight(m_hairStyleLabels, m_hairStyleColumnName);
        const int hairAdditionHeaderHeight =
            measureHeaderHeight(m_hairAdditionLabels, m_hairAdditionColumnName);
        const int hairHeaderHeight = std::max(hairStyleHeaderHeight, hairAdditionHeaderHeight);

        const auto hairStyleCount = static_cast<int>(m_hairStyleLabels.size());
        const auto hairAdditionCount = static_cast<int>(m_hairAdditionLabels.size());
        const auto hairCount = std::max(hairStyleCount, hairAdditionCount);
        // when either column has many entries, reserve only 1/3 of height for the face
        // so the hair-section legend has room for everything
        const double faceAreaFraction =
            hairCount > 4 ? math_constants::third : math_constants::two_thirds;
        int hairSectionHeight =
            hairCount == 0 ?
                0 :
                hairHeaderHeight + hairCount * (hairIconSize + hairEntryGap) + hairBottomPadding;
        // if hair section still doesn't fit in the allotted space, shrink icons to fit
        if (hairCount > 0)
            {
            const int maxHairSectionHeight =
                m_rect.GetHeight() - static_cast<int>(m_rect.GetHeight() * faceAreaFraction);
            if (hairSectionHeight > maxHairSectionHeight)
                {
                const int availableForIcons =
                    maxHairSectionHeight - hairHeaderHeight - hairBottomPadding;
                if (availableForIcons > 0)
                    {
                    hairIconSize =
                        std::max<int>(safe_divide<int>(availableForIcons, hairCount) - hairEntryGap,
                                      minHairIconSize);
                    hairSectionHeight = hairHeaderHeight +
                                        hairCount * (hairIconSize + hairEntryGap) +
                                        hairBottomPadding;
                    }
                }
            }
        const int faceAreaHeight = m_rect.GetHeight() - hairSectionHeight;

        // calculate face size - elongated (height = 1.5 * width), use 2/3 of available height
        const auto faceHeight = faceAreaHeight * math_constants::two_thirds;
        const auto faceWidth = faceHeight * math_constants::two_thirds;
        const auto lineGap = ScaleToScreenAndCanvas(5);

        if (faceHeight <= 0)
            {
            return m_rect;
            }

        // determine uniform label scaling to fit all labels
        double labelScaling = GetScaling() * 0.6;

        // measure label widths before positioning the face so that the face
        // can be placed based on actual label sizes (avoids extra whitespace
        // when left and right labels have different widths)
        auto measureLabelWidth = [&](const wxString& text, double scaling)
        {
            GraphItems::Label lbl(
                GraphItems::GraphItemInfo{ text }.Pen(wxNullPen).Scaling(scaling).DPIScaling(
                    GetDPIScaleFactor()));
            lbl.EnableMarkup(true);
            lbl.SetFontColor(m_outlineColor);
            return lbl.GetBoundingBox(dc).GetWidth();
        };

        int maxLeftWidth{ 0 };
        int maxRightWidth{ 0 };
        for (const auto& [labelText, featureId] : leftFeatures)
            {
            maxLeftWidth = std::max(maxLeftWidth, measureLabelWidth(labelText, labelScaling));
            }
        for (const auto& [labelText, featureId] : rightFeatures)
            {
            maxRightWidth = std::max(maxRightWidth, measureLabelWidth(labelText, labelScaling));
            }

        // center the face+labels block horizontally within the legend rect so that
        // when the bottom hair-key sections widen the rect, the empty space splits
        // evenly on either side of the face
        const int leftBlockWidth = leftFeatures.empty() ? 0 : (maxLeftWidth + lineGap);
        const int rightBlockWidth = rightFeatures.empty() ? 0 : (lineGap + maxRightWidth);
        const int faceBlockTotal = leftBlockWidth + faceWidth + rightBlockWidth;
        const int blockStartX =
            m_rect.GetX() + std::max(0, safe_divide<int>(m_rect.GetWidth() - faceBlockTotal, 2));
        const int faceX = blockStartX + leftBlockWidth;
        const wxRect faceRect(faceX,
                              m_rect.GetY() + safe_divide<int>(faceAreaHeight - faceHeight, 2),
                              faceWidth, faceHeight);

        // calculate label areas
        const int leftLabelAreaWidth = faceRect.GetX() - m_rect.GetX() - lineGap;
        const int rightLabelAreaWidth = m_rect.GetRight() - faceRect.GetRight() - lineGap;

        if (maxLeftWidth > leftLabelAreaWidth && leftLabelAreaWidth > 0)
            {
            labelScaling *= safe_divide<double>(leftLabelAreaWidth, maxLeftWidth);
            }
        if (maxRightWidth > rightLabelAreaWidth && rightLabelAreaWidth > 0)
            {
            const auto rightScale = safe_divide<double>(rightLabelAreaWidth, maxRightWidth);
            labelScaling = std::min(labelScaling, GetScaling() * 0.6 * rightScale);
            }

            // draw the face (scoped so GraphicsContext is flushed before drawing lines)
            {
            const GraphItems::GraphicsContextFallback gcf{ &dc, faceRect };
            auto* gc = gcf.GetGraphicsContext();
            if (gc != nullptr)
                {
                const FaceFeatures defaultFeatures;
                DrawFace(gc, faceRect, defaultFeatures, m_faceColorLighter, m_faceColorDarker,
                         m_outlineColor, m_lipstickColor, m_eyeColor, m_hairColor,
                         m_hairStyleFemale, m_hairStyleMale, m_gender);
                }
            }

        // calculate face geometry for feature connection points
        const double cx = faceRect.GetX() + faceRect.GetWidth() * math_constants::half;
        const double cy = faceRect.GetY() + faceRect.GetHeight() * math_constants::half;
        const double baseRadius = std::min(faceRect.GetWidth(), faceRect.GetHeight()) * 0.4;
        const double ellipseWidth = baseRadius * 1.0;
        const double ellipseHeight = baseRadius * 1.0;
        const double eyeY = cy - ellipseHeight * 0.25;
        const double eyeSpacing = ellipseWidth * 0.4;
        const double eyeRadius = ellipseHeight * 0.1;
        const double browY = eyeY - eyeRadius * 1.8;
        const double mouthY = cy + ellipseHeight * 0.45;
        const double noseY = cy + ellipseHeight * 0.1;

        // helper to get feature point on face
        const auto getFeaturePoint = [&](FeatureId featureId)
        {
            switch (featureId)
                {
            case FeatureId::FaceWidth:
                return wxPoint{ static_cast<int>(cx - ellipseWidth), static_cast<int>(cy) };
            case FeatureId::FaceHeight:
                return wxPoint{ static_cast<int>(cx), static_cast<int>(cy - ellipseHeight) };
            case FeatureId::EyeSize:
                [[fallthrough]];
            case FeatureId::PupilDirection:
                [[fallthrough]];
            case FeatureId::EyePosition:
                return wxPoint{ static_cast<int>(cx - eyeSpacing), static_cast<int>(eyeY) };
            case FeatureId::EyebrowSlant:
                return wxPoint{ static_cast<int>(cx - eyeSpacing), static_cast<int>(browY) };
            case FeatureId::NoseSize:
                return wxPoint{ static_cast<int>(cx), static_cast<int>(noseY) };
            case FeatureId::MouthWidth:
                return wxPoint{ static_cast<int>(cx), static_cast<int>(mouthY) };
            case FeatureId::SmileFrown:
                return wxPoint{ static_cast<int>(cx + ellipseWidth * 0.2),
                                static_cast<int>(mouthY) };
            case FeatureId::FaceColor:
                return wxPoint{ static_cast<int>(cx + ellipseWidth * 0.5),
                                static_cast<int>(cy + ellipseHeight * 0.3) };
            case FeatureId::EarSize:
                return wxPoint{ static_cast<int>(cx + ellipseWidth), static_cast<int>(cy) };
            default:
                return wxPoint{ static_cast<int>(cx), static_cast<int>(cy) };
                }
        };

        // calculate contrasting color for labels (same approach as Axis)
        const wxColour contrastingLabelColor{ Colors::ColorContrast::BlackOrWhiteContrast(
            m_canvasBackgroundColor) };

        const wxPen arrowPen{ Colors::ColorContrast::IsDark(m_canvasBackgroundColor) ?
                                  Colors::ColorBrewer::GetColor(Colors::Color::BlizzardBlue) :
                                  Colors::ColorBrewer::GetColor(Colors::Color::BondiBlue) };

        // create Lines object for arrow connections
        GraphItems::Lines arrowLines(arrowPen, GetScaling());
        arrowLines.SetDPIScaleFactor(GetDPIScaleFactor());

        // create Lines object for Face width dimension line (with ticks, not arrows)
        GraphItems::Lines dimensionLines(arrowPen, GetScaling());
        dimensionLines.SetLineStyle(LineStyle::Lines);
        dimensionLines.SetDPIScaleFactor(GetDPIScaleFactor());

        const auto tickLength = ScaleToScreenAndCanvas(8);

        // separate "Face width" from other left features - it always goes at top
        // because it draws a line across the top with ticks to convey the idea
        // of shape width
        std::vector<FeatureInfo> otherLeftFeatures;
        bool hasFaceWidth{ false };
        wxString faceWidthLabelText;
        for (const auto& feature : leftFeatures)
            {
            if (feature.featureId == FeatureId::FaceWidth)
                {
                hasFaceWidth = true;
                faceWidthLabelText = feature.labelText;
                }
            else
                {
                otherLeftFeatures.push_back(feature);
                }
            }

        // always draw "Face width" at the top if present
        if (hasFaceWidth)
            {
            GraphItems::Label label(GraphItems::GraphItemInfo{ faceWidthLabelText }
                                        .Pen(wxNullPen)
                                        .Scaling(labelScaling)
                                        .DPIScaling(GetDPIScaleFactor())
                                        .Padding(0, 4, 0, 0));
            label.EnableMarkup(true);
            label.SetFontColor(contrastingLabelColor);
            label.SetAnchoring(Anchoring::TopLeftCorner);
            label.SetAnchorPoint(wxPoint(blockStartX, m_rect.GetY()));
            const auto labelBox = label.GetBoundingBox(dc);
            const int labelCenterX = labelBox.GetX() + labelBox.GetWidth();
            const int labelCenterY = labelBox.GetY() + safe_divide(labelBox.GetHeight(), 2);

            // horizontal line straight from label to right edge of face
            dimensionLines.AddLine(wxPoint(labelCenterX, labelCenterY),
                                   wxPoint(faceRect.GetRight(), labelCenterY));
            // tick marks at face bounding box edges (half tick length)
            const auto shortTick = tickLength / 2;
            dimensionLines.AddLine(wxPoint(faceRect.GetX(), labelCenterY),
                                   wxPoint(faceRect.GetX(), labelCenterY + shortTick));
            dimensionLines.AddLine(wxPoint(faceRect.GetRight(), labelCenterY),
                                   wxPoint(faceRect.GetRight(), labelCenterY + shortTick));

            dimensionLines.Draw(dc);
            label.Draw(dc);
            }

        // draw remaining left labels with lines connecting to feature points
        const auto leftStartY = m_rect.GetY() + faceRect.GetY() - m_rect.GetY();
        const auto leftAvailableHeight = faceAreaHeight - (faceRect.GetY() - m_rect.GetY());
        const auto leftLabelSpacing =
            (otherLeftFeatures.size() > 1) ?
                safe_divide(leftAvailableHeight, static_cast<int>(otherLeftFeatures.size() + 1)) :
                safe_divide(leftAvailableHeight, 2);

        for (size_t i = 0; i < otherLeftFeatures.size(); ++i)
            {
            const auto labelY = leftStartY + static_cast<int>(i + 1) * leftLabelSpacing;

            GraphItems::Label label(GraphItems::GraphItemInfo{ otherLeftFeatures[i].labelText }
                                        .Pen(wxNullPen)
                                        .Scaling(labelScaling)
                                        .DPIScaling(GetDPIScaleFactor())
                                        .Padding(0, 4, 0, 0));
            label.EnableMarkup(true);
            label.SetFontColor(contrastingLabelColor);
            label.SetAnchoring(Anchoring::BottomLeftCorner);
            label.SetAnchorPoint(wxPoint{ blockStartX, labelY });
            const auto labelBox = label.GetBoundingBox(dc);
            const int labelCenterX = labelBox.GetX() + labelBox.GetWidth();
            const int labelCenterY = labelBox.GetY() + safe_divide(labelBox.GetHeight(), 2);

            const wxPoint featurePt = getFeaturePoint(otherLeftFeatures[i].featureId);
            arrowLines.AddLine(wxPoint{ labelCenterX, labelCenterY }, featurePt);

            label.Draw(dc);
            }

        // draw right labels with lines connecting to feature points
        const auto rightLabelSpacing =
            (rightFeatures.size() > 1) ?
                safe_divide(faceAreaHeight, static_cast<int>(rightFeatures.size() + 1)) :
                safe_divide(faceAreaHeight, 2);

        for (size_t i = 0; i < rightFeatures.size(); ++i)
            {
            const auto labelY = m_rect.GetY() + static_cast<int>(i + 1) * rightLabelSpacing;
            const wxPoint featurePt = getFeaturePoint(rightFeatures[i].featureId);

            // create and measure label to find its center (with left padding for line gap)
            GraphItems::Label label(GraphItems::GraphItemInfo{ rightFeatures[i].labelText }
                                        .Pen(wxNullPen)
                                        .Scaling(labelScaling)
                                        .DPIScaling(GetDPIScaleFactor())
                                        .Padding(0, 0, 0, 4));
            label.EnableMarkup(true);
            label.SetFontColor(contrastingLabelColor);
            label.SetAnchoring(Anchoring::BottomLeftCorner);
            label.SetAnchorPoint(wxPoint(faceRect.GetRight() + lineGap, labelY));
            const auto labelBox = label.GetBoundingBox(dc);
            const int labelCenterX = labelBox.GetX();
            const int labelCenterY = labelBox.GetY() + safe_divide(labelBox.GetHeight(), 2);

            // add arrow line from label to feature point
            arrowLines.AddLine(wxPoint{ labelCenterX, labelCenterY }, featurePt);

            label.Draw(dc);
            }

        // draw all arrow lines
        arrowLines.Draw(dc);

        // draw the hair-style and hair-addition key sections below the face
        // (rendered as side-by-side columns, centered horizontally in the legend rect)
        if (!m_hairStyleLabels.empty() || !m_hairAdditionLabels.empty())
            {
            const int sectionTopY = m_rect.GetY() + faceAreaHeight + ScaleToScreenAndCanvas(5);
            const auto sectionGap = ScaleToScreenAndCanvas(20);

            // width of a column = max of (icon + gap + widest label) vs header width
            const auto columnWidth = [&](const std::vector<wxString>& labels,
                                         const wxString& columnName) -> int
            {
                if (labels.empty())
                    {
                    return 0;
                    }
                int maxLabelWidth{ 0 };
                for (const auto& lbl : labels)
                    {
                    maxLabelWidth = std::max(maxLabelWidth, measureLabelWidth(lbl, labelScaling));
                    }
                int headerWidth{ 0 };
                if (!columnName.empty())
                    {
                    headerWidth = measureLabelWidth(L"<span style='font-style:italic;'>" +
                                                        columnName + L"</span>",
                                                    headerScaling);
                    }
                return std::max<int>(hairIconSize + lineGap + maxLabelWidth, headerWidth);
            };

            const int styleColWidth = columnWidth(m_hairStyleLabels, m_hairStyleColumnName);
            const int additionColWidth =
                columnWidth(m_hairAdditionLabels, m_hairAdditionColumnName);
            const bool bothExist = !m_hairStyleLabels.empty() && !m_hairAdditionLabels.empty();
            const int combinedWidth =
                styleColWidth + (bothExist ? sectionGap : 0) + additionColWidth;
            const int sectionStartX =
                m_rect.GetX() + std::max(0, safe_divide<int>(m_rect.GetWidth() - combinedWidth, 2));

            // renders a single key column (header above icons; labels to right of icons).
            // isHairStyleColumn switches the icon rendering: hair-style icons show a small
            // face wearing each style; hair-addition icons show an accessory (female) or
            // a small face with facial hair (male).
            const auto drawColumn = [&](const int colLeftX, const std::vector<wxString>& labels,
                                        const wxString& columnName, const bool isHairStyleColumn)
            {
                if (labels.empty())
                    {
                    return;
                    }

                // header (column name) above the entries
                if (!columnName.empty())
                    {
                    GraphItems::Label header(GraphItems::GraphItemInfo{
                        L"<span style='font-style:italic;'>" + columnName + L"</span>" }
                                                 .Pen(wxNullPen)
                                                 .Scaling(headerScaling)
                                                 .DPIScaling(GetDPIScaleFactor()));
                    header.EnableMarkup(true);
                    header.SetFontColor(contrastingLabelColor);
                    header.SetAnchoring(Anchoring::TopLeftCorner);
                    header.SetAnchorPoint(wxPoint(colLeftX, sectionTopY));
                    header.Draw(dc);
                    }

                const int entriesTopY = sectionTopY + hairHeaderHeight;

                std::vector<wxRect> iconRects;
                iconRects.reserve(labels.size());
                for (size_t i = 0; i < labels.size(); ++i)
                    {
                    const int iconY =
                        entriesTopY + static_cast<int>(i) * (hairIconSize + hairEntryGap);
                    iconRects.emplace_back(colLeftX, iconY, hairIconSize, hairIconSize);
                    }

                if (isHairStyleColumn)
                    {
                    // for each entry: a small face wearing that hair style
                    const wxRect allIconsRect{ iconRects.front().GetX(), iconRects.front().GetY(),
                                               iconRects.front().GetWidth(),
                                               iconRects.back().GetBottom() -
                                                   iconRects.front().GetY() + 1 };
                    const GraphItems::GraphicsContextFallback gcf{ &dc, allIconsRect };
                    auto* gc = gcf.GetGraphicsContext();
                    if (gc != nullptr)
                        {
                        // render the face oval + hair (skip eyes, nose, mouth, facial hair)
                        FaceParts minimalParts;
                        minimalParts.m_cheeks = false;
                        minimalParts.m_eyes = false;
                        minimalParts.m_nose = false;
                        minimalParts.m_mouth = false;
                        minimalParts.m_facialHair = false;
                        for (size_t i = 0; i < labels.size(); ++i)
                            {
                            const FaceFeatures features;
                            // pass the parallel index as both gender's style; DrawFace
                            // picks the right one based on m_gender
                            DrawFace(gc, iconRects[i], features, m_faceColorLighter,
                                     m_faceColorDarker, m_outlineColor, m_lipstickColor, m_eyeColor,
                                     m_hairColor, static_cast<HairStyleFemale>(i),
                                     static_cast<HairStyleMale>(i), m_gender, minimalParts);
                            }
                        }
                    }
                else if (m_gender == Gender::Female)
                    {
                    // hair-addition (female): draw HairAccessory IconShape directly
                    for (size_t i = 0; i < labels.size(); ++i)
                        {
                        Icons::IconShape iconShape{ Icons::IconShape::Blank };
                        switch (static_cast<HairAccessory>(i))
                            {
                        case HairAccessory::Butterfly:
                            iconShape = Icons::IconShape::Butterfly;
                            break;
                        case HairAccessory::Flower:
                            iconShape = Icons::IconShape::Flower;
                            break;
                        case HairAccessory::SunFlower:
                            iconShape = Icons::IconShape::Sunflower;
                            break;
                        case HairAccessory::Heart:
                            iconShape = Icons::IconShape::Heart;
                            break;
                        case HairAccessory::Leaf:
                            iconShape = Icons::IconShape::FallLeaf;
                            break;
                        case HairAccessory::Snowflake:
                            iconShape = Icons::IconShape::Snowflake;
                            break;
                        case HairAccessory::Star:
                            iconShape = Icons::IconShape::Star;
                            break;
                        case HairAccessory::Pumpkin:
                            iconShape = Icons::IconShape::Pumpkin;
                            break;
                        default:
                            break;
                            }
                        if (iconShape != Icons::IconShape::Blank)
                            {
                            GraphItems::Shape sh(GraphItems::GraphItemInfo{}
                                                     .Pen(wxPen{ m_outlineColor, 1 })
                                                     .Anchoring(Anchoring::TopLeftCorner)
                                                     .Scaling(GetScaling())
                                                     .DPIScaling(GetDPIScaleFactor()),
                                                 iconShape, iconRects[i].GetSize());
                            sh.Draw(iconRects[i], dc);
                            }
                        }
                    }
                else
                    {
                    // hair-addition (male): small face for each entry showing that facial hair
                    const wxRect allIconsRect{ iconRects.front().GetX(), iconRects.front().GetY(),
                                               iconRects.front().GetWidth(),
                                               iconRects.back().GetBottom() -
                                                   iconRects.front().GetY() + 1 };
                    const GraphItems::GraphicsContextFallback gcf{ &dc, allIconsRect };
                    auto* gc = gcf.GetGraphicsContext();
                    if (gc != nullptr)
                        {
                        // render face oval + facial hair (skip eyes, nose, mouth, head hair)
                        FaceParts minimalParts;
                        minimalParts.m_cheeks = false;
                        minimalParts.m_eyes = false;
                        minimalParts.m_nose = false;
                        minimalParts.m_mouth = false;
                        minimalParts.m_hair = false;
                        for (size_t i = 0; i < labels.size(); ++i)
                            {
                            FaceFeatures features;
                            features.m_facialHair = static_cast<FacialHair>(i);
                            DrawFace(gc, iconRects[i], features, m_faceColorLighter,
                                     m_faceColorDarker, m_outlineColor, m_lipstickColor, m_eyeColor,
                                     m_hairColor, m_hairStyleFemale, m_hairStyleMale, m_gender,
                                     minimalParts);
                            }
                        }
                    }

                // labels next to icons (vertically centered relative to each icon)
                for (size_t i = 0; i < labels.size(); ++i)
                    {
                    GraphItems::Label lbl(GraphItems::GraphItemInfo{ labels[i] }
                                              .Pen(wxNullPen)
                                              .Scaling(labelScaling)
                                              .DPIScaling(GetDPIScaleFactor()));
                    lbl.SetFontColor(contrastingLabelColor);
                    lbl.SetAnchoring(Anchoring::TopLeftCorner);
                    const auto labelBox = lbl.GetBoundingBox(dc);
                    lbl.SetAnchorPoint(
                        wxPoint(iconRects[i].GetRight() + lineGap,
                                iconRects[i].GetY() +
                                    (iconRects[i].GetHeight() - labelBox.GetHeight()) / 2));
                    lbl.Draw(dc);
                    }
            };

            drawColumn(sectionStartX, m_hairStyleLabels, m_hairStyleColumnName, true);
            drawColumn(sectionStartX + styleColWidth + (bothExist ? sectionGap : 0),
                       m_hairAdditionLabels, m_hairAdditionColumnName, false);
            }

        // draw the selection outline
        if (IsSelected())
            {
            const bool penIsLight{ (
                GetPen().IsOk() && GetPen().GetColour().IsOk() &&
                Wisteria::Colors::ColorContrast::IsLight(GetPen().GetColour())) };
            const wxDCPenChanger pc(
                dc, wxPen(penIsLight ? Colors::ColorBrewer::GetColor(Colors::Color::White) :
                                       Colors::ColorBrewer::GetColor(Colors::Color::Black),
                          ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
            std::array<wxPoint, 5> pts;
            GraphItems::Polygon::GetRectPoints(m_rect, pts);
            dc.DrawLines(pts.size(), pts.data());
            }

        return m_rect;
        }

    //----------------------------------------------------------------
    ChernoffFacesPlot::ChernoffFacesPlot(Canvas * canvas,
                                         const wxColour& faceColor /*= wxColour(255, 224, 189)*/)
        : Graph2D(canvas)
        {
        if (faceColor.IsOk())
            {
            m_faceColor = faceColor;
            }

        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    double ChernoffFacesPlot::NormalizeValue(const double value, const double minVal,
                                             const double maxVal) noexcept
        {
        if (!std::isfinite(value) || !std::isfinite(minVal) || !std::isfinite(maxVal))
            {
            return DEFAULT_FEATURE_VALUE;
            }
        if (compare_doubles_greater_or_equal(minVal, maxVal))
            {
            return DEFAULT_FEATURE_VALUE;
            }
        const auto normalized = safe_divide<double>(value - minVal, maxVal - minVal);
        return std::clamp(normalized, 0.0, 1.0);
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::CalculateColumnRange(const wxString& columnName, double& outMin,
                                                 double& outMax) const
        {
        outMin = std::numeric_limits<double>::max();
        outMax = std::numeric_limits<double>::lowest();

        if (GetDataset() == nullptr || columnName.empty())
            {
            outMin = 0;
            outMax = 1;
            return;
            }

        const auto colIter = GetDataset()->GetContinuousColumn(columnName);
        if (colIter == GetDataset()->GetContinuousColumns().cend())
            {
            outMin = 0;
            outMax = 1;
            return;
            }

        for (const auto& val : colIter->GetValues())
            {
            if (std::isfinite(val))
                {
                outMin = std::min(outMin, val);
                outMax = std::max(outMax, val);
                }
            }

        if (outMin > outMax)
            {
            outMin = 0;
            outMax = 1;
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                    const wxString& faceWidthColumn,
                                    const std::optional<wxString>& faceHeightColumn,
                                    const std::optional<wxString>& eyeSizeColumn,
                                    const std::optional<wxString>& eyePositionColumn,
                                    const std::optional<wxString>& eyebrowSlantColumn,
                                    const std::optional<wxString>& pupilPositionColumn,
                                    const std::optional<wxString>& noseSizeColumn,
                                    const std::optional<wxString>& mouthWidthColumn,
                                    const std::optional<wxString>& mouthCurvatureColumn,
                                    const std::optional<wxString>& faceSaturationColumn,
                                    const std::optional<wxString>& earSizeColumn,
                                    const std::optional<wxString>& hairStyleColumn,
                                    const std::optional<wxString>& hairAdditionColumn)
        {
        m_faces.clear();
        m_hairStyleLabels.clear();
        m_hairAdditionLabels.clear();
        SetDataset(data);

        if (GetDataset() == nullptr)
            {
            return;
            }

        // store column names for legend
        m_faceWidthColumnName = faceWidthColumn;
        m_faceHeightColumnName = faceHeightColumn.value_or(wxString{});
        m_eyeSizeColumnName = eyeSizeColumn.value_or(wxString{});
        m_eyePositionColumnName = eyePositionColumn.value_or(wxString{});
        m_eyebrowSlantColumnName = eyebrowSlantColumn.value_or(wxString{});
        m_pupilPositionColumnName = pupilPositionColumn.value_or(wxString{});
        m_noseSizeColumnName = noseSizeColumn.value_or(wxString{});
        m_mouthWidthColumnName = mouthWidthColumn.value_or(wxString{});
        m_mouthCurvatureColumnName = mouthCurvatureColumn.value_or(wxString{});
        m_faceSaturationColumnName = faceSaturationColumn.value_or(wxString{});
        m_earSizeColumnName = earSizeColumn.value_or(wxString{});
        m_hairStyleColumnName = hairStyleColumn.value_or(wxString{});
        m_hairAdditionColumnName = hairAdditionColumn.value_or(wxString{});

        // validate required column
        const auto faceWidthCol = GetDataset()->GetContinuousColumn(faceWidthColumn);
        if (faceWidthCol == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': face width column not found for Chernoff faces plot."),
                                 faceWidthColumn)
                    .ToUTF8());
            }

        // get optional categorical column for hair style
        const auto catColsEnd = GetDataset()->GetCategoricalColumns().cend();
        const auto hairStyleCol =
            GetDataset()->GetCategoricalColumn(hairStyleColumn.value_or(wxString{}));

        // Pre-build group-ID -> parallel-index map from the string table.
        // The index is later cast to HairStyleFemale or HairStyleMale per face.
        std::map<Data::GroupIdType, size_t> hairStyleMap;
        if (hairStyleCol != catColsEnd)
            {
            const auto categoryCount = hairStyleCol->GetStringTable().size();
            if (categoryCount > static_cast<size_t>(HairStyleFemale::FEMALE_HAIR_STYLE_COUNT))
                {
                throw std::runtime_error(
                    wxString::Format(
                        /* TRANSLATORS: Error message. Column name (%s),
                           actual category count (%zu), and maximum allowed (%d). */
                        _(L"'%s': hair style column has %zu categories; "
                          "maximum allowed is %d."),
                        hairStyleColumn.value(), categoryCount,
                        static_cast<int>(HairStyleFemale::FEMALE_HAIR_STYLE_COUNT))
                        .ToUTF8());
                }
            m_hairStyleLabels.reserve(hairStyleCol->GetStringTable().size());
            size_t styleIdx{ 0 };
            for (const auto& [id, label] : hairStyleCol->GetStringTable())
                {
                hairStyleMap[id] = styleIdx;
                m_hairStyleLabels.push_back(label);
                ++styleIdx;
                }
            }

        // get optional categorical column for hair addition
        const auto hairAdditionCol =
            GetDataset()->GetCategoricalColumn(hairAdditionColumn.value_or(wxString{}));

        // pre-build group-ID -> enum maps from the string table (one lookup per row instead of two)
        std::map<Data::GroupIdType, FacialHair> facialHairMap;
        std::map<Data::GroupIdType, HairAccessory> hairAccessoryMap;
        if (hairAdditionCol != catColsEnd)
            {
            const auto categoryCount = hairAdditionCol->GetStringTable().size();
            if (categoryCount > static_cast<size_t>(FacialHair::FACIAL_HAIR_COUNT))
                {
                throw std::runtime_error(
                    wxString::Format(
                        /* TRANSLATORS: Error message. Column name (%s),
                           actual category count (%zu), and maximum allowed (%d). */
                        _(L"'%s': hair addition column has %zu categories; "
                          "maximum allowed is %d."),
                        hairAdditionColumn.value(), categoryCount,
                        static_cast<int>(FacialHair::FACIAL_HAIR_COUNT))
                        .ToUTF8());
                }
            m_hairAdditionLabels.reserve(hairAdditionCol->GetStringTable().size());
            size_t enumIdx{ 0 };
            for (const auto& [id, label] : hairAdditionCol->GetStringTable())
                {
                facialHairMap[id] = static_cast<FacialHair>(enumIdx);
                hairAccessoryMap[id] = static_cast<HairAccessory>(enumIdx);
                m_hairAdditionLabels.push_back(label);
                ++enumIdx;
                }
            }

        // get optional continuous column iterators
        const auto faceHeightCol =
            GetDataset()->GetContinuousColumn(faceHeightColumn.value_or(wxString{}));
        const auto eyeSizeCol =
            GetDataset()->GetContinuousColumn(eyeSizeColumn.value_or(wxString{}));
        const auto eyePositionCol =
            GetDataset()->GetContinuousColumn(eyePositionColumn.value_or(wxString{}));
        const auto eyebrowSlantCol =
            GetDataset()->GetContinuousColumn(eyebrowSlantColumn.value_or(wxString{}));
        const auto pupilPositionCol =
            GetDataset()->GetContinuousColumn(pupilPositionColumn.value_or(wxString{}));
        const auto noseSizeCol =
            GetDataset()->GetContinuousColumn(noseSizeColumn.value_or(wxString{}));
        const auto mouthWidthCol =
            GetDataset()->GetContinuousColumn(mouthWidthColumn.value_or(wxString{}));
        const auto mouthCurvatureCol =
            GetDataset()->GetContinuousColumn(mouthCurvatureColumn.value_or(wxString{}));
        const auto faceSaturationCol =
            GetDataset()->GetContinuousColumn(faceSaturationColumn.value_or(wxString{}));
        const auto earSizeCol =
            GetDataset()->GetContinuousColumn(earSizeColumn.value_or(wxString{}));

        const auto colsEnd = GetDataset()->GetContinuousColumns().cend();

        // calculate ranges for normalization
        double fwMin{ 0.0 }, fwMax{ 0.0 };
        CalculateColumnRange(faceWidthColumn, fwMin, fwMax);

        double fhMin{ 0.0 }, fhMax{ 0.0 };
        CalculateColumnRange(faceHeightColumn.value_or(wxString{}), fhMin, fhMax);

        double esMin{ 0.0 }, esMax{ 0.0 };
        CalculateColumnRange(eyeSizeColumn.value_or(wxString{}), esMin, esMax);

        double epMin{ 0.0 }, epMax{ 0.0 };
        CalculateColumnRange(eyePositionColumn.value_or(wxString{}), epMin, epMax);

        double ebsMin{ 0.0 }, ebsMax{ 0.0 };
        CalculateColumnRange(eyebrowSlantColumn.value_or(wxString{}), ebsMin, ebsMax);

        double ppMin{ 0.0 }, ppMax{ 0.0 };
        CalculateColumnRange(pupilPositionColumn.value_or(wxString{}), ppMin, ppMax);

        double nsMin{ 0.0 }, nsMax{ 0.0 };
        CalculateColumnRange(noseSizeColumn.value_or(wxString{}), nsMin, nsMax);

        double mwMin{ 0.0 }, mwMax{ 0.0 };
        CalculateColumnRange(mouthWidthColumn.value_or(wxString{}), mwMin, mwMax);

        double mcMin{ 0.0 }, mcMax{ 0.0 };
        CalculateColumnRange(mouthCurvatureColumn.value_or(wxString{}), mcMin, mcMax);

        double fsMin{ 0.0 }, fsMax{ 0.0 };
        CalculateColumnRange(faceSaturationColumn.value_or(wxString{}), fsMin, fsMax);

        double erMin{ 0.0 }, erMax{ 0.0 };
        CalculateColumnRange(earSizeColumn.value_or(wxString{}), erMin, erMax);

        // limit observations
        const size_t rowCount = std::min(GetDataset()->GetRowCount(), MAX_FACES);
        if (GetDataset()->GetRowCount() > MAX_FACES)
            {
            wxLogWarning(L"Chernoff plot limited to %zu observations; "
                         "%zu observations truncated.",
                         MAX_FACES, GetDataset()->GetRowCount() - MAX_FACES);
            GetCaption().SetText(wxString::Format(
                _(L"Note: only the first %zu observations are being displayed."), MAX_FACES));
            }

        m_faces.reserve(rowCount);

        // process each observation
        for (size_t i = 0; i < rowCount; ++i)
            {
            FaceFeatures face;

            face.m_faceWidth = NormalizeValue(faceWidthCol->GetValue(i), fwMin, fwMax);

            if (faceHeightCol != colsEnd)
                {
                face.m_faceHeight = NormalizeValue(faceHeightCol->GetValue(i), fhMin, fhMax);
                }
            if (eyeSizeCol != colsEnd)
                {
                face.m_eyeSize = NormalizeValue(eyeSizeCol->GetValue(i), esMin, esMax);
                }
            if (eyePositionCol != colsEnd)
                {
                face.m_eyePosition = NormalizeValue(eyePositionCol->GetValue(i), epMin, epMax);
                }
            if (eyebrowSlantCol != colsEnd)
                {
                face.m_eyebrowSlant = NormalizeValue(eyebrowSlantCol->GetValue(i), ebsMin, ebsMax);
                }
            if (pupilPositionCol != colsEnd)
                {
                face.m_pupilPosition = NormalizeValue(pupilPositionCol->GetValue(i), ppMin, ppMax);
                }
            if (noseSizeCol != colsEnd)
                {
                face.m_noseSize = NormalizeValue(noseSizeCol->GetValue(i), nsMin, nsMax);
                }
            if (mouthWidthCol != colsEnd)
                {
                face.m_mouthWidth = NormalizeValue(mouthWidthCol->GetValue(i), mwMin, mwMax);
                }
            if (mouthCurvatureCol != colsEnd)
                {
                face.m_mouthCurvature =
                    NormalizeValue(mouthCurvatureCol->GetValue(i), mcMin, mcMax);
                }
            if (faceSaturationCol != colsEnd)
                {
                face.m_faceSaturation =
                    NormalizeValue(faceSaturationCol->GetValue(i), fsMin, fsMax);
                }
            if (earSizeCol != colsEnd)
                {
                face.m_earSize = NormalizeValue(earSizeCol->GetValue(i), erMin, erMax);
                }
            if (hairStyleCol != catColsEnd)
                {
                const auto groupId = hairStyleCol->GetValue(i);
                if (const auto hsIt = hairStyleMap.find(groupId); hsIt != hairStyleMap.cend())
                    {
                    face.m_hairStyleFemale = static_cast<HairStyleFemale>(hsIt->second);
                    face.m_hairStyleMale = static_cast<HairStyleMale>(hsIt->second);
                    }
                }
            if (hairAdditionCol != catColsEnd)
                {
                const auto groupId = hairAdditionCol->GetValue(i);
                if (const auto fhIt = facialHairMap.find(groupId); fhIt != facialHairMap.cend())
                    {
                    face.m_facialHair = fhIt->second;
                    }
                if (const auto haIt = hairAccessoryMap.find(groupId);
                    haIt != hairAccessoryMap.cend())
                    {
                    face.m_hairAccessory = haIt->second;
                    face.m_hasHairAccessory = true;
                    }
                }

                // check if all mapped columns have missing data for this row
                {
                bool allMissing{ !std::isfinite(faceWidthCol->GetValue(i)) };
                if (allMissing && faceHeightCol != colsEnd)
                    {
                    allMissing = !std::isfinite(faceHeightCol->GetValue(i));
                    }
                if (allMissing && eyeSizeCol != colsEnd)
                    {
                    allMissing = !std::isfinite(eyeSizeCol->GetValue(i));
                    }
                if (allMissing && eyePositionCol != colsEnd)
                    {
                    allMissing = !std::isfinite(eyePositionCol->GetValue(i));
                    }
                if (allMissing && eyebrowSlantCol != colsEnd)
                    {
                    allMissing = !std::isfinite(eyebrowSlantCol->GetValue(i));
                    }
                if (allMissing && pupilPositionCol != colsEnd)
                    {
                    allMissing = !std::isfinite(pupilPositionCol->GetValue(i));
                    }
                if (allMissing && noseSizeCol != colsEnd)
                    {
                    allMissing = !std::isfinite(noseSizeCol->GetValue(i));
                    }
                if (allMissing && mouthWidthCol != colsEnd)
                    {
                    allMissing = !std::isfinite(mouthWidthCol->GetValue(i));
                    }
                if (allMissing && mouthCurvatureCol != colsEnd)
                    {
                    allMissing = !std::isfinite(mouthCurvatureCol->GetValue(i));
                    }
                if (allMissing && faceSaturationCol != colsEnd)
                    {
                    allMissing = !std::isfinite(faceSaturationCol->GetValue(i));
                    }
                if (allMissing && earSizeCol != colsEnd)
                    {
                    allMissing = !std::isfinite(earSizeCol->GetValue(i));
                    }
                if (allMissing && hairStyleCol != catColsEnd)
                    {
                    allMissing = hairStyleCol->IsMissingData(i);
                    }
                if (allMissing && hairAdditionCol != catColsEnd)
                    {
                    allMissing = hairAdditionCol->IsMissingData(i);
                    }
                face.m_allDataMissing = allMissing;
                }

            // get label from ID column if available
            if (GetDataset()->GetIdColumn().GetRowCount() > i)
                {
                face.m_label = GetDataset()->GetIdColumn().GetValue(i);
                }

            m_faces.push_back(std::move(face));
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (m_faces.empty())
            {
            return;
            }

        const wxRect drawArea = GetPlotAreaBoundingBox();

        // calculate grid dimensions
        const size_t faceCount = m_faces.size();
        const auto cols = static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(faceCount))));
        const auto rows = static_cast<size_t>(std::ceil(
            safe_divide<double>(static_cast<double>(faceCount), static_cast<double>(cols))));

        // calculate cell size, leaving room for labels
        const int labelHeight = m_showLabels ? static_cast<int>(ScaleToScreenAndCanvas(20)) : 0;
        const int cellWidth = safe_divide<int>(drawArea.GetWidth(), static_cast<int>(cols));
        const int cellHeight = safe_divide<int>(drawArea.GetHeight(), static_cast<int>(rows));
        const int faceSize = std::min(cellWidth, cellHeight - labelHeight);

        if (faceSize <= 0)
            {
            return;
            }

        // center the grid
        const auto totalWidth = static_cast<int>(cols) * cellWidth;
        const auto totalHeight = static_cast<int>(rows) * cellHeight;
        const int offsetX = drawArea.GetX() + safe_divide<int>(drawArea.GetWidth() - totalWidth, 2);
        const int offsetY =
            drawArea.GetY() + safe_divide<int>(drawArea.GetHeight() - totalHeight, 2);

        // add each face as a drawable object
        std::vector<std::unique_ptr<GraphItems::Label>> faceLabels;
        double smallestTextScaling{ std::numeric_limits<double>::max() };
        size_t faceIndex{ 0 };
        for (size_t row = 0; row < rows && faceIndex < faceCount; ++row)
            {
            for (size_t col = 0; col < cols && faceIndex < faceCount; ++col)
                {
                const auto x = offsetX + static_cast<int>(col) * cellWidth +
                               safe_divide<int>(cellWidth - faceSize, 2);
                const auto y = offsetY + static_cast<int>(row) * cellHeight;

                if (m_faces[faceIndex].m_allDataMissing)
                    {
                    // show "No data" label centered where the face would be
                    const auto faceCenterX = x + safe_divide(faceSize, 2);
                    const auto faceCenterY = y + safe_divide(faceSize, 2);
                    auto noDataLabel = std::make_unique<GraphItems::Label>(
                        GraphItems::GraphItemInfo{ _(L"No data") }
                            .Pen(wxNullPen)
                            .Scaling(GetScaling())
                            .DPIScaling(GetDPIScaleFactor())
                            .Anchoring(Anchoring::Center)
                            .AnchorPoint(wxPoint{ faceCenterX, faceCenterY }));
                    AddObject(std::move(noDataLabel));
                    }
                else
                    {
                    // when a hair-style column is mapped, each face uses its own style;
                    // otherwise every face uses the plot-wide style
                    const HairStyleFemale effectiveHairStyleFemale =
                        m_hairStyleColumnName.empty() ? m_hairStyleFemale :
                                                        m_faces[faceIndex].m_hairStyleFemale;
                    const HairStyleMale effectiveHairStyleMale =
                        m_hairStyleColumnName.empty() ? m_hairStyleMale :
                                                        m_faces[faceIndex].m_hairStyleMale;
                    // create face
                    AddObject(std::make_unique<FaceObject>(
                        GraphItems::GraphItemInfo{}
                            .Pen(wxNullPen)
                            .Selectable(true)
                            .Anchoring(Anchoring::TopLeftCorner)
                            .AnchorPoint(wxPoint{ x, y }),
                        m_faces[faceIndex], wxSize{ faceSize, faceSize }, m_faceColorLighter,
                        m_faceColor, m_outlineColor, m_lipstickColor, m_eyeColor, m_hairColor,
                        effectiveHairStyleFemale, effectiveHairStyleMale, m_gender));
                    }

                // add label below face if enabled
                if (m_showLabels && !m_faces[faceIndex].m_label.empty())
                    {
                    auto label = std::make_unique<GraphItems::Label>(
                        GraphItems::GraphItemInfo{ m_faces[faceIndex].m_label }
                            .Pen(wxNullPen)
                            .Scaling(GetScaling())
                            .DPIScaling(GetDPIScaleFactor())
                            .Anchoring(Anchoring::TopLeftCorner)
                            .AnchorPoint(wxPoint{ x, y + faceSize }));
                    label->SetTextAlignment(TextAlignment::Centered);
                    label->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
                    label->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                    const wxRect labelBox(wxPoint{ x, y + faceSize },
                                          wxSize{ faceSize, labelHeight });
                    label->SetBoundingBox(labelBox, dc, GetScaling());
                    smallestTextScaling = std::min(label->GetScaling(), smallestTextScaling);
                    faceLabels.push_back(std::move(label));
                    }

                ++faceIndex;
                }
            }

        // homogenize labels' text scaling to the smallest size and add them
        for (auto& label : faceLabels)
            {
            const wxRect bBox = label->GetBoundingBox(dc);
            label->SetScaling(smallestTextScaling);
            label->LockBoundingBoxScaling();
            label->SetBoundingBox(bBox, dc, GetScaling());
            label->UnlockBoundingBoxScaling();
            AddObject(std::move(label));
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::DrawFace(
        wxGraphicsContext * gc, const wxRect& rect, const FaceFeatures& features,
        const wxColour& faceColorLighter, const wxColour& faceColorDarker,
        const wxColour& outlineColor, const wxColour& lipstickColor, const wxColour& eyeColor,
        const wxColour& hairColor, const HairStyleFemale hairStyleFemale,
        const HairStyleMale hairStyleMale, const Gender gender)
        {
        DrawFace(gc, rect, features, faceColorLighter, faceColorDarker, outlineColor, lipstickColor,
                 eyeColor, hairColor, hairStyleFemale, hairStyleMale, gender, FaceParts{});
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::DrawFace(
        wxGraphicsContext * gc, const wxRect& rect, const FaceFeatures& features,
        const wxColour& faceColorLighter, const wxColour& faceColorDarker,
        const wxColour& outlineColor, const wxColour& lipstickColor, const wxColour& eyeColor,
        const wxColour& hairColor, const HairStyleFemale hairStyleFemale,
        const HairStyleMale hairStyleMale, const Gender gender, const FaceParts& parts)
        {
        if (gc == nullptr)
            {
            return;
            }

        // collapse the gender-specific hair-style enums to a single internal kind so the
        // drawing branches below don't need to be duplicated per gender
        const HairStyleKind hairStyle = ToHairStyleKind(hairStyleFemale, hairStyleMale, gender);

        // interpolate between lighter and darker skin colors based on saturation
        // 0 = lighter color, 1 = darker color
        const double colorBlendFactor = features.m_faceSaturation;
        const wxColour faceCol(
            static_cast<unsigned char>(faceColorLighter.Red() +
                                       colorBlendFactor *
                                           (faceColorDarker.Red() - faceColorLighter.Red())),
            static_cast<unsigned char>(faceColorLighter.Green() +
                                       colorBlendFactor *
                                           (faceColorDarker.Green() - faceColorLighter.Green())),
            static_cast<unsigned char>(faceColorLighter.Blue() +
                                       colorBlendFactor *
                                           (faceColorDarker.Blue() - faceColorLighter.Blue())));

        // calculate face dimensions
        const double cx = rect.GetX() + rect.GetWidth() * math_constants::half;
        const double cy = rect.GetY() + rect.GetHeight() * math_constants::half;
        const double baseRadius = std::min(rect.GetWidth(), rect.GetHeight()) * 0.4;

        // face width: 0.7 to 1.3 multiplier
        const double faceWidth = baseRadius * (0.7 + 0.6 * features.m_faceWidth);
        // face height: 0.8 to 1.2 multiplier
        const double faceHeight = baseRadius * (0.8 + 0.4 * features.m_faceHeight);

        const wxPen outlinePen{ outlineColor, 1 };

        if (parts.m_face)
            {
            // draw ears first (behind face)
            const double earScale = math_constants::half + features.m_earSize;
            const double earWidth = faceHeight * 0.12 * earScale;
            const double earHeight = faceHeight * 0.2 * earScale;

            gc->SetBrush(wxBrush{ faceCol });
            gc->SetPen(outlinePen);
            // left ear
            gc->DrawEllipse(cx - faceWidth - earWidth * 0.3, cy - earHeight * 0.5, earWidth,
                            earHeight);
            // right ear
            gc->DrawEllipse(cx + faceWidth - earWidth * 0.7, cy - earHeight * 0.5, earWidth,
                            earHeight);

            // draw face oval
            gc->SetBrush(wxBrush{ faceCol });
            gc->SetPen(outlinePen);
            gc->DrawEllipse(cx - faceWidth, cy - faceHeight, faceWidth * 2, faceHeight * 2);
            }

        // draw rosy cheeks for female faces (radial gradient)
        if (parts.m_cheeks && gender == Gender::Female)
            {
            gc->SetPen(*wxTRANSPARENT_PEN);
            const double cheekRadius = faceHeight * 0.225;
            const double cheekY = cy + faceHeight * 0.1;
            const wxColour rosyCenter(255, 150, 170, 80);
            const wxColour rosyEdge(255, 200, 210, 0);

            // left cheek
            const double leftCheekX = cx - faceWidth * 0.55;
            auto leftCheekBrush = gc->CreateRadialGradientBrush(
                leftCheekX, cheekY, leftCheekX, cheekY, cheekRadius, rosyCenter, rosyEdge);
            gc->SetBrush(leftCheekBrush);
            gc->DrawEllipse(leftCheekX - cheekRadius, cheekY - cheekRadius, cheekRadius * 2,
                            cheekRadius * 2);

            // right cheek
            const double rightCheekX = cx + faceWidth * 0.55;
            auto rightCheekBrush = gc->CreateRadialGradientBrush(
                rightCheekX, cheekY, rightCheekX, cheekY, cheekRadius, rosyCenter, rosyEdge);
            gc->SetBrush(rightCheekBrush);
            gc->DrawEllipse(rightCheekX - cheekRadius, cheekY - cheekRadius, cheekRadius * 2,
                            cheekRadius * 2);
            }

        const double mouthY = cy + faceHeight * 0.45;
        const double mouthWidthVal = faceWidth * 0.35 * (0.5 + features.m_mouthWidth);

        // draw facial hair before head hair so that hair overlaps the beard and mustache
        if (parts.m_facialHair && gender != Gender::Female)
            {
            if (features.m_facialHair == FacialHair::FiveOClockShadow)
                {
                // stubble using hair color with transparency - make it more pronounced
                const wxColour stubbleColor{ hairColor.Red(), hairColor.Green(), hairColor.Blue(),
                                             170 };
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->SetBrush(wxBrush{ stubbleColor });

                const double dotSize = faceHeight * 0.018;
                const double spacing = dotSize * 1.1;

                // beard area - from below mouth to chin, and along jawline to ears
                const double beardTop = mouthY + faceHeight * 0.06;
                const double chinBottom = cy + faceHeight * 0.95;

                // fill the entire lower face area with stubble
                const int ySteps = static_cast<int>(safe_divide(chinBottom - beardTop, spacing));
                for (int yi = 0; yi <= ySteps; ++yi)
                    {
                    const double y = beardTop + yi * spacing;
                    // calculate face width at this y position (ellipse)
                    const auto normalizedY = safe_divide<double>(y - cy, faceHeight);
                    const double faceWidthAtY =
                        faceWidth * std::sqrt(std::max(0.0, 1.0 - normalizedY * normalizedY));

                    const double xStart = cx - faceWidthAtY * 0.95;
                    const int xSteps = static_cast<int>(safe_divide(faceWidthAtY * 1.9, spacing));
                    for (int xi = 0; xi <= xSteps; ++xi)
                        {
                        const double x = xStart + xi * spacing;
                        // randomize position
                        const double offsetX = std::sin(x * 0.5 + y * 0.7) * dotSize * 0.8;
                        const double offsetY = std::cos(x * 0.6 + y * 0.4) * dotSize * 0.8;
                        const double dotX = x + offsetX;
                        const double dotY = y + offsetY;

                        // check within face ellipse
                        const auto normX = safe_divide<double>(dotX - cx, faceWidth);
                        const auto normY = safe_divide<double>(dotY - cy, faceHeight);
                        if (normX * normX + normY * normY < 0.92)
                            {
                            // vary size
                            const double sizeVar = 0.6 + std::sin(x * 1.1 + y * 0.9) * 0.4;
                            gc->DrawEllipse(dotX - dotSize * sizeVar * 0.5,
                                            dotY - dotSize * sizeVar * 0.5, dotSize * sizeVar,
                                            dotSize * sizeVar);
                            }
                        }
                    }

                // sideburns along face edge going up toward ears
                for (const int side : { -1, 1 })
                    {
                    const double sideTop = cy + faceHeight * 0.1;
                    const double sideBottom = mouthY + faceHeight * 0.08;

                    const int sideYSteps =
                        static_cast<int>(safe_divide(sideBottom - sideTop, spacing));
                    for (int syi = 0; syi <= sideYSteps; ++syi)
                        {
                        const double y = sideTop + syi * spacing;
                        // find face edge at this y (ellipse x for given y)
                        const auto normY = safe_divide<double>(y - cy, faceHeight);
                        const double edgeX =
                            faceWidth * std::sqrt(std::max(0.0, 1.0 - normY * normY));

                        // stubble width narrows as we go up
                        const auto progress =
                            safe_divide<double>(sideBottom - y, sideBottom - sideTop);
                        const double stubbleWidth = faceWidth * 0.15 * (1.0 - progress * 0.5);

                        // draw stubble along the edge
                        const int inwardSteps =
                            static_cast<int>(safe_divide(stubbleWidth, spacing));
                        for (int ini = 0; ini <= inwardSteps; ++ini)
                            {
                            const double inward = ini * spacing;
                            const double offsetX = std::sin(inward * 1.2 + y * 0.8) * dotSize * 0.7;
                            const double offsetY = std::cos(inward * 0.9 + y * 1.1) * dotSize * 0.7;
                            const double dotX = cx + side * (edgeX * 0.92 - inward) + offsetX;
                            const double dotY = y + offsetY;

                            const auto normX = safe_divide<double>(dotX - cx, faceWidth);
                            const auto normDotY = safe_divide<double>(dotY - cy, faceHeight);
                            if (normX * normX + normDotY * normDotY < 0.88)
                                {
                                const double sizeVar = 0.6 + std::cos(inward * 1.3 + y * 0.7) * 0.4;
                                gc->DrawEllipse(dotX - dotSize * sizeVar * 0.5,
                                                dotY - dotSize * sizeVar * 0.5, dotSize * sizeVar,
                                                dotSize * sizeVar);
                                }
                            }
                        }
                    }

                // mustache above upper lip
                const double mustacheTop = mouthY - faceHeight * 0.12;
                const double mustacheBottom = mouthY - faceHeight * 0.01;
                const double mustacheSpacing = spacing * 0.9;
                const int mustacheYSteps =
                    static_cast<int>(safe_divide(mustacheBottom - mustacheTop, mustacheSpacing));
                for (int myi = 0; myi <= mustacheYSteps; ++myi)
                    {
                    const double y = mustacheTop + myi * mustacheSpacing;
                    const auto progress =
                        safe_divide<double>(y - mustacheTop, mustacheBottom - mustacheTop);
                    const double mustacheWidth = faceWidth * (0.15 + progress * 0.2);

                    const double mxStart = cx - mustacheWidth;
                    const int mustacheXSteps =
                        static_cast<int>(safe_divide(mustacheWidth * 2, mustacheSpacing));
                    for (int mxi = 0; mxi <= mustacheXSteps; ++mxi)
                        {
                        const double x = mxStart + mxi * mustacheSpacing;
                        const double offsetX = std::sin(x * 1.5 + y * 2.3) * dotSize * 0.5;
                        const double offsetY = std::cos(x * 1.8 + y * 1.6) * dotSize * 0.5;
                        const double dotX = x + offsetX;
                        const double dotY = y + offsetY;

                        // small gap under nose center
                        const double distFromCenter = std::abs(dotX - cx);
                        if (distFromCenter > faceWidth * 0.04 ||
                            y > mustacheTop + faceHeight * 0.04)
                            {
                            const double sizeVar = 0.7 + std::sin(x * 2.1 + y * 1.4) * 0.3;
                            gc->DrawEllipse(dotX - dotSize * sizeVar * 0.5,
                                            dotY - dotSize * sizeVar * 0.5, dotSize * sizeVar,
                                            dotSize * sizeVar);
                            }
                        }
                    }
                }
            else if (features.m_facialHair == FacialHair::VanDyke ||
                     features.m_facialHair == FacialHair::Mustache ||
                     features.m_facialHair == FacialHair::Goatee)
                {
                // solid mustache and/or pointed chin goatee (no cheek/jaw growth)
                const bool drawGoatee = (features.m_facialHair == FacialHair::Goatee ||
                                         features.m_facialHair == FacialHair::VanDyke);
                const bool drawMustache = (features.m_facialHair == FacialHair::Mustache ||
                                           features.m_facialHair == FacialHair::VanDyke);

                gc->SetPen(wxPen{ outlineColor, 1 });
                gc->SetBrush(wxBrush{ hairColor });

                if (drawGoatee)
                    {
                    // rounded top just under the lower lip,
                    // narrowing to a point near the chin
                    const double goateeTop = mouthY + faceHeight * 0.07;
                    const double goateeTipY = cy + faceHeight * 0.78;
                    const double goateeHalfTop = faceWidth * 0.18;
                    const double goateeHalfMid = faceWidth * 0.12;

                    wxGraphicsPath goatee = gc->CreatePath();
                    goatee.MoveToPoint(cx - goateeHalfTop, goateeTop);
                    goatee.AddQuadCurveToPoint(cx - goateeHalfMid,
                                               goateeTop + (goateeTipY - goateeTop) * 0.6, cx,
                                               goateeTipY);
                    goatee.AddQuadCurveToPoint(cx + goateeHalfMid,
                                               goateeTop + (goateeTipY - goateeTop) * 0.6,
                                               cx + goateeHalfTop, goateeTop);
                    goatee.AddQuadCurveToPoint(cx, goateeTop - faceHeight * 0.02,
                                               cx - goateeHalfTop, goateeTop);
                    goatee.CloseSubpath();
                    gc->FillPath(goatee);
                    gc->StrokePath(goatee);
                    }

                if (drawMustache)
                    {
                    // two curved halves with a philtrum gap at the center
                    const double mustacheY = mouthY - faceHeight * 0.07;
                    const double mustacheHalfWidth = faceWidth * 0.28;
                    const double mustacheThickness = faceHeight * 0.06;
                    const double philtrumHalf = faceWidth * 0.03;

                    for (const int side : { -1, 1 })
                        {
                        wxGraphicsPath mustache = gc->CreatePath();
                        const double innerX = cx + side * philtrumHalf;
                        const double outerX = cx + side * mustacheHalfWidth;
                        mustache.MoveToPoint(innerX, mustacheY - mustacheThickness * 0.2);
                        mustache.AddQuadCurveToPoint(cx + side * mustacheHalfWidth * 0.6,
                                                     mustacheY - mustacheThickness * 0.9, outerX,
                                                     mustacheY);
                        mustache.AddQuadCurveToPoint(cx + side * mustacheHalfWidth * 0.7,
                                                     mustacheY + mustacheThickness * 0.6, innerX,
                                                     mustacheY + mustacheThickness * 0.3);
                        mustache.CloseSubpath();
                        gc->FillPath(mustache);
                        gc->StrokePath(mustache);
                        }
                    }
                }
            else if (features.m_facialHair == FacialHair::Beard)
                {
                gc->SetPen(wxPen{ outlineColor, 1 });
                gc->SetBrush(wxBrush{ hairColor });

                // beard follows the jawline: outer edges trace the face ellipse;
                // inner boundary sweeps from each sideburn down to the mouth corners,
                // leaving an opening around the mouth/nose for the mustache
                const double sideTopY = cy - faceHeight * 0.08;
                const double chinY = cy + faceHeight * 0.80;
                const double jawMidY = cy + faceHeight * 0.30;
                const double mouthCornerX = mouthWidthVal * 1.4;

                const auto faceEdgeX = [&](double y) -> double
                {
                    const auto normY = safe_divide<double>(y - cy, faceHeight);
                    return faceWidth * std::sqrt(std::max(0.0, 1.0 - normY * normY));
                };

                wxGraphicsPath beard = gc->CreatePath();
                beard.MoveToPoint(cx - faceEdgeX(sideTopY), sideTopY);

                // left outer edge: face ellipse from sideburn to chin
                constexpr int edgeSamples{ 16 };
                for (int i = 1; i <= edgeSamples; ++i)
                    {
                    const double t = static_cast<double>(i) / edgeSamples;
                    const double y = sideTopY + t * (chinY - sideTopY);
                    beard.AddLineToPoint(cx - faceEdgeX(y), y);
                    }

                // chin: round below the face ellipse
                beard.AddQuadCurveToPoint(cx, cy + faceHeight * 1.50, cx + faceEdgeX(chinY), chinY);

                // right outer edge: face ellipse from chin to sideburn
                for (int i = edgeSamples - 1; i >= 1; --i)
                    {
                    const double t = static_cast<double>(i) / edgeSamples;
                    const double y = sideTopY + t * (chinY - sideTopY);
                    beard.AddLineToPoint(cx + faceEdgeX(y), y);
                    }

                // right sideburn -> right mouth corner (follows jawline inward)
                // beardInnerY sits well below the mouth so lips are visible
                const double beardInnerY = mouthY + faceHeight * 0.18;
                beard.AddCurveToPoint(
                    cx + faceEdgeX(jawMidY) * 0.85, jawMidY, cx + mouthCornerX + faceWidth * 0.08,
                    beardInnerY - faceHeight * 0.05, cx + mouthCornerX, beardInnerY);

                // across the mouth opening: bow downward in the center
                beard.AddQuadCurveToPoint(cx, beardInnerY + faceHeight * 0.10, cx - mouthCornerX,
                                          beardInnerY);

                // left mouth corner -> left sideburn (follows jawline outward)
                beard.AddCurveToPoint(
                    cx - mouthCornerX - faceWidth * 0.08, beardInnerY - faceHeight * 0.05,
                    cx - faceEdgeX(jawMidY) * 0.85, jawMidY, cx - faceEdgeX(sideTopY), sideTopY);

                beard.CloseSubpath();
                gc->FillPath(beard, wxODDEVEN_RULE);
                gc->StrokePath(beard);

                    // stipple texture over beard
                    {
                    const wxColour stubbleColor{ Colors::ColorContrast::ShadeOrTint(hairColor,
                                                                                    0.35) };
                    gc->SetPen(*wxTRANSPARENT_PEN);
                    gc->SetBrush(wxBrush{ stubbleColor });
                    const double dotSize = faceHeight * 0.012;
                    const double dotSpacing = dotSize * 1.4;

                    const int ySteps = static_cast<int>(safe_divide(chinY - sideTopY, dotSpacing));
                    for (int yi = 0; yi <= ySteps; ++yi)
                        {
                        const double y = sideTopY + yi * dotSpacing;
                        const double edgeX = faceEdgeX(y);

                        // linear approximation of the inner bezier side boundary
                        double innerX{ 0.0 };
                        if (y < beardInnerY)
                            {
                            const auto frac =
                                safe_divide<double>(y - sideTopY, beardInnerY - sideTopY);
                            innerX = faceEdgeX(sideTopY) * (1.0 - frac) + mouthCornerX * frac;
                            }
                        if (edgeX <= innerX)
                            {
                            continue;
                            }

                        const int xSteps = static_cast<int>(safe_divide(edgeX * 2.0, dotSpacing));
                        for (int xi = 0; xi <= xSteps; ++xi)
                            {
                            const double x = (cx - edgeX) + xi * dotSpacing;
                            if (std::abs(x - cx) < innerX)
                                {
                                continue;
                                }
                            // skip dots above the bowing inner boundary in the chin transition zone
                            if (y >= beardInnerY && y < beardInnerY + faceHeight * 0.11)
                                {
                                const double tBow =
                                    0.5 * (1.0 + safe_divide<double>(x - cx, mouthCornerX));
                                if (tBow >= 0.0 && tBow <= 1.0)
                                    {
                                    const double bowY =
                                        beardInnerY + 2.0 * tBow * (1.0 - tBow) * faceHeight * 0.10;
                                    if (y < bowY)
                                        {
                                        continue;
                                        }
                                    }
                                }
                            const double offX = std::sin(x * 0.5 + y * 0.7) * dotSize * 0.8;
                            const double offY = std::cos(x * 0.6 + y * 0.4) * dotSize * 0.8;
                            const double dotX = x + offX;
                            const double dotY = y + offY;
                            const auto normX = safe_divide<double>(dotX - cx, faceWidth);
                            const auto normDotY = safe_divide<double>(dotY - cy, faceHeight);
                            if (normX * normX + normDotY * normDotY >= 1.0)
                                {
                                continue;
                                }
                            const double sizeVar = 0.6 + std::sin(dotX * 1.1 + dotY * 0.9) * 0.4;
                            gc->DrawEllipse(dotX - dotSize * sizeVar * 0.5,
                                            dotY - dotSize * sizeVar * 0.5, dotSize * sizeVar,
                                            dotSize * sizeVar);
                            }
                        }

                    // chin bezier area: quad bezier extends from chinY down to cy+1.15*fH;
                    // at a given y, x extent =
                    // cx + or - faceEdgeX(chinY)*sqrt(1 -2*(y-chinY)/(0.70*fH))
                    const double chinEdgeX = faceEdgeX(chinY);
                    const double chinBezierScale = 0.70 * faceHeight;
                    const double chinMaxY = chinY + 0.35 * faceHeight;
                    const int chinYSteps =
                        static_cast<int>(safe_divide(chinMaxY - chinY, dotSpacing));
                    for (int cyi = 1; cyi <= chinYSteps; ++cyi)
                        {
                        const double y = chinY + cyi * dotSpacing;
                        const double chinWidthFactorSq =
                            1.0 - 2.0 * safe_divide<double>(y - chinY, chinBezierScale);
                        if (chinWidthFactorSq <= 0.0)
                            {
                            break;
                            }
                        const double halfW = chinEdgeX * std::sqrt(chinWidthFactorSq);
                        const int xSteps = static_cast<int>(safe_divide(halfW * 2.0, dotSpacing));
                        for (int xi = 0; xi <= xSteps; ++xi)
                            {
                            const double x = (cx - halfW) + xi * dotSpacing;
                            const double offX = std::sin(x * 0.5 + y * 0.7) * dotSize * 0.8;
                            const double offY = std::cos(x * 0.6 + y * 0.4) * dotSize * 0.8;
                            const double sizeVar =
                                0.6 + std::sin((x + offX) * 1.1 + (y + offY) * 0.9) * 0.4;
                            gc->DrawEllipse(x + offX - dotSize * sizeVar * 0.5,
                                            y + offY - dotSize * sizeVar * 0.5, dotSize * sizeVar,
                                            dotSize * sizeVar);
                            }
                        }
                    }

                // restore pen/brush for mustache
                gc->SetPen(wxPen{ outlineColor, 1 });
                gc->SetBrush(wxBrush{ hairColor });

                // mustache: narrow band across top lip, wraps around each mouth corner, down to
                // beard
                const double mustacheThickness = faceHeight * 0.12;
                const double philtrumHalf = faceWidth * 0.03;
                const double mustacheTopY = mouthY - mustacheThickness * 0.6;

                for (const int side : { -1, 1 })
                    {
                    wxGraphicsPath mustache = gc->CreatePath();
                    const double sideInnerX = cx + side * mouthWidthVal;
                    const double sideOuterX = cx + side * mouthCornerX;
                    // outer corner: just past the mouth edge
                    const double cornerOuterX =
                        cx + side * (mouthWidthVal + mustacheThickness * 0.6);

                    // outer edge: philtrum -> arc across top lip ->
                    // wrap corner -> down side to beard
                    mustache.MoveToPoint(cx + side * philtrumHalf, mustacheTopY);
                    mustache.AddCurveToPoint(
                        cx + side * mouthWidthVal * 0.5, mustacheTopY - mustacheThickness * 0.5,
                        cx + side * mouthWidthVal, mustacheTopY - mustacheThickness * 0.1,
                        cornerOuterX, mouthY);
                    mustache.AddCurveToPoint(cornerOuterX, mouthY + faceHeight * 0.04, sideOuterX,
                                             beardInnerY - faceHeight * 0.03, sideOuterX,
                                             beardInnerY);

                    // inner edge: return from beard up mouth side and across top lip to philtrum
                    mustache.AddLineToPoint(sideInnerX, beardInnerY);
                    mustache.AddCurveToPoint(sideInnerX, mouthY + mustacheThickness * 0.3,
                                             sideInnerX, mouthY - mustacheThickness * 0.2,
                                             cx + side * mouthWidthVal * 0.9,
                                             mustacheTopY + mustacheThickness * 0.3);
                    mustache.AddCurveToPoint(
                        cx + side * mouthWidthVal * 0.5, mustacheTopY + mustacheThickness * 0.1,
                        cx + side * philtrumHalf * 1.5, mustacheTopY + mustacheThickness * 0.4,
                        cx + side * philtrumHalf, mustacheTopY + mustacheThickness * 0.5);
                    mustache.CloseSubpath();
                    gc->FillPath(mustache);
                    gc->StrokePath(mustache);
                    }
                }
            else if (features.m_facialHair == FacialHair::ChinCurtain)
                {
                // chin curtain (a.k.a. Shenandoah, whaler, Lincoln beard):
                // same beard shape as Beard but no mustache -- upper lip is clean-shaved
                gc->SetPen(wxPen{ outlineColor, 1 });
                gc->SetBrush(wxBrush{ hairColor });

                const double sideTopY = cy - faceHeight * 0.08;
                const double chinY = cy + faceHeight * 0.80;
                const double jawMidY = cy + faceHeight * 0.30;
                const double mouthCornerX = mouthWidthVal * 1.4;

                const auto faceEdgeX = [&](double y) -> double
                {
                    const auto normY = safe_divide<double>(y - cy, faceHeight);
                    return faceWidth * std::sqrt(std::max(0.0, 1.0 - normY * normY));
                };

                wxGraphicsPath beard = gc->CreatePath();
                beard.MoveToPoint(cx - faceEdgeX(sideTopY), sideTopY);

                constexpr int edgeSamples{ 16 };
                for (int i = 1; i <= edgeSamples; ++i)
                    {
                    const double t = static_cast<double>(i) / edgeSamples;
                    const double y = sideTopY + t * (chinY - sideTopY);
                    beard.AddLineToPoint(cx - faceEdgeX(y), y);
                    }

                beard.AddQuadCurveToPoint(cx, cy + faceHeight * 1.50, cx + faceEdgeX(chinY), chinY);

                for (int i = edgeSamples - 1; i >= 1; --i)
                    {
                    const double t = static_cast<double>(i) / edgeSamples;
                    const double y = sideTopY + t * (chinY - sideTopY);
                    beard.AddLineToPoint(cx + faceEdgeX(y), y);
                    }

                const double beardInnerY = mouthY + faceHeight * 0.18;
                beard.AddCurveToPoint(
                    cx + faceEdgeX(jawMidY) * 0.85, jawMidY, cx + mouthCornerX + faceWidth * 0.08,
                    beardInnerY - faceHeight * 0.05, cx + mouthCornerX, beardInnerY);

                beard.AddQuadCurveToPoint(cx, beardInnerY + faceHeight * 0.10, cx - mouthCornerX,
                                          beardInnerY);

                beard.AddCurveToPoint(
                    cx - mouthCornerX - faceWidth * 0.08, beardInnerY - faceHeight * 0.05,
                    cx - faceEdgeX(jawMidY) * 0.85, jawMidY, cx - faceEdgeX(sideTopY), sideTopY);

                beard.CloseSubpath();
                gc->FillPath(beard, wxODDEVEN_RULE);
                gc->StrokePath(beard);

                    {
                    const wxColour stubbleColor{ Colors::ColorContrast::ShadeOrTint(hairColor,
                                                                                    0.35) };
                    gc->SetPen(*wxTRANSPARENT_PEN);
                    gc->SetBrush(wxBrush{ stubbleColor });
                    const double dotSize = faceHeight * 0.012;
                    const double dotSpacing = dotSize * 1.4;

                    const int ySteps = static_cast<int>(safe_divide(chinY - sideTopY, dotSpacing));
                    for (int yi = 0; yi <= ySteps; ++yi)
                        {
                        const double y = sideTopY + yi * dotSpacing;
                        const double edgeX = faceEdgeX(y);

                        double innerX{ 0.0 };
                        if (y < beardInnerY)
                            {
                            const auto frac =
                                safe_divide<double>(y - sideTopY, beardInnerY - sideTopY);
                            innerX = faceEdgeX(sideTopY) * (1.0 - frac) + mouthCornerX * frac;
                            }
                        if (edgeX <= innerX)
                            {
                            continue;
                            }

                        const int xSteps = static_cast<int>(safe_divide(edgeX * 2.0, dotSpacing));
                        for (int xi = 0; xi <= xSteps; ++xi)
                            {
                            const double x = (cx - edgeX) + xi * dotSpacing;
                            if (std::abs(x - cx) < innerX)
                                {
                                continue;
                                }
                            if (y >= beardInnerY && y < beardInnerY + faceHeight * 0.11)
                                {
                                const double tBow =
                                    0.5 * (1.0 + safe_divide<double>(x - cx, mouthCornerX));
                                if (tBow >= 0.0 && tBow <= 1.0)
                                    {
                                    const double bowY =
                                        beardInnerY + 2.0 * tBow * (1.0 - tBow) * faceHeight * 0.10;
                                    if (y < bowY)
                                        {
                                        continue;
                                        }
                                    }
                                }
                            const double offX = std::sin(x * 0.5 + y * 0.7) * dotSize * 0.8;
                            const double offY = std::cos(x * 0.6 + y * 0.4) * dotSize * 0.8;
                            const double dotX = x + offX;
                            const double dotY = y + offY;
                            const auto normX = safe_divide<double>(dotX - cx, faceWidth);
                            const auto normDotY = safe_divide<double>(dotY - cy, faceHeight);
                            if (normX * normX + normDotY * normDotY >= 1.0)
                                {
                                continue;
                                }
                            const double sizeVar = 0.6 + std::sin(dotX * 1.1 + dotY * 0.9) * 0.4;
                            gc->DrawEllipse(dotX - dotSize * sizeVar * 0.5,
                                            dotY - dotSize * sizeVar * 0.5, dotSize * sizeVar,
                                            dotSize * sizeVar);
                            }
                        }

                    const double chinEdgeX = faceEdgeX(chinY);
                    const double chinBezierScale = 0.70 * faceHeight;
                    const double chinMaxY = chinY + 0.35 * faceHeight;
                    const int chinYSteps =
                        static_cast<int>(safe_divide(chinMaxY - chinY, dotSpacing));
                    for (int cyi = 1; cyi <= chinYSteps; ++cyi)
                        {
                        const double y = chinY + cyi * dotSpacing;
                        const double chinWidthFactorSq =
                            1.0 - 2.0 * safe_divide<double>(y - chinY, chinBezierScale);
                        if (chinWidthFactorSq <= 0.0)
                            {
                            break;
                            }
                        const double halfW = chinEdgeX * std::sqrt(chinWidthFactorSq);
                        const int xSteps = static_cast<int>(safe_divide(halfW * 2.0, dotSpacing));
                        for (int xi = 0; xi <= xSteps; ++xi)
                            {
                            const double x = (cx - halfW) + xi * dotSpacing;
                            const double offX = std::sin(x * 0.5 + y * 0.7) * dotSize * 0.8;
                            const double offY = std::cos(x * 0.6 + y * 0.4) * dotSize * 0.8;
                            const double sizeVar =
                                0.6 + std::sin((x + offX) * 1.1 + (y + offY) * 0.9) * 0.4;
                            gc->DrawEllipse(x + offX - dotSize * sizeVar * 0.5,
                                            y + offY - dotSize * sizeVar * 0.5, dotSize * sizeVar,
                                            dotSize * sizeVar);
                            }
                        }
                    }
                }
            else if (features.m_facialHair == FacialHair::FuManchu)
                {
                // thin mustache with two long narrow strands drooping past the mouth corners
                gc->SetPen(wxPen{ outlineColor, 1 });
                gc->SetBrush(wxBrush{ hairColor });

                const double mustacheY = mouthY - faceHeight * 0.07;
                const double mustacheHalfWidth = mouthWidthVal * 1.15;
                const double mustacheThickness = faceHeight * 0.07;
                const double philtrumHalf = faceWidth * 0.03;
                const double strandW = faceWidth * 0.07;
                const double strandTopY = mustacheY + mustacheThickness * 0.15;
                const double connBotY = cy + faceHeight * 0.82;
                const double midMouthY = mouthY + faceHeight * 0.15;

                for (const int side : { -1, 1 })
                    {
                    const double innerX = cx + side * philtrumHalf;
                    const double outerX = cx + side * mustacheHalfWidth;

                    // mustache half with philtrum gap
                    wxGraphicsPath mustache = gc->CreatePath();
                    mustache.MoveToPoint(innerX, mustacheY - mustacheThickness * 0.2);
                    mustache.AddQuadCurveToPoint(cx + side * mustacheHalfWidth * 0.6,
                                                 mustacheY - mustacheThickness * 0.9, outerX,
                                                 mustacheY);
                    mustache.AddQuadCurveToPoint(cx + side * mustacheHalfWidth * 0.7,
                                                 mustacheY + mustacheThickness * 0.6, innerX,
                                                 mustacheY + mustacheThickness * 0.3);
                    mustache.CloseSubpath();
                    gc->FillPath(mustache);
                    gc->StrokePath(mustache);
                    }

                // horseshoe connector: both strands drop around the mouth corners and
                // connect across the bottom under the chin as one continuous U shape.
                // Trace inner boundary right-to-left, then outer boundary left-to-right.
                const double hangX = faceWidth * 0.42;

                wxGraphicsPath horseshoe = gc->CreatePath();

                // right inner top -> inner bottom center
                horseshoe.MoveToPoint(cx + hangX - strandW, strandTopY);
                horseshoe.AddCurveToPoint(cx + hangX - strandW, midMouthY, cx + hangX * 0.35,
                                          connBotY - strandW, cx, connBotY - strandW);

                // inner bottom center -> left inner top
                horseshoe.AddCurveToPoint(cx - hangX * 0.35, connBotY - strandW,
                                          cx - hangX + strandW, midMouthY, cx - hangX + strandW,
                                          strandTopY);

                // across to left outer top
                horseshoe.AddLineToPoint(cx - hangX - strandW, strandTopY);

                // left outer top -> outer bottom center
                horseshoe.AddCurveToPoint(cx - hangX - strandW, midMouthY, cx - hangX * 0.35,
                                          connBotY + strandW, cx, connBotY + strandW);

                // outer bottom center -> right outer top
                horseshoe.AddCurveToPoint(cx + hangX * 0.35, connBotY + strandW,
                                          cx + hangX + strandW, midMouthY, cx + hangX + strandW,
                                          strandTopY);

                // across to start
                horseshoe.AddLineToPoint(cx + hangX - strandW, strandTopY);
                horseshoe.CloseSubpath();
                gc->FillPath(horseshoe);
                gc->StrokePath(horseshoe);
                }
            }

        // draw hair that goes over forehead (after face, before eyebrows)
        if (parts.m_hair && hairStyle != HairStyleKind::Bald &&
            hairStyle != HairStyleKind::HighTopFade && hairStyle != HairStyleKind::FlatTop &&
            hairStyle != HairStyleKind::PartiallyBald && hairStyle != HairStyleKind::BaldCombOver &&
            hairStyle != HairStyleKind::CombOver)
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(130);
            const wxColour hairShadow = hairColor.ChangeLightness(80);
            // for dark hair, artists use a blue tint for strands instead of black
            const wxColour hairStrandColor =
                Colors::ColorContrast::IsDark(hairColor) ?
                    wxColour{ 70, 90, 160, 90 } : // blue tint for dark hair
                    wxColour{ 0, 0, 0, 40 };      // dark strands for light hair

            // calculate where eyebrows will be drawn (to stop hair above them)
            const double eyeYPreCalc = cy - faceHeight * (0.15 + 0.2 * features.m_eyePosition);
            const double eyeRadiusPreCalc = faceHeight * 0.1 * (0.6 + 0.8 * features.m_eyeSize);
            const double browYLimit = eyeYPreCalc - eyeRadiusPreCalc * 1.8 - faceHeight * 0.05;

            if (hairStyle == HairStyleKind::Bob)
                {
                // bob: hair frames face with bangs covering forehead
                wxGraphicsPath bobHair = gc->CreatePath();
                // outer edge - start from bottom left, go up and around
                bobHair.MoveToPoint(cx - faceWidth * 0.92, cy + faceHeight * 0.6);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.98, cy + faceHeight * 0.2,
                                            cx - faceWidth * 0.98, cy - faceHeight * 0.3);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 1.0, cy - faceHeight * 0.98, cx,
                                            cy - faceHeight * 1.03);
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 1.0, cy - faceHeight * 0.98,
                                            cx + faceWidth * 0.98, cy - faceHeight * 0.3);
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 0.98, cy + faceHeight * 0.2,
                                            cx + faceWidth * 0.92, cy + faceHeight * 0.6);
                // inner edge - cut out for face, going back up
                bobHair.AddLineToPoint(cx + faceWidth * 0.82, cy + faceHeight * 0.48);
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 0.88, cy + faceHeight * 0.1,
                                            cx + faceWidth * 0.85, browYLimit + faceHeight * 0.15);
                // bangs across forehead - asymmetric sweep (higher on right, lower on left)
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 0.3, browYLimit - faceHeight * 0.06,
                                            cx, browYLimit);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.4, browYLimit + faceHeight * 0.04,
                                            cx - faceWidth * 0.72, browYLimit + faceHeight * 0.06);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.92, browYLimit + faceHeight * 0.02,
                                            cx - faceWidth * 0.88, cy + faceHeight * 0.1);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.86, cy + faceHeight * 0.25,
                                            cx - faceWidth * 0.82, cy + faceHeight * 0.48);
                bobHair.CloseSubpath();

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
                gc->FillPath(bobHair);
                gc->StrokePath(bobHair);

                // curved sheen following top curve of hair
                wxGraphicsPath bobHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                bobHairSheen.MoveToPoint(cx - faceWidth * 0.75, cy - faceHeight * 0.5);
                bobHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.6, cy - faceHeight * 0.9,
                                                 cx - faceWidth * 0.1, cy - faceHeight * 0.98);
                // inner arc (parallel, inside the hair)
                bobHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.5, cy - faceHeight * 0.8,
                                                 cx - faceWidth * 0.65, cy - faceHeight * 0.5);
                bobHairSheen.CloseSubpath();
                auto bobSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.5, cy - faceHeight * 0.95, cx - faceWidth * 0.5,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
                gc->SetBrush(bobSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(bobHairSheen);

                // hair strands for texture
                gc->SetPen(wxPen(hairStrandColor, 1));
                // top/crown strands - stay well above bangs line
                const double strandBottomLimit = browYLimit - faceHeight * 0.08;
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    // add variation to spacing, arc, and length
                    const double spacingVar = std::sin(s * 2.1) * faceWidth * 0.025;
                    const double arcVar = std::cos(s * 1.7) * faceWidth * 0.05;
                    const double lengthVar = std::sin(s * 3.2) * faceHeight * 0.04;
                    // spread across the hair width
                    const double startX = cx - faceWidth * 0.45 + s * faceWidth * 0.18 + spacingVar;
                    const double topY =
                        cy - faceHeight * 0.98 + std::cos(s * 2.5) * faceHeight * 0.02;
                    strand.MoveToPoint(startX, topY);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.12 + arcVar,
                                               topY + faceHeight * 0.22, startX + faceWidth * 0.06,
                                               strandBottomLimit + lengthVar);
                    gc->StrokePath(strand);
                    }
                // left outer side strands - stay within hair boundary, curve outward
                for (int s = 0; s < 4; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.88;
                    const double startY = cy - faceHeight * 0.4 + s * faceHeight * 0.22;
                    strand.MoveToPoint(startX, startY);
                    strand.AddQuadCurveToPoint(
                        startX - faceWidth * 0.15, startY + faceHeight * 0.12,
                        startX - faceWidth * 0.08, startY + faceHeight * 0.28);
                    gc->StrokePath(strand);
                    }
                // right outer side strands - stay within hair boundary, curve outward
                for (int s = 0; s < 4; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx + faceWidth * 0.88;
                    const double startY = cy - faceHeight * 0.4 + s * faceHeight * 0.22;
                    strand.MoveToPoint(startX, startY);
                    strand.AddQuadCurveToPoint(
                        startX + faceWidth * 0.15, startY + faceHeight * 0.12,
                        startX + faceWidth * 0.08, startY + faceHeight * 0.28);
                    gc->StrokePath(strand);
                    }
                }
            else if (hairStyle == HairStyleKind::Pixie)
                {
                // pixie: short textured hair covering forehead
                wxGraphicsPath pixieHair = gc->CreatePath();
                // outer edge
                pixieHair.MoveToPoint(cx - faceWidth * 0.82, cy - faceHeight * 0.3);
                pixieHair.AddQuadCurveToPoint(cx - faceWidth * 0.92, cy - faceHeight * 0.95, cx,
                                              cy - faceHeight * 1.05);
                pixieHair.AddQuadCurveToPoint(cx + faceWidth * 0.92, cy - faceHeight * 0.95,
                                              cx + faceWidth * 0.82, cy - faceHeight * 0.3);
                // inner edge with textured bangs
                pixieHair.AddQuadCurveToPoint(cx + faceWidth * 0.55, browYLimit - faceHeight * 0.05,
                                              cx + faceWidth * 0.28,
                                              browYLimit + faceHeight * 0.02);
                pixieHair.AddQuadCurveToPoint(cx + faceWidth * 0.1, browYLimit - faceHeight * 0.03,
                                              cx - faceWidth * 0.1, browYLimit + faceHeight * 0.03);
                pixieHair.AddQuadCurveToPoint(cx - faceWidth * 0.38, browYLimit - faceHeight * 0.02,
                                              cx - faceWidth * 0.82, cy - faceHeight * 0.3);
                pixieHair.CloseSubpath();

                gc->SetBrush(wxBrush{ hairColor });
                gc->SetPen(wxPen{ hairShadow, 1 });
                gc->FillPath(pixieHair);
                gc->StrokePath(pixieHair);

                // curved sheen following top curve of hair
                wxGraphicsPath pixieHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                pixieHairSheen.MoveToPoint(cx - faceWidth * 0.65, cy - faceHeight * 0.5);
                pixieHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.55, cy - faceHeight * 0.88,
                                                   cx - faceWidth * 0.1, cy - faceHeight * 1.0);
                // inner arc (parallel, inside the hair)
                pixieHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.45, cy - faceHeight * 0.78,
                                                   cx - faceWidth * 0.55, cy - faceHeight * 0.5);
                pixieHairSheen.CloseSubpath();
                auto pixieSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.45, cy - faceHeight * 0.95, cx - faceWidth * 0.45,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour{ hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0 });
                gc->SetBrush(pixieSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(pixieHairSheen);

                // hair strands for texture - short wispy strands
                gc->SetPen(wxPen(hairStrandColor, 1));
                // top strands radiating outward with more arc
                for (int s = 0; s < 7; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    // narrower range and variation
                    const double spacingVar = std::sin(s * 2.3) * 0.02;
                    const double offsetX = -0.38 + s * 0.13 + spacingVar;
                    const double startX = cx + faceWidth * offsetX;
                    // outer strands start lower to stay within hair
                    const double outerAdj = std::abs(offsetX) * faceHeight * 0.15;
                    const double startY = cy - faceHeight * 0.95 + outerAdj;
                    strand.MoveToPoint(startX, startY);
                    const double arcVar = std::cos(s * 1.9) * faceWidth * 0.04;
                    strand.AddQuadCurveToPoint(
                        startX + faceWidth * 0.18 * offsetX + arcVar, startY + faceHeight * 0.22,
                        startX + faceWidth * 0.1 * offsetX, browYLimit + faceHeight * 0.01);
                    gc->StrokePath(strand);
                    }
                // additional crown strands with strong arc
                for (int s = 0; s < 5; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double spacingVar = std::sin(s * 2.7) * faceWidth * 0.02;
                    const double arcVar = std::cos(s * 1.5) * faceWidth * 0.04;
                    const double startX = cx - faceWidth * 0.28 + s * faceWidth * 0.14 + spacingVar;
                    strand.MoveToPoint(startX, cy - faceHeight * 0.97);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.15 + arcVar,
                                               cy - faceHeight * 0.65, startX + faceWidth * 0.06,
                                               browYLimit - faceHeight * 0.02);
                    gc->StrokePath(strand);
                    }
                }
            else if (hairStyle == HairStyleKind::LongStraight)
                {
                // long straight: flowing hair with side-swept bangs
                wxGraphicsPath longHair = gc->CreatePath();
                // outer edge - start bottom left, go up and around, down to bottom right
                longHair.MoveToPoint(cx - faceWidth * 0.95, cy + faceHeight * 0.95);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 1.0, cy + faceHeight * 0.4,
                                             cx - faceWidth * 0.98, cy - faceHeight * 0.3);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 1.0, cy - faceHeight * 0.98, cx,
                                             cy - faceHeight * 1.03);
                longHair.AddQuadCurveToPoint(cx + faceWidth * 1.0, cy - faceHeight * 0.98,
                                             cx + faceWidth * 0.98, cy - faceHeight * 0.3);
                longHair.AddQuadCurveToPoint(cx + faceWidth * 1.0, cy + faceHeight * 0.4,
                                             cx + faceWidth * 0.95, cy + faceHeight * 0.95);
                // inner cutout for face - go back up
                longHair.AddLineToPoint(cx + faceWidth * 0.82, cy + faceHeight * 0.75);
                longHair.AddQuadCurveToPoint(cx + faceWidth * 0.88, cy + faceHeight * 0.25,
                                             cx + faceWidth * 0.85, browYLimit + faceHeight * 0.12);
                // side-swept bangs
                longHair.AddQuadCurveToPoint(cx + faceWidth * 0.38, browYLimit - faceHeight * 0.02,
                                             cx, browYLimit + faceHeight * 0.04);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 0.45, browYLimit + faceHeight * 0.02,
                                             cx - faceWidth * 0.85, browYLimit + faceHeight * 0.08);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 0.88, cy + faceHeight * 0.25,
                                             cx - faceWidth * 0.82, cy + faceHeight * 0.75);
                longHair.CloseSubpath();

                gc->SetBrush(wxBrush{ hairColor });
                gc->SetPen(wxPen{ hairShadow, 1 });
                gc->FillPath(longHair);
                gc->StrokePath(longHair);

                // curved sheen following top curve of hair
                wxGraphicsPath longHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                longHairSheen.MoveToPoint(cx - faceWidth * 0.75, cy - faceHeight * 0.5);
                longHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.6, cy - faceHeight * 0.9,
                                                  cx - faceWidth * 0.1, cy - faceHeight * 0.98);
                // inner arc (parallel, inside the hair)
                longHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.5, cy - faceHeight * 0.8,
                                                  cx - faceWidth * 0.65, cy - faceHeight * 0.5);
                longHairSheen.CloseSubpath();
                auto longSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.5, cy - faceHeight * 0.95, cx - faceWidth * 0.5,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour{ hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0 });
                gc->SetBrush(longSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(longHairSheen);

                // hair strands for texture - long flowing strands
                gc->SetPen(wxPen(hairStrandColor, 1));
                // top/crown strands flowing down with arc
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.5 + s * faceWidth * 0.2;
                    strand.MoveToPoint(startX, cy - faceHeight * 0.98);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.18, cy - faceHeight * 0.4,
                                               startX + faceWidth * 0.1,
                                               browYLimit + faceHeight * 0.03);
                    gc->StrokePath(strand);
                    }
                // bangs strands - side swept with more arc
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.5 + s * faceWidth * 0.2;
                    strand.MoveToPoint(startX, cy - faceHeight * 0.92);
                    strand.AddQuadCurveToPoint(
                        startX + faceWidth * 0.22, browYLimit - faceHeight * 0.12,
                        startX + faceWidth * 0.15, browYLimit + faceHeight * 0.05);
                    gc->StrokePath(strand);
                    }
                // outer edge strands (on the hair portion outside the face)
                for (const int side : { -1, 1 })
                    {
                    for (int s = 0; s < 3; ++s)
                        {
                        wxGraphicsPath strand = gc->CreatePath();
                        const double startX = cx + side * faceWidth * (0.92 + s * 0.03);
                        const double startY = cy - faceHeight * (0.5 - s * 0.2);
                        strand.MoveToPoint(startX, startY);
                        strand.AddQuadCurveToPoint(
                            startX + side * faceWidth * 0.12, startY + faceHeight * 0.4,
                            startX + side * faceWidth * 0.05, cy + faceHeight * 0.85);
                        gc->StrokePath(strand);
                        }
                    }
                }
            else if (hairStyle == HairStyleKind::Bun)
                {
                // bun: hair pulled back smoothly with bun on top (no bangs)
                wxGraphicsPath bunHair = gc->CreatePath();
                // outer edge
                bunHair.MoveToPoint(cx - faceWidth * 0.85, cy - faceHeight * 0.15);
                bunHair.AddQuadCurveToPoint(cx - faceWidth * 0.92, cy - faceHeight * 0.9, cx,
                                            cy - faceHeight * 1.0);
                bunHair.AddQuadCurveToPoint(cx + faceWidth * 0.92, cy - faceHeight * 0.9,
                                            cx + faceWidth * 0.85, cy - faceHeight * 0.15);
                // inner edge - follows hairline, no bangs
                bunHair.AddQuadCurveToPoint(cx + faceWidth * 0.55, cy - faceHeight * 0.45, cx,
                                            cy - faceHeight * 0.5);
                bunHair.AddQuadCurveToPoint(cx - faceWidth * 0.55, cy - faceHeight * 0.45,
                                            cx - faceWidth * 0.85, cy - faceHeight * 0.15);
                bunHair.CloseSubpath();

                gc->SetBrush(wxBrush{ hairColor });
                gc->SetPen(wxPen{ hairShadow, 1 });
                gc->FillPath(bunHair);
                gc->StrokePath(bunHair);

                // curved sheen following top curve of hair
                wxGraphicsPath bunHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                bunHairSheen.MoveToPoint(cx - faceWidth * 0.7, cy - faceHeight * 0.55);
                bunHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.55, cy - faceHeight * 0.85,
                                                 cx - faceWidth * 0.1, cy - faceHeight * 0.95);
                // inner arc (parallel, inside the hair)
                bunHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.45, cy - faceHeight * 0.75,
                                                 cx - faceWidth * 0.6, cy - faceHeight * 0.55);
                bunHairSheen.CloseSubpath();
                auto bunHairSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.5, cy - faceHeight * 0.9, cx - faceWidth * 0.5,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour{ hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0 });
                gc->SetBrush(bunHairSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(bunHairSheen);

                // hair strands for texture - swept back look (drawn before bun)
                gc->SetPen(wxPen(hairStrandColor, 1));
                // crown strands sweeping back toward bun with strong arc
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.4 + s * faceWidth * 0.16;
                    const double startY = cy - faceHeight * 0.48;
                    strand.MoveToPoint(startX, startY);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.15, cy - faceHeight * 0.75,
                                               cx, cy - faceHeight * 0.95);
                    gc->StrokePath(strand);
                    }
                // side strands curving up toward bun
                for (const int side : { -1, 1 })
                    {
                    for (int s = 0; s < 3; ++s)
                        {
                        wxGraphicsPath strand = gc->CreatePath();
                        const double startX = cx + side * faceWidth * (0.82 - s * 0.06);
                        const double startY = cy - faceHeight * (0.18 + s * 0.1);
                        strand.MoveToPoint(startX, startY);
                        strand.AddQuadCurveToPoint(
                            startX + side * faceWidth * 0.15, cy - faceHeight * 0.55,
                            cx + side * faceWidth * 0.1, cy - faceHeight * 0.9);
                        gc->StrokePath(strand);
                        }
                    }

                // the bun itself - drawn on top of strands
                const double bunRadius = faceHeight * 0.18;
                const double bunX = cx;
                const double bunY = cy - faceHeight * 1.05;
                gc->SetBrush(wxBrush{ hairColor });
                gc->SetPen(wxPen{ hairShadow, 1 });
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // sheen on bun
                auto bunSheen =
                    gc->CreateRadialGradientBrush(bunX - bunRadius * 0.3, bunY - bunRadius * 0.3,
                                                  bunX, bunY, bunRadius, hairHighlight, hairColor);
                gc->SetBrush(bunSheen);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // outline the bun
                gc->SetPen(wxPen{ hairShadow, 1 });
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // strands on the bun itself - spiral pattern with more curve
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double angle = -1.0 + s * 0.5;
                    const double innerR = bunRadius * 0.2;
                    const double outerR = bunRadius * 0.9;
                    strand.MoveToPoint(bunX + innerR * std::cos(angle),
                                       bunY + innerR * std::sin(angle));
                    strand.AddQuadCurveToPoint(bunX + bunRadius * 0.7 * std::cos(angle + 0.5),
                                               bunY + bunRadius * 0.7 * std::sin(angle + 0.5),
                                               bunX + outerR * std::cos(angle + 0.8),
                                               bunY + outerR * std::sin(angle + 0.8));
                    gc->StrokePath(strand);
                    }
                }
            else if (hairStyle == HairStyleKind::Curly || hairStyle == HairStyleKind::LongCurly)
                {
                const wxColour curlHighlightColor =
                    Colors::ColorContrast::BlackOrWhiteContrast(hairColor);
                const wxColour curlyStrandColor{ curlHighlightColor.Red(),
                                                 curlHighlightColor.Green(),
                                                 curlHighlightColor.Blue(), 130 };

                // helper to draw an individual filled "puff" of hair
                auto drawCurlyPuff = [&](double x, double y, double r)
                {
                    gc->SetBrush(wxBrush(hairColor));
                    gc->SetPen(wxPen(hairShadow, 1));
                    gc->DrawEllipse(x - r, y - r, r * 2, r * 2);

                    gc->SetPen(wxPen(curlyStrandColor, 1));
                    wxGraphicsPath strand = gc->CreatePath();
                    strand.AddArc(x, y, r * 0.6, 0.5, 3.5, true);
                    gc->StrokePath(strand);
                };

                // build the hair mass using a high-density "cloud" distribution
                // to create a natural, rounded volume
                const int puffCount = (hairStyle == HairStyleKind::LongCurly) ? 500 : 350;
                for (int i = 0; i < puffCount; ++i)
                    {
                    const double t = i / static_cast<double>(puffCount - 1);
                    // spread around the head from left shoulder to right shoulder
                    const double angle = -std::numbers::pi * 1.15 + t * std::numbers::pi * 2.3;

                    // vary distance to fill the volume, ensuring a minimum depth
                    const double dist = 0.4 + 0.6 * (std::abs(std::cos(i * 23.456)));

                    // wider at the sides, rounded at the top
                    const double px = cx + faceWidth * 1.35 * dist * std::cos(angle);
                    // use a shifted vertical center for long hair so it goes down more
                    // without becoming taller at the top
                    const double py =
                        (hairStyle == HairStyleKind::LongCurly) ?
                            cy - faceHeight * 0.15 + faceHeight * 1.15 * dist * std::sin(angle) :
                            cy - faceHeight * 0.35 + faceHeight * 0.9 * dist * std::sin(angle);

                    // positioning constraints:
                    // vertical length
                    const double verticalLimit =
                        (hairStyle == HairStyleKind::LongCurly) ?
                            cy + faceHeight * 1.05 : // shoulder/chest level
                            cy + faceHeight * 0.45;  // neck level
                    // keep bangs well above the eyes, high on forehead
                    const double bangsLevel = browYLimit - faceHeight * 0.25;

                    bool showPuff = (py < verticalLimit);
                    // if over the face center, move the limit even higher for bangs
                    // and push it to the sides more to expose the face
                    if (showPuff && std::abs(px - cx) < faceWidth * 0.95)
                        {
                        showPuff = (py < bangsLevel);
                        }

                    if (showPuff)
                        {
                        // vary puff size slightly for more organic texture
                        const double puffSize =
                            faceWidth * (0.2 + 0.1 * std::abs(std::sin(i * 7.89)));
                        drawCurlyPuff(px, py, puffSize);
                        }
                    }
                }
            }
        // high top fade - works for both genders
        else if (parts.m_hair && hairStyle == HairStyleKind::HighTopFade)
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(130);
            const wxColour hairShadow = hairColor.ChangeLightness(80);

            // thin arc of hair on top - connects to head at sides
            wxGraphicsPath hair = gc->CreatePath();
            // outer edge - starts where head curve is, follows around
            hair.MoveToPoint(cx - faceWidth * 0.95, cy - faceHeight * 0.3);
            hair.AddCurveToPoint(cx - faceWidth * 1.0, cy - faceHeight * 0.7, cx - faceWidth * 0.55,
                                 cy - faceHeight * 1.05, cx, cy - faceHeight * 1.08);
            hair.AddCurveToPoint(cx + faceWidth * 0.55, cy - faceHeight * 1.05,
                                 cx + faceWidth * 1.0, cy - faceHeight * 0.7, cx + faceWidth * 0.95,
                                 cy - faceHeight * 0.3);
            // inner edge - higher hairline showing forehead
            hair.AddCurveToPoint(cx + faceWidth * 0.9, cy - faceHeight * 0.6, cx + faceWidth * 0.5,
                                 cy - faceHeight * 0.85, cx, cy - faceHeight * 0.88);
            hair.AddCurveToPoint(cx - faceWidth * 0.5, cy - faceHeight * 0.85, cx - faceWidth * 0.9,
                                 cy - faceHeight * 0.6, cx - faceWidth * 0.95,
                                 cy - faceHeight * 0.3);
            hair.CloseSubpath();

            gc->SetBrush(wxBrush{ hairColor });
            gc->SetPen(wxPen{ hairShadow, 1 });
            gc->FillPath(hair);
            gc->StrokePath(hair);

            // curved sheen
            wxGraphicsPath sheen = gc->CreatePath();
            sheen.MoveToPoint(cx - faceWidth * 0.7, cy - faceHeight * 0.65);
            sheen.AddQuadCurveToPoint(cx - faceWidth * 0.4, cy - faceHeight * 0.95,
                                      cx - faceWidth * 0.05, cy - faceHeight * 1.0);
            sheen.AddQuadCurveToPoint(cx - faceWidth * 0.35, cy - faceHeight * 0.88,
                                      cx - faceWidth * 0.6, cy - faceHeight * 0.65);
            sheen.CloseSubpath();

            auto sheenBrush = gc->CreateLinearGradientBrush(
                cx - faceWidth * 0.4, cy - faceHeight * 0.98, cx - faceWidth * 0.4,
                cy - faceHeight * 0.75, hairHighlight,
                wxColour{ hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0 });
            gc->SetBrush(sheenBrush);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->FillPath(sheen);
            }
        else if (parts.m_hair && hairStyle == HairStyleKind::FlatTop)
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(130);
            const wxColour hairShadow = hairColor.ChangeLightness(80);

            // flat top: boxy top, flat surface
            wxGraphicsPath hair = gc->CreatePath();
            // left side vertical-ish
            hair.MoveToPoint(cx - faceWidth * 0.95, cy - faceHeight * 0.3);
            hair.AddCurveToPoint(cx - faceWidth * 0.98, cy - faceHeight * 0.7,
                                 cx - faceWidth * 0.92, cy - faceHeight * 0.95,
                                 cx - faceWidth * 0.85, cy - faceHeight * 1.0);
            // flat top
            hair.AddLineToPoint(cx + faceWidth * 0.85, cy - faceHeight * 1.0);
            // right side vertical-ish
            hair.AddCurveToPoint(cx + faceWidth * 0.92, cy - faceHeight * 0.95,
                                 cx + faceWidth * 0.98, cy - faceHeight * 0.7,
                                 cx + faceWidth * 0.95, cy - faceHeight * 0.3);
            // inner edge
            hair.AddCurveToPoint(cx + faceWidth * 0.8, cy - faceHeight * 0.5, cx + faceWidth * 0.4,
                                 cy - faceHeight * 0.65, cx, cy - faceHeight * 0.68);
            hair.AddCurveToPoint(cx - faceWidth * 0.4, cy - faceHeight * 0.65, cx - faceWidth * 0.8,
                                 cy - faceHeight * 0.5, cx - faceWidth * 0.95,
                                 cy - faceHeight * 0.3);
            hair.CloseSubpath();

            gc->SetBrush(wxBrush{ hairColor });
            gc->SetPen(wxPen{ hairShadow, 1 });
            gc->FillPath(hair);
            gc->StrokePath(hair);

            // top flat surface sheen
            wxGraphicsPath sheen = gc->CreatePath();
            sheen.MoveToPoint(cx - faceWidth * 0.85, cy - faceHeight * 1.0);
            sheen.AddLineToPoint(cx + faceWidth * 0.85, cy - faceHeight * 1.0);
            sheen.AddLineToPoint(cx + faceWidth * 0.8, cy - faceHeight * 0.95);
            sheen.AddLineToPoint(cx - faceWidth * 0.8, cy - faceHeight * 0.95);
            sheen.CloseSubpath();

            auto sheenBrush = gc->CreateLinearGradientBrush(
                cx, cy - faceHeight * 1.0, cx, cy - faceHeight * 0.95, hairHighlight, hairColor);
            gc->SetBrush(sheenBrush);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->FillPath(sheen);
            }
        else if (parts.m_hair && (hairStyle == HairStyleKind::PartiallyBald ||
                                  hairStyle == HairStyleKind::BaldCombOver))
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(130);
            const wxColour hairShadow = hairColor.ChangeLightness(80);
            const wxColour hairStrandColor = Colors::ColorContrast::IsDark(hairColor) ?
                                                 wxColour{ 70, 90, 160, 90 } :
                                                 wxColour{ 0, 0, 0, 40 };

            // small tuft on each side of the head behind the ears; the tuft ends
            // around ear level and bumps just outside the head outline, leaving the
            // top and crown bald
            for (const int side : { -1, 1 })
                {
                wxGraphicsPath tuft = gc->CreatePath();
                // top-inner corner sits at the temple - flat across so the tuft
                // doesn't curve down toward the eyebrows
                tuft.MoveToPoint(cx + side * faceWidth * 0.72, cy - faceHeight * 0.52);
                // top edge - mostly flat across to the outer corner
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 0.95, cy - faceHeight * 0.55,
                                         cx + side * faceWidth * 1.04, cy - faceHeight * 0.4);
                // outer edge down to ear-bottom, extending slightly past head
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 1.04, cy - faceHeight * 0.05,
                                         cx + side * faceWidth * 0.9, cy + faceHeight * 0.1);
                // inner edge curves back up, leaving the crown bald
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 0.78, cy - faceHeight * 0.02,
                                         cx + side * faceWidth * 0.78, cy - faceHeight * 0.25);
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 0.75, cy - faceHeight * 0.45,
                                         cx + side * faceWidth * 0.72, cy - faceHeight * 0.52);
                tuft.CloseSubpath();

                gc->SetBrush(wxBrush{ hairColor });
                gc->SetPen(wxPen{ hairShadow, 1 });
                gc->FillPath(tuft);
                gc->StrokePath(tuft);

                // subtle sheen along the upper portion of the tuft
                wxGraphicsPath sheen = gc->CreatePath();
                sheen.MoveToPoint(cx + side * faceWidth * 0.85, cy - faceHeight * 0.5);
                sheen.AddQuadCurveToPoint(cx + side * faceWidth * 0.98, cy - faceHeight * 0.35,
                                          cx + side * faceWidth * 0.96, cy - faceHeight * 0.1);
                sheen.AddQuadCurveToPoint(cx + side * faceWidth * 0.85, cy - faceHeight * 0.22,
                                          cx + side * faceWidth * 0.8, cy - faceHeight * 0.45);
                sheen.CloseSubpath();
                auto sheenBrush = gc->CreateLinearGradientBrush(
                    cx + side * faceWidth * 0.9, cy - faceHeight * 0.5, cx + side * faceWidth * 0.9,
                    cy - faceHeight * 0.1, hairHighlight,
                    wxColour{ hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0 });
                gc->SetBrush(sheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(sheen);

                // short hair strands for texture
                gc->SetPen(wxPen{ hairStrandColor, 1 });
                for (int idx = 0; idx < 3; ++idx)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startY = cy - faceHeight * (0.45 - idx * 0.18);
                    strand.MoveToPoint(cx + side * faceWidth * 0.96, startY);
                    strand.AddQuadCurveToPoint(
                        cx + side * faceWidth * 0.88, startY + faceHeight * 0.05,
                        cx + side * faceWidth * 0.82, startY + faceHeight * 0.09);
                    gc->StrokePath(strand);
                    }
                }

            // comb-over: 10 thick hair strands sweeping across the bald crown,
            // each curving up to follow the crown's curvature. all strands start
            // at the right-side crown edge; left ends vary so some come up short.
            // strands overlap vertically so the comb-over reads as thicker
            if (hairStyle == HairStyleKind::BaldCombOver)
                {
                constexpr size_t combOverCount{ 10 };
                const double strandThickness = std::max(1.0, faceHeight * 0.026);
                const double combTopY = cy - faceHeight * 0.92;
                const double combBottomY = cy - faceHeight * 0.6;

                // shortening factors for the left end (right end is always at the
                // crown edge); values < 1.0 make a strand stop short of the left edge
                constexpr std::array<double, combOverCount> leftShorten{
                    1.0, 0.92, 1.0, 0.85, 0.95, 1.0, 0.78, 0.9, 1.0, 0.7
                };

                const auto crownHalfAt = [&](const double yPos)
                {
                    const double yOff = safe_divide<double>(yPos - cy, faceHeight);
                    return faceWidth * std::sqrt(std::max(0.0, 1.0 - yOff * yOff));
                };

                gc->SetPen(wxPen(hairColor, strandThickness));
                for (size_t idx = 0; idx < combOverCount; ++idx)
                    {
                    const double tParam = safe_divide<double>(
                        static_cast<double>(idx), static_cast<double>(combOverCount - 1));
                    const double yPos = combTopY + tParam * (combBottomY - combTopY);
                    const double crownHalf = crownHalfAt(yPos);
                    const double rightX = cx + crownHalf;
                    const double leftX = cx - crownHalf * leftShorten.at(idx);
                    // lower strands arc up more (they have to travel further over the
                    // curve of the crown to reach the other side)
                    const double arcLift = faceHeight * (0.03 + 0.06 * tParam);

                    wxGraphicsPath strand = gc->CreatePath();
                    strand.MoveToPoint(rightX, yPos);
                    strand.AddQuadCurveToPoint((leftX + rightX) * math_constants::half,
                                               yPos - 2 * arcLift, leftX, yPos);
                    gc->StrokePath(strand);
                    }

                // thin highlight strands layered on top for sheen
                gc->SetPen(wxPen(hairHighlight, std::max(1.0, strandThickness * 0.4)));
                for (size_t idx = 0; idx < combOverCount; idx += 2)
                    {
                    const double tParam = safe_divide<double>(
                        static_cast<double>(idx), static_cast<double>(combOverCount - 1));
                    const double yPos = combTopY + tParam * (combBottomY - combTopY);
                    const double crownHalf = crownHalfAt(yPos);
                    const double rightX = cx + crownHalf;
                    const double leftX = cx - crownHalf * leftShorten.at(idx);
                    const double arcLift = faceHeight * (0.03 + 0.06 * tParam);
                    // highlight runs along the upper inner portion of the strand
                    const double hlRight = cx + crownHalf * 0.55;
                    const double hlLeft = cx - crownHalf * 0.35 * leftShorten.at(idx);

                    wxGraphicsPath strand = gc->CreatePath();
                    strand.MoveToPoint(hlRight, yPos - arcLift * 0.5);
                    strand.AddQuadCurveToPoint((hlLeft + hlRight) * math_constants::half,
                                               yPos - 1.6 * arcLift, hlLeft, yPos - arcLift * 0.5);
                    gc->StrokePath(strand);
                    }
                }
            }
        else if (parts.m_hair && hairStyle == HairStyleKind::CombOver)
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(135);
            const wxColour hairShadow = hairColor.ChangeLightness(80);
            const wxColour hairStrandColor = Colors::ColorContrast::IsDark(hairColor) ?
                                                 wxColour{ 70, 90, 160, 110 } :
                                                 wxColour{ 0, 0, 0, 55 };

            // taller side tufts that reach up the sides of the head, with a thick
            // swoosh of hair across the crown combed from the viewer's right (the
            // part side) toward the viewer's left
            for (const int side : { -1, 1 })
                {
                wxGraphicsPath tuft = gc->CreatePath();
                // top-inner anchor high up the side of the head
                tuft.MoveToPoint(cx + side * faceWidth * 0.55, cy - faceHeight * 0.85);
                // top curve out and up to the upper-outer point
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 0.9, cy - faceHeight * 0.92,
                                         cx + side * faceWidth * 1.05, cy - faceHeight * 0.6);
                // outer edge down to ear, just outside the head
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 1.05, cy - faceHeight * 0.05,
                                         cx + side * faceWidth * 0.9, cy + faceHeight * 0.1);
                // inner edge up alongside the temple
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 0.78, cy - faceHeight * 0.02,
                                         cx + side * faceWidth * 0.78, cy - faceHeight * 0.4);
                // back up the inner edge to the top-inner anchor
                tuft.AddQuadCurveToPoint(cx + side * faceWidth * 0.6, cy - faceHeight * 0.7,
                                         cx + side * faceWidth * 0.55, cy - faceHeight * 0.85);
                tuft.CloseSubpath();

                gc->SetBrush(wxBrush{ hairColor });
                gc->SetPen(wxPen{ hairShadow, 1 });
                gc->FillPath(tuft);
                gc->StrokePath(tuft);

                // sheen along the upper-outer side of the tuft - kept inside the
                // tuft outline (which arches highest near x = ±0.9, y = -0.92)
                wxGraphicsPath sheen = gc->CreatePath();
                sheen.MoveToPoint(cx + side * faceWidth * 0.88, cy - faceHeight * 0.72);
                sheen.AddQuadCurveToPoint(cx + side * faceWidth * 0.98, cy - faceHeight * 0.5,
                                          cx + side * faceWidth * 0.96, cy - faceHeight * 0.1);
                sheen.AddQuadCurveToPoint(cx + side * faceWidth * 0.86, cy - faceHeight * 0.3,
                                          cx + side * faceWidth * 0.82, cy - faceHeight * 0.65);
                sheen.CloseSubpath();
                auto sheenBrush = gc->CreateLinearGradientBrush(
                    cx + side * faceWidth * 0.9, cy - faceHeight * 0.72,
                    cx + side * faceWidth * 0.9, cy - faceHeight * 0.1, hairHighlight,
                    wxColour{ hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0 });
                gc->SetBrush(sheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(sheen);

                // short vertical strands on the sides for texture
                gc->SetPen(wxPen{ hairStrandColor, 1 });
                for (int idx = 0; idx < 4; ++idx)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startY = cy - faceHeight * (0.75 - idx * 0.2);
                    strand.MoveToPoint(cx + side * faceWidth * 0.96, startY);
                    strand.AddQuadCurveToPoint(
                        cx + side * faceWidth * 0.88, startY + faceHeight * 0.06,
                        cx + side * faceWidth * 0.82, startY + faceHeight * 0.1);
                    gc->StrokePath(strand);
                    }
                }

            // thick swoosh of hair across the top, volume peak on viewer's right
            // (the part side); sweeps down and left across the forehead. endpoints
            // ride atop the side tufts so the swoosh reads as wide and full
            wxGraphicsPath swoosh = gc->CreatePath();
            // start over the right tuft, near its highest point
            swoosh.MoveToPoint(cx + faceWidth * 0.75, cy - faceHeight * 0.9);
            // up and over to the volume peak on the right
            swoosh.AddQuadCurveToPoint(cx + faceWidth * 0.7, cy - faceHeight * 1.17,
                                       cx + faceWidth * 0.15, cy - faceHeight * 1.13);
            // across the top with a slight dip toward the left
            swoosh.AddQuadCurveToPoint(cx - faceWidth * 0.25, cy - faceHeight * 1.08,
                                       cx - faceWidth * 0.6, cy - faceHeight * 0.95);
            // down to the top of the left tuft
            swoosh.AddQuadCurveToPoint(cx - faceWidth * 0.75, cy - faceHeight * 0.92,
                                       cx - faceWidth * 0.75, cy - faceHeight * 0.9);
            // swoosh front edge — low on left (where hair ends), curving up to the
            // part on viewer's right
            swoosh.AddQuadCurveToPoint(cx - faceWidth * 0.65, cy - faceHeight * 0.4,
                                       cx - faceWidth * 0.1, cy - faceHeight * 0.5);
            swoosh.AddQuadCurveToPoint(cx + faceWidth * 0.35, cy - faceHeight * 0.7,
                                       cx + faceWidth * 0.75, cy - faceHeight * 0.9);
            swoosh.CloseSubpath();

            gc->SetBrush(wxBrush{ hairColor });
            gc->SetPen(wxPen{ hairShadow, 1 });
            gc->FillPath(swoosh);
            gc->StrokePath(swoosh);

            // strand highlights flowing across the swoosh in the comb direction
            // (from the part on the right, sweeping leftward and slightly downward).
            // per-strand jitter tables break up the regular fan and add thickness
            // variation, so the strands read as a hand-combed swoosh rather than a
            // uniform pattern
            constexpr size_t strandCount{ 8 };
            constexpr std::array<double, strandCount> startYJitter{ 0.00,  -0.022, 0.018, -0.028,
                                                                    0.012, -0.016, 0.025, -0.01 };
            constexpr std::array<double, strandCount> endYJitter{ -0.015, 0.022, -0.028, 0.014,
                                                                  -0.02,  0.03,  -0.008, 0.018 };
            constexpr std::array<double, strandCount> endXJitter{ -0.025, 0.035, -0.018, 0.022,
                                                                  -0.032, 0.014, -0.026, 0.02 };
            constexpr std::array<double, strandCount> cpYJitter{ 0.018, -0.028, 0.024, -0.014,
                                                                 0.026, -0.02,  0.012, -0.032 };
            constexpr std::array<double, strandCount> thicknessVar{ 1.00, 1.20, 0.80, 1.15,
                                                                    0.90, 1.05, 0.95, 1.25 };

            const double strandThickness = std::max(1.0, faceHeight * 0.014);
            for (size_t idx = 0; idx < strandCount; ++idx)
                {
                const double tParam = safe_divide<double>(static_cast<double>(idx),
                                                          static_cast<double>(strandCount - 1));
                // strands fan out: upper strands start near the volume peak, lower
                // strands start closer to the forehead/part. left endpoints stay
                // inside the swoosh outline (upper strands can extend further left
                // since the swoosh is wider up top)
                const double startX = cx + faceWidth * (0.68 - 0.05 * tParam);
                const double startY =
                    cy - faceHeight * (1.05 - 0.18 * tParam + startYJitter.at(idx));
                const double endX = cx - faceWidth * (0.5 - 0.1 * tParam + endXJitter.at(idx));
                const double endY = cy - faceHeight * (0.92 - 0.27 * tParam + endYJitter.at(idx));
                const double cpX = cx + faceWidth * (0.1 - 0.15 * tParam);
                const double cpY = cy - faceHeight * (1.1 - 0.2 * tParam + cpYJitter.at(idx));

                gc->SetPen(
                    wxPen(hairHighlight, std::max(1.0, strandThickness * thicknessVar.at(idx))));
                wxGraphicsPath strand = gc->CreatePath();
                strand.MoveToPoint(startX, startY);
                strand.AddQuadCurveToPoint(cpX, cpY, endX, endY);
                gc->StrokePath(strand);
                }

            // darker contrast strands interleaved for depth - different jitter so
            // they don't visually align with the highlights
            constexpr std::array<double, strandCount> darkStartYJitter{ -0.018, 0.026, -0.012,
                                                                        0.022,  -0.03, 0.015,
                                                                        -0.024, 0.028 };
            constexpr std::array<double, strandCount> darkEndYJitter{
                0.02, -0.026, 0.014, -0.022, 0.028, -0.012, 0.024, -0.018
            };
            constexpr std::array<double, strandCount> darkEndXJitter{
                0.022, -0.03, 0.016, -0.026, 0.012, -0.028, 0.02, -0.014
            };
            constexpr std::array<double, strandCount> darkCpYJitter{ -0.024, 0.018, -0.028, 0.012,
                                                                     -0.02,  0.026, -0.014, 0.022 };
            constexpr std::array<double, strandCount> darkThicknessVar{ 0.95, 1.10, 0.75, 1.05,
                                                                        0.85, 1.15, 0.80, 1.00 };
            for (size_t idx = 0; idx < strandCount; ++idx)
                {
                const double tParam = safe_divide<double>(static_cast<double>(idx) + 0.5,
                                                          static_cast<double>(strandCount - 1));
                const double startX = cx + faceWidth * (0.66 - 0.05 * tParam);
                const double startY =
                    cy - faceHeight * (1.02 - 0.18 * tParam + darkStartYJitter.at(idx));
                const double endX = cx - faceWidth * (0.48 - 0.1 * tParam + darkEndXJitter.at(idx));
                const double endY =
                    cy - faceHeight * (0.86 - 0.25 * tParam + darkEndYJitter.at(idx));
                const double cpX = cx + faceWidth * (0.08 - 0.15 * tParam);
                const double cpY = cy - faceHeight * (1.06 - 0.2 * tParam + darkCpYJitter.at(idx));

                gc->SetPen(wxPen(hairStrandColor,
                                 std::max(1.0, strandThickness * 0.7 * darkThicknessVar.at(idx))));
                wxGraphicsPath strand = gc->CreatePath();
                strand.MoveToPoint(startX, startY);
                strand.AddQuadCurveToPoint(cpX, cpY, endX, endY);
                gc->StrokePath(strand);
                }
            }

        if (parts.m_eyes)
            {
            // draw eyes
            const double eyeY = cy - faceHeight * (0.15 + 0.2 * features.m_eyePosition);
            const double eyeSpacing = faceWidth * 0.4;
            const double eyeRadius = faceHeight * 0.1 * (0.6 + 0.8 * features.m_eyeSize);

            // sclera (white of eye)
            gc->SetBrush(*wxWHITE_BRUSH);
            gc->SetPen(outlinePen);
            // left eye
            gc->DrawEllipse(cx - eyeSpacing - eyeRadius, eyeY - eyeRadius, eyeRadius * 2,
                            eyeRadius * 1.6);
            // right eye
            gc->DrawEllipse(cx + eyeSpacing - eyeRadius, eyeY - eyeRadius, eyeRadius * 2,
                            eyeRadius * 1.6);

            // iris (colored part) and pupils
            const double pupilOffset = (features.m_pupilPosition - 0.5) * eyeRadius * 0.6;
            const double irisRadius = eyeRadius * 0.55;
            const double pupilRadius = eyeRadius * 0.25;

            // draw iris
            gc->SetBrush(wxBrush{ eyeColor });
            gc->SetPen(*wxTRANSPARENT_PEN);
            // left iris
            gc->DrawEllipse(cx - eyeSpacing + pupilOffset - irisRadius, eyeY - irisRadius * 0.9,
                            irisRadius * 2, irisRadius * 2);
            // right iris
            gc->DrawEllipse(cx + eyeSpacing + pupilOffset - irisRadius, eyeY - irisRadius * 0.9,
                            irisRadius * 2, irisRadius * 2);

            // draw pupils (black center)
            gc->SetBrush(*wxBLACK_BRUSH);
            // left pupil
            gc->DrawEllipse(cx - eyeSpacing + pupilOffset - pupilRadius, eyeY - pupilRadius * 0.9,
                            pupilRadius * 2, pupilRadius * 2);
            // right pupil
            gc->DrawEllipse(cx + eyeSpacing + pupilOffset - pupilRadius, eyeY - pupilRadius * 0.9,
                            pupilRadius * 2, pupilRadius * 2);

            // draw feminine eyelashes
            if (gender == Gender::Female)
                {
                gc->SetPen(wxPen{ outlineColor, 1 });
                const double lashLength = eyeRadius * 0.4;
                const double eyeTop = eyeY - eyeRadius * 0.8;

                // draw 3 lashes per eye, angled outward
                for (int i = 0; i < 3; ++i)
                    {
                    const double offset = (i - 1) * eyeRadius * 0.5;
                    const double angle = (i - 1) * 0.3; // angle outward

                    // left eyelashes
                    wxGraphicsPath leftLash = gc->CreatePath();
                    leftLash.MoveToPoint(cx - eyeSpacing + offset, eyeTop);
                    leftLash.AddLineToPoint(cx - eyeSpacing + offset - angle * lashLength,
                                            eyeTop - lashLength);
                    gc->StrokePath(leftLash);

                    // right eyelashes
                    wxGraphicsPath rightLash = gc->CreatePath();
                    rightLash.MoveToPoint(cx + eyeSpacing + offset, eyeTop);
                    rightLash.AddLineToPoint(cx + eyeSpacing + offset + angle * lashLength,
                                             eyeTop - lashLength);
                    gc->StrokePath(rightLash);
                    }
                }

            // draw eyebrows using lines
            gc->SetPen(wxPen{ outlineColor, 2 });
            const double browY = eyeY - eyeRadius * 1.8;
            const double browSlant = (features.m_eyebrowSlant - 0.5) * eyeRadius * 0.8;
            const double browLength = eyeRadius * 1.2;

            wxGraphicsPath leftBrow = gc->CreatePath();
            leftBrow.MoveToPoint(cx - eyeSpacing - browLength, browY + browSlant);
            leftBrow.AddLineToPoint(cx - eyeSpacing + browLength, browY - browSlant);
            gc->StrokePath(leftBrow);

            wxGraphicsPath rightBrow = gc->CreatePath();
            rightBrow.MoveToPoint(cx + eyeSpacing - browLength, browY - browSlant);
            rightBrow.AddLineToPoint(cx + eyeSpacing + browLength, browY + browSlant);
            gc->StrokePath(rightBrow);
            }

        if (parts.m_nose)
            {
            // draw nose (simple line with small base)
            gc->SetPen(outlinePen);
            const double noseScale = 0.6 + 0.8 * features.m_noseSize;
            const double noseLength = faceHeight * 0.25 * noseScale;
            const double noseWidth = faceWidth * 0.08 * noseScale;
            const double noseTop = cy - faceHeight * 0.05;
            const double noseBottom = noseTop + noseLength;

            wxGraphicsPath nose = gc->CreatePath();
            nose.MoveToPoint(cx, noseTop);
            nose.AddLineToPoint(cx, noseBottom);
            nose.AddLineToPoint(cx - noseWidth, noseBottom);
            nose.MoveToPoint(cx, noseBottom);
            nose.AddLineToPoint(cx + noseWidth, noseBottom);
            gc->StrokePath(nose);
            }

        if (parts.m_mouth)
            {
            // draw mouth using quadratic curve
            // curvature: 0=frown, 0.5=neutral, 1=smile
            const double curvature = (features.m_mouthCurvature - 0.5) * faceHeight * 0.2;

            if (gender == Gender::Female)
                {
                // draw lips with lipstick
                gc->SetBrush(wxBrush{ lipstickColor });
                gc->SetPen(wxPen{ lipstickColor.ChangeLightness(70), 1 });

                // lip thickness is keyed off baseRadius (constant per chart) rather
                // than faceHeight (which varies with the m_faceHeight feature), so
                // every face's lips have the same thickness
                const double lipThickness = baseRadius * 0.06;
                const double upperLipHeight = baseRadius * 0.045;

                // the whole mouth bows with curvature (smile bows down, frown bows up).
                // both lip outer edges and the separator follow the same bow, so the
                // lips keep a uniform thickness regardless of the smile/frown amount
                const double separatorMidY = mouthY + curvature;
                const double cupidBowY = separatorMidY - upperLipHeight * 0.5;
                const double upperLipControlY = separatorMidY - upperLipHeight;
                const double lowerLipBottomY = separatorMidY + lipThickness;

                // upper lip with cupid's bow
                wxGraphicsPath upperLip = gc->CreatePath();
                upperLip.MoveToPoint(cx - mouthWidthVal, mouthY);
                upperLip.AddQuadCurveToPoint(cx - mouthWidthVal * 0.5, upperLipControlY, cx,
                                             cupidBowY);
                upperLip.AddQuadCurveToPoint(cx + mouthWidthVal * 0.5, upperLipControlY,
                                             cx + mouthWidthVal, mouthY);
                upperLip.AddQuadCurveToPoint(cx, separatorMidY, cx - mouthWidthVal, mouthY);
                gc->FillPath(upperLip);
                gc->StrokePath(upperLip);

                // lower lip - bottom hugs the separator at a constant offset
                wxGraphicsPath lowerLip = gc->CreatePath();
                lowerLip.MoveToPoint(cx - mouthWidthVal, mouthY);
                lowerLip.AddQuadCurveToPoint(cx, lowerLipBottomY, cx + mouthWidthVal, mouthY);
                lowerLip.AddQuadCurveToPoint(cx, separatorMidY, cx - mouthWidthVal, mouthY);
                gc->FillPath(lowerLip);
                gc->StrokePath(lowerLip);
                }
            else
                {
                // male: simple line mouth
                gc->SetPen(wxPen{ outlineColor, 2 });
                wxGraphicsPath mouth = gc->CreatePath();
                mouth.MoveToPoint(cx - mouthWidthVal, mouthY);
                mouth.AddQuadCurveToPoint(cx, mouthY + curvature, cx + mouthWidthVal, mouthY);
                gc->StrokePath(mouth);
                }
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> ChernoffFacesPlot::CreateLegend(const LegendOptions& options)
        {
        m_lastLegendType = LegendType::Regular;
        SetLegendInfo(options);
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{}
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));

        wxString legendText;

        // add header if requested
        if (options.IsIncludingHeader())
            {
            legendText = _(L"Features:\n");
            legend->GetLinesIgnoringLeftMargin().insert(0);
            legend->GetHeaderInfo().Enable(true);
            }

        const auto addFeatureEntry = [&](FeatureId featureId, const wxString& columnName)
        {
            if (!columnName.empty())
                {
                legendText.append(wxString::Format(L"%s: %s\n", GetFeatureDisplayName(featureId),
                                                   TruncateLabel(columnName)));
                legend->GetLegendIcons().emplace_back(Icons::IconShape::HorizontalLine,
                                                      wxPen(m_outlineColor),
                                                      wxBrush(m_outlineColor));
                }
        };

        addFeatureEntry(FeatureId::FaceWidth, m_faceWidthColumnName);
        addFeatureEntry(FeatureId::FaceHeight, m_faceHeightColumnName);
        addFeatureEntry(FeatureId::EyeSize, m_eyeSizeColumnName);
        addFeatureEntry(FeatureId::EyePosition, m_eyePositionColumnName);
        addFeatureEntry(FeatureId::EyebrowSlant, m_eyebrowSlantColumnName);
        addFeatureEntry(FeatureId::PupilDirection, m_pupilPositionColumnName);
        addFeatureEntry(FeatureId::NoseSize, m_noseSizeColumnName);
        addFeatureEntry(FeatureId::MouthWidth, m_mouthWidthColumnName);
        addFeatureEntry(FeatureId::SmileFrown, m_mouthCurvatureColumnName);
        addFeatureEntry(FeatureId::FaceColor, m_faceSaturationColumnName);
        addFeatureEntry(FeatureId::EarSize, m_earSizeColumnName);
        addFeatureEntry(FeatureId::HairStyle, m_hairStyleColumnName);
        addFeatureEntry(FeatureId::HairAddition, m_hairAdditionColumnName);

        legend->SetText(legendText.Trim());
        AdjustLegendSettings(*legend, options.GetPlacementHint());

        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<ChernoffFacesPlot::ChernoffLegend> ChernoffFacesPlot::CreateEnhancedLegend(
        const LegendOptions& options)
        {
        m_lastLegendType = LegendType::Enhanced;
        SetLegendInfo(options);
        auto legend = std::make_unique<ChernoffLegend>(
            GraphItems::GraphItemInfo{}.DPIScaling(GetDPIScaleFactor()).Pen(wxNullPen));

        std::vector<ChernoffLegend::FeatureLabel> features;

        // left side features
        if (!m_faceWidthColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_faceWidthColumnName), FeatureId::FaceWidth, true);
            }
        if (!m_faceHeightColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_faceHeightColumnName), FeatureId::FaceHeight,
                                  true);
            }
        if (!m_eyeSizeColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_eyeSizeColumnName), FeatureId::EyeSize, true);
            }
        if (!m_eyePositionColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_eyePositionColumnName), FeatureId::EyePosition,
                                  true);
            }
        if (!m_eyebrowSlantColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_eyebrowSlantColumnName), FeatureId::EyebrowSlant,
                                  true);
            }
        if (!m_pupilPositionColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_pupilPositionColumnName),
                                  FeatureId::PupilDirection, true);
            }

        // right side features
        if (!m_noseSizeColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_noseSizeColumnName), FeatureId::NoseSize, false);
            }
        if (!m_mouthWidthColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_mouthWidthColumnName), FeatureId::MouthWidth,
                                  false);
            }
        if (!m_mouthCurvatureColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_mouthCurvatureColumnName), FeatureId::SmileFrown,
                                  false);
            }
        if (!m_faceSaturationColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_faceSaturationColumnName), FeatureId::FaceColor,
                                  false);
            }
        if (!m_earSizeColumnName.empty())
            {
            features.emplace_back(TruncateLabel(m_earSizeColumnName), FeatureId::EarSize, false);
            }
        // note: hair style and hair addition are rendered as their own key sections
        // below the face (handled via the SetHair*Labels / SetHair*ColumnName setters
        // below), not as connection-line features

        legend->SetFeatures(std::move(features));
        legend->SetHairStyleLabels(m_hairStyleLabels);
        legend->SetHairStyleColumnName(m_hairStyleColumnName);
        legend->SetHairAdditionLabels(m_hairAdditionLabels);
        legend->SetHairAdditionColumnName(m_hairAdditionColumnName);
        legend->SetFaceColors(m_faceColorLighter, m_faceColor);
        legend->SetOutlineColor(m_outlineColor);
        legend->SetGender(m_gender);
        legend->SetHairStyle(m_hairStyleFemale);
        legend->SetHairStyle(m_hairStyleMale);
        legend->SetHairColor(m_hairColor);
        legend->SetEyeColor(m_eyeColor);
        legend->SetLipstickColor(m_lipstickColor);
        legend->SetCanvasBackgroundColor(GetPlotOrCanvasColor());

        // apply canvas settings based on placement hint
        if (GetCanvas() != nullptr)
            {
            const auto hint = options.GetPlacementHint();
            if (hint == LegendCanvasPlacementHint::RightOfGraph ||
                hint == LegendCanvasPlacementHint::LeftOfGraph)
                {
                legend->SetCanvasWidthProportion(GetCanvas()->CalcMinWidthProportion(*legend));
                legend->SetPageHorizontalAlignment(hint == LegendCanvasPlacementHint::RightOfGraph ?
                                                       PageHorizontalAlignment::RightAligned :
                                                       PageHorizontalAlignment::LeftAligned);
                legend->GetGraphItemInfo().CanvasPadding(4, 4, 4, 4).FixedWidthOnCanvas(true);
                }
            }

        return legend;
        }
    } // namespace Wisteria::Graphs
