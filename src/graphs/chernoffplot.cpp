///////////////////////////////////////////////////////////////////////////////
// Name:        chernoffplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "chernoffplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ChernoffFacesPlot, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
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
        default:
            return wxString{};
            }
        }

    //----------------------------------------------------------------
    wxRect ChernoffFacesPlot::FaceObject::GetBoundingBox([[maybe_unused]]
                                                         wxDC &
                                                         dc) const
        {
        const auto scaledSize =
            wxSize(m_size.GetWidth() * GetScaling(), m_size.GetHeight() * GetScaling());

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
            return wxRect{};
            }

        const wxRect boundingBox = GetBoundingBox(dc);

        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc != nullptr)
            {
            ChernoffFacesPlot::DrawFace(
                gc, boundingBox, m_features, m_faceColorLighter, m_faceColorDarker, m_outlineColor,
                m_lipstickColor, m_eyeColor, m_hairColor, m_hairStyle, m_gender, m_facialHair);
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
        // face is elongated: height = 1.5 * width, and we want it 2x larger
        const auto minFaceHeight = ScaleToScreenAndCanvas(160);
        const auto minFaceWidth = minFaceHeight * math_constants::two_thirds;

        // measure label widths at reduced scaling (labels will be scaled to fit)
        const double labelScaling = GetScaling() * 0.6;
        int maxLeftLabelWidth = 0;
        int maxRightLabelWidth = 0;
        int leftCount = 0;
        int rightCount = 0;

        for (const auto& feature : m_features)
            {
            if (!feature.columnName.empty())
                {
                const GraphItems::Label label(GraphItems::GraphItemInfo{
                    GetFeatureDisplayName(feature.featureId) +
                    // make long variable names fit more nicely by pushing down to next line
                    (feature.columnName.length() <= 8 ? L": " : L":\n") + feature.columnName }
                                                  .Pen(wxNullPen)
                                                  .Scaling(labelScaling)
                                                  .DPIScaling(GetDPIScaleFactor()));
                const auto labelSize = label.GetBoundingBox(dc).GetSize();

                if (feature.leftSide)
                    {
                    maxLeftLabelWidth = std::max(maxLeftLabelWidth, labelSize.GetWidth());
                    ++leftCount;
                    }
                else
                    {
                    maxRightLabelWidth = std::max(maxRightLabelWidth, labelSize.GetWidth());
                    ++rightCount;
                    }
                }
            }

        // calculate minimum dimensions
        // width: left labels + gap + face + gap + right labels
        const int minWidth =
            maxLeftLabelWidth + lineGap + minFaceWidth + lineGap + maxRightLabelWidth;

        // height: face uses 2/3 of height, so total = faceHeight * 3/2
        const auto labelHeight = ScaleToScreenAndCanvas(14);
        const int labelsHeight = std::max(leftCount, rightCount) * labelHeight + (labelHeight / 2);
        const auto minHeight = std::max<int>(minFaceHeight * 3 / 2, labelsHeight);

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
        for (const auto& feature : m_features)
            {
            if (!feature.columnName.empty())
                {
                const wxString displayName = GetFeatureDisplayName(feature.featureId);
                const FeatureInfo info{ displayName +
                                            (feature.columnName.length() <= 8 ? L": " : L":\n") +
                                            feature.columnName,
                                        feature.featureId };
                if (feature.leftSide)
                    {
                    leftFeatures.push_back(info);
                    }
                else
                    {
                    rightFeatures.push_back(info);
                    }
                }
            }

        // calculate face size - elongated (height = 1.5 * width), use 2/3 of available height
        const auto faceHeight = m_rect.GetHeight() * math_constants::two_thirds;
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
            lbl.SetFontColor(m_outlineColor);
            return lbl.GetBoundingBox(dc).GetWidth();
        };

        int maxLeftWidth{ 0 };
        int maxRightWidth{ 0 };
        for (const auto& feature : leftFeatures)
            {
            maxLeftWidth =
                std::max(maxLeftWidth, measureLabelWidth(feature.labelText, labelScaling));
            }
        for (const auto& feature : rightFeatures)
            {
            maxRightWidth =
                std::max(maxRightWidth, measureLabelWidth(feature.labelText, labelScaling));
            }

        // position face based on measured label widths so that each side
        // gets only the space it needs
        int faceX{ 0 };
        if (rightFeatures.empty() && !leftFeatures.empty())
            {
            // position face on the right side (with small margin)
            faceX = m_rect.GetRight() - faceWidth - lineGap;
            }
        else if (!leftFeatures.empty() || !rightFeatures.empty())
            {
            // place face right after the left label area
            faceX = m_rect.GetX() + maxLeftWidth + lineGap;
            }
        else
            {
            // no labels on either side, center the face
            faceX = m_rect.GetX() + safe_divide<int>(m_rect.GetWidth() - faceWidth, 2);
            }
        const wxRect faceRect(faceX,
                              m_rect.GetY() + safe_divide<int>(m_rect.GetHeight() - faceHeight, 2),
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
            const double rightScale = safe_divide<double>(rightLabelAreaWidth, maxRightWidth);
            labelScaling = std::min(labelScaling, GetScaling() * 0.6 * rightScale);
            }

            // draw the face (scoped so GraphicsContext is flushed before drawing lines)
            {
            const GraphItems::GraphicsContextFallback gcf{ &dc, faceRect };
            auto* gc = gcf.GetGraphicsContext();
            if (gc != nullptr)
                {
                const FaceFeatures defaultFeatures;
                ChernoffFacesPlot::DrawFace(gc, faceRect, defaultFeatures, m_faceColorLighter,
                                            m_faceColorDarker, m_outlineColor, m_lipstickColor,
                                            m_eyeColor, m_hairColor, m_hairStyle, m_gender,
                                            m_facialHair);
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
            label.SetFontColor(contrastingLabelColor);
            label.SetAnchoring(Anchoring::TopLeftCorner);
            label.SetAnchorPoint(wxPoint(m_rect.GetX(), m_rect.GetY()));
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
        const auto leftAvailableHeight = m_rect.GetHeight() - (faceRect.GetY() - m_rect.GetY());
        const auto leftLabelSpacing =
            (otherLeftFeatures.size() > 1) ?
                safe_divide(leftAvailableHeight, static_cast<int>(otherLeftFeatures.size() + 1)) :
                safe_divide(leftAvailableHeight, 2);

        for (size_t i = 0; i < otherLeftFeatures.size(); ++i)
            {
            const auto labelY = leftStartY + static_cast<int>(i + 1) * leftLabelSpacing;

            GraphItems::Label label(GraphItems::GraphItemInfo(otherLeftFeatures[i].labelText)
                                        .Pen(wxNullPen)
                                        .Scaling(labelScaling)
                                        .DPIScaling(GetDPIScaleFactor())
                                        .Padding(0, 4, 0, 0));
            label.SetFontColor(contrastingLabelColor);
            label.SetAnchoring(Anchoring::BottomLeftCorner);
            label.SetAnchorPoint(wxPoint{ m_rect.GetX(), labelY });
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
                safe_divide(m_rect.GetHeight(), static_cast<int>(rightFeatures.size() + 1)) :
                safe_divide(m_rect.GetHeight(), 2);

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
    double ChernoffFacesPlot::NormalizeValue(double value, double minVal, double maxVal) noexcept
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
                                    const std::optional<wxString>& earSizeColumn)
        {
        m_faces.clear();
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

        // validate required column
        const auto faceWidthCol = GetDataset()->GetContinuousColumn(faceWidthColumn);
        if (faceWidthCol == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': face width column not found for Chernoff faces plot."),
                                 faceWidthColumn)
                    .ToUTF8());
            }

        // get optional column iterators
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

            face.faceWidth = NormalizeValue(faceWidthCol->GetValue(i), fwMin, fwMax);

            if (faceHeightCol != colsEnd)
                {
                face.faceHeight = NormalizeValue(faceHeightCol->GetValue(i), fhMin, fhMax);
                }
            if (eyeSizeCol != colsEnd)
                {
                face.eyeSize = NormalizeValue(eyeSizeCol->GetValue(i), esMin, esMax);
                }
            if (eyePositionCol != colsEnd)
                {
                face.eyePosition = NormalizeValue(eyePositionCol->GetValue(i), epMin, epMax);
                }
            if (eyebrowSlantCol != colsEnd)
                {
                face.eyebrowSlant = NormalizeValue(eyebrowSlantCol->GetValue(i), ebsMin, ebsMax);
                }
            if (pupilPositionCol != colsEnd)
                {
                face.pupilPosition = NormalizeValue(pupilPositionCol->GetValue(i), ppMin, ppMax);
                }
            if (noseSizeCol != colsEnd)
                {
                face.noseSize = NormalizeValue(noseSizeCol->GetValue(i), nsMin, nsMax);
                }
            if (mouthWidthCol != colsEnd)
                {
                face.mouthWidth = NormalizeValue(mouthWidthCol->GetValue(i), mwMin, mwMax);
                }
            if (mouthCurvatureCol != colsEnd)
                {
                face.mouthCurvature = NormalizeValue(mouthCurvatureCol->GetValue(i), mcMin, mcMax);
                }
            if (faceSaturationCol != colsEnd)
                {
                face.faceSaturation = NormalizeValue(faceSaturationCol->GetValue(i), fsMin, fsMax);
                }
            if (earSizeCol != colsEnd)
                {
                face.earSize = NormalizeValue(earSizeCol->GetValue(i), erMin, erMax);
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
                face.allDataMissing = allMissing;
                }

            // get label from ID column if available
            if (GetDataset()->GetIdColumn().GetRowCount() > i)
                {
                face.label = GetDataset()->GetIdColumn().GetValue(i);
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
        const auto rows = static_cast<size_t>(
            std::ceil(safe_divide<double>(faceCount, static_cast<double>(cols))));

        // calculate cell size, leaving room for labels
        const int labelHeight = m_showLabels ? ScaleToScreenAndCanvas(20) : 0;
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

                if (m_faces[faceIndex].allDataMissing)
                    {
                    // show "No data" label centered where the face would be
                    const auto faceCenterX = x + safe_divide(faceSize, 2);
                    const auto faceCenterY = y + safe_divide(faceSize, 2);
                    auto noDataLabel = std::make_unique<GraphItems::Label>(
                        GraphItems::GraphItemInfo(_(L"No data"))
                            .Pen(wxNullPen)
                            .Scaling(GetScaling())
                            .DPIScaling(GetDPIScaleFactor())
                            .Anchoring(Anchoring::Center)
                            .AnchorPoint(wxPoint{ faceCenterX, faceCenterY }));
                    AddObject(std::move(noDataLabel));
                    }
                else
                    {
                    // create face
                    AddObject(std::make_unique<FaceObject>(
                        GraphItems::GraphItemInfo{}
                            .Pen(wxNullPen)
                            .Selectable(true)
                            .Anchoring(Anchoring::TopLeftCorner)
                            .AnchorPoint(wxPoint{ x, y }),
                        m_faces[faceIndex], wxSize{ faceSize, faceSize }, m_faceColorLighter,
                        m_faceColor, m_outlineColor, m_lipstickColor, m_eyeColor, m_hairColor,
                        m_hairStyle, m_gender, m_facialHair));
                    }

                // add label below face if enabled
                if (m_showLabels && !m_faces[faceIndex].label.empty())
                    {
                    auto label = std::make_unique<GraphItems::Label>(
                        GraphItems::GraphItemInfo(m_faces[faceIndex].label)
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
    void ChernoffFacesPlot::DrawFace(wxGraphicsContext * gc, const wxRect& rect,
                                     const FaceFeatures& features, const wxColour& faceColorLighter,
                                     const wxColour& faceColorDarker, const wxColour& outlineColor,
                                     const wxColour& lipstickColor, const wxColour& eyeColor,
                                     const wxColour& hairColor, const HairStyle hairStyle,
                                     const Gender gender, const FacialHair facialHair)
        {
        if (gc == nullptr)
            {
            return;
            }

        // interpolate between lighter and darker skin colors based on saturation
        // 0 = lighter color, 1 = darker color
        const double colorBlendFactor = features.faceSaturation;
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
        const double faceWidth = baseRadius * (0.7 + 0.6 * features.faceWidth);
        // face height: 0.8 to 1.2 multiplier
        const double faceHeight = baseRadius * (0.8 + 0.4 * features.faceHeight);

        const wxPen outlinePen{ outlineColor, 1 };

        // draw ears first (behind face)
        const double earScale = math_constants::half + features.earSize;
        const double earWidth = faceHeight * 0.12 * earScale;
        const double earHeight = faceHeight * 0.2 * earScale;

        gc->SetBrush(wxBrush(faceCol));
        gc->SetPen(outlinePen);
        // left ear
        gc->DrawEllipse(cx - faceWidth - earWidth * 0.3, cy - earHeight * 0.5, earWidth, earHeight);
        // right ear
        gc->DrawEllipse(cx + faceWidth - earWidth * 0.7, cy - earHeight * 0.5, earWidth, earHeight);

        // draw face oval
        gc->SetBrush(wxBrush{ faceCol });
        gc->SetPen(outlinePen);
        gc->DrawEllipse(cx - faceWidth, cy - faceHeight, faceWidth * 2, faceHeight * 2);

        // draw rosy cheeks for female faces (radial gradient)
        if (gender == Gender::Female)
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

        // draw hair over forehead for female (after face, before eyebrows)
        if (gender == Gender::Female && hairStyle != HairStyle::Bald)
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(130);
            const wxColour hairShadow = hairColor.ChangeLightness(80);
            // for dark hair, artists use a blue tint for strands instead of black
            const wxColour hairStrandColor =
                Colors::ColorContrast::IsDark(hairColor) ?
                    wxColour(70, 90, 160, 90) : // blue tint for dark hair
                    wxColour(0, 0, 0, 40);      // dark strands for light hair

            // calculate where eyebrows will be drawn (to stop hair above them)
            const double eyeYPreCalc = cy - faceHeight * (0.15 + 0.2 * features.eyePosition);
            const double eyeRadiusPreCalc = faceHeight * 0.1 * (0.6 + 0.8 * features.eyeSize);
            const double browYLimit = eyeYPreCalc - eyeRadiusPreCalc * 1.8 - faceHeight * 0.05;

            if (hairStyle == HairStyle::Bob)
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
            else if (hairStyle == HairStyle::Pixie)
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

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
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
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
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
            else if (hairStyle == HairStyle::LongStraight)
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

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
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
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
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
                for (int side = -1; side <= 1; side += 2)
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
            else if (hairStyle == HairStyle::Bun)
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

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
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
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
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
                for (int side = -1; side <= 1; side += 2)
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
                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // sheen on bun
                auto bunSheen =
                    gc->CreateRadialGradientBrush(bunX - bunRadius * 0.3, bunY - bunRadius * 0.3,
                                                  bunX, bunY, bunRadius, hairHighlight, hairColor);
                gc->SetBrush(bunSheen);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // outline the bun
                gc->SetPen(wxPen(hairShadow, 1));
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
            }
        // high top fade - works for both genders
        else if (hairStyle == HairStyle::HighTopFade)
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

            gc->SetBrush(wxBrush(hairColor));
            gc->SetPen(wxPen(hairShadow, 1));
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
                wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
            gc->SetBrush(sheenBrush);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->FillPath(sheen);
            }

        // draw eyes
        const double eyeY = cy - faceHeight * (0.15 + 0.2 * features.eyePosition);
        const double eyeSpacing = faceWidth * 0.4;
        const double eyeRadius = faceHeight * 0.1 * (0.6 + 0.8 * features.eyeSize);

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
        const double pupilOffset = (features.pupilPosition - 0.5) * eyeRadius * 0.6;
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

                // left eye lashes
                wxGraphicsPath leftLash = gc->CreatePath();
                leftLash.MoveToPoint(cx - eyeSpacing + offset, eyeTop);
                leftLash.AddLineToPoint(cx - eyeSpacing + offset - angle * lashLength,
                                        eyeTop - lashLength);
                gc->StrokePath(leftLash);

                // right eye lashes
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
        const double browSlant = (features.eyebrowSlant - 0.5) * eyeRadius * 0.8;
        const double browLength = eyeRadius * 1.2;

        wxGraphicsPath leftBrow = gc->CreatePath();
        leftBrow.MoveToPoint(cx - eyeSpacing - browLength, browY + browSlant);
        leftBrow.AddLineToPoint(cx - eyeSpacing + browLength, browY - browSlant);
        gc->StrokePath(leftBrow);

        wxGraphicsPath rightBrow = gc->CreatePath();
        rightBrow.MoveToPoint(cx + eyeSpacing - browLength, browY - browSlant);
        rightBrow.AddLineToPoint(cx + eyeSpacing + browLength, browY + browSlant);
        gc->StrokePath(rightBrow);

        // draw nose (simple line with small base)
        gc->SetPen(outlinePen);
        const double noseScale = 0.6 + 0.8 * features.noseSize;
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

        // draw mouth using quadratic curve
        const double mouthY = cy + faceHeight * 0.45;
        const double mouthWidthVal = faceWidth * 0.35 * (0.5 + features.mouthWidth);
        // curvature: 0=frown, 0.5=neutral, 1=smile
        const double curvature = (features.mouthCurvature - 0.5) * faceHeight * 0.2;

        if (gender == Gender::Female)
            {
            // draw lips with lipstick
            gc->SetBrush(wxBrush{ lipstickColor });
            gc->SetPen(wxPen{ lipstickColor.ChangeLightness(70), 1 });

            // ensure lips have fullness even when mouth is straight
            const double lipFullness = faceHeight * 0.06;
            const double upperLipHeight = faceHeight * 0.045;
            const double lowerLipDepth = std::max(lipFullness, std::abs(curvature) + lipFullness);

            // upper lip with cupid's bow
            wxGraphicsPath upperLip = gc->CreatePath();
            upperLip.MoveToPoint(cx - mouthWidthVal, mouthY);
            upperLip.AddQuadCurveToPoint(cx - mouthWidthVal * 0.5, mouthY - upperLipHeight, cx,
                                         mouthY - upperLipHeight * 0.5);
            upperLip.AddQuadCurveToPoint(cx + mouthWidthVal * 0.5, mouthY - upperLipHeight,
                                         cx + mouthWidthVal, mouthY);
            upperLip.AddQuadCurveToPoint(cx, mouthY + curvature * 0.3 + lipFullness * 0.3,
                                         cx - mouthWidthVal, mouthY);
            gc->FillPath(upperLip);
            gc->StrokePath(upperLip);

            // lower lip
            wxGraphicsPath lowerLip = gc->CreatePath();
            lowerLip.MoveToPoint(cx - mouthWidthVal, mouthY);
            lowerLip.AddQuadCurveToPoint(cx, mouthY + curvature + lowerLipDepth, cx + mouthWidthVal,
                                         mouthY);
            lowerLip.AddQuadCurveToPoint(cx, mouthY + curvature * 0.3 + lipFullness * 0.3,
                                         cx - mouthWidthVal, mouthY);
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

            // draw facial hair for males
            if (facialHair == FacialHair::FiveOClockShadow)
                {
                // stubble using hair color with transparency
                const wxColour stubbleColor(hairColor.Red(), hairColor.Green(), hairColor.Blue(),
                                            80);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->SetBrush(wxBrush(stubbleColor));

                const double dotSize = faceHeight * 0.012;
                const double spacing = dotSize * 1.4;

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
                for (int side = -1; side <= 1; side += 2)
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
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> ChernoffFacesPlot::CreateLegend(const LegendOptions& options)
        {
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
                legendText.append(
                    wxString::Format(L"%s: %s\n", GetFeatureDisplayName(featureId), columnName));
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

        legend->SetText(legendText.Trim());
        AdjustLegendSettings(*legend, options.GetPlacementHint());

        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<ChernoffFacesPlot::ChernoffLegend> ChernoffFacesPlot::CreateExtendedLegend(
        const LegendOptions& options)
        {
        auto legend = std::make_unique<ChernoffLegend>(
            GraphItems::GraphItemInfo{}.DPIScaling(GetDPIScaleFactor()).Pen(wxNullPen));

        std::vector<ChernoffLegend::FeatureLabel> features;

        // left side features
        if (!m_faceWidthColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_faceWidthColumnName), FeatureId::FaceWidth, true });
            }
        if (!m_faceHeightColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_faceHeightColumnName), FeatureId::FaceHeight, true });
            }
        if (!m_eyeSizeColumnName.empty())
            {
            features.push_back({ TruncateLabel(m_eyeSizeColumnName), FeatureId::EyeSize, true });
            }
        if (!m_eyePositionColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_eyePositionColumnName), FeatureId::EyePosition, true });
            }
        if (!m_eyebrowSlantColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_eyebrowSlantColumnName), FeatureId::EyebrowSlant, true });
            }
        if (!m_pupilPositionColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_pupilPositionColumnName), FeatureId::PupilDirection, true });
            }

        // right side features
        if (!m_noseSizeColumnName.empty())
            {
            features.push_back({ TruncateLabel(m_noseSizeColumnName), FeatureId::NoseSize, false });
            }
        if (!m_mouthWidthColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_mouthWidthColumnName), FeatureId::MouthWidth, false });
            }
        if (!m_mouthCurvatureColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_mouthCurvatureColumnName), FeatureId::SmileFrown, false });
            }
        if (!m_faceSaturationColumnName.empty())
            {
            features.push_back(
                { TruncateLabel(m_faceSaturationColumnName), FeatureId::FaceColor, false });
            }
        if (!m_earSizeColumnName.empty())
            {
            features.push_back({ TruncateLabel(m_earSizeColumnName), FeatureId::EarSize, false });
            }

        legend->SetFeatures(std::move(features));
        legend->SetFaceColors(m_faceColorLighter, m_faceColor);
        legend->SetOutlineColor(m_outlineColor);
        legend->SetGender(m_gender);
        legend->SetHairStyle(m_hairStyle);
        legend->SetHairColor(m_hairColor);
        legend->SetEyeColor(m_eyeColor);
        legend->SetLipstickColor(m_lipstickColor);
        legend->SetFacialHair(m_facialHair);
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
